import type {Engine} from './engine.js';
import type {Command,Decision,Event,Relation} from './types.js';
import {ACTIVITIES,CHARACTER_NAMES,DRINK_ACCEPT,DRINK_ALTERNATIVE,DRINK_SELF_ORDER,DRINKS,FIRST_NIGHT_CONTENT_VERSION,HIT_LINES,LIGHT_QUESTIONS,MISS_LINES,NEXT_STATIONS,SONGS} from './first-night-content.js';
import {once,phase} from './story.js';
import {distance} from './navigation.js';
import {facePair} from './scene-one.js';
import {clamp,relation} from './world.js';

export type DrinkRecord={id:string;owner:string;status:'served'|'accepted'|'refused'|'consumed';purchasedBy:string;eventId:string};
export type DrinkOffer={eventId:string;drinkId:string;target:string;price:number;usesVoucher:boolean;status:'pending'|'accepted'|'refused'};
export type ThrowRecord={actor:string;round:number;aim:number;power:number;hit:boolean;eventId:string};
export type PendingThrow={id:string;actor:string;round:number;aim:number;power:number;startedAt:number};
export type RelationEvent={eventId:string;actor:string;target:string;action:string;drinkId?:string;witnessedBy:string[];accepted?:boolean;consumed?:boolean;reasonTag?:string};
export type FirstNightState={
  version:2;contentVersion:string;phase:'arrival'|'free_time'|'game_call'|'game_choice'|'game_round'|'post_game'|'meteor_window'|'rooftop'|'settled';
  enteredAt:number;stageAt:number;firstAction:boolean;openingBall:'rolling'|'returned'|'tried'|'ignored';openingActor:string;
  drinkMenuPage:number;pendingAim:number;
  cash:Record<string,number>;intoxication:Record<string,number>;drinkStage:Record<string,'sober'|'light'|'impaired'>;drinks:DrinkRecord[];drinkOffers:DrinkOffer[];
  feigningDrunk:Record<string,boolean>;stoppedDrinking:Record<string,boolean>;npcOrdered:Record<string,boolean>;
  gameChoice:''|'join'|'watch'|'decline';participants:string[];round:number;turn:number;throws:ThrowRecord[];landed:string[];winner:string;lastPlace:string;gameEndedAt:number;
  pendingThrow?:PendingThrow;cupAssist:boolean;
  postGameChoice:string;postGameMenu:boolean;postGameStep:string;vouchers:Record<string,number>;
  song:string;activity:string;question:string;questionAnswered:boolean;
  meteorAt:number;invited:string;invitationResult:''|'romance'|'friend'|'declined';inviteFrom:string;npcInviteResult:''|'romance'|'friend'|'declined';
  roofCompanions:string[];ending:string;nextStation:string;intoxFx:'full'|'low'|'off';lastRecoveryAt:number;bartenderBeats:number;
  rooftopChosen:boolean;gathered:boolean;seatedNearKiko:boolean;relationEvents:RelationEvent[];
};

const CAST=['A','B','C','D'] as const;
const THROW_LINE:Record<string,{x:number;z:number;yaw:number}>={
  USER:{x:-2.45,z:-1.5,yaw:90},
  A:{x:-2.45,z:-2.05,yaw:80},
  B:{x:-2.45,z:-.95,yaw:100},
  C:{x:-2.6,z:-2.45,yaw:75},
  D:{x:-2.6,z:-.55,yaw:105}
};
const REST:{[id:string]:{x:number;z:number;yaw:number}}={
  A:{x:-4,z:-3,yaw:48},B:{x:-3,z:3,yaw:142},C:{x:.85,z:-1.55,yaw:275},D:{x:3.4,z:.9,yaw:220}
};
const stage=(g:Engine,value:FirstNightState['phase'])=>{const s=g.world.firstNight!;if(s.phase===value)return;s.phase=value;s.stageAt=g.world.elapsed;phase(g,value);};
const thresholds:Record<string,[number,number]>={USER:[2,4],A:[1.5,3],B:[2,3.5],C:[2,4],D:[1.5,3.5]};
const bump=(g:Engine,id:string,target:string,delta:Partial<Relation>)=>{
  const current=g.actor(id).relations[target]||relation();
  for(const [key,value] of Object.entries(delta)){
    const field=key as keyof Relation;
    current[field]=clamp((current[field]??0)+(value??0));
  }
  g.actor(id).relations[target]=current;
};
const talkedWith=(g:Engine,id:string)=>g.world.events.some(e=>e.actor==='USER'&&e.target===id&&e.type==='speech');
const witnessed=(g:Engine,id:string,intent:string)=>g.world.events.some(e=>e.intent===intent&&e.perceptions.some(p=>p.actor===id&&p.level==='full'));
const gesture=(g:Engine,id:string,value:string)=>{const a=g.actor(id);a.gesture=value;a.gestureAt=g.world.elapsed;a.animation=value;};
const walkTo=(g:Engine,id:string,spot:{x:number;z:number;yaw:number},label:string)=>{
  const a=g.actor(id);if(!a.active||a.withdrawn)return;
  if(distance(a,spot)<.45){Object.assign(a,{yaw:spot.yaw,animation:'idle'});return;}
  if(a.destination===label&&a.route.length)return;
  g.go(a,spot,label);a.yaw=spot.yaw;
};
const conversationOpen=(g:Engine)=>{
  const w=g.world;
  if((w.replies||[]).some(r=>['queued','running'].includes(r.status)))return true;
  return w.jobs.some(j=>{
    const e=w.events.find(x=>x.id===j.eventId);
    return !!e&&e.type==='speech'&&(e.actor==='USER'||e.target==='USER');
  });
};
const noteRelation=(g:Engine,partial:Omit<RelationEvent,'witnessedBy'>&{witnessedBy?:string[]})=>{
  const s=g.world.firstNight!;
  const witnessedBy=partial.witnessedBy??g.world.events.find(e=>e.id===partial.eventId)?.perceptions.filter(p=>p.level==='full').map(p=>p.actor)??[];
  s.relationEvents.push({...partial,witnessedBy});
};
const willAccept=(s:FirstNightState,id:string,drinkId:string)=>{
  if(s.drinkStage[id]==='impaired'||s.stoppedDrinking[id])return false;
  return (DRINK_ACCEPT[id]??[]).includes(drinkId);
};
export function hydrateFirstNight(s:FirstNightState){
  s.feigningDrunk??={};s.stoppedDrinking??={};s.npcOrdered??={};s.cupAssist??=false;s.postGameStep??='';
  s.song??='';s.activity??='';s.question??='';s.questionAnswered??=false;s.inviteFrom??='';s.npcInviteResult??='';
  s.nextStation??='';s.intoxFx??='full';s.lastRecoveryAt??=s.enteredAt??0;s.bartenderBeats??=0;s.seatedNearKiko??=false;s.relationEvents??=[];
  s.drinkOffers??=[];s.rooftopChosen??=false;s.postGameChoice??='';s.postGameMenu??=false;s.vouchers??={USER:0};s.gathered??=false;
}

