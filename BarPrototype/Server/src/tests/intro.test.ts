import test from 'node:test';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
import {readFileSync} from 'node:fs';
import {Engine} from '../engine.js';
import {loadScenario} from '../config.js';
import {Navigator} from '../navigation.js';
import {backgroundIntro,introPerception} from '../intro.js';
import {acceptIntroMessage,generateIntro} from '../intro-model.js';
const scenario=loadScenario(fileURLToPath(new URL('../../scenarios/last_call.json',import.meta.url)));
function game(online=false){return new Engine(structuredClone(scenario),{playerId:'scene0-test',opening:'scene0_v1',online,seed:42});}
function finish(g:Engine){g.command({id:'ready',type:'intro_ready'});for(let j=0;j<29;j++)g.advance(.25);g.command({id:'finish',type:'intro_complete'});}
test('Scene 0 is opt-in; legacy initial cast and immediate third drink are unchanged',()=>{
  const old=new Engine(scenario,{playerId:'legacy'});assert.equal(old.world.intro,undefined);assert.equal(old.actor('A').active,false);assert.equal(old.world.events.length,1);
  const g=game();assert.equal(g.world.events.length,0);for(const id of ['A','B','C'])assert.equal(g.actor(id).active,true);assert.equal(g.actor('D').active,false);
});
test('intro waits for client readiness, freezes on pause, and uses a separate seven-second clock',()=>{
  const g=game();g.advance(2);assert.equal(g.world.intro!.progress,0);
  g.command({id:'r',type:'intro_ready'});g.advance(.25);g.command({id:'p',type:'pause',paused:true});g.advance(2);
  assert.equal(g.world.intro!.progress,.25);assert.equal(g.world.elapsed,0);assert.throws(()=>g.command({id:'early',type:'intro_complete'}));
});
test('exactly one background event, different evidence for A/C, no player leak',()=>{
  const g=game();backgroundIntro(g);backgroundIntro(g);
  assert.equal(g.world.events.length,1);const e=g.world.events[0];
  assert.equal(e.perceptions.find(p=>p.actor==='A')?.level,'partial');
  assert.equal(e.perceptions.find(p=>p.actor==='C')?.level,'gesture');
  assert.equal(e.perceptions.some(p=>p.actor==='USER'),false);
  assert.notEqual(g.actor('A').beliefs[0].interpretation,g.actor('C').beliefs[0].interpretation);
  const view=g.view();assert.equal(view.events.length,0);assert.equal(view.cards.length,0);
  assert.equal(JSON.stringify(view).includes('我等的那位'),false);
  assert.equal(JSON.stringify(view).includes('backgroundEventId'),false);
});
test('background perception respects obstruction, distance and facing',()=>{
  const g=game();backgroundIntro(g);const e=g.world.events[0],c=g.actor('C');
  c.yaw=80;assert.equal(introPerception(scenario,g.world,g.navigation,c,e),undefined);
  c.yaw=260;
  const nav=new Navigator({...g.navigation.data,walls:[{x:0,z:0,w:1,h:20}]});
  assert.equal(introPerception(scenario,g.world,nav,c,e),undefined);
});
test('phone and private input are idempotent and never become public dialogue',()=>{
  const g=game();g.command({id:'hide',type:'intro_phone',open:false});g.command({id:'hide',type:'intro_phone',open:true});
  assert.equal(g.world.intro!.phoneVisible,false);
  g.command({id:'words',type:'intro_text',text:'有点紧张，PRIVATE_TEST_442'});
  assert.equal(g.world.intro!.attitude,'hesitant');
  finish(g);assert.equal(g.world.intro!.checkedMessage,false);
  assert.equal(g.world.events.some(e=>e.text.includes('PRIVATE_TEST')),false);
  assert.throws(()=>g.command({id:'long',type:'interact',text:'x'.repeat(201)}));
});
test('handoff starts third drink once and prevents duplicate A/C arrivals',()=>{
  const g=game();finish(g);g.command({id:'duplicate-finish',type:'intro_complete'});
  assert.equal(g.world.events.filter(e=>e.intent==='observe').length,1);
  for(let n=0;n<205;n++)g.advance(2);
  assert.equal(g.world.events.some(e=>e.intent==='arrival'&&['A','C'].includes(e.target)),false);
});
test('mid-intro snapshot preserves message, checkpoint and evidence',()=>{
  const g=game();g.command({id:'ready',type:'intro_ready'});for(let n=0;n<12;n++)g.advance(.25);
  const restored=new Engine(scenario,{playerId:'scene0-test'},g.world);
  assert.equal(restored.world.intro!.progress,3);assert.equal(restored.world.paused,true);
  backgroundIntro(restored);assert.equal(restored.world.events.length,1);
});
test('model invitation rejects leaked names and late results',()=>{
  const g=game(true);assert.equal(acceptIntroMessage(g,{message:'A在等你',hint:'',attitude:'curious'}),false);
  assert.equal(acceptIntroMessage(g,{message:'今晚见。',hint:'给你留了位置。',attitude:'curious'}),true);
  finish(g);assert.equal(acceptIntroMessage(g,{message:'今晚见！',hint:'',attitude:'direct'}),false);
  assert.equal(g.world.intro!.message,'今晚见。');
});
test('gateway failure is a local preset fallback, with budget counted and no world loss',async()=>{
  const g=game(true),prior=globalThis.fetch;globalThis.fetch=async()=>new Response('',{status:401});
  try{await generateIntro(g,{base:'https://invalid.test/v1',model:'gpt-4.1-mini',key:'test-key'});
    assert.equal(g.world.calls,1);assert.equal(g.world.intro!.messageSource,'preset');finish(g);assert.equal(g.world.intro!.phase,'bar');
  }finally{globalThis.fetch=prior;}
});
test('production geometry supports A/C perception and a continuous elevator exit',()=>{
  const nav=new Navigator(JSON.parse(readFileSync(new URL('../../scenarios/navigation.json',import.meta.url),'utf8')));
  const g=new Engine(scenario,{playerId:'geometry',opening:'scene0_v1'},undefined,nav);
  backgroundIntro(g);
  assert.equal(g.world.events[0].perceptions.find(p=>p.actor==='A')?.level,'partial');
  assert.equal(g.world.events[0].perceptions.find(p=>p.actor==='C')?.level,'gesture');
  assert.ok(nav.path({x:-1,z:-8},g.location('entrance')).length>0);
});
test('next night does not replay elevator and retains the new arrival route',()=>{
  const g=game();finish(g);g.finish();const next=g.nextNight();
  assert.equal(next.world.intro?.phase,'bar');assert.equal(next.world.flags.scene0Route,true);
  assert.equal(next.actor('A').active,true);assert.equal(next.actor('C').active,true);
});

