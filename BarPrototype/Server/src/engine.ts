import type {Actor,Command,Decision,Event,Point,Scenario,World} from './types.js';
import {Navigator,distance,emptyNavigation} from './navigation.js';
import {actor,clamp,createWorld,location,random,trimMemory,zone} from './world.js';
import {perceive} from './visibility.js';
import {runBeats} from './beats.js';
import {handleCommand} from './commands.js';
import {applyDecision,agentContext,ruleDecision} from './decisions.js';
import {viewState,reflection} from './view.js';
export class Engine {
  world:World;
  busy=false;
  lastError='';
  constructor(public scenario:Scenario,options:{playerId:string;role?:string;entryIntent?:string;style?:string;seed?:number;online?:boolean},snapshot?:World,public navigation=new Navigator(emptyNavigation)){
    if(snapshot){if(snapshot.version!==1||snapshot.scenarioId!==scenario.id)throw new Error('存档与剧本不兼容');this.world=structuredClone(snapshot);this.world.paused=true;}
    else {this.world=createWorld(scenario,navigation,options);runBeats(this);}
  }
  actor(id:string){return actor(this.world,id);}
  location(id:string){return location(this.scenario,id);}
  zone(p:Point){return zone(this.scenario,p);}
  random(){return random(this.world);}
  emit(type:string,from:string,target:string,intent:string,text:string,parentId='',privacy='normal',evidenceId=''){
    const w=this.world,parent=w.events.find(e=>e.id===parentId),seq=++w.sequence;
    const e:Event={id:`${w.id}:${seq}`,seq,time:w.elapsed,type,actor:from,target,intent,text:text.slice(0,450),location:this.zone(this.actor(from)).id,privacy,parentId,rootId:parent?.rootId??`${w.id}:${seq}`,depth:parent?parent.depth+1:0,evidenceId,perceptions:[]};
    for(const a of w.actors){const p=perceive(this.scenario,w,this.navigation,a,e);if(!p)continue;e.perceptions.push(p);
      if(a.id!==from&&!a.knownActors.includes(from))a.knownActors.push(from);
      a.memory.push({eventId:e.id,summary:p.text,source:p.source,importance:type==='speech'?.7:.35,time:e.time,tier:type==='speech'?'relationship':'short'});trimMemory(a);
      if(!['USER','OWNER'].includes(a.id)&&a.id!==from&&p.level!=='gesture'&&['speech','message'].includes(type)&&e.depth<this.scenario.rules.maxDepth){
        if(a.id===target||w.jobs.length<12)w.jobs.push({actor:a.id,eventId:e.id,due:w.elapsed+(a.id===target?1:5+this.random()*4)});
      }
    }
    w.events.push(e);w.updatedAt=new Date().toISOString();return e;
  }
  go(a:Actor,to:Point,id=''){
    let p=this.navigation.nearest(to);
    const occupied=(q:Point)=>this.world.actors.some(other=>other.active&&other.id!==a.id&&(
      distance(other.route.at(-1)??other,q)<.55));
    if(occupied(p)){
      const choices:Point[]=[];
      for(let ring=1;ring<=3;ring++)for(let i=0;i<8;i++){
        const candidate=this.navigation.nearest({x:to.x+Math.cos(i*Math.PI/4)*ring*.55,z:to.z+Math.sin(i*Math.PI/4)*ring*.55});
        if(!occupied(candidate)&&this.navigation.path(a,candidate).length)choices.push(candidate);
      }
      choices.sort((x,y)=>distance(x,to)-distance(y,to));p=choices[0]??p;
    }
    a.route=this.navigation.path(a,p);a.routeVersion=(a.routeVersion??0)+1;a.destination=id||this.zone(p).id;a.animation=a.route.length?'walk':'idle';
    return a.route.length>0;
  }
  near(a:Actor,t:Actor){const dx=a.x-t.x,dz=a.z-t.z,len=Math.hypot(dx,dz)||1;return this.navigation.nearest({x:t.x+dx/len*.95,z:t.z+dz/len*.95});}
  command(c:Command){return handleCommand(this,c);}
  advance(seconds:number){const w=this.world;if(w.status!=='playing'||w.paused||this.busy)return;w.elapsed=Math.min(this.scenario.duration,w.elapsed+clamp(seconds,0,2));runBeats(this);
    for(const a of w.actors){if(!aActive(a)||a.route.length||a.nextAction>w.elapsed)continue;a.nextAction=w.elapsed+25+this.random()*20;
      if(a.withdrawn){if(distance(a,this.location('outside'))<.6){a.active=false;a.animation='leave';}else this.go(a,this.location('outside'),'outside');continue;}
      const recent=a.memory.filter(m=>m.importance>=.7&&w.elapsed-m.time>15&&!a.consideredEvents?.includes(m.eventId)).at(-1);
      if(recent&&!w.jobs.some(j=>j.actor===a.id))w.jobs.push({actor:a.id,eventId:recent.eventId,due:w.elapsed});
      else if(this.random()<a.initiative){const places=this.scenario.locations.filter(l=>!['outside','service','seat13'].includes(l.id));const l=places[Math.floor(this.random()*places.length)];if(w.actors.filter(x=>x.active&&x.id!==a.id&&distance(x,l)<.8).length<l.capacity)this.go(a,l,l.id);}
    }
  }
  dueJobs(){const w=this.world,result:typeof w.jobs=[];w.jobs=w.jobs.filter(j=>{const a=this.actor(j.actor),e=w.events.find(x=>x.id===j.eventId);if(!a.active||!e||e.depth>=this.scenario.rules.maxDepth)return false;
    if(result.length<2&&j.due<=w.elapsed&&!a.route.length&&w.elapsed-a.lastSpoke>4&&!result.some(x=>x.actor===j.actor)){result.push(j);return false;}return true;}).slice(-80);return result;}
  context(id:string,eventId:string){return agentContext(this,id,eventId);}
  rule(id:string,eventId:string){return ruleDecision(this,id,eventId);}
  apply(id:string,d:Decision,parentId:string){return applyDecision(this,id,d,parentId);}
  finish(){const w=this.world;if(w.status==='ended')return;w.status='ended';w.paused=true;w.jobs=[];for(const a of w.actors){a.route=[];a.pending=undefined;for(const m of a.memory)if(m.importance>=.7)m.tier='long';trimMemory(a);}this.emit('system','OWNER','USER','close','这一晚结束了，但有些关系才刚刚开始。');}
  nextNight(){const old=this.world,next=new Engine(this.scenario,{playerId:old.playerId,role:old.role,entryIntent:old.entryIntent,style:old.style,seed:old.seed+1,online:old.modelMode==='online'},undefined,this.navigation);next.world.night=old.night+1;
    for(const a of next.world.actors){const prior=actor(old,a.id);a.relations=structuredClone(prior.relations);a.knownActors=[...prior.knownActors];a.beliefs=structuredClone(prior.beliefs);a.memory=prior.memory.filter(m=>m.importance>=.7).slice(-16).map(m=>({...m,tier:'long'}));}
    next.world.initialRelations=Object.fromEntries(next.world.actors.filter(a=>a.id!=='USER').map(a=>[a.id,structuredClone(a.relations.USER)]));next.world.events=[];next.world.sequence=0;next.world.jobs=[];next.world.beatIds=[];runBeats(next);return next;
  }
  view(fullHistory=false){return viewState(this,fullHistory);}
  reflection(){return reflection(this);}
}
function aActive(a:Actor){return a.active&&!['USER','OWNER'].includes(a.id);}