export function initializeFirstNight(g:Engine){
  if(g.world.firstNight)return;
  const cash:Record<string,number>={USER:18,A:30,B:30,C:24,D:24};
  const intoxication:Record<string,number>={USER:0,A:0,B:0,C:0,D:0};
  const drinkStage:Record<string,'sober'|'light'|'impaired'>={USER:'sober',A:'sober',B:'sober',C:'sober',D:'sober'};
  g.world.firstNight={version:2,contentVersion:FIRST_NIGHT_CONTENT_VERSION,phase:'arrival',enteredAt:g.world.elapsed,stageAt:g.world.elapsed,firstAction:false,openingBall:'rolling',openingActor:'C',drinkMenuPage:-1,pendingAim:-1,cash,intoxication,drinkStage,drinks:[],drinkOffers:[],feigningDrunk:{},stoppedDrinking:{},npcOrdered:{},gameChoice:'',participants:[],round:0,turn:0,throws:[],landed:[],winner:'',lastPlace:'',gameEndedAt:-1,cupAssist:false,postGameChoice:'',postGameMenu:false,postGameStep:'',vouchers:{USER:0},song:'',activity:'',question:'',questionAnswered:false,meteorAt:-1,invited:'',invitationResult:'',inviteFrom:'',npcInviteResult:'',roofCompanions:[],ending:'',nextStation:'',intoxFx:'full',lastRecoveryAt:g.world.elapsed,bartenderBeats:0,rooftopChosen:false,gathered:false,seatedNearKiko:false,relationEvents:[]};
  for(const id of CAST){
    const a=g.actor(id);a.name=CHARACTER_NAMES[id];a.active=true;a.withdrawn=false;
    a.knownFacts=[];a.beliefs=[];a.memory=[];
    a.knownActors=['USER','A','B','C','D','BARTENDER','OWNER'].filter(x=>x!==id);
  }
  const user=g.actor('USER');user.knownFacts=[];user.beliefs=[];user.memory=[];user.knownActors=['A','B','C','D','BARTENDER','OWNER'];
  for(const from of g.world.actors)for(const to of g.world.actors)if(from.id!==to.id)from.relations[to.id]=relation();
  Object.assign(g.actor('A').relations.C,{trust:.62,closeness:.48,attraction:.08,safety:.68,tension:.12,uncertainty:.28});
  Object.assign(g.actor('C').relations.A,{trust:.6,closeness:.46,attraction:.08,safety:.66,tension:.14,uncertainty:.3});
  Object.assign(g.actor('A').relations.B,{trust:.52,closeness:.28,attraction:.1,safety:.58,tension:.16,uncertainty:.48});
  Object.assign(g.actor('B').relations.A,{trust:.52,closeness:.28,attraction:.1,safety:.58,tension:.16,uncertainty:.48});
  Object.assign(g.actor('B').relations.C,{trust:.5,closeness:.22,attraction:.08,safety:.58,tension:.14,uncertainty:.5});
  g.world.initialRelations=Object.fromEntries(g.world.actors.filter(a=>a.id!=='USER').map(a=>[a.id,structuredClone(a.relations.USER)]));
  g.actor('A').goal='放松、看夜景，按自己的节奏认识眼前的人。';g.actor('A').voice='简洁具体，偶有干幽默；不把普通聊天转向项目。';
  g.actor('B').goal='享受现场，也确认别人是否在意她自己的选择。';g.actor('B').voice='自然轻快，认真时简短直接；不替全场做决定。';
  g.actor('C').goal='认真玩一局，也允许自己不负责照顾所有人。';g.actor('C').voice='行动快、有竞技心，熟后有干幽默；尊重身体边界。';
  g.actor('D').goal='按自己的判断尝试新玩法，拒绝被当成小朋友。';g.actor('D').voice='直接、反应快，谈具体问题时给出依据。';
  for(const id of CAST)Object.assign(g.actor(id),g.navigation.nearest(REST[id]),{yaw:REST[id].yaw,route:[],destination:'',animation:'idle'});
}

export function firstNightContext(g:Engine,id:string,event:Event){
  const s=g.world.firstNight;if(!s)return null;
  const pendingOffer=s.drinkOffers.find(o=>o.eventId===event.id&&o.status==='pending');
  const sawConsume=witnessed(g,id,'drink_consumed')||s.drinks.some(d=>d.owner==='USER'&&d.status==='consumed'&&g.world.events.some(e=>e.id===d.eventId&&e.perceptions.some(p=>p.actor===id&&p.level==='full')));
  const knownEvents=g.world.events.filter(e=>e.perceptions.some(p=>p.actor===id&&p.level==='full')).slice(-8).map(e=>({id:e.id,actor:e.actor,target:e.target,intent:e.intent,text:e.perceptions.find(p=>p.actor===id)!.text}));
  const peerCue=event.type==='action'&&event.actor===id&&event.intent.startsWith('peer_');
  const peerReply=['speech','message'].includes(event.type)&&event.actor!=='USER'&&event.target===id;
  return {
    contentVersion:s.contentVersion,phase:s.phase,location:g.zone(g.actor(id)).id,
    self:{cash:s.cash[id]??0,intoxicationPoints:s.intoxication[id]??0,drinkStage:s.drinkStage[id]??'sober',stoppedDrinking:!!s.stoppedDrinking[id],currentGoal:g.actor(id).goal},
    perceivedPlayer:{drinkStage:sawConsume?s.drinkStage.USER:'unknown',feigningDrunk:witnessed(g,id,'feign_drunk')?true:undefined},
    relationshipToPlayer:g.actor(id).relations.USER,
    knownEvents,
    drinkOffer:pendingOffer?{drink:DRINKS.find(d=>d.id===pendingOffer.drinkId),from:'USER'}:undefined,
    game:{choice:s.gameChoice,round:s.round,participant:s.participants.includes(id),winner:s.winner,lastPlace:s.lastPlace,cupAssist:s.cupAssist},
    meteor:{window:s.meteorAt>=0,invited:s.invited===id,inviteFrom:s.inviteFrom===id,result:s.invitationResult},
    peerExchange:peerCue?{role:'initiate',partner:event.target,topic:event.text}:peerReply?{role:'reply',partner:event.actor,latestLine:event.text}:undefined,
    availableActions:pendingOffer?['accept_drink','refuse_drink']:event.actor==='USER'&&event.target===id?['reply','ask','invite_friend','invite_romance','decline','wait']:peerCue||peerReply?['speak','ask','share','observe','wait']:['observe','wait']
  };
}

