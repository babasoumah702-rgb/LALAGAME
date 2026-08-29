import {writeFileSync} from 'node:fs';
import {fileURLToPath} from 'node:url';
import {dirname,join} from 'node:path';
import {Engine} from '../BarPrototype/Server/dist/engine.js';
import {loadScenario} from '../BarPrototype/Server/dist/config.js';
import {ModelAdapter} from '../BarPrototype/Server/dist/model.js';
import {applyReadyReplies} from '../BarPrototype/Server/dist/reply-runtime.js';
import {enterChapter} from '../BarPrototype/Server/dist/story.js';
import {initializeSceneTwo} from '../BarPrototype/Server/dist/scene-two.js';
import {initializeSceneThree} from '../BarPrototype/Server/dist/scene-three.js';
import {initializeLateNight} from '../BarPrototype/Server/dist/late-night.js';
const project=join(dirname(fileURLToPath(import.meta.url)),'../BarPrototype');
const g=new Engine(loadScenario(join(project,'Server/scenarios/last_call.json')),{playerId:'fullnight-live-synthetic-only',story:'scene1_v1',online:true,choices:{preferred_topic_density:'life'}});
g.advance(0);const adapter=new ModelAdapter();const report={configured:!!adapter.config.key,model:adapter.config.model,passed:false,synthetic:true,exchanges:[],errors:[],budgets:{},calls:0};
const rounds=[
 [1,'今晚推开这扇门，是因为想见你。'],[1,'我想见你'],[1,'不是合作，我只是想坐下来和你聊一会。'],[1,'你叫什么名字？'],
 [2,'这首歌听起来有点熟，你喜欢这种音乐吗？'],[3,'我想跳过这一题，可以只看着吗？'],
 [4,'这是巧克力做的吗？我能尝一支吗？'],[5,'灯熄了，我想先留一会儿。'],[6,'风挺舒服的。我们安静待会儿吧。']
];
if(adapter.config.key)for(const [chapter,text] of rounds){
 if(g.world.calls>=16){report.errors.push({code:'TEST_CAP'});break;}
 if(g.world.story.chapter!==chapter){enterChapter(g,chapter,'synthetic_acceptance');if(chapter===2)initializeSceneTwo(g);if(chapter===3)initializeSceneThree(g);if(chapter>=4)initializeLateNight(g,chapter);}
 for(const [id,x] of [['USER',0],['B',1]])Object.assign(g.actor(id),{x,z:0,y:chapter===6?4.2:0,area:chapter===6?'rooftop':chapter===4?'corridor':'bar',active:true,route:[],withdrawn:false});
 const e=g.emit('speech','USER','B',text.includes('名字')?'ask_name':chapter===3?'boundary':'chat',text,'','normal','','player');g.world.jobs=[];
 try{const d=await adapter.decide(g,{chapter,actor:'B',eventId:e.id,due:g.world.elapsed});applyReadyReplies(g);const r=g.world.replies.at(-1);report.exchanges.push({chapter,player:text,response:d.expression,action:d.action,source:d.generationSource,accepted:r.status==='complete',requestId:r.id,elapsedMs:r.elapsedMs});}catch(error){report.errors.push({chapter,code:error.code??'UNKNOWN'});}
 g.world.elapsed+=5;
}
report.calls=g.world.calls;report.budgets=g.world.story.budgets;
const lines=report.exchanges.filter(x=>x.chapter===1).map(x=>x.response).filter(Boolean);
report.passed=report.configured&&report.exchanges.length===9&&report.errors.length===0&&report.exchanges.every(x=>x.source==='ai'&&x.accepted)&&new Set(lines).size===lines.length;
writeFileSync(join(project,'Verification/fullnight-live.json'),JSON.stringify(report,null,2));console.log(JSON.stringify(report,null,2));if(!report.passed)process.exitCode=1;
