import {lateNightView} from './late-night.js';
import type {Engine} from './engine.js';
import type {Event} from './types.js';
import {clock} from './world.js';
import {distance} from './navigation.js';
import {introActive,displayName} from './intro.js';
import {interactionView} from './interaction.js';
// One continuous night across the chapters, but not at one rate. Scene 1 runs close to real time;
// Scene 2's montage compresses roughly an hour and a half of drinking into a few minutes of play;
// Scene 3 slows down again for the closing round. Classified by the fixed chapter entry stamps, so a
// historic event keeps the clock it was shown with.
function timeLabel(game:Engine,time:number){
  const w=game.world;
  if(!w.scene1)return clock(game.scenario,time);
  const two=w.scene2,three=w.scene3;
  let minutes=22*60+35+time/60;
  if(two&&time>=two.enteredAt)minutes=22*60+40+(time-two.enteredAt)*.4;
  if(three&&time>=three.enteredAt)minutes=24*60+10+(time-three.enteredAt)*.125;
  const late=w.story?.transitions.filter(t=>t.to>=4&&t.at<=time).at(-1);if(late)minutes=late.to===4?24*60+40+(time-late.at)*.12:late.to===5?24*60+55+Math.min(25,(time-late.at)*25/210):25*60+22+(time-late.at)/60;
  const total=Math.floor(minutes);
  return String(Math.floor(total/60)%24).padStart(2,'0')+':'+String(total%60).padStart(2,'0');
}
export function reflection(game:Engine){
  const world=game.world;
  const visible=world.events.filter(e=>e.perceptions.some(p=>p.actor==='USER'));
  const important=visible.filter(e=>e.type==='speech'||e.intent==='arrival').slice(-3);
  const ripple=visible.filter(e=>e.parentId).sort((a,b)=>b.depth-a.depth)[0];
  const chain:Event[]=[];
  let current:Event|undefined=ripple;
  while(current){
    chain.unshift(current);
    const parentId:string=current.parentId;
    current=world.events.find(e=>e.id===parentId);
  }
  const seenChain=chain.map(e=>{
    const perception=e.perceptions.find(p=>p.actor==='USER');
    return perception?displayName(game,e.actor)+'：'+perception.text:'中间有一段你未能感知的交流；来源暂时未知。';
  });
  const trends=world.actors.filter(a=>!['USER','OWNER'].includes(a.id)&&visible.some(e=>e.actor===a.id)).map(a=>{
    const initial=a.id==='A'?'今晚认识的克制旁观者':a.id==='B'?'今晚认识的场面主人':a.id==='C'?'今晚认识的沉默来客':a.id==='D'?'晚些到场的来客':'认识的调酒师';
    const last=visible.filter(e=>e.actor===a.id).at(-1);
    const final=last?last.intent==='boundary'?'她表达了需要空间':['approach','reveal'].includes(last.intent)?'出现了进一步交流的信号':'还有一些话没有说清楚':'没有足够的可观察信息';
    return displayName(game,a.id)+'｜'+initial+' → '+final;
  });
  const names:Record<string,string>={approach:'靠近',probe:'试探',reveal:'表达',boundary:'边界',connect:'连接',observe:'观察'};
  const counts=Object.entries(world.moves).map(([key,value])=>(names[key]||key)+' '+value+' 次').join('，');
  return {
    interaction:interactionView(game),
    title:'这一晚，如何走到这里',
    trends,
    events:important.map(e=>timeLabel(game,e.time)+' '+displayName(game,e.actor)+'：'+e.perceptions.find(p=>p.actor==='USER')!.text),
    chain:seenChain.length?seenChain:['本局尚未形成你能追踪的传播链。未知不等于没有发生。'],
    behavior:counts?'本局行动：'+counts+'。这不是人格判断。':'本局没有直接表达。这只是行动记录。',
    ending:world.late?.ending?world.late.ending+' · 今晚留在这里。不为没有亲见的事情下结论。':'今晚结束了。那些还没说清楚的话，可以留给下一次。'
  };
}
export function viewState(game:Engine,fullHistory=false){
  const w=game.world,user=game.actor('USER');
  // Every directed interaction with the player — speech, cards (note/photo/seat/drink),
  // approach/observe actions, and withdrawals — counts toward the cast member on the other end.
  const interactions:Record<string,number>={};
  for(const e of w.events){
    if(e.actor==='USER'&&e.target&&e.target!=='USER')interactions[e.target]=(interactions[e.target]||0)+1;
    else if(e.target==='USER'&&e.actor&&e.actor!=='USER')interactions[e.actor]=(interactions[e.actor]||0)+1;
  }
  return {
    interaction:interactionView(game),
    story:w.story?{chapter:w.story.chapter,phase:w.story.phase,stageAt:w.story.stageAt,budgetCalls:w.story.budgets[w.story.chapter].calls,budgetTokens:w.story.budgets[w.story.chapter].tokens}:null,late:lateNightView(game),version:1,sessionId:w.id,cursor:w.sequence,scene1:w.scene1?{phase:w.scene1.phase,drinkPlaced:!!w.scene1.drinkEventId,drinkPlacedAt:w.scene1.drinkPlacedAt,arrivalAt:w.scene1.arrivalAt,phoneAt:w.scene1.phoneAt,seated:w.scene1.seated}:null,
    // Scene 2 exposes only what the room looks and sounds like plus the deck cue. The relationship
    // impressions it collected stay server-side; the design forbids a relationship panel.
    scene2:w.scene2?{phase:w.scene2.phase,drinkLevel:w.scene2.drinkLevel,coasters:w.scene2.coasters,
      guests:w.scene2.guests,rainStopped:w.scene2.rainStopped,musicLevel:w.scene2.musicLevel,
      deckPlaced:w.scene2.deckAt>=0,gamePrompt:w.elapsed-w.scene2.gameAskedAt<45?w.scene2.gamePrompt:'',
      games:w.scene2.games}:null,
    // Scene 3 exposes the card face, the turn and the observable beats. Never tension numbers,
    // never who anyone actually meant.
    scene3:w.scene3&&!w.late?{phase:w.scene3.phase,reader:w.scene3.reader,round:w.scene3.round,
      cardName:cardSeen(game)?w.scene3.cardName:'',theme:cardSeen(game)?w.scene3.theme:'',question:cardSeen(game)?w.scene3.question:'',
      isJoker:cardSeen(game)&&w.scene3.tags.includes('joker'),highTension:cardSeen(game)&&w.scene3.tags.includes('high_tension'),
      firstResponder:'',responded:w.scene3.responded.filter(id=>w.events.some(e=>e.actor===id&&e.time>=w.scene3!.askedAt&&e.type==='speech'&&e.perceptions.some(p=>p.actor==='USER'&&p.level==='full'))),
      playerStance:w.scene3.playerStance,playerMove:w.scene3.playerMove,
      askedAt:w.scene3.askedAt,rounds:w.scene3.history.length,jokerUsed:w.scene3.jokerUsed,
      lastGaze:null,leaver:w.events.some(e=>e.actor===w.scene3!.leaver&&e.intent==='step_out'&&e.perceptions.some(p=>p.actor==='USER'))?w.scene3.leaver:'',follower:''}:null,replies:(w.replies||[]).filter(r=>w.events.find(e=>e.id===r.eventId)?.perceptions.some(p=>p.actor==='USER')).filter((r,index,all)=>r.status!=='complete'||index>=all.length-12).map(({decision,...r})=>r),clock:introActive(w)?'22:30':timeLabel(game,w.elapsed),
    intro:w.intro?{
      version:w.intro.version,phase:w.intro.phase,progress:w.intro.progress,checkpoint:w.intro.checkpoint,
      entryMode:w.intro.entryMode,checkedMessage:w.intro.checkedMessage,phoneVisible:w.intro.phoneVisible,
      message:w.intro.message,hint:w.intro.hint,messageSource:w.intro.messageSource,generationStatus:w.intro.generationStatus,
      attitude:w.intro.attitude,intent:w.intro.intent,playerText:w.intro.playerText
    }:null,
    elapsed:w.elapsed,night:w.night,status:w.status,paused:w.paused,busy:game.busy,
    mode:w.modelMode,modeReason:w.modelReason,role:w.role,calls:w.calls,tokens:w.tokens,
    cardsOffered:!!w.flags.cardsOffered,cardsJoined:!!w.flags.cardsJoined,
    lastCall:!!w.flags.lastCall,pastDrink:!!w.flags.pastDrink,
    lastTarget:String(w.flags.lastTarget||''),
    characters:w.actors.filter(a=>a.active).map(a=>({
      id:a.id,name:displayName(game,a.id),color:a.color,x:a.x,z:a.z,y:a.y??0,area:a.area??'bar',posture:a.posture??'stand',gesture:a.gesture??'',gestureAt:a.gestureAt??-1,yaw:a.yaw,animation:a.animation,
      conversationTarget:a.conversationTarget||'',facingUntil:a.facingUntil||0,destination:a.destination,route:a.route,routeVersion:a.routeVersion??0,
      interactable:!introActive(w)&&a.id!=='USER'&&distance(a,user)<=3&&game.navigation.visible(a,user),location:introActive(w)?'':game.zone(a).name,interactions:interactions[a.id]||0
    })),
    events:w.events.filter(e=>e.perceptions.some(p=>p.actor==='USER')).slice(fullHistory?0:-45).map(e=>{
      const p=e.perceptions.find(p=>p.actor==='USER')!;
      return {type:e.type,target:p.level==='full'?e.target:'',objectTarget:e.objectTarget||'',generationSource:e.generationSource||'unknown',privacy:e.privacy,id:e.id,seq:e.seq,time:timeLabel(game,e.time),actor:e.actor,name:displayName(game,e.actor),text:p.text,source:p.source,level:p.level,hasParent:!!e.parentId,audio:e.audio||''};
    }),
    cards:(introActive(w)?[]:game.scenario.cards).map(c=>{
      const unlocked=c.type==='social'||(c.id==='last_call'?!!w.flags.lastCall:!!w.flags.cardsJoined);
      const cooldownRemaining=Math.max(0,(w.cooldowns[c.id]||0)-w.elapsed);
      const lockReason=!unlocked?(c.id==='last_call'?'Last Call 时开放':'加入牌局后开放'):cooldownRemaining>0?'冷却中 · '+Math.ceil(cooldownRemaining)+' 秒（暂停不计时）':'';
      const style=game.scenario.styles.find(item=>item.id===w.style) as {expressions?:Record<string,string[]>}|undefined;
      return {...c,expressions:style?.expressions?.[c.id]||c.expressions,unlocked,lockReason,cooldownRemaining,ready:unlocked&&cooldownRemaining===0};
    }),
    locations:introActive(w)?[]:game.scenario.locations.filter(l=>l.id!=='service'||['staff'].includes(w.role)),
    reflection:w.status==='ended'?reflection(game):null
  };
}

function cardSeen(g:Engine){return !!g.world.events.find(e=>e.id===g.world.scene3?.questionEventId)?.perceptions.some(p=>p.actor==='USER'&&p.level==='full');}