export function firstNightRuleDecision(g:Engine,id:string,eventId:string):Decision{
  const s=g.world.firstNight!,event=g.world.events.find(e=>e.id===eventId)!;
  const base:Record<string,string>={A:'嗯，我在听。',B:'你说，我听着。',C:'行，你接着说。',D:'可以，你具体说说。'};
  let expression=base[id]??'我听见了。',signal:'warm'|'probe'|'boundary'|'neutral'='neutral',intent='reply';
  if(event.intent==='rooftop_invite'){
    if(s.drinkStage[id]==='impaired'||s.stoppedDrinking[id]){expression=({A:'我先坐一会儿，今晚不上去了。',B:'我不太能走远，你先去吧。',C:'我得歇着。别等我。',D:'我先不去露台了。'} as Record<string,string>)[id]??'这次不了。';signal='boundary';intent='decline';}
    else if(s.invited===id&&s.invitationResult==='romance'){expression=({A:'可以。我们一起上去。',B:'好啊，正好想和你单独吹会儿风。',C:'走吧。别错过时间。',D:'好，我刚才就在等这个。'} as Record<string,string>)[id]??'好。';signal='warm';intent='invite_romance';}
    else if(s.invited===id&&s.invitationResult==='friend'){expression=({A:'可以，当作一起看场夜景。',B:'走吧，朋友也可以一起看。',C:'行，一起上去。',D:'可以，先去看流星。'} as Record<string,string>)[id]??'可以。';signal='warm';intent='invite_friend';}
    else {expression=({A:'谢谢你问我，但我今晚想自己待会儿。',B:'这次我先不上去了，你别在这儿等我。',C:'不了。我想自己吹会儿风。',D:'我不想一起去，但你可以自己上楼。'} as Record<string,string>)[id]??'这次不了。';signal='boundary';intent='decline';}
  }else if(event.intent==='feign_drunk'){
    if(id==='C'&&!s.drinks.some(d=>d.owner==='USER'&&d.status==='consumed')){expression='你没喝多少。如果真不舒服就坐下，别借这个靠近。';signal='boundary';intent='boundary';}
    else expression=base[id]??expression;
  }else if(event.objectTarget==='opening_ball'){
    expression=id==='C'?(event.intent==='return'?'谢了。等正式开局，你也来一球？':event.intent==='try'?'可以，先落桌再进杯。正式成绩还没开始算。':'没事，我自己捡。'):(base[id]??expression);
  }else if(event.intent==='drink_offer'){
    const drink=DRINKS.find(d=>d.id===event.objectTarget);
    const accept=!!drink&&willAccept(s,id,drink.id);
    intent=accept?'accept_drink':'refuse_drink';signal=accept?'warm':'probe';
    const alt=DRINKS.find(d=>d.id===DRINK_ALTERNATIVE[id]);
    expression=accept?`谢谢，${drink!.name}这个口味我愿意试试。`:alt&&alt.id!==drink?.id?`谢谢你先问。我这次不想喝${drink?.name??'这杯'}。如果可以，我更想要${alt.name}。`:`谢谢你先问。我这次不想喝${drink?.name??'这杯'}。`;
  }else if(event.intent==='game_call'){
    expression=({A:'规则再说一遍，桌面弹几次都算？',B:'彩头先讲清楚，大家再决定。',C:'行，这个我参加。',D:'我想试个不同的落点。'} as Record<string,string>)[id]??expression;
  }else if(event.intent==='peer_react_hit'){
    expression=HIT_LINES[id]??expression;intent='cheer';signal='warm';
  }else if(event.intent==='peer_react_miss'){
    expression=MISS_LINES[id]??expression;intent='ease';signal='neutral';
  }else if(event.intent==='peer_unwind'){
    expression=id==='C'?'今晚不复盘，训练馆的事留到明天。':expression;intent='chat';signal='neutral';
  }else if(event.intent==='peer_table'){
    expression=id==='B'?'要不要先看看球桌？正式开局还早。':expression;intent='invite';signal='neutral';
  }else if(event.actor==='USER'&&event.target===id){
    signal=event.intent==='boundary'?'boundary':'probe';
    if(/[名姓]|叫什/.test(event.text)){expression='我叫'+g.actor(id).name+'。你呢？';intent='introduce';}
    else if(id==='D'&&/小朋友|可爱/.test(event.text)){expression='我不是小朋友。有具体问题我可以讲清楚。';signal='boundary';intent='boundary';}
    else if(id==='A'&&/冷|不需要人|不需要亲密/.test(event.text)){expression='别替我下这个判断。';signal='boundary';intent='boundary';}
    else if(id==='B'&&/人脉|资源|买单|请大家/.test(event.text)){expression='今晚我不是来当场面资源的。';signal='boundary';intent='boundary';}
    else if(id==='C'&&/装醉|保护我|你保护/.test(event.text)){expression='需要帮忙可以说，但别用示弱让我负责照顾你。';signal='boundary';intent='boundary';}
    else if(s.openingBall==='returned'&&id==='C')expression='刚才那颗球是你捡的。正式开局时别紧张。';
    else if(s.openingBall==='tried'&&id==='C')expression='你刚才试过一球。正式成绩从零开始。';
  }
  const target=event.actor===id&&event.intent.startsWith('peer_')?event.target:event.actor==='USER'?'USER':event.target==='USER'?'USER':event.actor;
  return {generationSource:'rules',action:'speak',target,intent,expression,interpretation:'依据今夜亲见事件回应，不补写旧关系。',evidenceIds:[eventId],signal,confidence:1};
}

export function applyFirstNightDecision(g:Engine,id:string,decision:Decision,parentId:string){
  const s=g.world.firstNight;if(!s)return;
  const offer=s.drinkOffers.find(o=>o.eventId===parentId&&o.target===id&&o.status==='pending');
  if(!offer)return;
  const accepted=decision.intent==='accept_drink'&&decision.signal!=='boundary';
  if(!accepted){
    offer.status='refused';s.drinks.push({id:offer.drinkId,owner:id,status:'refused',purchasedBy:'USER',eventId:parentId});
    noteRelation(g,{eventId:parentId,actor:'USER',target:id,action:'drink_offer',drinkId:offer.drinkId,accepted:false,reasonTag:'refused'});
    return;
  }
  if((s.cash.USER??0)<offer.price){offer.status='refused';return;}
  offer.status='accepted';s.cash.USER-=offer.price;if(offer.usesVoucher)s.vouchers.USER=Math.max(0,(s.vouchers.USER??0)-1);
  s.drinks.push({id:offer.drinkId,owner:id,status:'accepted',purchasedBy:'USER',eventId:parentId});
  bump(g,id,'USER',{closeness:.04,trust:.02,uncertainty:-.02});
  const menu=DRINKS.find(d=>d.id===offer.drinkId);if(menu)bartenderMake(g,menu);
  const acceptedEvent=g.emit('action',id,'USER','drink_accepted',`${g.actor(id).name} 接过了这杯饮品。`,parentId,'normal',parentId,'script',offer.drinkId);
  noteRelation(g,{eventId:acceptedEvent.id,actor:'USER',target:id,action:'drink_offer',drinkId:offer.drinkId,accepted:true});
}

export function advanceFirstNight(g:Engine,seconds=.5){
  const s=g.world.firstNight;if(!s)return;
  hydrateFirstNight(s);
  stepNpcRoutes(g,seconds);
  sipAcceptedDrinks(g);
  recoverIntoxication(g);
  handleImpaired(g);
  playAmbientCast(g);
  npcSelfOrder(g);
  timeoutOpeningBall(g);
  gatherForGame(g);
  maybeNpcInvite(g);
  const since=g.world.elapsed-s.enteredAt;
  if(s.phase==='arrival'&&s.firstAction)stage(g,'free_time');
  const autoplayWaitingForPlayerDrink=process.env.LASTCALL_AUTOPLAY==='1'&&!s.drinks.some(d=>d.owner==='USER'&&d.status==='consumed');
  if(['arrival','free_time'].includes(s.phase)&&!s.gameChoice&&!autoplayWaitingForPlayerDrink&&since>=75&&(!conversationOpen(g)||since>=110)){
    if(s.openingBall==='rolling')s.openingBall='ignored';
    stage(g,'game_call');
    once(g,'first-night-game-call',()=>g.emit('system','OWNER','USER','game_call','到点了，桌上开一局弹球。球先落桌再进杯就算；赢家有特调券，末位可以选轻任务或付 2 Cash。','','normal','','script','bounce_table'));
  }
  if(s.phase==='game_call'&&g.world.elapsed-s.stageAt>=4&&!conversationOpen(g))stage(g,'game_choice');
  if(s.phase==='game_round')advanceNpcTurns(g);
  const playerOwesChoice=(s.winner==='USER'||s.lastPlace==='USER')&&!s.postGameChoice;
  if(s.phase==='post_game'&&!playerOwesChoice&&!s.postGameStep&&g.world.elapsed-s.gameEndedAt>=45&&(!conversationOpen(g)||g.world.elapsed-s.gameEndedAt>=80))openMeteor(g);
  if(s.phase==='free_time'&&s.gameChoice==='decline'&&since>=130&&(!conversationOpen(g)||since>=160))openMeteor(g);
  if(s.phase==='meteor_window'&&s.rooftopChosen&&g.actor('USER').area==='rooftop'){
    stage(g,'rooftop');g.emit('movement','USER','OWNER','rooftop_arrival',s.roofCompanions.length?'你和同行的人走完楼梯，抵达上层露台。':'你独自走完楼梯，抵达上层露台。','','normal','','player','rooftop');
  }
}

