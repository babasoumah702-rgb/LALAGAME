import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {Engine} from '../engine.js';
import {loadScenario} from '../config.js';
import {Navigator,distance} from '../navigation.js';
import {ModelAdapter} from '../model.js';
import {applyReadyReplies} from '../reply-runtime.js';
import {facePair,SEAT} from '../scene-one.js';
const scenario=loadScenario('scenarios/last_call.json');
const nav=new Navigator(JSON.parse(readFileSync('scenarios/navigation.json','utf8')));
function game(online=false){const g=new Engine(scenario,{playerId:'scene-one-test',story:'scene1_v1',online},undefined,nav);g.advance(0);return g;}
function tick(g:Engine,seconds:number){for(let n=0;n<seconds*4;n++){
  for(const a of g.world.actors){if(!a.active||!a.route.length)continue;const p=a.route[0],d=distance(a,p),step=Math.min(d,.5);g.command({id:'pos',type:'position',actor:a.id,x:a.x+(p.x-a.x)*step/Math.max(d,.001),z:a.z+(p.z-a.z)*step/Math.max(d,.001),yaw:a.yaw});}
  g.advance(.25);
}}
function nearby(g:Engine,id='B'){Object.assign(g.actor('USER'),g.navigation.nearest({x:g.actor(id).x-.7,z:g.actor(id).z}));}

test('third drink is delivered once through the production navigation and D waits for an interaction',()=>{
  const g=game();tick(g,130);assert.ok(g.world.scene1!.drinkEventId);assert.equal(g.actor('D').active,false);
  g.command({id:'observe',type:'observe'});tick(g,12);
  assert.equal(g.world.scene1!.phase,'scene2_ready');assert.equal(g.actor('D').active,true);
  assert.equal(g.world.events.filter(e=>e.intent==='reserved_drink'&&e.text.includes('杯底')).length,1);
  assert.equal(g.world.events.filter(e=>e.actor==='D'&&e.intent==='arrival').length,1);
  tick(g,750);assert.equal(g.world.status,'playing');assert.equal(g.world.flags.lastCall,undefined);
});
test('new chapter is opt-in and resume never replays the cup or inserts it in an old save',()=>{
  const old=new Engine(scenario,{playerId:'old'});assert.equal(old.world.scene1,undefined);
  const g=game();tick(g,65);const id=g.world.scene1!.drinkEventId;
  const restored=new Engine(scenario,{playerId:'scene-one-test'},g.world,nav);assert.equal(restored.world.paused,true);
  restored.command({id:'unpause',type:'pause',paused:false});tick(restored,20);assert.equal(restored.world.scene1!.drinkEventId,id);
});
for(const id of ['A','B','C'])test('first conversation targets '+id+' without automatically disclosing names',()=>{
  const g=game();nearby(g,id);g.command({id:'talk',type:'talk',target:id,text:'你好，能在这里聊一会吗？'});
  assert.equal(g.world.scene1!.firstTarget,id);assert.equal(g.world.scene1!.knownNames[id],undefined);
  const speech=g.world.events.at(-1)!;g.emit('speech',id,'USER','introduce','我叫'+g.actor(id).name+'。',speech.id,'normal','','ai');
  assert.ok(g.world.scene1!.knownNames[id]);assert.equal(g.view().characters.find(a=>a.id===id)!.name,g.actor(id).name);
});
test('unheard introductions and gestures never disclose a name or private text',()=>{
  const g=game();Object.assign(g.actor('USER'),{x:-7,z:-3});
  g.emit('speech','B','A','introduce','我叫X。','','normal','','ai');assert.equal(g.world.scene1!.knownNames.B,undefined);
  assert.ok(!JSON.stringify(g.view()).includes('firstImpressions'));
});
test('reserved seat requires proximity and records an action rather than a personality judgment',()=>{
  const g=game();assert.throws(()=>g.command({id:'far',type:'sit_reserved'}));
  Object.assign(g.actor('USER'),nav.nearest(SEAT));g.command({id:'sit',type:'sit_reserved'});
  assert.equal(g.world.scene1!.occupiedReservedSeat,true);assert.equal(g.world.scene1!.firstSocialMove,'sit');
});
test('conversation facing survives client yaw reports and releases on movement',()=>{
  const g=game();nearby(g);facePair(g,'USER','B');const yaw=g.actor('B').yaw;
  g.command({id:'pos',type:'position',actor:'B',x:g.actor('B').x,z:g.actor('B').z,yaw:0});assert.equal(g.actor('B').yaw,yaw);
  g.command({id:'move',type:'cancel_move'});assert.equal(g.actor('B').conversationTarget,'');
});
test('missing key keeps the original event and retry is idempotent while the world keeps running',async()=>{
  const g=game(true);nearby(g);g.command({id:'talk',type:'talk',target:'B',text:'我想见你'});
  const e=g.world.events.at(-1)!,job={actor:'B',eventId:e.id,due:0},adapter=new ModelAdapter();adapter.config.key='';
  await assert.rejects(adapter.decide(g,job),{code:'NO_KEY'});const before=g.world.elapsed;g.advance(1);assert.ok(g.world.elapsed>before);
  const requestId=g.world.replies![0].id;g.command({id:'retry',type:'retry_reply',requestId});g.command({id:'retry2',type:'retry_reply',requestId});
  assert.equal(g.world.jobs.filter(j=>j.actor==='B'&&j.eventId===e.id).length,1);
  assert.equal(g.world.events.filter(e=>e.actor==='USER'&&e.type==='speech').length,1);
});
test('repeated AI response is regenerated once, accepted source stays AI, and pause defers application',async t=>{
  const g=game(true);nearby(g);const old=g.emit('speech','B','USER','chat','这句话我收下了。','','normal','','ai');
  const e=g.emit('speech','USER','B','reveal','我想见你。','','normal','','player'),adapter=new ModelAdapter();adapter.config.key='synthetic';let calls=0;
  t.mock.method(globalThis,'fetch',async()=>Response.json({choices:[{message:{content:JSON.stringify({action:'speak',target:'USER',intent:'chat',expression:++calls===1?old.text:'嗯，那先坐过来聊一会。',interpretation:'接住表达',evidenceIds:[e.id],signal:'warm',confidence:.7})}}]}));
  await adapter.decide(g,{actor:'B',eventId:e.id,due:0});g.world.paused=true;applyReadyReplies(g);assert.equal(g.world.events.at(-1)!.id,e.id);
  g.world.paused=false;applyReadyReplies(g);assert.equal(calls,2);assert.equal(g.world.events.at(-1)!.generationSource,'ai');
  const count=g.world.events.length;applyReadyReplies(g);assert.equal(g.world.events.length,count);
});
test('ordinary dialogue context includes authors and everyday voice without raw background',()=>{
  const g=game();nearby(g);const e=g.emit('speech','USER','B','chat','我想见你');const c=g.context('B',e.id);
  assert.equal(c.conversation.at(-1)!.speaker,'USER');assert.ok(c.identity!.userDefault.includes('不把玩家自动当项目'));assert.ok(c.identity!.everyday.life.length);
});

