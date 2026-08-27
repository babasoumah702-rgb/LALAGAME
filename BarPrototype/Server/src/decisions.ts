import type {Engine} from './engine.js';
import type {Decision,Relation} from './types.js';
import {clamp,relation} from './world.js';
import {distance} from './navigation.js';
export function agentContext(game:Engine,id:string,eventId:string){
  const character=game.actor(id);
  const event=game.world.events.find(item=>item.id===eventId);
  const perceived=event?.perceptions.find(item=>item.actor===id);
  if(!event || !perceived) throw new Error('Event not perceived');
  const full=perceived.level==='full';
  return {
    actor:{
      id:character.id,
      goal:character.goal,
      voice:character.voice,
      privatePhoto:character.privatePhoto,
      boundary:{withdrawn:character.withdrawn,needsSpace:character.beliefs.some(b=>b.subject==='distance')},
      autonomy:{initiative:character.initiative,share:character.share},
      facts:character.knownFacts.map(key=>game.scenario.facts[key]),
      relations:Object.fromEntries(Object.entries(character.relations).filter(([key])=>character.knownActors.includes(key)))
    },
    event:{id:event.id,actor:event.actor,target:full?event.target:'unknown',intent:full?event.intent:'unknown',content:perceived.text,source:perceived.source},
    memory:character.memory.slice(-8).map(item=>({id:item.eventId,text:item.summary,source:item.source})),
    beliefs:character.beliefs.slice(-4),
    targets:character.knownActors.filter(key=>game.actor(key).active)
  };
}
export function ruleDecision(game:Engine,id:string,eventId:string):Decision{
  const character=game.actor(id);
  const event=game.world.events.find(item=>item.id===eventId)!;
  const perceived=event.perceptions.find(item=>item.actor===id)!;
  const phrases=game.scenario.voices[id]||game.scenario.voices.BARTENDER;
  const intent=perceived.level==='full'?event.intent:'probe';
  const signal=['approach','reveal','connect'].includes(intent)?'warm':intent==='boundary'?'boundary':'probe';
  let action='speak',target=event.actor,kind=signal;
  const delayed=game.world.elapsed-event.time>12;
  if(event.actor===id||event.depth>=3){action='wait';target='USER';}
  else if(id==='A'&&event.intent==='arrival'){
    action=character.relations.USER.uncertainty>.4?'ask':'wait';target='BARTENDER';kind='share';
  }
  else if(id==='A'&&event.actor==='USER'&&event.target!=='A'){
    action='ask';target='BARTENDER';kind='share';
  }else if(delayed&&id==='B'&&event.actor==='USER'&&intent!=='boundary'&&character.share>.35){
    action='share';target='BARTENDER';kind='share';
  }else if(delayed&&id==='BARTENDER'&&event.actor!=='USER'){
    action='share';target='USER';kind='relay';
  }else if(intent==='boundary'&&event.actor==='USER'&&character.beliefs.some(b=>b.subject==='distance')){
    action='withdraw';kind='boundary';
  }else if(character.relations.USER.safety<.25&&character.initiative>.3){
    action='withdraw';kind='boundary';
  }else if(event.depth>1&&game.random()<.5){
    action='wait';
  }
  const lines=phrases[kind]||phrases.probe;
  let interpretation=signal==='warm'?'这可能是靠近，不代表承诺。':signal==='boundary'?'她需要空间。':'我还不能确定这句话的意思。';
  if(id==='A'&&signal==='warm'&&event.target!=='A')interpretation='她似乎在对别人表达亲近，我还不知道为什么。';
  if(id==='B'&&signal==='warm')interpretation='她可能是在确认我对她很特别，但我不能确定。';
  if(perceived.level!=='full')interpretation='我只听见片段，不能断言真实含义。';
  return {action,target,intent,expression:lines[Math.floor(game.random()*lines.length)],interpretation,evidenceIds:[eventId],signal,confidence:perceived.confidence};
}
export function applyDecision(game:Engine,id:string,decision:Decision,parentId:string):boolean{
  const world=game.world;
  const character=game.actor(id);
  const parent=world.events.find(event=>event.id===parentId);
  const perceived=parent?.perceptions.find(item=>item.actor===id);
  if(!character.active||!parent||!perceived)return false;
  const allowed=['speak','ask','share','observe','withdraw','wait','leave'];
  if (!allowed.includes(decision.action)) return false;
  if(!Array.isArray(decision.evidenceIds))return false;
  if(decision.evidenceIds.some(key=>!character.memory.some(m=>m.eventId===key)))return false;
  const target=world.actors.find(item=>item.id===decision.target);
  if(!target?.active||!character.knownActors.includes(target.id)||target.id===id)return false;
  if(['speak','ask','share'].includes(decision.action)&&(!decision.expression||decision.expression.length>240))return false;
  character.consideredEvents=[...(character.consideredEvents??[]).filter(key=>key!==parentId),parentId].slice(-80);
  if(world.events.filter(event=>event.rootId===parent.rootId).length>=9)return true;
  if(['wait','observe'].includes(decision.action)){
    character.nextAction=world.elapsed+25;
    return true;
  }
  if(['withdraw','leave'].includes(decision.action)){
    character.withdrawn=true;
    game.go(character,game.location('outside'),'outside');
    game.emit('movement',id,'USER','boundary',character.name+' 起身走向门外，给彼此留了一点空间。',parentId);
    return true;
  }
  if(character.withdrawn&&!['withdraw','leave','wait','observe'].includes(decision.action))return false;
  if(parent.intent==='boundary'&&parent.actor==='USER'&&decision.signal==='warm')return false;
  if(distance(character,target)>2.6||!game.navigation.visible(character,target)){
    if(!character.pending){
      character.pending=decision;
      character.pendingParent=parentId;
      if(!game.go(character,game.near(character,target))){character.pending=undefined;return false;}
    }
    return true;
  }
  const signal=perceived.level==='full'?decision.signal:'probe';
  const relationship=character.relations[parent.actor]||relation();
  const changes:Partial<Relation>=signal==='warm'?{trust:.02,closeness:.055,attraction:['A','B','C'].includes(id)?.015:0,uncertainty:-.025}:signal==='boundary'?{trust:.015,safety:.045,closeness:-.045,tension:.025}:{uncertainty:.025,tension:.025};
  for(const [key,value] of Object.entries(changes)){
    const field=key as keyof Relation;
    relationship[field]=clamp(relationship[field]+clamp(value,-game.scenario.rules.clamp,game.scenario.rules.clamp));
  }
  character.relations[parent.actor]=relationship;
  character.beliefs.push({subject:signal==='boundary'?'distance':signal==='warm'?'possible_closeness':'uncertain',confidence:clamp(decision.confidence||.5),sourceEventId:parent.id,interpretation:decision.interpretation.slice(0,180)});
  character.beliefs=character.beliefs.slice(-24);
  character.lastSpoke=world.elapsed;
  character.animation='speak';
  character.yaw=Math.atan2(target.x-character.x,target.z-character.z)*180/Math.PI;
  const count=world.events.filter(event=>event.rootId===parent.rootId).length;
  if(count>=9||parent.depth>=game.scenario.rules.maxDepth)return true;
  game.emit('speech',id,target.id,decision.intent||'probe',decision.expression,parentId,'normal',decision.evidenceIds[0]||parentId);
  if(world.elapsed-parent.time<12&&(parent.actor==='USER'&&parent.depth===0||id==='BARTENDER'&&parent.depth===1)&&!world.jobs.some(j=>j.actor===id&&j.eventId===parent.id)){
    world.jobs.push({actor:id,eventId:parent.id,due:world.elapsed+18});
  }
  if(world.flags.tableInvitation===id&&decision.signal==='warm'){
    world.flags.tableInvitation='';
    game.go(character,game.location('main_table'),'main_table');
    game.go(game.actor('USER'),game.location('main_table'),'main_table');
  }
  return true;
}
