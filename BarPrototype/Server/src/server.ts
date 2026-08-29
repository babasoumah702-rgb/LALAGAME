import Fastify from 'fastify';
import websocket from '@fastify/websocket';
import type {WebSocket} from 'ws';
import {mkdirSync,existsSync,readFileSync} from 'node:fs';
import {join,dirname} from 'node:path';
import {fileURLToPath} from 'node:url';
import {randomBytes} from 'node:crypto';
import {loadScenario} from './config.js';
import {Engine} from './engine.js';
import {Store} from './store.js';
import {dataRoot,ModelAdapter,modelConfigFile} from './model.js';
import {Navigator,emptyNavigation} from './navigation.js';
import type {Command} from './types.js';
import {introActive} from './intro.js';
import {generateIntro,pendingIntro} from './intro-model.js';
import {generateReply,applyReadyReplies} from './reply-runtime.js';
import {TtsAdapter} from './tts.js';
const root=dirname(dirname(fileURLToPath(import.meta.url)));
const scenario=loadScenario(join(root,'scenarios','last_call.json'));
const navFile=join(root,'scenarios','navigation.json');
const navigation=new Navigator(existsSync(navFile)?JSON.parse(readFileSync(navFile,'utf8')):emptyNavigation);
mkdirSync(dataRoot,{recursive:true});
const database=new Store(join(dataRoot,'last-call.db'));
const token=process.env.LASTCALL_SESSION_TOKEN||randomBytes(32).toString('hex');
const adapter=new ModelAdapter();
const tts=new TtsAdapter();
const app=Fastify({logger:false,bodyLimit:16384});
await app.register(websocket,{options:{maxPayload:16384}});
let engine:Engine|undefined;
const sockets=new Set<WebSocket>();
let closing=false;
app.addHook('onRequest',async(request,reply)=>{
  if(request.url==='/health')return;
  const supplied=request.headers.authorization?.replace(/^Bearer /,'');
  if(supplied!==token)return reply.code(401).send({error:'Unauthorized'});
});
app.get('/health',async()=>({ready:true,version:1}));
app.get('/api/bootstrap',async(request)=>{
  adapter.reload();
  const playerId=String((request.query as any).playerId||'');
  return {version:1,title:scenario.title,roles:scenario.roles,intents:scenario.intents,styles:scenario.styles,choices:scenario.choices,sessions:database.list(playerId),modelConfigured:!!adapter.config.key,model:adapter.config.model,modelConfigFile:modelConfigFile()};
});
app.post('/api/session',async(request,reply)=>{
  const body=request.body as any;
  if(!body||typeof body.playerId!=='string'||body.playerId.length>100)return reply.code(400).send({error:'Invalid player ID'});
  if(engine)database.save(engine.world);
  try{
    adapter.reload();
    tts.reload();
    const previous=body.sessionId?database.load(body.sessionId,body.playerId):undefined;
    if(body.mode==='resume'||body.mode==='next'){
      if(!previous)return reply.code(404).send({error:'Save not found'});
      const resumed=new Engine(scenario,{playerId:body.playerId},previous,navigation);
      engine=body.mode==='next'?resumed.nextNight():resumed;
      // Retry only a previous missing-key fallback; never override an explicitly chosen offline mode.
      if(adapter.config.key&&engine.world.modelReason==='未配置密钥，已使用规则模式'){
        engine.world.modelMode='online';engine.world.modelReason='已读取模型配置 · '+adapter.config.model;
      }
    }else{
      engine=new Engine(scenario,{playerId:body.playerId,role:body.role,entryIntent:body.entryIntent,style:body.style,online:body.online!==false,seed:Number(body.seed)||821,opening:body.opening,entryMode:body.entryMode,entryContext:body.entryContext,choices:body.choices,story:body.story},undefined,navigation);
    }
    engine.voice=tts;
    database.save(engine.world);
    void generateIntro(engine);
    broadcast();
    return {state:engine.view(true)};
  }catch{return reply.code(400).send({error:'Cannot open this save'});}
});
app.get('/api/state',async()=>({state:engine?.view()||null}));
app.get('/api/reflection',async()=>({reflection:engine?.reflection()||null}));
app.get('/api/audio/:id',async(request,reply)=>{
  const id=String((request.params as any).id||'');
  const bytes=tts.audio(id);
  if(!bytes)return reply.code(404).send({error:'Not found'});
  return reply.type(tts.config.format==='mp3'?'audio/mpeg':'audio/wav').send(bytes);
});
app.post('/api/save',async()=>{if(engine)database.save(engine.world);return{saved:!!engine};});
app.post('/api/command',async(request,reply)=>{
  try{
    if(!engine)throw new Error('No active session');
    const command=request.body as Command;
    if(command.type==='retry_reply'||command.type==='mode'&&command.online)adapter.reload();
    engine.command(command);
    return {state:engine.view()};
  }catch(error){return reply.code(400).send({error:message(error)});}
});
app.get('/api/events',{websocket:true},socket=>{
  sockets.add(socket);
  socket.send(JSON.stringify({type:'state',version:1,state:engine?.view(true)||null}));
  socket.on('message',buffer=>{
    let commandId='';
    try{
      if(!engine)throw new Error('No active session');
      const command=JSON.parse(buffer.toString()) as Command;
      if(command.type==='retry_reply'||command.type==='mode'&&command.online)adapter.reload();
      commandId=command.id;
      if(process.env.LASTCALL_TRACE==='1'&&!['position','positions'].includes(command.type))
        process.stdout.write(JSON.stringify({type:'trace',message:JSON.stringify({command:command.type,paused:command.paused,before:engine.world.paused,busy:engine.busy,elapsed:engine.world.elapsed})})+'\n');
      engine.command(command);
      socket.send(JSON.stringify({type:'ack',id:command.id,version:1}));
    }catch(error){socket.send(JSON.stringify({type:'error',id:commandId,message:message(error),version:1}));}
  });
  socket.on('close',()=>sockets.delete(socket));
});
function message(error:unknown){
  const value=error instanceof Error?error.message:'Invalid request';
  return value.replace(/sk-[A-Za-z0-9_-]+/g,'[redacted]').slice(0,160);
}
function broadcast(){
  if(!engine)return;
  const payload=JSON.stringify({type:'state',version:1,cursor:engine.world.sequence,state:engine.view()});
  for(const socket of sockets)if(socket.readyState===1)socket.send(payload);
}
let lastTick=performance.now();
let lastSave=Date.now();
const timer=setInterval(async()=>{
  const now=performance.now();
  const dt=Math.min(.25,(now-lastTick)/1000);
  lastTick=now;
  if(closing||!engine)return;
  const testPlayer=['full-night-verification','scene-two-three-verification'].includes(engine.world.playerId);
  const testDirectory=/(FullNightVerification|SceneTwoThreeVerification)$/.test(String(process.env.LASTCALL_DATA_DIR));
  const testClock=testPlayer&&testDirectory?Math.max(1,Math.min(4,Number(process.env.LASTCALL_TEST_CLOCK)||1)):1;
  engine.advance(dt*testClock);
  applyReadyReplies(engine,database);
  if(!engine.world.paused&&engine.world.status==='playing'&&!introActive(engine.world)&&!pendingIntro.has(engine)){
    const active=engine;
    const running=(active.world.replies||[]).filter(r=>r.status==='running').length;
    if(running<2)for(const job of active.dueJobs(2-running))void generateReply(active,adapter,job);
  }
  if(Date.now()-lastSave>4000){database.save(engine.world);lastSave=Date.now();}
},100);
const pushTimer=setInterval(broadcast,150);
async function shutdown(){
  if(closing)return;
  closing=true;
  clearInterval(timer);clearInterval(pushTimer);
  if(engine)database.save(engine.world);
  for(const socket of sockets)socket.close();
  await app.close();
  database.close();
  process.exit(0);
}
app.post('/api/shutdown',async()=>{setTimeout(()=>void shutdown(),100);return{stopping:true};});
process.on('SIGTERM',()=>void shutdown());
process.on('SIGINT',()=>void shutdown());
if(process.argv.includes('--managed')){
  process.stdin.resume();
  process.stdin.on('end',()=>void shutdown());
}
await app.listen({host:'127.0.0.1',port:Number(process.env.LASTCALL_PORT)||0});
const address=app.server.address();
if(address&&typeof address==='object')process.stdout.write(JSON.stringify({ready:true,port:address.port,version:1})+'\n');
