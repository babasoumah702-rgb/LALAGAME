// Opt-in integration probe. Uses the configured gateway only with --online.
import assert from 'node:assert/strict';
import {readFileSync,writeFileSync,mkdirSync} from 'node:fs';
import {join} from 'node:path';
import {Engine} from './engine.js';
import {Navigator,distance} from './navigation.js';
import {ModelAdapter,dataRoot} from './model.js';
import {loadScenario} from './config.js';
import {Store} from './store.js';
const scenario=loadScenario('scenarios/last_call.json');
const navigation=new Navigator(JSON.parse(readFileSync('scenarios/navigation.json','utf8')));
const online=process.argv.includes('--online');
const g=new Engine(scenario,{playerId:'live-night-verification',seed:821,online},undefined,navigation);
const model=new ModelAdapter();
const privateOutput=join(dataRoot,'Verification');mkdirSync(privateOutput,{recursive:true});
const db=new Store(join(privateOutput,'live-night.db'));
g.command({id:'approach',type:'approach_target',target:'B'});
let spoke=false,declined=false,accepted=0,rejected=0;
const started=Date.now();
for(let i=0;i<3700&&g.world.status==='playing';i++){
  g.advance(.2);
  for(const a of g.world.actors.filter(a=>a.active&&a.route.length)){
    const p=a.route[0];
    g.command({id:'position',type:'position',actor:a.id,x:p.x,z:p.z,yaw:a.yaw});
  }
  if(!spoke&&distance(g.actor('USER'),g.actor('B'))<2.7&&navigation.visible(g.actor('USER'),g.actor('B'))){
    g.command({id:'hello',type:'card',target:'B',card:'reveal',text:'今晚我其实是为了见你。'});
    spoke=true;
  }
  if(!declined&&g.world.flags.cardsOffered){g.command({id:'decline',type:'decline'});declined=true;}
  const jobs=g.dueJobs();
  const results=await Promise.all(jobs.map(async job=>({job,decision:await model.decide(g,job)})));
  for(const {job,decision} of results){
    const applied=g.apply(job.actor,decision,job.eventId);
    db.recordDecision(g.world,job,decision,applied);
    if(applied)accepted++;else{rejected++;g.apply(job.actor,g.rule(job.actor,job.eventId),job.eventId);}
  }
  if(jobs.length)console.log(JSON.stringify({elapsed:Math.round(g.world.elapsed),calls:g.world.calls,mode:g.world.modelMode,accepted,rejected}));
}
db.save(g.world);
const resumed=new Engine(scenario,{playerId:g.world.playerId},db.load(g.world.id,g.world.playerId),navigation);
const summary={
  requestedMode:online?'online':'offline',finalMode:g.world.modelMode,reason:g.world.modelReason,
  status:g.world.status,elapsed:g.world.elapsed,wallSeconds:(Date.now()-started)/1000,
  calls:g.world.calls,tokens:g.world.tokens,accepted,rejected,events:g.world.events.length,
  visibleEvents:g.view().events.length,reflected:g.reflection().events.length,
  resumeMatches:JSON.stringify(resumed.world.events)===JSON.stringify(g.world.events),
  note:'World clock accelerated in the server harness; movement acknowledged along exported grid. This is not a real-time 3D playthrough.'
};
db.close();
mkdirSync('../Verification',{recursive:true});
writeFileSync('../Verification/lastcall-'+(online?'online':'offline')+'-night.json',JSON.stringify(summary,null,2));
console.log(JSON.stringify(summary,null,2));
assert.equal(g.world.status,'ended');assert.equal(g.world.elapsed,720);assert.ok(summary.resumeMatches);
if(online)assert.ok(summary.calls>0);