function openMeteor(g:Engine){
  const s=g.world.firstNight!;if(s.meteorAt>=0||s.phase==='meteor_window'||s.phase==='rooftop'||s.phase==='settled')return;
  s.meteorAt=g.world.elapsed;stage(g,'meteor_window');
  g.emit('system','OWNER','USER','meteor_window','流星雨快到了。楼上的露台视野更开阔，你可以邀请一个人，也可以自己去。','','normal','','script','rooftop');
}

function timeoutOpeningBall(g:Engine){
  // During a cold Shipping start the render thread may compile shaders while
  // the managed local service already advances its accelerated audit clock.
  // The real player route keeps the 25 second natural timeout; the explicit
  // automated route waits until its scripted opening action is delivered.
  if(process.env.LASTCALL_AUTOPLAY==='1')return;
  const s=g.world.firstNight!;
  if(s.openingBall!=='rolling')return;
  if(g.world.elapsed-s.enteredAt<25)return;
  s.openingBall='ignored';
  g.emit('action','C','USER','ignore','万塞自己把滚来的球捡了回去，没有把这当成失礼。','','normal','','script','opening_ball');
}

function playAmbientCast(g:Engine){
  const s=g.world.firstNight!;if(!['arrival','free_time'].includes(s.phase))return;
  const since=g.world.elapsed-s.enteredAt;
  const user=g.actor('USER');
  if(since>=8)once(g,'first-night-x-greet',()=>{
    g.emit('action','B','USER','greet','X 朝入口点了点头，没有把你当成必须由她接待的客人。','','normal','','script');
  });
  if(since>=12)once(g,'first-night-ac-chat',()=>{
    queuePeerCue(g,'C','A','peer_unwind','万塞结束了训练话题，想和 Kiko 说一句下班后的闲话。');
  });
  if(since>=18&&distance(user,g.actor('A'))<2.4)once(g,'first-night-kiko-seat',()=>{
    g.emit('speech','A','USER','sit_offer','这边空着。你要坐吗？','','normal','','script');
  });
  if(since>=24&&distance(user,g.actor('D'))<2.6)once(g,'first-night-yitong-window',()=>{
    g.emit('speech','D','USER','ask','旁边这扇窗和楼上露台，你觉得哪边更适合看夜景？','','normal','','script');
  });
  if(since>=28)once(g,'first-night-bd-look',()=>{
    queuePeerCue(g,'B','D','peer_table','X 看向球桌，想问一桐要不要先过去看看。');
  });
}

// A peer cue is a visible intention, not prerecorded dialogue.  The initiating actor receives one
// model job; their spoken result is then directed to exactly one partner, who alone may answer.
export function queuePeerCue(g:Engine,from:string,target:string,intent:string,text:string,objectTarget=''){
  const e=g.emit('action',from,target,intent,text,'','normal','','script',objectTarget);
  if(e.perceptions.some(p=>p.actor===from)&&!g.world.jobs.some(j=>j.actor===from&&j.eventId===e.id))
    g.world.jobs.push({actor:from,eventId:e.id,due:g.world.elapsed+.6});
  return e;
}

function npcSelfOrder(g:Engine){
  const s=g.world.firstNight!;if(!['arrival','free_time'].includes(s.phase))return;
  CAST.forEach((id,index)=>{
    if(s.npcOrdered[id]||s.stoppedDrinking[id]||s.drinkStage[id]==='impaired')return;
    if(g.world.elapsed-s.enteredAt<20+index*8)return;
    const drink=DRINKS.find(d=>d.id===DRINK_SELF_ORDER[id]);if(!drink)return;
    if((s.cash[id]??0)<drink.price)return;
    s.npcOrdered[id]=true;s.cash[id]-=drink.price;
    const e=g.emit('action',id,'BARTENDER','drink_served',`${g.actor(id).name} 自己点了${drink.name}。`,'','normal','','script',drink.id);
    s.drinks.push({id:drink.id,owner:id,status:'accepted',purchasedBy:id,eventId:e.id});
    bartenderMake(g,drink);
  });
}

function bartenderMake(g:Engine,drink:{id:string;name:string;recipe:string}){
  const s=g.world.firstNight!;s.bartenderBeats++;
  const beat=s.bartenderBeats%3;
  const text=beat===1?`调酒师核对酒单，取杯，开始做${drink.name}。`:beat===2?`冰块入杯，${drink.recipe.split('；')[0]}，${drink.name}正在成形。`:`${drink.name}做好了，推到吧台边。今晚只按酒单做，没有现场自创。`;
  g.emit('action','BARTENDER','USER','make_drink',text,'','normal','','script',drink.id);
}

function gatherForGame(g:Engine){
  const s=g.world.firstNight!;
  if(['game_call','game_choice','game_round'].includes(s.phase)){
    const lineup=s.participants.length?s.participants:CAST.slice();
    for(const id of lineup){
      if(id==='USER')continue;
      if(id==='A'&&s.phase!=='game_round'&&g.world.elapsed-s.stageAt<3)continue;
      walkTo(g,id,THROW_LINE[id],`throw:${id}`);
    }
    if(s.phase==='game_round'||g.world.elapsed-s.stageAt>=3)s.gathered=true;
  }else if(s.phase==='post_game'&&s.gathered){
    for(const id of CAST)walkTo(g,id,REST[id],`rest:${id}`);
    s.gathered=false;
  }
}

function maybeNpcInvite(g:Engine){
  const s=g.world.firstNight!;
  if(s.phase!=='meteor_window'||s.invited||s.inviteFrom||s.rooftopChosen)return;
  if(g.world.elapsed-s.meteorAt<12||conversationOpen(g))return;
  const ranked=[...CAST].filter(id=>talkedWith(g,id)&&s.drinkStage[id]!=='impaired'&&!s.stoppedDrinking[id])
    .sort((a,b)=>(g.actor(b).relations.USER.closeness||0)-(g.actor(a).relations.USER.closeness||0));
  const id=ranked.find(x=>(g.actor(x).relations.USER.closeness||0)>=.32);if(!id)return;
  s.inviteFrom=id;
  g.emit('speech',id,'USER','rooftop_invite',`${g.actor(id).name} 问你要不要一起上露台看流星雨。`,'','normal','','script','rooftop');
}

function stepNpcRoutes(g:Engine,seconds=.5){
  const speed=1.35,dt=Math.max(.05,Math.min(2,seconds));
  for(const a of g.world.actors){
    if(a.id==='USER'||!a.active||!a.route.length)continue;
    const p=a.route[0],d=distance(a,p),step=Math.min(d,speed*dt);
    if(d<.05){a.route.shift();if(!a.route.length)a.animation='idle';continue;}
    a.x+=((p.x-a.x)/d)*step;a.z+=((p.z-a.z)/d)*step;a.y=p.y??a.y;a.area=p.area??a.area;
    a.yaw=Math.atan2(p.x-a.x,p.z-a.z)*180/Math.PI;a.animation='walk';
    if(distance(a,p)<.28)a.route.shift();
    if(!a.route.length){a.animation='idle';a.destination='';}
  }
}