test('unknown names stay hidden in partial hearing, private note gestures and reflection',()=>{
  const g=game();Object.assign(g.actor('USER'),{x:-.8,z:-2.2,yaw:90});
  g.emit('speech','B','A','chat','今晚下雨了。');
  g.emit('speech','B','A','chat','这张纸条只给你看。','','private');
  const text=JSON.stringify(g.view().events);
  assert.ok(!text.includes('X'));assert.ok(!text.includes('这张纸条只给你看'));
  assert.ok(!JSON.stringify(g.reflection()).includes('X'));
});

test('scene actions require sight, and an idle NPC does not accept stale client facing',()=>{
  const g=game();const c=g.actor('C');Object.assign(c,{x:3.4,z:1.2,yaw:0});
  const event=g.emit('action','USER','B','observe','你看向桌边。','','normal','','player');
  assert.equal(event.perceptions.some(p=>p.actor==='C'),false);
  const yaw=g.actor('B').yaw;g.command({id:'idle',type:'position',actor:'B',x:g.actor('B').x,z:g.actor('B').z,yaw:0});assert.equal(g.actor('B').yaw,yaw);
});

test('a second identical AI response fails without substituting a rules sentence',async t=>{
  const g=game(true);nearby(g);g.emit('speech','B','USER','chat','我听见了。');
  const e=g.emit('speech','USER','B','chat','我想见你'),adapter=new ModelAdapter();adapter.config.key='synthetic';let calls=0;
  t.mock.method(globalThis,'fetch',async()=>{calls++;return Response.json({choices:[{message:{content:JSON.stringify({action:'speak',target:'USER',intent:'chat',expression:'我听见了。',interpretation:'接话',evidenceIds:[e.id],signal:'neutral',confidence:.7})}}]});});
  await assert.rejects(adapter.decide(g,{actor:'B',eventId:e.id,due:0}),{code:'REPEATED'});applyReadyReplies(g);
  assert.equal(calls,2);assert.equal(g.world.events.at(-1)!.id,e.id);assert.equal(g.world.replies![0].errorCode,'REPEATED');
});

test('a retry does not charge a card twice',async()=>{
  const g=game(true);nearby(g);g.world.modelMode='offline';
  const card=scenario.cards.find(c=>c.type==='social')!;
  g.command({id:'card',type:'card',card:card.id,target:'B'});const cooldown=g.world.cooldowns[card.id],e=g.world.events.at(-1)!;
  const adapter=new ModelAdapter();const job={actor:'B',eventId:e.id,due:0};
  await adapter.decide(g,job);
  g.command({id:'retry',type:'retry_reply',requestId:g.world.replies![0].id});
  assert.equal(g.world.cooldowns[card.id],cooldown);assert.equal(g.world.moves[card.intent],1);
});

test('online photo request cannot override an existing refusal',()=>{
  const g=game(true);nearby(g,'A');g.world.flags.cardsJoined=true;
  const card=scenario.cards.find(c=>c.effect==='photo')!;g.command({id:'photo',type:'card',card:card.id,target:'A'});
  const e=g.world.events.at(-1)!;assert.equal(e.objectTarget,'photo_request');
  assert.equal(g.apply('A',{...g.rule('A',e.id),target:'USER',signal:'warm',expression:'好，一起拍吧。'},e.id),false);
  assert.equal(g.world.events.at(-1)!.id,e.id);
});
