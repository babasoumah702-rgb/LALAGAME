import test from 'node:test';
import assert from 'node:assert/strict';
import {join,dirname} from 'node:path';
import {fileURLToPath} from 'node:url';
import {Engine} from '../engine.js';
import {loadScenario} from '../config.js';
import {Store} from '../store.js';
import {Navigator,emptyNavigation} from '../navigation.js';
import {runBeats} from '../beats.js';
import {ModelAdapter} from '../model.js';
const root=dirname(dirname(dirname(fileURLToPath(import.meta.url))));
const scenario=loadScenario(join(root,'scenarios','last_call.json'));
function game(role='friend_guest'){return new Engine(structuredClone(scenario),{playerId:'test-player',role,seed:44,online:false});}
function place(g:Engine,id:string,x:number,z:number){Object.assign(g.actor(id),{x,z,active:true,route:[],pending:undefined});}
function speech(g:Engine){place(g,'USER',0,0);place(g,'B',1,0);return g.emit('speech','USER','B','reveal','今晚我其实是为了见你。');}
for(const role of scenario.roles){
  test('entry and finish: '+role.id,()=>{
    const g=game(role.id);
    assert.equal(g.world.role,role.id);
    g.command({id:'leave',type:'leave'});
    assert.equal(g.view().status,'ended');
    assert.ok(g.reflection().behavior);
  });
}
test('unseen character cannot read an event',()=>{
  const g=game();const event=speech(g);
  assert.equal(g.actor('A').active,false);
  assert.throws(()=>g.context('A',event.id));
  assert.equal(g.actor('A').memory.length,0);
});
test('same signal creates distinct local interpretations',()=>{
  const g=game();place(g,'A',-1,0);
  const event=speech(g);
  assert.notEqual(g.rule('A',event.id).interpretation,g.rule('B',event.id).interpretation);
  assert.equal(JSON.stringify(g.context('A',event.id)).includes('user_b_attraction'),false);
});
test('private note reveals gesture, not content, to bystander',()=>{
  const g=game();place(g,'USER',0,0);place(g,'B',1,0);place(g,'BARTENDER',0,-1);
  g.actor('BARTENDER').yaw=0;
  const e=g.emit('speech','USER','B','reveal','SECRET_NOTE_129','','private');
  const p=e.perceptions.find(p=>p.actor==='BARTENDER')!;
  assert.equal(p.level,'gesture');
  assert.equal(p.text.includes('SECRET_NOTE_129'),false);
  assert.equal(e.perceptions.find(p=>p.actor==='B')!.text,'SECRET_NOTE_129');
});
test('boundary cannot be bypassed by attraction',()=>{
  const g=game();place(g,'USER',0,0);place(g,'A',1,0);
  g.actor('A').relations.USER.attraction=1;g.world.flags.cardsJoined=true;
  g.command({id:'photo',type:'card',card:'photo',target:'A',text:'可以合照吗？'});
  assert.ok(g.world.events.some(e=>e.actor==='A'&&e.intent==='boundary'));
});
test('command idempotency, length limit and distance',()=>{
  const g=game();place(g,'USER',0,0);place(g,'B',1,0);
  const c={id:'once',type:'card',card:'reveal',target:'B',text:'今晚很想见你'};
  g.command(c);g.command(c);
  assert.equal(g.world.moves.reveal,1);
  assert.throws(()=>g.command({...c,id:'large',card:'probe',text:'x'.repeat(201)}));
  place(g,'B',5,4);
  assert.throws(()=>g.command({...c,id:'far',card:'approach'}));
});
test('refusing cards does not stop the night',()=>{
  const g=game();g.world.elapsed=180;runBeats(g);
  g.command({id:'decline',type:'decline'});
  g.advance(1);assert.equal(g.world.elapsed,181);
  g.command({id:'exit',type:'leave'});
  assert.equal(g.world.status,'ended');
});
test('conditional C and D entry, not unconditional spawning',()=>{
  const g=game();g.world.elapsed=400;runBeats(g);assert.equal(g.actor('C').active,false);
  const seat=g.location('seat13');place(g,'USER',seat.x,seat.z);g.world.flags.pastDrink=true;runBeats(g);assert.equal(g.actor('C').active,true);
  g.world.elapsed=600;
  for(let i=0;i<3;i++)g.command({id:'observe'+i,type:'observe'});
  runBeats(g);assert.equal(g.actor('D').active,true);
});
test('pause freezes world time',()=>{const g=game();g.command({id:'pause',type:'pause',paused:true});g.advance(1);assert.equal(g.world.elapsed,0);});
test('invalid evidence and targets cannot mutate state',()=>{
  const g=game(),e=speech(g),d=g.rule('B',e.id);
  assert.equal(g.apply('B',{...d,evidenceIds:['not-known']},e.id),false);
  assert.equal(g.apply('B',{...d,target:'imaginary'},e.id),false);
});
test('delayed autonomous relay keeps causal source',()=>{
  const g=game();place(g,'BARTENDER',2,0);const e=speech(g);
  g.world.elapsed=20;
  const decision=g.rule('B',e.id);
  assert.equal(decision.action,'share');
  assert.equal(decision.target,'BARTENDER');
  assert.equal(g.apply('B',decision,e.id),true);
  const relay=g.world.events.at(-1)!;
  assert.equal(relay.parentId,e.id);assert.equal(relay.evidenceId,e.id);
  g.actor('B').active=false;
  assert.ok(g.actor('BARTENDER').memory.some(m=>m.eventId===relay.id));
});
test('save ownership, resume and next night preserve memories',()=>{
  const g=game(),e=speech(g);g.apply('B',g.rule('B',e.id),e.id);
  const db=new Store(':memory:');db.save(g.world);
  assert.equal(db.load(g.world.id,'someone-else'),undefined);
  const saved=db.load(g.world.id,'test-player')!;
  assert.deepEqual(saved.actors,JSON.parse(JSON.stringify(g.world.actors)));
  const resumed=new Engine(scenario,{playerId:'test-player'},saved);
  assert.equal(resumed.world.paused,true);
  const next=resumed.nextNight();assert.equal(next.world.night,2);
  assert.notEqual(next.world.id,g.world.id);
  assert.ok(next.actor('B').memory.some(m=>m.tier==='long'));
  db.close();
});
test('seed reproduces rules and source logs',()=>{
  const one=game(),two=game();
  const a=speech(one),b=speech(two);
  const x=one.rule('B',a.id),y=two.rule('B',b.id);
  assert.deepEqual({...x,evidenceIds:[]},{...y,evidenceIds:[]});
});
test('player view never serializes private minds or world facts',()=>{
  const g=game(),e=speech(g);g.apply('B',g.rule('B',e.id),e.id);
  const text=JSON.stringify(g.view());
  for(const word of ['knownFacts','beliefs','interpretation','relations','user_b_attraction'])assert.equal(text.includes('"'+word+'"'),false);
});
test('A star avoids obstacles and blocked corners',()=>{
  const nav=new Navigator({...emptyNavigation,minX:0,minZ:0,width:5,height:5,cell:1,blocked:[6,7,8,11,13,16,17,18]});
  const path=nav.path({x:.5,z:.5},{x:4.5,z:4.5});
  assert.ok(path.length>0);assert.ok(path.every(p=>nav.walkable(p)));
});
test('model request budget forces explicit offline mode without fetch',async()=>{
  const g=game(),e=speech(g);g.world.modelMode='online';g.world.calls=80;
  const model=new ModelAdapter();model.config.key='test-key';
  await model.decide(g,{actor:'B',eventId:e.id,due:0});
  assert.equal(g.world.modelMode,'offline');assert.equal(g.world.calls,80);
});
test('exhausted event action budget cannot change relationships',()=>{
  const g=game(),e=speech(g);
  for(let i=0;i<8;i++)g.emit('movement','B','USER','probe','B 看了看附近。',e.id);
  const before=structuredClone(g.actor('B').relations);
  g.apply('B',g.rule('B',e.id),e.id);
  assert.deepEqual(g.actor('B').relations,before);
});
test('old reliable command remains idempotent throughout a world',()=>{
  const g=game();g.command({id:'first',type:'observe'});
  for(let i=0;i<2100;i++)g.command({id:'pause'+i,type:'pause',paused:false});
  g.command({id:'first',type:'observe'});
  assert.equal(g.world.moves.observe,1);
});
test('batched position reports respect pause and finite coordinates',()=>{
  const g=game();place(g,'USER',0,0);
  g.command({id:'batch',type:'positions',items:[{id:'p',type:'position',actor:'USER',x:.1,z:.1}]});
  assert.equal(g.actor('USER').x,.1);
  g.world.paused=true;
  g.command({id:'batch2',type:'positions',items:[{id:'p',type:'position',actor:'USER',x:.2,z:.2}]});
  assert.equal(g.actor('USER').x,.1);
});