function sipAcceptedDrinks(g:Engine){
  const s=g.world.firstNight!;
  for(const drink of s.drinks){
    if(drink.status!=='accepted'||drink.owner==='USER')continue;
    const cause=g.world.events.find(e=>e.id===drink.eventId);if(!cause||g.world.elapsed-cause.time<8)continue;
    const menu=DRINKS.find(d=>d.id===drink.id);if(!menu)continue;
    drink.status='consumed';s.intoxication[drink.owner]=(s.intoxication[drink.owner]??0)+menu.intoxication;updateDrinkStage(s,drink.owner);
    const e=g.emit('action',drink.owner,'USER','drink_consumed',`${g.actor(drink.owner).name} 喝了一口${menu.name}。`,'','normal','','script',drink.id);
    noteRelation(g,{eventId:e.id,actor:drink.owner,target:'USER',action:'drink_consumed',drinkId:drink.id,consumed:true});
  }
}

function recoverIntoxication(g:Engine){
  const s=g.world.firstNight!;
  if(g.world.elapsed-(s.lastRecoveryAt||s.enteredAt)<90)return;
  s.lastRecoveryAt=g.world.elapsed;
  for(const id of ['USER',...CAST]){
    if((s.intoxication[id]??0)<=0)continue;
    s.intoxication[id]=Math.max(0,(s.intoxication[id]??0)-.5);
    updateDrinkStage(s,id);
    if(s.drinkStage[id]!=='impaired')s.stoppedDrinking[id]=false;
  }
}

function handleImpaired(g:Engine){
  const s=g.world.firstNight!;
  for(const id of CAST){
    if(s.drinkStage[id]!=='impaired')continue;
    s.stoppedDrinking[id]=true;
    g.actor(id).posture='sit';
  }
}

function advanceNpcTurns(g:Engine){
  const s=g.world.firstNight!;if(!s.participants.length||s.winner&&s.lastPlace)return;
  if(s.pendingThrow)return;
  const actor=s.participants[s.turn%s.participants.length];if(actor==='USER')return;
  if(g.world.elapsed-s.stageAt<1.4)return;
  if(s.drinkStage[actor]==='impaired'){s.turn=(s.turn+1)%s.participants.length;if(s.turn===0)rotateRound(s);s.stageAt=g.world.elapsed;return;}
  const assisted=s.cupAssist||s.round>=4;
  const aim=assisted?.62+g.random()*.1:.35+g.random()*.55,power=assisted?.52+g.random()*.12:.35+g.random()*.55;
  beginPhysicalThrow(g,actor,aim,power);
}

function beginPhysicalThrow(g:Engine,actor:string,aim:number,power:number){
  const s=g.world.firstNight!;
  if(s.pendingThrow)throw new Error('上一颗球仍在运动');
  s.pendingThrow={id:`bounce:${s.round}:${s.turn}:${actor}:${s.throws.length}`,actor,round:s.round,aim,power,startedAt:g.world.elapsed};
  gesture(g,actor,'throw');
  g.emit('action',actor,'OWNER','bounce_launch',`${g.actor(actor).name} 投出了球。`,'','normal','','script','bounce_ball');
}

function recordThrow(g:Engine,actor:string,aim:number,power:number,hit:boolean){
  const s=g.world.firstNight!;const e=g.emit('action',actor,'OWNER',hit?'bounce_hit':'bounce_miss',hit?`${g.actor(actor).name} 的球先落在桌面，再弹进了目标杯。`:`${g.actor(actor).name} 投出一球，球没有进入目标杯。`,'','normal','','script','bounce_ball');
  s.throws.push({actor,round:s.round,aim,power,hit,eventId:e.id});
  if(hit&&!s.landed.includes(actor)){s.landed.push(actor);if(!s.winner)s.winner=actor;}
  const active=s.participants.filter(id=>!s.landed.includes(id)&&s.drinkStage[id]!=='impaired');
  if(actor!=='USER')bump(g,actor,'USER',{closeness:.015,trust:.01});
  const speaker=actor==='C'?'B':actor==='B'?'C':actor==='A'?'D':'A';
  queuePeerCue(g,speaker,actor,hit?'peer_react_hit':'peer_react_miss',
    hit?`${g.actor(speaker).name} 看清了 ${g.actor(actor).name} 的进球，准备自然回应。`:`${g.actor(speaker).name} 看见 ${g.actor(actor).name} 失手，准备自然回应。`,'bounce_ball');
  // Nobody may be assigned last place before they have physically thrown at
  // least once.  In the old flow USER/A/B/C could all land in turn and D was
  // immediately declared last without ever releasing a ball in Unreal.
  const everyoneAttempted=s.participants.every(id=>s.throws.some(record=>record.actor===id));
  if(everyoneAttempted&&active.length===0){s.lastPlace=actor;finishGame(g);return;}
  if(everyoneAttempted&&active.length===1&&s.landed.length){s.lastPlace=active[0];finishGame(g);return;}
  s.turn=(s.turn+1)%s.participants.length;if(s.turn===0)rotateRound(s);
  s.stageAt=g.world.elapsed;
  if(s.round>=8&&active.length>1){s.lastPlace='none';finishGame(g);}
}

function rotateRound(s:FirstNightState){
  s.round++;
  if(s.participants.length)s.participants.push(s.participants.shift()!);
  if(s.round>=4&&!s.cupAssist)s.cupAssist=true;
}

function finishGame(g:Engine){
  const s=g.world.firstNight!;s.gameEndedAt=g.world.elapsed;stage(g,'post_game');
  const winner=s.winner?g.actor(s.winner).name:'没有人';const tail=s.lastPlace==='none'?'其余人并列未上岸，不指定末位。':s.lastPlace?`${g.actor(s.lastPlace).name} 最后还没上岸。`:'';
  g.emit('system','OWNER','USER','game_result',`${winner} 先命中。${tail}`,'','normal','','script','bounce_table');
  for(const id of s.participants)if(id!=='USER')bump(g,id,'USER',{closeness:.03,trust:.02,uncertainty:-.02});
}

function updateDrinkStage(s:FirstNightState,id:string){const [light,impaired]=thresholds[id]??[2,4],points=s.intoxication[id]??0;s.drinkStage[id]=points>=impaired?'impaired':points>=light?'light':'sober';}

function decideInvitation(g:Engine,target:string){
  const s=g.world.firstNight!,r=g.actor(target).relations.USER,impaired=s.drinkStage[target]==='impaired'||s.stoppedDrinking[target];
  if(impaired||r.safety<.4)return 'declined' as const;
  if(!talkedWith(g,target))return 'declined' as const;
  if(s.feigningDrunk.USER&&!s.drinks.some(d=>d.owner==='USER'&&d.status==='consumed'))return 'declined' as const;
  if(r.attraction>.38&&r.closeness>.38)return 'romance' as const;
  if(r.trust>.48&&r.closeness>.33)return 'friend' as const;
  return 'declined' as const;
}

