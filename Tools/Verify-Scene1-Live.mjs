import {writeFileSync} from 'node:fs';
import {fileURLToPath} from 'node:url';
import {dirname,join} from 'node:path';
import {Engine} from '../BarPrototype/Server/dist/engine.js';
import {loadScenario} from '../BarPrototype/Server/dist/config.js';
import {ModelAdapter} from '../BarPrototype/Server/dist/model.js';
import {applyReadyReplies} from '../BarPrototype/Server/dist/reply-runtime.js';
const project=join(dirname(fileURLToPath(import.meta.url)),'../BarPrototype');
const g=new Engine(loadScenario(join(project,'Server/scenarios/last_call.json')),{playerId:'synthetic-scene1-live',story:'scene1_v1',online:true,choices:{preferred_topic_density:'life'}});
g.advance(0);Object.assign(g.actor('USER'),{x:0,z:0});Object.assign(g.actor('B'),{x:1,z:0});
const adapter=new ModelAdapter();
const report={configured:!!adapter.config.key,model:adapter.config.model,passed:false,calls:0,exchanges:[],errors:[]};
if(adapter.config.key){
  for(const text of ['今晚推开这扇门，是因为想见你。','我想见你','不是合作，我只是想坐下来和你聊一会。','你叫什么名字？']){
    if(g.world.calls>=10)break;
    const e=g.emit('speech','USER','B',text.includes('名字')?'ask_name':'reveal',text,'','normal','','player');
    g.world.jobs=[];
    try{
      const d=await adapter.decide(g,{actor:'B',eventId:e.id,due:g.world.elapsed});
      applyReadyReplies(g);
      report.exchanges.push({player:text,response:d.expression,action:d.action,source:d.generationSource,accepted:g.world.replies.at(-1).status==='complete'});
    }catch(error){report.errors.push({code:error.code||'UNKNOWN',message:'这条合成对话未获得可用模型回复'});}
    g.world.elapsed+=5;
  }
}
report.calls=g.world.calls;
const lines=report.exchanges.map(e=>e.response).filter(Boolean);
report.passed=report.configured&&report.errors.length===0&&report.exchanges.length===4&&report.exchanges.every(e=>e.source==='ai'&&e.accepted)&&new Set(lines).size===lines.length;
writeFileSync(join(project,'Verification/scene1-live.json'),JSON.stringify(report,null,2));
console.log(JSON.stringify(report,null,2));if(!report.passed)process.exitCode=1;
