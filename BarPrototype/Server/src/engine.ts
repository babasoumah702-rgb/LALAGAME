import {initializeStory,enterChapter,chapterOf,currentRequest,phase} from './story.js';
import {advanceCrowd} from './crowd-navigation.js';
import {NightNavigator,NIGHT,areaOf} from './night-navigation.js';
import {initializeLateNight,advanceLateNight,observeLateNightEvent} from './late-night.js';
import type {Actor,Command,Decision,Event,Point,Scenario,World,GenerationSource} from './types.js';
import type {TtsAdapter} from './tts.js';
import {Navigator,distance,emptyNavigation} from './navigation.js';
import {actor,clamp,createWorld,location,random,trimMemory,zone} from './world.js';
import {perceive} from './visibility.js';
import {runBeats} from './beats.js';
import {handleCommand} from './commands.js';
import {applyDecision,agentContext,ruleDecision} from './decisions.js';
import {viewState,reflection} from './view.js';
import {initializeIntro,introActive,advanceIntro,type IntroOptions} from './intro.js';
import {initializeSceneOne,advanceSceneOne,observeSceneOneEvent,releaseFacing} from './scene-one.js';
import {initializeSceneTwo,advanceSceneTwo,advanceFollowing,observeSceneTwoEvent,sceneTwoHandoff} from './scene-two.js';
import {initializeSceneThree,advanceSceneThree,observeSceneThreeEvent,sceneThreeHandoff} from './scene-three.js';