function attitudeOf(g:Engine,id:string){
  const r=g.actor(id).relations.USER,talked=talkedWith(g,id);
  const stageLabel=!talked?'刚认识':r.closeness>.4&&r.safety>.55?'愿意单独相处':r.closeness>.32||r.trust>.52?'聊得来':'刚认识';
  const s=g.world.firstNight!;
  let reason='今晚还没有共同做过一件具体的事。';
  if(s.invited===id||s.inviteFrom===id)reason=s.invitationResult==='declined'?'她记得你问过她要不要一起看流星雨，也记得自己的答复。':s.invitationResult==='romance'?'她答应和你一起上露台。':'她愿意以朋友身份一起看流星雨。';
  else if(s.openingBall!=='rolling'&&id==='C')reason=s.openingBall==='ignored'?'她自己把滚来的球捡了回去，没有把这当成失礼。':s.openingBall==='tried'?'她记得你在正式比赛前试过一球。':'她记得是你把滚来的球递回去的。';
  else if(s.drinks.some(d=>d.owner===id&&d.status==='refused'))reason='她记得你问过她想不想喝，也记得自己说过这次不想。';
  else if(s.drinks.some(d=>d.owner===id&&(d.status==='accepted'||d.status==='consumed')))reason='她记得你请她喝过一杯，并且先问过她愿不愿意。';
  else if(s.participants.includes(id)&&s.participants.includes('USER'))reason='她记得你们刚在同一局里投过球。';
  else if(talked)reason='她记得你们今晚说过话，内容以她亲耳听见的为准。';
  return {id,name:g.actor(id).name,stage:stageLabel,reason,drinkStage:s.drinkStage[id]??'sober'};
}

function keyActionsOf(g:Engine){
  const s=g.world.firstNight!,actions:string[]=[];
  if(s.openingBall==='returned')actions.push('开场时把滚来的球递回给万塞');
  else if(s.openingBall==='tried')actions.push('开场时试投了一球，不计入正式成绩');
  else if(s.openingBall==='ignored')actions.push('开场时没有去捡那颗球');
  if(s.feigningDrunk.USER)actions.push('装作有些醉意，实际醉意并未因此改变');
  const consumed=s.drinks.filter(d=>d.status==='consumed');
  if(consumed.length)actions.push(`真正喝下了 ${consumed.map(d=>DRINKS.find(x=>x.id===d.id)?.name??d.id).join('、')}`);
  const gifted=s.drinks.filter(d=>d.owner!=='USER'&&d.purchasedBy==='USER'&&d.status!=='refused');
  if(gifted.length)actions.push(`请人喝过 ${gifted.map(d=>DRINKS.find(x=>x.id===d.id)?.name??d.id).join('、')}`);
  if(s.gameChoice==='join')actions.push(s.winner==='USER'?'加入五人弹球并先命中':s.lastPlace==='USER'?'加入五人弹球，最后未上岸':s.winner?`加入五人弹球，${g.actor(s.winner).name} 先命中`:'加入了五人弹球');
  else if(s.gameChoice==='watch')actions.push('旁观了四人弹球局');
  else if(s.gameChoice==='decline')actions.push('没有参加弹球局');
  if(s.song)actions.push(`点播了${SONGS.find(x=>x.id===s.song)?.label??s.song}`);
  if(s.activity)actions.push(`选定了赛后活动：${ACTIVITIES.find(x=>x.id===s.activity)?.label??s.activity}`);
  if(s.invitationResult==='romance')actions.push(`与 ${g.actor(s.invited||s.inviteFrom).name} 浪漫同行上露台`);
  else if(s.invitationResult==='friend')actions.push(`与 ${g.actor(s.invited||s.inviteFrom).name} 以朋友身份一同看流星雨`);
  else if(s.invited)actions.push(`${g.actor(s.invited).name} 拒绝了一同看流星雨`);
  if(s.ending)actions.push(s.ending);
  return actions;
}

function evaluationOf(g:Engine,id:string){
  const name=g.actor(id).name,talked=talkedWith(g,id),s=g.world.firstNight!;
  const privateToOthers=g.world.events.filter(e=>e.actor==='USER'&&e.type==='speech'&&e.target!==id&&e.target!=='USER'&&!e.perceptions.some(p=>p.actor===id&&p.level==='full'));
  void privateToOthers;
  if(!talked&&!witnessed(g,id,'opening_ball')&&!s.participants.includes(id)&&!s.drinks.some(d=>d.owner===id))return `${name}：今晚我们没怎么聊。`;
  if(id==='A'){
    if(s.seatedNearKiko&&talked)return `${name}：你在观景位坐下来，也没有急着把聊天变成项目。`;
    if(s.drinks.some(d=>d.owner==='A'&&d.status==='refused')&&talked)return `${name}：你问过我不想喝的那杯，后来也没有把拒绝当成需要解释的谜。`;
    if(s.participants.includes('A')&&s.participants.includes('USER'))return `${name}：我记得我们看过同一条球路。你有把话说完。`;
    return talked?`${name}：今晚你愿意听我说完眼前的事，这就够当作认识。`:`${name}：我看见你在场，但我们几乎没说上话。`;
  }
  if(id==='B'){
    if(s.invitationResult&&(s.invited==='B'||s.inviteFrom==='B'))return `${name}：你把我当成同行的人来问，而不是来借场面或资源。`;
    return talked?`${name}：你接住过我的话，也没有默认今晚该由我照顾所有人。`:`${name}：我们没怎么聊。我不会凭远远一眼替你下结论。`;
  }
  if(id==='C'){
    if(s.feigningDrunk.USER&&witnessed(g,'C','feign_drunk')&&!s.drinks.some(d=>d.owner==='USER'&&d.status==='consumed'))return `${name}：你装醉的时候我看见了。需要帮忙可以说，别靠这个让我负责。`;
    if(s.lastPlace==='USER'&&s.participants.includes('C'))return `${name}：你输了也认。这比球技更有意思。`;
    if(s.winner==='USER'&&s.participants.includes('C'))return `${name}：你先上岸。下一次要不要再来一球，我自己决定。`;
    if(s.openingBall==='ignored')return `${name}：你没捡那颗球，我不当成失礼。`;
    return talked?`${name}：我们玩过或说过话。你没有靠示弱让我负责照顾你。`:`${name}：今晚交集不多，我不会据此判断你这个人。`;
  }
  if(s.participants.includes('D')&&s.participants.includes('USER'))return `${name}：你看见我试过不同落点，也没有把我叫成小朋友。`;
  return talked?`${name}：你问过我想看什么，并听完了。`:`${name}：今晚几乎没交流，我不评价没听见的事。`;
}

function summarizeNight(g:Engine){
  const s=g.world.firstNight!,shared=s.invitationResult==='romance'||s.invitationResult==='friend';
  const game=s.winner?`${g.actor(s.winner).name} 先命中`:'本局没有产生唯一赢家';
  return `${s.ending||'首夜结束'}。${shared?'本晚达成了与人共同看流星雨。':'本晚没有与人共同看流星雨，关系可以留到下次。'}比赛：${game}。剩余 ${s.cash.USER} Cash。攀岩馆与温泉尚未开放。`;
}

function applyActivity(g:Engine,id:string){
  if(id==='second_throw')g.emit('system','OWNER','USER','reward_activity','你获得一次轻松试投。这一球不算正式成绩。','','normal','','script','bounce_table');
  else if(id==='window_look'){walkTo(g,'D',REST.D,'window');g.emit('action','D','USER','window_look','一桐朝窗边让了让，夜景还在。','','normal','','script');}
  else if(id==='bar_round')g.emit('system','BARTENDER','USER','bar_round','愿意的人可以去吧台坐一会儿。特调券可以在那里用。','','normal','','script');
}