test('late network result cannot replace a phone message already on screen',async t=>{
  const g=game(true);let resolve!:(v:Response)=>void;
  t.mock.method(globalThis,'fetch',()=>new Promise<Response>(r=>{resolve=r;}));
  const request=generateIntro(g,{base:'https://invalid.test/v1',model:'gpt-4.1-mini',key:'test-key'});
  g.command({id:'ready',type:'intro_ready'});for(let n=0;n<10;n++)g.advance(.25);
  assert.equal(g.world.intro!.messageLocked,true);
  resolve(Response.json({choices:[{message:{content:JSON.stringify({message:'今晚不见不散。',hint:'路上小心。',attitude:'curious'})}}]}));
  await request;assert.equal(g.world.intro!.messageSource,'preset');assert.equal(g.world.intro!.message,'今晚见。');
  finish(g);assert.equal(g.world.intro!.phase,'bar');
});

test('valid JSON is insufficient; extra fields, long text and identity claims are rejected',()=>{
  for(const candidate of [
    {message:'今晚见。',hint:'',attitude:'curious',sender:'B'},
    {message:'今晚见。'.repeat(9),hint:'',attitude:'curious'},
    {message:'今晚见，你的前任在等你。',hint:'',attitude:'curious'},
    {message:'今晚见。',hint:'',attitude:'permanent_personality'}
  ])assert.equal(acceptIntroMessage(game(true),candidate),false);
});
