import test from 'node:test';
import assert from 'node:assert/strict';
import {spawn} from 'node:child_process';
import {mkdtempSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {once} from 'node:events';
import {setTimeout as delay} from 'node:timers/promises';

test('Scene0 survives a real backend process restart with private text and event deduplication',async()=>{
  const data=mkdtempSync(join(tmpdir(),'scene0-restart-'));
  const headers={Authorization:'Bearer scene0-integration','Content-Type':'application/json'};
  async function boot(){
    const child=spawn(process.execPath,['dist/server.js','--managed'],{env:{...process.env,
      LASTCALL_SESSION_TOKEN:'scene0-integration',LASTCALL_DATA_DIR:data,LASTCALL_CONFIG_DIR:data},stdio:['pipe','pipe','pipe']});
    const port=await new Promise<number>((resolve,reject)=>{
      const timer=setTimeout(()=>reject(new Error('Startup timeout')),15000);
      child.stdout.on('data',chunk=>{for(const line of chunk.toString().split('\n'))try{const value=JSON.parse(line);if(value.port){clearTimeout(timer);resolve(value.port);}}catch{}});
      child.once('exit',()=>{clearTimeout(timer);reject(new Error('Unexpected exit'));});
    });
    const request=async(path:string,body?:unknown)=>{
      const response=await fetch('http://127.0.0.1:'+port+path,{headers,method:body?'POST':'GET',body:body?JSON.stringify(body):undefined});
      assert.equal(response.status,200);return await response.json() as any;
    };
    return {request,stop:async()=>{const stopped=once(child,'exit');child.stdin.end();await stopped;}};
  }
  let service=await boot();
  try{
    const initial=await service.request('/api/session',{playerId:'restart-test',online:false,opening:'scene0_v1'});
    const id=initial.state.sessionId;
    await service.request('/api/command',{id:'ready',type:'intro_ready'});await delay(800);
    await service.request('/api/command',{id:'pause',type:'pause',paused:true});
    await service.request('/api/command',{id:'text',type:'intro_text',text:'有一点紧张。'});
    const before=(await service.request('/api/state')).state;
    await service.stop();service=await boot();
    const restored=(await service.request('/api/session',{mode:'resume',playerId:'restart-test',sessionId:id})).state;
    assert.equal(restored.intro.progress,before.intro.progress);assert.equal(restored.intro.playerText,'有一点紧张。');
    assert.equal(restored.paused,true);assert.equal(restored.events.length,0);
    await service.request('/api/command',{id:'resume',type:'pause',paused:false});await delay(6600);
    const arrived=(await service.request('/api/command',{id:'complete',type:'intro_complete'})).state;
    assert.equal(arrived.intro.phase,'bar');
    const count=arrived.events.length;
    const duplicate=(await service.request('/api/command',{id:'complete',type:'intro_complete'})).state;
    assert.equal(duplicate.events.length,count);assert.ok(!JSON.stringify(duplicate.events).includes('我等的那位'));
  }finally{await service.stop();}
});