export function firstNightCommand(g:Engine,c:Command){
  const s=g.world.firstNight;if(!s)return false;
  hydrateFirstNight(s);
  switch(c.type){
    case 'talk':{
      const target=g.actor(c.target??'');const user=g.actor('USER');const text=(c.text??'').trim();
      if(!['A','B','C','D'].includes(target.id)||!target.active)throw new Error('请先选择在场的人');
      if(!text||[...text].length>200)throw new Error('请输入 1–200 个字符');
      if(distance(user,target)>3||!g.navigation.visible(user,target))throw new Error('请先走近对方，并确认中间没有遮挡');
      s.firstAction=true;facePair(g,'USER',target.id);
      if(s.postGameStep==='question'&&s.question){s.questionAnswered=true;s.postGameStep='';}
      if(s.drinks.some(d=>d.owner===target.id&&d.status==='refused'))bump(g,target.id,'USER',{safety:.04,trust:.02});
      bump(g,target.id,'USER',{closeness:.04,trust:.03,uncertainty:-.02});
      const e=g.emit('speech','USER',target.id,c.intent||'talk',text,'','normal','','player',c.objectTarget??'');
      noteRelation(g,{eventId:e.id,actor:'USER',target:target.id,action:'talk'});
      return true;
    }
    case 'opening_ball':{
      if(s.openingBall!=='rolling')throw new Error('这颗球已经有人处理了');
      if(c.intent==='return')s.openingBall='returned';else if(c.intent==='try')s.openingBall='tried';else if(c.intent==='ignore')s.openingBall='ignored';else throw new Error('无效的开场动作');
      s.firstAction=true;
      if(s.openingBall!=='ignored')bump(g,'C','USER',{closeness:.03,trust:.02});
      g.emit('action','USER',s.openingActor,c.intent||'observe',c.intent==='return'?'你捡起球，递回给万塞。':c.intent==='try'?'你问过之后，试着投了一球。':'你没有打断眼前的事，球由旁人捡了回去。','','normal','','player','opening_ball');return true;
    }
    case 'order_drink':{
      const drink=DRINKS.find(x=>x.id===c.objectTarget);if(!drink)throw new Error('酒单里没有这款饮品');
      const owner=c.target&&['A','B','C','D'].includes(c.target)?c.target:'USER';const usesVoucher=(s.vouchers.USER??0)>0;const price=usesVoucher?0:drink.price;if((s.cash.USER??0)<price)throw new Error('Cash 不足');s.drinkMenuPage=-1;
      if(owner!=='USER'){
        if(s.drinkOffers.some(o=>o.target===owner&&o.status==='pending'))throw new Error('先等对方回应上一杯饮品');
        if(distance(g.actor('USER'),g.actor(owner))>3||!g.navigation.visible(g.actor('USER'),g.actor(owner)))throw new Error('请先走近并看向对方');
        const line=usesVoucher?`我想用特调券请你去吧台喝${drink.name}，你愿意吗？`:`我想请你喝${drink.name}，你愿意吗？`;
        const e=g.emit('speech','USER',owner,'drink_offer',line,'','normal','','player',drink.id);
        s.drinkOffers.push({eventId:e.id,drinkId:drink.id,target:owner,price,usesVoucher,status:'pending'});return true;
      }
      s.cash.USER-=price;if(usesVoucher)s.vouchers.USER=Math.max(0,s.vouchers.USER-1);bartenderMake(g,drink);const e=g.emit('action','USER',owner,'drink_served',usesVoucher?`你用特调券点了${drink.name}。`:`你点了${drink.name}（${drink.price} Cash）。`,'','normal','','player',drink.id);s.drinks.push({id:drink.id,owner,status:'served',purchasedBy:'USER',eventId:e.id});return true;
    }
    case 'drink_menu':{
      const pages=Math.ceil(DRINKS.length/2);
      if(c.intent==='close')s.drinkMenuPage=-1;else if(c.intent==='next')s.drinkMenuPage=(Math.max(0,s.drinkMenuPage)+1)%pages;else s.drinkMenuPage=0;return true;
    }
    case 'consume_drink':{
      const record=[...s.drinks].reverse().find(x=>x.owner==='USER'&&x.id===c.objectTarget&&x.status==='served');if(!record)throw new Error('手边没有这杯饮品');
      const drink=DRINKS.find(x=>x.id===record.id)!;record.status='consumed';s.intoxication.USER+=drink.intoxication;updateDrinkStage(s,'USER');
      if(s.drinkStage.USER==='impaired')s.stoppedDrinking.USER=true;
      g.emit('action','USER','BARTENDER','drink_consumed',`你喝下了${drink.name}。`,'','normal','','player',drink.id);return true;
    }
    case 'feign_drunk':{
      s.feigningDrunk.USER=true;s.firstAction=true;
      g.emit('action','USER','OWNER','feign_drunk','你装作有些醉意，步态和说话都放慢了一点。实际醉意没有因此改变。','','normal','','player');
      if(distance(g.actor('USER'),g.actor('C'))<3)g.emit('speech','USER','C','feign_drunk','（装作有些站不稳）','','normal','','player');
      return true;
    }
    case 'sit_view':{
      const user=g.actor('USER');user.posture='sit';s.firstAction=true;
      s.seatedNearKiko=distance(user,g.actor('A'))<2.6;
      if(s.seatedNearKiko)bump(g,'A','USER',{closeness:.03,safety:.02});
      g.emit('action','USER',s.seatedNearKiko?'A':'OWNER','sit_view',s.seatedNearKiko?'你在 Kiko 旁边的观景位坐了下来。':'你找了个位子坐下。','','normal','','player');return true;
    }
    case 'bounce_choice':{
      if(s.phase!=='game_choice')throw new Error('工作人员还没有开始报名');if(!['join','watch','decline'].includes(c.intent??''))throw new Error('无效的参赛选择');s.gameChoice=c.intent as FirstNightState['gameChoice'];
      if(s.gameChoice==='join'){s.participants=['USER','A','B','C','D'];s.round=1;s.turn=0;stage(g,'game_round');}
      else if(s.gameChoice==='watch'){s.participants=['A','B','C','D'];s.round=1;s.turn=0;stage(g,'game_round');}
      else {s.participants=[];stage(g,'free_time');}
      g.emit('action','USER','OWNER',`bounce_${s.gameChoice}`,s.gameChoice==='join'?'你走到投球线后，加入了这一局。':s.gameChoice==='watch'?'你留在桌边，先看这一局。':'你谢绝了这一局，继续留在酒吧。','','normal','','player','bounce_table');return true;
    }
    case 'throw_ball':{
      if(s.phase!=='game_round'||s.gameChoice!=='join'||s.participants[s.turn%s.participants.length]!=='USER')throw new Error('现在还没轮到你');
      let aim=s.pendingAim>=0?s.pendingAim:Math.max(0,Math.min(1,Number(c.x)||0));const power=Math.max(0,Math.min(1,Number(c.z)||0));s.pendingAim=-1;
      if(s.drinkStage.USER==='light')aim=Math.max(0,Math.min(1,aim+(g.random()-.5)*.08));
      if(s.drinkStage.USER==='impaired')aim=Math.max(0,Math.min(1,aim+(g.random()-.5)*.18));
      beginPhysicalThrow(g,'USER',aim,power);return true;
    }
    case 'set_throw_aim':{
      if(s.phase!=='game_round'||s.participants[s.turn%s.participants.length]!=='USER')throw new Error('现在还没轮到你');s.pendingAim=Math.max(0,Math.min(1,Number(c.x)||0));return true;
    }
    case 'bounce_result':{
      const pending=s.pendingThrow;
      if(!pending||c.requestId!==pending.id)throw new Error('投球结果已过期或不属于当前回合');
      const tableContacts=Math.max(0,Math.floor(Number(c.x)||0));
      const hit=!!c.open&&tableContacts>0;
      s.pendingThrow=undefined;
      recordThrow(g,pending.actor,pending.aim,pending.power,hit);return true;
    }
    case 'post_game_choice':{
      if(s.phase!=='post_game')throw new Error('现在不是赛后结算时间');
      const value=c.intent??'';
      if(s.winner==='USER'&&!s.postGameChoice){
        if(!['reward_voucher','reward_song','reward_activity'].includes(value))throw new Error('请选择一个赢家彩头');
        s.postGameChoice=value;if(value==='reward_voucher')s.vouchers.USER=(s.vouchers.USER??0)+1;
        if(value==='reward_song'){s.postGameStep='songs';return true;}
        if(value==='reward_activity'){s.postGameStep='activities';return true;}
      }else if(s.lastPlace==='USER'&&!s.postGameChoice){
        if(value==='penalty_tasks'){s.postGameMenu=true;return true;}
        if(!['penalty_mocktail','penalty_question','penalty_song','penalty_pay'].includes(value))throw new Error('请选择一项末位处理');
        if(value==='penalty_pay'){if((s.cash.USER??0)<2)throw new Error('Cash 不足');s.cash.USER-=2;}
        if(value==='penalty_mocktail'){
          const mock=DRINKS.find(d=>d.id==='lime_soda_0')!;bartenderMake(g,mock);
          const e=g.emit('action','BARTENDER','USER','penalty_mocktail','吧台做了一杯青柠苏打，推到你面前。','','normal','','script',mock.id);
          s.drinks.push({id:mock.id,owner:'USER',status:'served',purchasedBy:'OWNER',eventId:e.id});
        }
        if(value==='penalty_question'){s.question=LIGHT_QUESTIONS[Math.floor(g.random()*LIGHT_QUESTIONS.length)];s.postGameStep='question';s.postGameChoice=value;return true;}
        if(value==='penalty_song'){s.postGameChoice=value;s.postGameStep='songs';return true;}
        s.postGameChoice=value;
      }else throw new Error('你没有待处理的赛后彩头');
      g.emit('action','USER','OWNER',value,`你完成了赛后选择：${value.replace(/^reward_|^penalty_/,'')}。`,'','normal','','player','bounce_table');return true;
    }
    case 'pick_song':{
      const song=SONGS.find(x=>x.id===c.objectTarget||x.id===c.intent);if(!song)throw new Error('没有这首歌');
      s.song=song.id;s.postGameStep='';
      g.emit('system','BARTENDER','USER','play_song',`吧台换上了${song.label}，整场气氛跟着慢了一拍。`,'','normal','','script');return true;
    }
    case 'pick_activity':{
      const activity=ACTIVITIES.find(x=>x.id===c.objectTarget||x.id===c.intent);if(!activity)throw new Error('没有这项活动');
      s.activity=activity.id;s.postGameStep='';applyActivity(g,activity.id);return true;
    }
    case 'invite_rooftop':{
      if(s.phase!=='meteor_window')throw new Error('流星雨邀请还没有开放');const target=c.target??'';if(!['A','B','C','D'].includes(target))throw new Error('请选择一个在场的人');if(s.invited)throw new Error('你已经发出过今晚的邀请');s.invited=target;
      s.invitationResult=decideInvitation(g,target);s.roofCompanions=s.invitationResult==='declined'?[]:[target];
      const e=g.emit('speech','USER',target,'rooftop_invite','要不要一起上露台看流星雨？','','normal','','player','rooftop');
      noteRelation(g,{eventId:e.id,actor:'USER',target,action:'rooftop_invite',accepted:s.invitationResult!=='declined',reasonTag:s.invitationResult});
      return true;
    }
    case 'npc_invite_reply':{
      if(!s.inviteFrom||s.invited)throw new Error('现在没有待回应的邀请');
      const target=s.inviteFrom;
      if(c.intent==='accept_romance'){s.invitationResult='romance';s.roofCompanions=[target];s.npcInviteResult='romance';}
      else if(c.intent==='accept_friend'){s.invitationResult='friend';s.roofCompanions=[target];s.npcInviteResult='friend';}
      else {s.invitationResult='declined';s.roofCompanions=[];s.npcInviteResult='declined';}
      s.invited=target;
      g.emit('speech','USER',target,c.intent||'decline',c.intent==='accept_romance'?'好，我们一起上去。':c.intent==='accept_friend'?'可以，当作朋友一起看。':'这次我想自己待着。','','normal','','player','rooftop');return true;
    }
    case 'go_rooftop':{
      if(s.phase!=='meteor_window')throw new Error('露台还没有开放');if(s.rooftopChosen)return true;s.rooftopChosen=true;
      for(const id of s.roofCompanions)g.go(g.actor(id),g.location('rooftop'),'rooftop');
      g.emit('action','USER','OWNER','rooftop_route','你决定沿亮灯楼梯前往上层露台。需要亲自走到楼梯尽头。','','normal','','player','rooftop');return true;
    }
    case 'set_intox_fx':{
      s.intoxFx=c.intent==='off'||c.intent==='low'||c.intent==='full'?c.intent:'full';return true;
    }
    case 'choose_next':{
      const station=NEXT_STATIONS.find(x=>x.id===c.objectTarget||x.id===c.intent);if(!station)throw new Error('没有这个去向');
      if(!station.open)throw new Error('尚未开放');
      s.nextStation=station.id;return true;
    }
    case 'end_first_night':{
      s.ending=s.roofCompanions.length?(s.invitationResult==='romance'?'浪漫同行':'朋友同行'):s.phase==='rooftop'?'独自上楼':'提前结束';stage(g,'settled');g.finish();return true;
    }
    default:return false;
  }
}

