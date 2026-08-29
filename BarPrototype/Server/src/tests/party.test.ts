import test from 'node:test';
import assert from 'node:assert/strict';
import {Engine} from '../engine.js';
import {loadScenario} from '../config.js';
import {runBeats} from '../beats.js';
const scenario=loadScenario('scenarios/last_call.json');
function start(role='passerby'){
  const g=new Engine(scenario,{playerId:'party-test',role,online:false,opening:'scene0_v1'});
  g.command({id:'ready',type:'intro_ready'});for(let n=0;n<28;n++)g.advance(.25);
  g.command({id:'enter',type:'intro_complete'});return g;
}
for(const role of scenario.roles)test('every entry can request a party immediately: '+role.id,()=>{
  const g=start(role.id);assert.equal(g.world.elapsed,0);
  assert.match(g.view().cards.find(c=>c.id==='truth')!.lockReason,/加入/);
  g.command({id:'start',type:'start_party'});
  assert.equal(g.world.flags.cardsJoined,true);assert.equal(g.world.flags.cardsOffered,true);
  assert.equal(g.view().cards.find(c=>c.id==='truth')!.ready,true);
  assert.equal(g.view().cards.find(c=>c.id==='last_call')!.ready,false);
});
test('opening a party is idempotent and prevents the later duplicate invitation',()=>{
  const g=start();g.command({id:'start',type:'start_party'});const count=g.world.events.length;
  g.command({id:'start',type:'start_party'});g.command({id:'new-start',type:'start_party'});
  assert.equal(g.world.events.length,count);
  g.world.elapsed=180;runBeats(g);
  assert.equal(g.world.events.filter(e=>e.intent==='invite').length,1);
});
test('join, play, cooldown, independent response, decline and rejoin complete a real card flow',()=>{
  const g=start();g.command({id:'join',type:'start_party'});
  Object.assign(g.actor('USER'),{x:-2.2,z:-1});
  g.command({id:'truth',type:'card',card:'truth',target:'B',text:'我可以先不回答吗？'});
  const e=g.world.events.at(-1)!;assert.equal(e.actor,'USER');
  assert.equal(g.apply('B',g.rule('B',e.id),e.id),true);
  assert.ok(g.view().events.some(p=>p.actor==='B'&&p.hasParent));
  assert.equal(g.view().cards.find(c=>c.id==='truth')!.ready,false);
  assert.match(g.view().cards.find(c=>c.id==='truth')!.lockReason,/冷却/);
  assert.throws(()=>g.command({id:'again',type:'card',card:'truth',target:'B'}),/冷却/);
  g.command({id:'out',type:'decline'});assert.equal(g.view().cards.find(c=>c.id==='drink')!.ready,false);
  g.command({id:'back',type:'start_party'});assert.equal(g.view().cards.find(c=>c.id==='drink')!.ready,true);
});
test('party consent does not bypass elevator, pause, distance, visibility or Last Call',()=>{
  const intro=new Engine(scenario,{playerId:'blocked',opening:'scene0_v1'});
  assert.throws(()=>intro.command({id:'early',type:'start_party'}),/电梯/);
  const g=start();g.command({id:'pause',type:'pause',paused:true});
  assert.throws(()=>g.command({id:'during-pause',type:'start_party'}));
  g.command({id:'resume',type:'pause',paused:false});g.command({id:'start',type:'start_party'});
  assert.throws(()=>g.command({id:'far',type:'card',card:'truth',target:'B'}),/走近/);
  Object.assign(g.actor('USER'),{x:-2.2,z:-1});
  assert.throws(()=>g.command({id:'too-early',type:'card',card:'last_call',target:'B'}),/Last Call/);
});
test('a queued walk can be cancelled while waiting for a reply or while paused',()=>{
  const g=start();g.actor('USER').route=[{x:0,z:0}];g.actor('USER').destination='bar';
  g.busy=true;g.world.paused=true;g.command({id:'stop',type:'cancel_move'});
  assert.deepEqual(g.actor('USER').route,[]);assert.equal(g.actor('USER').destination,'');
  assert.equal(g.world.paused,true);assert.equal(g.world.elapsed,0);
});
