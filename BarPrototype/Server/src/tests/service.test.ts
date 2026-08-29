import test from 'node:test';
import assert from 'node:assert/strict';
import {spawn} from 'node:child_process';
import {mkdtempSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join,dirname} from 'node:path';
import {fileURLToPath} from 'node:url';
import WebSocket from 'ws';

test('HTTP ownership, WebSocket commands and reconnect',async()=>{
  const root=dirname(dirname(dirname(fileURLToPath(import.meta.url))));
  const child=spawn(process.execPath,[join(root,'dist/server.js')],{
    env:{...process.env,LASTCALL_SESSION_TOKEN:'integration-token',LASTCALL_DATA_DIR:mkdtempSync(join(tmpdir(),'lastcall-test-')),LASTCALL_CONFIG_DIR:mkdtempSync(join(tmpdir(),'lastcall-test-config-'))},
    stdio:['pipe','pipe','pipe']
  });
  const port=await new Promise<number>((resolve,reject)=>{
    const timeout=setTimeout(()=>reject(new Error('Server startup timeout')),15000);
    child.stdout.on('data',data=>{
      try{const ready=JSON.parse(data.toString());if(ready.port){clearTimeout(timeout);resolve(ready.port);}}catch{}
    });
    child.on('exit',()=>reject(new Error('Server exited before ready')));
  });
  const base='http://127.0.0.1:'+port;
  const headers={Authorization:'Bearer integration-token','Content-Type':'application/json'};
  let socket:WebSocket|undefined;
  try{
    const denied=await fetch(base+'/api/state');
    assert.equal(denied.status,401);
    const opened=await fetch(base+'/api/session',{method:'POST',headers,body:JSON.stringify({playerId:'owner',role:'passerby',online:false})});
    const session=await opened.json() as any;
    assert.equal(session.state.mode,'offline');
    socket=new WebSocket(base.replace('http:','ws:')+'/api/events',{headers});
    await new Promise<void>(resolve=>socket!.once('open',resolve));
    const ack=new Promise<void>((resolve,reject)=>{
      const timeout=setTimeout(()=>reject(new Error('No command ACK')),5000);
      socket!.on('message',data=>{const response=JSON.parse(data.toString());if(response.type==='ack'&&response.id==='observe-one'){clearTimeout(timeout);resolve();}});
    });
    socket.send(JSON.stringify({id:'observe-one',type:'observe'}));
    await ack;
    socket.close();
    const duplicate=await fetch(base+'/api/command',{method:'POST',headers,body:JSON.stringify({id:'observe-one',type:'observe'})});
    assert.equal(duplicate.status,200);
    const view=await duplicate.json() as any;
    assert.ok(view.state.events.length>=2);
    const replay=await new Promise<any>((resolve,reject)=>{
      const timeout=setTimeout(()=>reject(new Error('No reconnect snapshot')),5000);
      socket=new WebSocket(base.replace('http:','ws:')+'/api/events',{headers});
      socket.on('message',data=>{
        const message=JSON.parse(data.toString());
        if(message.type==='state'){clearTimeout(timeout);resolve(message.state);}
      });
    });
    assert.ok(replay.events.some((event:any)=>event.id===view.state.events[0].id));
    const previousCount=replay.events.length;
    await fetch(base+'/api/command',{method:'POST',headers,body:JSON.stringify({id:'observe-one',type:'observe'})});
    const afterReconnect=await fetch(base+'/api/state',{headers});
    assert.equal((await afterReconnect.json() as any).state.events.length,previousCount);
    await fetch(base+'/api/save',{method:'POST',headers,body:'{}'});
    const wrong=await fetch(base+'/api/session',{method:'POST',headers,body:JSON.stringify({mode:'resume',playerId:'intruder',sessionId:session.state.sessionId})});
    assert.equal(wrong.status,404);
    const resumed=await fetch(base+'/api/session',{method:'POST',headers,body:JSON.stringify({mode:'resume',playerId:'owner',sessionId:session.state.sessionId})});
    assert.equal(resumed.status,200);
    assert.equal((await resumed.json() as any).state.paused,true);
  }finally{
    socket?.terminate();
    await fetch(base+'/api/shutdown',{method:'POST',headers,body:'{}'}).catch(()=>{});
    setTimeout(()=>child.kill(),1500).unref();
  }
});