// A line addressed to the player or spoken by the player can still be overheard and remembered,
// but it is not an open invitation for every witness to answer. Scene code explicitly queues any
// group reaction it wants (tarot gaze order, deflection, social ripples).
export function canActorReplyToEvent(e:Event,actorId:string){
  if(!['speech','message'].includes(e.type)||e.actor!=='USER'&&e.target!=='USER')return true;
  return e.actor==='USER'&&actorId===e.target;
}
export class Engine {
  world:World;
  busy=false;
  lastError='';
  voice?:TtsAdapter;
  constructor(public scenario:Scenario,options:{playerId:string;role?:string;entryIntent?:string;style?:string;seed?:number;online?:boolean;choices?:Record<string,string>}&IntroOptions,snapshot?:World,public navigation=new Navigator(emptyNavigation)){
    if(snapshot){if(snapshot.version!==1||snapshot.scenarioId!==scenario.id)throw new Error('存档与剧本不兼容');this.world=structuredClone(snapshot);this.world.paused=true;}
    else {this.world=createWorld(scenario,navigation,options);initializeIntro(this,options);if(options.story==='scene1_v1')initializeSceneOne(this);runBeats(this);}
    initializeStory(this.world);if(this.world.story&&!(this.navigation instanceof NightNavigator))this.navigation=new NightNavigator(this.navigation);
    this.world.replies??=[];for(const r of this.world.replies){
      const event=this.world.events.find(e=>e.id===r.eventId);
      if(event&&!canActorReplyToEvent(event,r.actor)){r.status='complete';r.error='';r.errorCode='SUPPRESSED';r.decision=undefined;continue;}
      if(r.status==='running'){r.status='error';r.error='上次回复被中断，请重试';}
    }
  }
  actor(id:string){return actor(this.world,id);}
  location(id:string){if(this.world.story&&id in NIGHT)return {...NIGHT[id as keyof typeof NIGHT],id,name:id,radius:1,privacy:.9,capacity:5};return location(this.scenario,id);}
  zone(p:Point){return zone(this.scenario,p);}
  random(){return random(this.world);}
  emit(type:string,from:string,target:string,intent:string,text:string,parentId='',privacy='normal',evidenceId='',generationSource:GenerationSource='unknown',objectTarget=''){
    const w=this.world,parent=w.events.find(e=>e.id===parentId),seq=++w.sequence;
    const e:Event={chapter:w.story?.chapter,generationSource,objectTarget,id:`${w.id}:${seq}`,seq,time:w.elapsed,type,actor:from,target,intent,text:text.slice(0,450),location:this.zone(this.actor(from)).id,privacy,parentId,rootId:parent?.rootId??`${w.id}:${seq}`,depth:parent?parent.depth+1:0,evidenceId,perceptions:[]};
    for(const a of w.actors){const p=perceive(this.scenario,w,this.navigation,a,e);if(!p)continue;e.perceptions.push(p);
      if(a.id!==from&&!a.knownActors.includes(from))a.knownActors.push(from);
      a.memory.push({eventId:e.id,summary:p.text,source:p.source,importance:type==='speech'?.7:.35,time:e.time,tier:type==='speech'?'relationship':'short'});trimMemory(a);
      if(!['USER','OWNER'].includes(a.id)&&a.id!==from&&p.level!=='gesture'&&canActorReplyToEvent(e,a.id)&&(['speech','message'].includes(type)||(this.world.scene1&&type==='action'&&['reserved_drink','arrival','sit','follow','listen','silence'].includes(intent)))&&e.depth<this.scenario.rules.maxDepth){
        if(a.id===target||w.jobs.length<12)w.jobs.push({actor:a.id,eventId:e.id,due:w.elapsed+(a.id===target?1:5+this.random()*4)});
      }
    }
    w.events.push(e);observeSceneOneEvent(this,e);observeSceneTwoEvent(this,e);observeSceneThreeEvent(this,e);observeLateNightEvent(this,e);
    if(type==='speech'&&this.voice&&from!=='USER'&&text.trim())void this.voice.speak(e);
    w.updatedAt=new Date().toISOString();return e;
  }
  go(a:Actor,to:Point,id=''){
    releaseFacing(this,a.id);a.posture='stand';
    let p=this.navigation.nearest(to);
    const occupied=(q:Point)=>this.world.actors.some(other=>other.active&&other.id!==a.id&&(
      distance(other.route.at(-1)??other,q)<.55));
    if(occupied(p)){
      const choices:Point[]=[];
      for(let ring=1;ring<=3;ring++)for(let i=0;i<8;i++){
        const candidate=this.navigation.nearest({y:to.y,area:to.area,x:to.x+Math.cos(i*Math.PI/4)*ring*.55,z:to.z+Math.sin(i*Math.PI/4)*ring*.55});
        if(!occupied(candidate)&&this.navigation.path(a,candidate).length)choices.push(candidate);
      }
      choices.sort((x,y)=>distance(x,to)-distance(y,to));p=choices[0]??p;
    }
    a.route=this.navigation.path(a,p);a.routeVersion=(a.routeVersion??0)+1;a.destination=id||this.zone(p).id;a.animation=a.route.length?'walk':'idle';
    return a.route.length>0;
  }
  near(a:Actor,t:Actor){
    const rad=t.yaw*Math.PI/180;
    return this.navigation.nearest({y:t.y,area:t.area,x:t.x+Math.sin(rad)*1.05,z:t.z+Math.cos(rad)*1.05});
  }
  command(c:Command){return handleCommand(this,c);}
  advance(seconds:number){const w=this.world;if(w.status!=='playing'||w.paused)return;if(introActive(w)){advanceIntro(this,seconds);return;}if(this.busy)return;w.elapsed=this.world.scene1?w.elapsed+clamp(seconds,0,2):Math.min(this.scenario.duration,w.elapsed+clamp(seconds,0,2));runBeats(this);
    // Scene 1 hands to Scene 2 the moment the room is complete, and Scene 2 hands to Scene 3 when the
    // deck lands. Each chapter owns its own clock; the legacy night keeps the declarative beats.
    advanceFollowing(this);advanceCrowd(this);
    if(w.late){advanceLateNight(this);return;}
    if(w.scene3){if(w.scene3.phase==='scene4_ready'){initializeLateNight(this,4);advanceLateNight(this);return;}advanceSceneThree(this);phase(this,w.scene3.phase);return;}
    if(w.scene2){if(w.scene2.phase==='tarot_ready'){enterChapter(this,3,'seating');initializeSceneThree(this);advanceSceneThree(this);return;}advanceSceneTwo(this);phase(this,w.scene2.phase);return;}
    if(w.scene1){advanceSceneOne(this);if(w.scene1.phase==='scene2_ready'&&!w.scene2){enterChapter(this,2,'cross_intro');initializeSceneTwo(this);}phase(this,this.world.scene2?.phase??w.scene1.phase);return;}
    for(const a of w.actors){if(!aActive(a)||a.route.length||a.nextAction>w.elapsed)continue;a.nextAction=w.elapsed+25+this.random()*20;
      if(a.withdrawn){if(distance(a,this.location('outside'))<.6){a.active=false;a.animation='leave';}else this.go(a,this.location('outside'),'outside');continue;}
      const recent=a.memory.filter(m=>m.importance>=.7&&w.elapsed-m.time>15&&!a.consideredEvents?.includes(m.eventId)).at(-1);
      if(recent&&!w.jobs.some(j=>j.actor===a.id))w.jobs.push({actor:a.id,eventId:recent.eventId,due:w.elapsed});
      else if(this.random()<a.initiative){const places=this.scenario.locations.filter(l=>!['outside','service','seat13'].includes(l.id));const l=places[Math.floor(this.random()*places.length)];if(w.actors.filter(x=>x.active&&x.id!==a.id&&distance(x,l)<.8).length<l.capacity)this.go(a,l,l.id);}
    }
  }
  dueJobs(limit=2){const w=this.world,result:typeof w.jobs=[];w.jobs=w.jobs.filter(j=>{const a=this.actor(j.actor),e=w.events.find(x=>x.id===j.eventId);if(!a.active||!e||!canActorReplyToEvent(e,j.actor)||!e.perceptions.some(p=>p.actor===j.actor)||(!currentRequest(w,j.eventId)&&!w.replies?.some(r=>r.actor===j.actor&&r.eventId===j.eventId&&r.status==='queued'))||e.depth>=this.scenario.rules.maxDepth)return false;
    if((w.replies||[]).some(r=>r.actor===j.actor&&r.eventId===j.eventId&&r.status!=='queued'))return false;
    if((w.replies||[]).some(r=>r.actor===j.actor&&['running','ready'].includes(r.status)))return true;
    if(result.length<limit&&j.due<=w.elapsed&&!a.route.length&&w.elapsed-a.lastSpoke>4&&!result.some(x=>x.actor===j.actor)){j.chapter??=chapterOf(w,j.eventId);result.push(j);return false;}return true;}).slice(-80);return result;}
  context(id:string,eventId:string){return agentContext(this,id,eventId);}
  handoff(){return {scene2:sceneTwoHandoff(this),scene3:sceneThreeHandoff(this)};}
  rule(id:string,eventId:string){return ruleDecision(this,id,eventId);}
  apply(id:string,d:Decision,parentId:string){return applyDecision(this,id,d,parentId);}
  finish(){const w=this.world;if(w.status==='ended')return;w.status='ended';w.paused=true;w.jobs=[];for(const a of w.actors){a.route=[];a.pending=undefined;for(const m of a.memory)if(m.importance>=.7)m.tier='long';trimMemory(a);}this.emit('system','OWNER','USER','close','这一晚结束了，但有些关系才刚刚开始。');}
  nextNight(){const old=this.world,next=new Engine(this.scenario,{playerId:old.playerId,role:old.role,entryIntent:old.entryIntent,style:old.style,seed:old.seed+1,online:old.modelMode==='online'},undefined,this.navigation);next.world.night=old.night+1;
    next.world.identityPack=old.identityPack;next.world.contextProfile=structuredClone(old.contextProfile);
    for(const a of next.world.actors){const prior=actor(old,a.id);a.relations=structuredClone(prior.relations);a.knownActors=[...prior.knownActors];a.beliefs=structuredClone(prior.beliefs);a.memory=prior.memory.filter(m=>m.importance>=.7).slice(-16).map(m=>({...m,tier:'long'}));}
    if(old.flags.scene0Route&&old.intro){
      next.world.flags.scene0Route=true;next.world.intro={...structuredClone(old.intro),phase:'bar',progress:7,checkpoint:5};
      for(const id of ['A','C'])next.actor(id).active=true;
    }
    if(old.scene1){next.world.scene1={...structuredClone(old.scene1),phase:'scene2_ready'};}
    // A later night keeps the chapter the player reached; it does not replay the cup or the deck.
    if(old.scene2)next.world.scene2={...structuredClone(old.scene2),phase:'tarot_ready'};
    if(old.scene3)next.world.scene3=structuredClone(old.scene3);
    next.world.initialRelations=Object.fromEntries(next.world.actors.filter(a=>a.id!=='USER').map(a=>[a.id,structuredClone(a.relations.USER)]));next.world.events=[];next.world.sequence=0;next.world.jobs=[];next.world.beatIds=[];runBeats(next);return next;
  }
  view(fullHistory=false){return viewState(this,fullHistory);}
  reflection(){return reflection(this);}
}
function aActive(a:Actor){return a.active&&!['USER','OWNER'].includes(a.id);}