export function firstNightView(g:Engine){
  const s=g.world.firstNight;if(!s)return null;
  return {
    version:s.version,contentVersion:s.contentVersion,phase:s.phase,openingBall:s.openingBall,gameChoice:s.gameChoice,
    winner:s.winner,lastPlace:s.lastPlace,invited:s.invited,invitationResult:s.invitationResult,ending:s.ending,
    playerDrinkStage:s.drinkStage.USER,availableCash:s.cash.USER,voucherCount:s.vouchers.USER??0,round:s.round,turn:s.turn,
    participants:s.participants,pendingThrow:s.pendingThrow??{id:'',actor:'',round:0,aim:0,power:0,startedAt:0},
    drinks:s.drinks.map(d=>({id:d.id,owner:d.owner,status:d.status})),
    attitudes:CAST.map(id=>attitudeOf(g,id)),
    keyActions:keyActionsOf(g),
    evaluations:s.phase==='settled'?CAST.map(id=>({id,name:g.actor(id).name,text:evaluationOf(g,id)})):[],
    settlementSummary:s.phase==='settled'?summarizeNight(g):'',
    cupAssist:s.cupAssist,feigningDrunk:!!s.feigningDrunk.USER,inviteFrom:s.inviteFrom,intoxFx:s.intoxFx||'full',
    songChoice:s.song,activity:s.activity,question:s.question,
    nextStations:NEXT_STATIONS.map(x=>({id:x.id,label:x.label,open:x.open}))
  };
}
