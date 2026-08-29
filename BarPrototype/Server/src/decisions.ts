import {currentRequest} from './story.js';
import {lateNightContext} from './late-night.js';
import type {Engine} from './engine.js';
import type {Decision,Relation} from './types.js';
import {clamp,relation} from './world.js';
import {distance} from './navigation.js';
import {identityBrief} from './identity.js';
import {sceneOneContext,facePair,sceneOneDisplayName} from './scene-one.js';
import {sceneTwoContext} from './scene-two.js';
import {sceneThreeContext} from './scene-three.js';
export function agentContext(game:Engine,id:string,eventId:string){
  const character=game.actor(id);
  const event=game.world.events.find(item=>item.id===eventId);
  const perceived=event?.perceptions.find(item=>item.actor===id);
  if(!event || !perceived) throw new Error('Event not perceived');
  const full=perceived.level==='full';
  const conversation=game.world.events.filter(e=>e.type==='speech'&&e.seq<=event.seq&&e.perceptions.some(p=>p.actor===id&&p.level==='full')&&(e.actor===id||e.target===id)).slice(-10);
  const answeredQuestions=conversation.filter(e=>e.actor===id&&e.target===event.actor&&/[？?]/.test(e.text)).flatMap(question=>{const answer=conversation.find(e=>e.seq>question.seq&&e.actor===event.actor&&e.target===id);return answer?[{questionId:question.id,question:question.text,answerId:answer.id,answer:answer.text}]:[];});
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
    identity:identityBrief(game.scenario,game.world.identityPack,id,game.world.contextProfile?.preferred_topic_density),
    scene:currentRequest(game.world,eventId)?lateNightContext(game,id,event)||sceneThreeContext(game,id,event)||sceneTwoContext(game,id,event)||sceneOneContext(game,id,event):{chapter:event.chapter,expired:true,reminder:'这是上一章的补回回复，只针对这条旧事件；不安排当前章节的动作。'},
    event:{id:event.id,actor:event.actor,target:full?event.target:'unknown',intent:full?event.intent:'unknown',content:perceived.text,source:perceived.source,objectTarget:full?event.objectTarget:undefined},
    memory:character.memory.slice(-12).map(item=>{const e=game.world.events.find(e=>e.id===item.eventId);const p=e?.perceptions.find(p=>p.actor===id);return {id:item.eventId,text:item.summary,source:item.source,speaker:e?.actor||'unknown',target:p?.level==='full'?e?.target:'unknown',sequence:e?.seq,time:item.time};}),
    answeredQuestions,
    conversation:conversation.map(e=>({id:e.id,speaker:e.actor,target:e.target,sequence:e.seq,text:e.perceptions.find(p=>p.actor===id)!.text})),
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
  if(event.intent==='cross_introduce'&&perceived.level==='full')return {action:'speak',target:'USER',intent:'introduce',expression:'这位是'+game.actor('D').name+'。她是'+(game.scenario.identityPacks[game.world.identityPack]?.actors.D.publicRole??'刚忙完的朋友')+'，你们自己聊。',interpretation:'offline introduction',evidenceIds:[eventId],signal:'neutral',confidence:1};
  const signal=['approach','reveal','connect'].includes(intent)?'warm':intent==='boundary'?'boundary':'probe';
  let action='speak',target=event.actor,kind=signal;
  const delayed=game.world.elapsed-event.time>12;
  if(event.actor===id||event.depth>=3){action='wait';target='USER';}
  else if(id==='A'&&event.intent==='arrival'){
    // A checks with the bartender rather than the newcomer; her open question is about B, not the player.
    action=(character.relations.B?.uncertainty??.5)>.4?'ask':'wait';target='BARTENDER';kind='share';
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
  if(id==='A'&&signal==='warm'&&event.target!=='A')interpretation='她在对别人表达亲近，我记下措辞，不替她解释。';
  if(id==='B'&&signal==='warm')interpretation='我先别急着把这句话算成对我的意思。';
  if(perceived.level!=='full')interpretation='我只听见片段，不能断言真实含义。';
  let expression=lines[Math.floor(game.random()*lines.length)];
  if(game.world.scene1&&intent==='ask_name')expression='我叫'+character.name+'。你呢？';
  if(game.world.scene1&&event.objectTarget==='third_drink')expression=({BARTENDER:'给还没到的客人留的，先放这里。',B:'她那杯少放了冰，等会儿就该来了。',A:'你倒是记得很清楚。',C:'今晚这个位置安排，有点意思。'} as Record<string,string>)[id]||expression;
  // Offline lines for the later chapters. They stay deliberately short and non-committal: the rules
  // mode must never invent a relationship history the model would have been careful about.
  const two=game.world.scene2;
  if(two&&!game.world.scene3){
    if(event.intent==='turn_to')expression=({A:'嗯，我在听。',B:'你也过来啊。',C:'我就站一会儿。',D:'刚回完一条消息。',BARTENDER:'再来一杯？'} as Record<string,string>)[id]||expression;
    else if(event.intent==='follow')expression=({A:'跟着我干嘛。',B:'来，这边人多一点。',C:'……你也来了。',D:'正好，陪我站会儿。',BARTENDER:'想喝点什么？'} as Record<string,string>)[id]||expression;
    else if(event.intent==='listen')expression=({A:'想听就坐近点。',B:'别站那么远。',C:'没什么好听的。',D:'我们在说工作，没意思。',BARTENDER:'他们聊得挺热闹。'} as Record<string,string>)[id]||expression;
  }
  const three=game.world.scene3;
  if(three&&three.askedAt>=0){
    const joker=three.tags.includes('joker'),high=three.tags.includes('high_tension');
    if(event.intent==='silence')expression=({A:'不想答就不用答。',B:'跳过也算一种回答。',C:'我也不想答这题。',D:'这题本来就不该逼人。',BARTENDER:'谁都可以过。'} as Record<string,string>)[id]||expression;
    else if(joker)expression=({A:'这题我不评价。',B:'这不明摆着吗。',C:'……你们自己心里清楚。',D:'凭什么先看我。',BARTENDER:'我只管倒酒。'} as Record<string,string>)[id]||expression;
    else if(high)expression=({A:'有。就这一句。',B:'我先喝一口再说。',C:'有。不说是谁。',D:'这题我先过。',BARTENDER:'要不要换一张？'} as Record<string,string>)[id]||expression;
    else expression=({A:'我想一下。',B:'这个可以聊。',C:'算有吧。',D:'有点意思。',BARTENDER:'慢慢答。'} as Record<string,string>)[id]||expression;
    if(joker)interpretation='这只是玩笑，不用当真。';
    else if(high)interpretation='我只说到这里，不解释是谁。';
  }
  return {generationSource:'rules',action,target,intent,expression,interpretation,evidenceIds:[eventId],signal,confidence:perceived.confidence};
}
export function applyDecision(game:Engine,id:string,decision:Decision,parentId:string):boolean{
  const world=game.world;
  const character=game.actor(id);
  const parent=world.events.find(event=>event.id===parentId);
  const perceived=parent?.perceptions.find(item=>item.actor===id);
  if(!currentRequest(world,parentId)||!character.active||!parent||!perceived)return false;
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
    game.go(character,game.location(world.story?'corridor':'outside'),world.story?'corridor':'outside');
    game.emit('movement',id,'USER','boundary',sceneOneDisplayName(game,id)+' 起身走向门外，给彼此留了一点空间。',parentId,'normal','',decision.generationSource||'unknown');
    return true;
  }
  if(character.withdrawn&&!['withdraw','leave','wait','observe'].includes(decision.action))return false;
  if(parent.intent==='boundary'&&parent.actor==='USER'&&decision.signal==='warm')return false;
  if(parent.objectTarget==='photo_request'&&character.privatePhoto&&decision.signal!=='boundary')return false;
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
  facePair(game,id,target.id);
  const count=world.events.filter(event=>event.rootId===parent.rootId).length;
  if(count>=9||parent.depth>=game.scenario.rules.maxDepth)return true;
  game.emit('speech',id,target.id,decision.intent||'probe',decision.expression,parentId,'normal',decision.evidenceIds[0]||parentId,decision.generationSource||'unknown');
  if(world.elapsed-parent.time<12&&(parent.actor==='USER'&&parent.depth===0||id==='BARTENDER'&&parent.depth===1)&&!world.jobs.some(j=>j.actor===id&&j.eventId===parent.id)){
    world.jobs.push({actor:id,eventId:parent.id,due:world.elapsed+18});
  }
  if(world.flags.tableInvitation===id&&decision.signal==='warm'){
    world.flags.tableInvitation='';
    game.go(character,game.location('main_table'),'main_table');
    if(!world.story)game.go(game.actor('USER'),game.location('main_table'),'main_table');
  }
  return true;
}
