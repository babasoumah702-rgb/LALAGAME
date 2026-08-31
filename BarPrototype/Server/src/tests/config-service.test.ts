import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdtempSync,readFileSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {spawn} from 'node:child_process';
import {once} from 'node:events';
import {Engine} from '../engine.js';
import {Store} from '../store.js';
import {loadScenario} from '../config.js';

test('config refresh repairs a missing-key fallback save without overriding deliberate offline mode',async()=>{
  const dir=mkdtempSync(join(tmpdir(),'lastcall-config-service-'));
  const saved=new Engine(loadScenario('scenarios/last_call.json'),{playerId:'config-test',online:false});
  saved.world.modelReason='未配置密钥，已使用规则模式';
  const store=new Store(join(dir,'last-call.db'));store.save(saved.world);store.close();
  const child=spawn(process.execPath,['dist/server.js','--managed'],{env:{...process.env,LASTCALL_DATA_DIR:dir,LASTCALL_CONFIG_DIR:dir,LASTCALL_SESSION_TOKEN:'synthetic-local-token'},stdio:['pipe','pipe','pipe']});
  const port=await new Promise<number>((resolve,reject)=>{
    const timeout=setTimeout(()=>reject(new Error('startup timeout')),15000);
    child.stdout.on('data',b=>{for(const line of b.toString().split('\n'))try{const r=JSON.parse(line);if(r.port){clearTimeout(timeout);resolve(r.port);}}catch{}});
    child.once('exit',()=>{clearTimeout(timeout);reject(new Error('early exit'));});
  });
  const request=async(path:string,body?:unknown)=>{
    const r=await fetch('http://127.0.0.1:'+port+path,{method:body?'POST':'GET',headers:{Authorization:'Bearer synthetic-local-token','Content-Type':'application/json'},body:body?JSON.stringify(body):undefined});
    assert.equal(r.status,200);return await r.json() as any;
  };
  try{
    assert.equal((await request('/api/bootstrap')).modelConfigured,false);
    const resume={mode:'resume',sessionId:saved.world.id,playerId:'config-test'};
    assert.equal((await request('/api/session',resume)).state.mode,'offline');
    const configured=await request('/api/model-config',{base:'https://gateway.example/v1',model:'demo-model',key:'synthetic-test-only'});
    assert.equal(configured.configured,true);assert.equal(configured.base,'https://gateway.example/v1');assert.equal(configured.model,'demo-model');
    assert.equal(JSON.stringify(configured).includes('synthetic-test-only'),false,'secret is never returned');
    assert.match(readFileSync(join(dir,'model.env'),'utf8'),/LASTCALL_API_KEY=synthetic-test-only/);
    const kept=await request('/api/model-config',{base:'https://gateway.example/v2/',model:'demo-model-2',keepKey:true});
    assert.equal(kept.configured,true);assert.equal(kept.base,'https://gateway.example/v2');
    const bootstrap=await request('/api/bootstrap');
    assert.equal(bootstrap.modelConfigured,true);assert.equal(bootstrap.model,'demo-model-2');assert.equal(bootstrap.modelBase,'https://gateway.example/v2');
    const restored=(await request('/api/session',resume)).state;
    assert.equal(restored.mode,'online');assert.equal(restored.paused,true);assert.equal(restored.calls,0);
    await request('/api/command',{id:'offline',type:'mode',online:false});
    assert.equal((await request('/api/session',resume)).state.mode,'offline');
    const cleared=await request('/api/model-config',{base:'https://gateway.example/v2',model:'demo-model-2',clearKey:true});
    assert.equal(cleared.configured,false);assert.equal(JSON.stringify(cleared).includes('synthetic-test-only'),false);
  }finally{const stopped=once(child,'exit');child.stdin.end();await stopped;}
});
