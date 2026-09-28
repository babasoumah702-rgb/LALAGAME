import {readFileSync} from 'node:fs';
import {Engine} from './engine.js';
import {loadScenario} from './config.js';
import {Navigator} from './navigation.js';
import {queuePeerCue} from './first-night.js';
import {ModelAdapter} from './model.js';
import {applyReadyReplies} from './reply-runtime.js';

const scenario=loadScenario('scenarios/last_call.json');
const nav=new Navigator(JSON.parse(readFileSync('scenarios/navigation.json','utf8')));
const game=new Engine(scenario,{playerId:'live-a2a-check',opening:'first_night_v2',story:'first_night_v2',online:true},undefined,nav);
game.world.intro!.phase='bar';game.world.intro!.progress=7;
for(const id of ['USER','A','B','C','D'])Object.assign(game.actor(id),{x:0,z:0,area:'bar',route:[]});

const adapter=new ModelAdapter();
if(!adapter.config.key)throw new Error('NO_KEY: configure the model in Lalaland before running the live A2A check');
const pairs:[string,string,string,string][]=[
  ['A','B','peer_table','眼前的弹球桌和刚才的落点'],
  ['B','C','peer_unwind','酒吧里逐渐放松下来的气氛'],
  ['C','D','peer_table','下一轮谁先试球'],
  ['D','A','peer_unwind','窗外夜景和楼上的露台']
];
const report:any={model:adapter.config.model,startedAt:new Date().toISOString(),calls:[],passed:false};

for(const [from,target,intent,topic] of pairs){
  game.world.jobs=[];
  const cue=queuePeerCue(game,from,target,intent,`${game.actor(from).name} 想和 ${game.actor(target).name} 谈谈${topic}。`);
  const firstJob=game.world.jobs.find(j=>j.actor===from&&j.eventId===cue.id);
  if(!firstJob)throw new Error(`missing initiator job ${from}->${target}`);
  const firstStart=performance.now();
  const first=await adapter.decide(game,firstJob);
  applyReadyReplies(game);
  const line=game.world.events.find(e=>e.parentId===cue.id&&e.actor===from&&e.target===target&&e.generationSource==='ai');
  if(!line)throw new Error(`initiator did not produce a directed AI line ${from}->${target}: ${first.action}`);
  report.calls.push({role:'initiate',actor:from,target,source:line.generationSource,elapsedMs:Math.round(performance.now()-firstStart),text:line.text});

  const replyJob=game.world.jobs.find(j=>j.actor===target&&j.eventId===line.id);
  if(!replyJob)throw new Error(`missing addressed reply job ${target}->${from}`);
  const replyStart=performance.now();
  const reply=await adapter.decide(game,replyJob);
  applyReadyReplies(game);
  const answer=game.world.events.find(e=>e.parentId===line.id&&e.actor===target&&e.target===from&&e.generationSource==='ai');
  if(!answer)throw new Error(`partner did not produce a directed AI line ${target}->${from}: ${reply.action}`);
  report.calls.push({role:'reply',actor:target,target:from,source:answer.generationSource,elapsedMs:Math.round(performance.now()-replyStart),text:answer.text});
}

report.passed=report.calls.length===8&&report.calls.every((entry:any)=>entry.source==='ai');
report.finishedAt=new Date().toISOString();
console.log(JSON.stringify(report,null,2));
if(!report.passed)process.exitCode=1;
