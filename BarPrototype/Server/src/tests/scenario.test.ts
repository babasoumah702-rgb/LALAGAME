import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {Engine} from '../engine.js';
import {Navigator,distance} from '../navigation.js';
import {loadScenario} from '../config.js';
import {Store} from '../store.js';
const scenario=loadScenario('scenarios/last_call.json');
const navigation=new Navigator(JSON.parse(readFileSync('scenarios/navigation.json','utf8')));
const create=(role='friend_guest')=>new Engine(scenario,{playerId:'simulation',seed:821,role,online:false},undefined,navigation);
function pump(g:Engine){
  g.advance(.2);
  for(const a of g.world.actors.filter(a=>a.active&&a.route.length)){
    const p=a.route[0];
    g.command({id:'position',type:'position',actor:a.id,x:p.x,z:p.z,yaw:a.yaw});
  }
  for(const job of g.dueJobs())g.apply(job.actor,g.rule(job.actor,job.eventId),job.eventId);
}
test('every configured location is reachable on the exported Unity grid',()=>{
  const origin=navigation.nearest(scenario.locations[0]);
  for(const place of scenario.locations.slice(1)){
    const end=navigation.nearest(place);
    assert.ok(navigation.path(origin,end).length,place.id);
  }
});
for(const role of scenario.roles){
  test('full 720-second offline night with navigation: '+role.id,()=>{
    const g=create(role.id);
    g.command({id:'approach',type:'approach_target',target:'B'});
    let submitted=false,declined=false;
    for(let frame=0;frame<3700&&g.world.status==='playing';frame++){
      pump(g);
      if(!submitted&&distance(g.actor('USER'),g.actor('B'))<2.8&&g.navigation.visible(g.actor('USER'),g.actor('B'))){
        g.command({id:'expression',type:'card',card:'reveal',target:'B',text:'今晚我其实是为了见你。'});
        submitted=true;
      }
      if(g.world.flags.cardsOffered&&!declined){g.command({id:'decline',type:'decline'});declined=true;}
    }
    assert.equal(g.world.status,'ended');assert.equal(g.world.elapsed,720);
    assert.ok(submitted);assert.ok(declined);assert.ok(g.world.beatIds.includes('a_arrival'));
    assert.equal(g.world.calls,0);assert.ok(g.reflection().events.length);
    assert.ok(g.world.events.some(e=>e.actor!=='USER'&&e.parentId));
  });
}
test('seat reservations do not allocate the same endpoint',()=>{
  const g=create();const target=g.location('bar');
  g.go(g.actor('USER'),target);g.go(g.actor('B'),target);
  assert.ok(distance(g.actor('USER').route.at(-1)!,g.actor('B').route.at(-1)!)>=.55);
});
test('pending speech waits for physical arrival',()=>{
  const g=create();const e=g.emit('message','USER','B','reveal','能过来聊聊吗？');
  g.actor('B').x=5.375;g.actor('B').z=-2.375;
  const count=g.world.events.length;g.apply('B',g.rule('B',e.id),e.id);
  assert.equal(g.world.events.length,count);assert.ok(g.actor('B').pending);
  for(let i=0;i<200&&g.actor('B').pending;i++)pump(g);
  assert.ok(g.world.events.some(item=>item.actor==='B'&&item.parentId===e.id));
});
test('old observation actions cannot trigger D later',()=>{
  const g=create();const p=navigation.nearest(g.location('seat13'));Object.assign(g.actor('USER'),p);
  for(let i=0;i<3;i++)g.command({id:'old'+i,type:'observe'});
  g.world.elapsed=600;g.advance(.2);
  assert.equal(g.actor('D').active,false);
});
test('decision records are owner isolated and preserved',()=>{
  const g=create();const e=g.emit('message','USER','B','probe','你好吗？');
  const job={actor:'B',eventId:e.id,due:0};const db=new Store(':memory:');
  db.save(g.world);db.recordDecision(g.world,job,g.rule('B',e.id),true);
  assert.equal(db.decisions(g.world.id,'simulation').length,1);
  assert.equal(db.decisions(g.world.id,'another-player').length,0);db.close();
});
test('entry willingness adjusts invitations, not relationship history',()=>{
  const early=new Engine(scenario,{playerId:'early',entryIntent:'meet_people'});
  const late=new Engine(scenario,{playerId:'late',entryIntent:'low_energy'});
  early.world.elapsed=150;late.world.elapsed=150;early.advance(.2);late.advance(.2);
  assert.equal(early.world.flags.cardsOffered,true);assert.equal(!!late.world.flags.cardsOffered,false);
  assert.deepEqual(early.actor('A').relations.USER,late.actor('A').relations.USER);
});
test('same seed reproduces an entire rules-mode event sequence',()=>{
  function run(){
    const g=create();g.command({id:'go',type:'approach_target',target:'B'});
    let spoke=false;
    for(let i=0;i<1200;i++){
      pump(g);
      if(!spoke&&distance(g.actor('USER'),g.actor('B'))<2.7){
        g.command({id:'talk',type:'card',card:'reveal',target:'B',text:'今晚想见你。'});spoke=true;
      }
    }
    return g.world.events.map(e=>({time:e.time,actor:e.actor,target:e.target,text:e.text,depth:e.depth}));
  }
  assert.deepEqual(run(),run());
});
test('alternate declarative scenario starts an independent world',()=>{
  const changed=structuredClone(scenario);changed.id='different-night';changed.title='另一个夜晚';
  changed.actors.find(a=>a.id==='B')!.voice='更简短、直接的表达。';
  const g=new Engine(changed,{playerId:'alternate',online:false},undefined,navigation);
  assert.equal(g.world.scenarioId,'different-night');
  assert.equal(g.actor('B').voice,'更简短、直接的表达。');
  assert.notEqual(g.world.id,create().world.id);
});
