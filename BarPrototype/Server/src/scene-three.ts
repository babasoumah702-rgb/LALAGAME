import type {Engine} from './engine.js';
import type {Command,Event} from './types.js';
import {distance} from './navigation.js';
import {facePair,sceneOneDisplayName} from './scene-one.js';

// Scene 3 is the tarot round. The deck is an excuse to say things nobody would volunteer, so the
// system fixes only the question, the turn order and what was observable. Every answer, every
// deflection and every joke is generated; none of the relationship history is ever asserted here.
export type SceneThreeState={
  version:1;questionEventId?:string;gazeCue?:{actor:string;target:string;due:number};
  phase:'seating'|'reader_chosen'|'round_open'|'answering'|'comedy'|'closing'|'scene4_ready';
  reader:string;seatedActors:string[];playerSeated:boolean;playerStance:'undecided'|'seated'|'watching'|'declined';
  enteredAt:number;stageAt:number;round:number;cardId:string;cardName:string;theme:string;question:string;tags:string[];
  askedAt:number;firstResponder:string;responded:string[];playerMove:string;playerMovedAt:number;
  history:{cardId:string;question:string;firstResponder:string;playerMove:string}[];
  gazes:{round:number;actor:string;order:string[];pauseMs:number;gesture:string}[];
  jokerUsed:boolean;jokerAt:number;tension:number;peakTension:number;
  silences:number;nameCalled:number;boundaryHits:number;
  leaver:string;follower:string;leftAt:number;playerFollow:string;
};
// La La Land Social Tarot: symbol + relationship theme + question. Weighted by tag, never random().
export type TarotCard={id:string;name:string;theme:string;tags:string[];question:string};
export const DECK:TarotCard[]=[
  {id:'Q01',name:'THE UNWANTED GUEST｜不该遇见的人',theme:'过去 / 回避',tags:['past','relationship','high_tension'],
    question:'今晚，你最不想再遇见的人是谁？'},
  {id:'Q02',name:'THE LAST ONE｜最想留下的人',theme:'选择 / 缺席',tags:['relationship','ambiguous','misread'],
    question:'如果今晚只能留下一个人，你希望是谁？'},
  {id:'Q03',name:'THE EARLIER YEARS｜如果早点认识',theme:'时机 / 人生阶段',tags:['ambiguous','relationship','light'],
    question:'在场有没有一个人，你偶尔会想：如果我们早几年认识，会不会不一样？'},
  {id:'Q04',name:'THE MIRROR｜镜',theme:'投射 / 被看见',tags:['past','identity','misread'],
    question:'有没有一个人，现在已经和你记忆里的她完全不一样了？'},
  {id:'Q05',name:'THE UNSAID｜没说出口的话',theme:'克制 / 未完成',tags:['relationship','high_tension'],
    question:'最近一次，你明明想说，最后却没说出口的话是什么？'},
  {id:'Q06',name:'THE MISREADING｜误会',theme:'误解 / 重新理解',tags:['misread','relationship','high_tension'],
    question:'在场有没有一个人，你后来才发现自己可能一直误会了她？'},
  {id:'Q07',name:'THE CONTRACT｜契约',theme:'利益 / 真心',tags:['business_emotion','identity','high_tension'],
    question:'你有没有过一次，自己都分不清：靠近一个人，到底因为她有价值，还是因为真的喜欢她？'},
  {id:'Q08',name:'THE SEEN｜被看穿',theme:'暴露 / 防御',tags:['identity','high_tension'],
    question:'如果今晚有人真的看穿你，你最不希望她看见哪一部分？'},
  {id:'Q09',name:'THE SAME CHOICE｜再来一次',theme:'转折 / 重来',tags:['past','future','high_tension'],
    question:'如果能回到你和某个人关系发生变化的那一天，你还会做同样的选择吗？'},
  {id:'Q10',name:'THE EXIT｜出口',theme:'离开 / 留下',tags:['future','relationship'],
    question:'这一桌有没有一个人，你希望今晚之后，你们的关系和现在不一样？'},
  {id:'Q11',name:'THE FIRST IMPRESSION｜第一印象',theme:'认识 / 误认',tags:['light','misread'],
    question:'在场谁最不像你第一次认识她时以为的样子？'},
  {id:'Q12',name:'THE EMPTY CHAIR｜空椅',theme:'缺席 / 等待',tags:['identity','future','relationship'],
    question:'如果今晚不谈职位、钱、项目和过去，你最想以什么身份重新认识这里的某个人？'}
];
// Joker pool: at most one per game, used to break the room open after the highest-tension round.
export const JOKERS:TarotCard[]=[
  {id:'J01',name:'THE DUE DILIGENCE｜约会尽调',theme:'玩笑 / 职业病',tags:['joker'],
    question:'这一桌谁最有可能把约会做成尽调？'},
  {id:'J02',name:'THE DISASTER｜创业灾难',theme:'玩笑 / 甩锅',tags:['joker'],
    question:'如果这一桌一起创业，谁最可能第一个把公司搞黄？'},
  {id:'J03',name:'THE ACCIDENT｜无意识撩人',theme:'玩笑 / 抗议',tags:['joker'],
    question:'谁最容易让别人以为她在撩人，但本人坚称自己什么都没做？'},
  {id:'J04',name:'THE LATE TEXT｜前任短信',theme:'玩笑 / 破功',tags:['joker'],
    question:'谁最可能喝多以后给前任发一句“睡了吗”？'},
  {id:'J05',name:'THE HAZARD｜职业病',theme:'玩笑 / 互相拆台',tags:['joker'],
    question:'说一个在场某人的职业病。'}
];
export const TABLE={x:1.65,z:-1.8};
// The arc is fixed, the card inside each stage is not: laugh -> ambiguity -> something is off ->
// truth -> awkwardness -> the room breaks -> soft. A pure random() would flatten this into noise.
const ARC=['light','ambiguous','relationship','high_tension','future'];
export function initializeSceneThree(g:Engine){
  const w=g.world;
  w.scene3={version:1,phase:'seating',reader:'',seatedActors:[],playerSeated:false,playerStance:'undecided',
    enteredAt:w.elapsed,stageAt:w.elapsed,round:0,cardId:'',cardName:'',theme:'',question:'',tags:[],askedAt:-1,firstResponder:'',responded:[],
    playerMove:'',playerMovedAt:-1,history:[],gazes:[],jokerUsed:false,jokerAt:-1,
    tension:0,peakTension:0,silences:0,nameCalled:0,boundaryHits:0,
    leaver:'',follower:'',leftAt:-1,playerFollow:''};
  const seats=[{x:.65,z:-2.9},{x:2.8,z:-2.85},{x:2.9,z:-.75},{x:.65,z:-.65}];
  for(const [index,id] of ['A','B','C','D'].entries())if(g.actor(id).active&&!g.actor(id).withdrawn){w.scene3.seatedActors.push(id);g.go(g.actor(id),seats[index],'main_table');}
}
// The reader is not fixed to the bartender. B takes the deck most readily, D when she is enjoying
// herself, A only under pressure, C prefers to watch first, and the bartender covers an empty table.
export function chooseReader(g:Engine){
  const s=g.world.scene3!;
  const weight=(id:string)=>{
    const a=g.actor(id);
    if(!a.active||a.withdrawn||!s.seatedActors.includes(id))return 0;
    const base=({B:1.6,D:1,A:.45,C:.35} as Record<string,number>)[id]??0;
    const closeness=a.relations.USER?.closeness??.3;
    return base*(.6+a.initiative)*(.7+closeness);
  };
  const pool=['B','D','A','C'].map(id=>[id,weight(id)] as const).filter(([,n])=>n>0);
  const total=pool.reduce((sum,[,n])=>sum+n,0);
  if(!total)return 'BARTENDER';
  let roll=g.random()*total;
  for(const [id,n] of pool){roll-=n;if(roll<=0)return id;}
  return pool[0][0];
}
// Card selection walks the arc, skips what has already been drawn, and never repeats a question.
export function pickCard(g:Engine){
  const s=g.world.scene3!;
  const used=s.history.map(h=>h.cardId);
  const stage=ARC[Math.min(s.round,ARC.length-1)];
  const staged=DECK.filter(c=>!used.includes(c.id)&&c.tags.includes(stage));
  const rest=DECK.filter(c=>!used.includes(c.id));
  const pool=staged.length?staged:rest;
  if(!pool.length)return undefined;
  return pool[Math.floor(g.random()*pool.length)];
}
export function sceneThreeDuty(g:Engine,id:string){
  const s=g.world.scene3;if(!s)return '';
  const reader=s.reader===id?'你在主持这一轮：抽牌、念问题、维持轮次。你没有权限逼任何人回答，也不是全知的主持人。':'';
  return reader+({
    A:'你不抢话。你可以只回答一半，也可以把问题挡回去。你记得的措辞很准，但不替别人解释。',
    B:'你习惯让场面继续下去，可以用玩笑化解，也可以承认一部分。你不主动点名。',
    C:'你可以只说“有”，不说是谁。沉默对你是有效回答。',
    D:'你比刚到时放松了一些。你可以反问，也可以直接拒答。',
    BARTENDER:'你在收尾。你可以递一杯水、换一首歌，不评判任何人的回答。'
  } as Record<string,string>)[id]||'';
}
export function sceneThreeContext(g:Engine,id:string,event:Event){
  const s=g.world.scene3;if(!s||g.world.late)return null;
  const heard=g.world.events.find(e=>e.id===s.questionEventId)?.perceptions.some(p=>p.actor===id&&p.level==='full');
  const asked=heard&&s.askedAt>=0&&s.question?{card:s.cardName,theme:s.theme,question:s.question,
    isReader:s.reader===id,youAnswered:s.responded.includes(id),firstResponder:s.responded.find(actor=>g.world.events.some(e=>e.actor===actor&&e.type==='speech'&&e.time>=s.askedAt&&e.perceptions.some(p=>p.actor===id&&p.level==='full')))??''}:null;
  return {chapter:'闭店前最后一局：塔罗',phase:s.phase,round:s.round,duty:sceneThreeDuty(g,id),
    currentCard:asked,jokerRound:!!heard&&s.tags.includes('joker'),
    playerMove:!g.world.events.some(e=>e.actor==='USER'&&e.time>=s.askedAt&&e.perceptions.some(p=>p.actor===id&&p.level==='full'))?'':s.playerMove==='skip'?'玩家明确选择了不回答':s.playerMove==='observe'?'玩家没有回答，只在看':s.playerMove||'',
    reminder:'塔罗只是一个借口，不是预言。你只回答被抽到的问题，可以拒答、玩笑化解、反问或沉默。'+
      '不要替别人认领答案，不要断言别人指的是谁，不要揭穿别人。你只能根据自己看见和听见的部分形成怀疑。'};
}
function script(g:Engine,type:string,from:string,target:string,intent:string,text:string,object=''){
  return g.emit(type,from,target,intent,text,'','normal','','script',object);
}
// The 0.5 seconds before an answer carries more than the answer. Gaze order, pause length and
// gesture are recorded as observable facts; nobody is told what they mean.
function recordGaze(g:Engine,id:string){
  const s=g.world.scene3!,a=g.actor(id);
  const others=s.seatedActors.filter(x=>x!==id&&g.actor(x).active);
  if(!others.length)return;
  const ranked=others.map(x=>{
    const r=a.relations[x];
    return [x,(r?.tension??0)*1.4+(r?.uncertainty??0)+(r?.closeness??0)*.5+g.random()*.35] as const;
  }).sort((p,q)=>q[1]-p[1]).map(([x])=>x);
  const order=ranked.slice(0,g.random()<.45?2:1);
  const pauseMs=Math.round(300+g.random()*1400);
  const gesture=(['转杯子','手指敲桌沿','抬头','低头喝了一口','笑到一半停下来'] as const)[Math.floor(g.random()*5)];
  s.gazes.push({round:s.round,actor:id,order,pauseMs,gesture});
  if(s.gazes.length>40)s.gazes.splice(0,s.gazes.length-40);
  const first=g.actor(order[0]);
  a.yaw=Math.atan2(first.x-a.x,first.z-a.z)*180/Math.PI;
  a.facingUntil=g.world.elapsed+2.5;a.conversationTarget=order[0];
  // Visible to whoever is looking; the meaning stays with the observer.
  a.gesture=gesture.includes('喝')?'drink':'offer';a.gestureAt=g.world.elapsed;
  script(g,'movement',id,order[0],'gaze',sceneOneDisplayName(g,id)+'回答之前，先看了一眼'+sceneOneDisplayName(g,order[0])+'。');
  if(order[1])s.gazeCue={actor:id,target:order[1],due:g.world.elapsed+pauseMs/1000};
}
export function openRound(g:Engine,joker=false){
  const s=g.world.scene3!,w=g.world;
  if(s.history.length>=5||joker&&s.jokerUsed)return false;
  const card=joker?JOKERS[Math.floor(g.random()*JOKERS.length)]:pickCard(g);
  if(!card)return false;
  s.round++;s.cardId=card.id;s.cardName=card.name;s.theme=card.theme;s.question=card.question;
  s.tags=card.tags;s.askedAt=w.elapsed;s.responded=[];s.playerMove='';s.playerMovedAt=-1;
  s.phase=joker?'comedy':'round_open';
  if(joker){s.jokerUsed=true;s.jokerAt=w.elapsed;}
  script(g,'action',s.reader||'BARTENDER','USER','tarot_flip','牌划过桌面，翻了过来：'+card.name,'tarot_card');
  g.actor(s.reader||'BARTENDER').gesture='flip';g.actor(s.reader||'BARTENDER').gestureAt=w.elapsed;
  s.questionEventId=script(g,'action',s.reader||'BARTENDER','USER','tarot_question',card.question,'tarot_card').id;
  w.jobs.push({actor:s.reader||'BARTENDER',eventId:s.questionEventId,due:w.elapsed+1});
  // The reader keeps the turn moving but does not choose who must speak; the first responder is the
  // one this question actually pulls at.
  const pool=s.seatedActors.filter(id=>g.actor(id).active&&!g.actor(id).withdrawn&&id!==s.reader);
  const scored=pool.map(id=>{
    const r=g.actor(id).relations;
    const pull=card.tags.includes('high_tension')
      ?Math.max(...Object.values(r).map(v=>v.tension))
      :card.tags.includes('past')||card.tags.includes('misread')
        ?Math.max(...Object.values(r).map(v=>v.uncertainty))
        :g.actor(id).initiative;
    return [id,pull+g.random()*.3] as const;
  }).sort((p,q)=>q[1]-p[1]);
  s.firstResponder=scored[0]?.[0]??'';
  if(s.firstResponder){
    recordGaze(g,s.firstResponder);
    w.jobs.push({actor:s.firstResponder,eventId:s.questionEventId!,due:w.elapsed+2.5});
  }
  return true;
}
// Answers, refusals and gazes all move tension. Nothing here is shown as a number to the player;
// it only changes who speaks next, who stays and who needs to step outside.
export function observeSceneThreeEvent(g:Engine,e:Event){
  const s=g.world.scene3;if(!s||g.world.late)return;
  if(s.askedAt<0)return;
  if(e.type==='speech'&&e.actor!=='USER'&&e.intent!=='tarot_question'&&s.seatedActors.includes(e.actor)&&!s.responded.includes(e.actor)){
    s.responded.push(e.actor);
    const high=s.tags.includes('high_tension');
    if(e.intent==='boundary'){s.boundaryHits++;s.tension=Math.min(1,s.tension+(high?.2:.12));}
    else if(s.tags.includes('joker'))s.tension=Math.max(0,s.tension-.22);
    else s.tension=Math.min(1,s.tension+(high?.14:.05));
    // A named answer raises the stakes for everyone at the table, not just the two involved.
    for(const id of s.seatedActors)if(id!==e.actor&&e.text.includes(g.actor(id).name)){s.nameCalled++;s.tension=Math.min(1,s.tension+.16);}
    s.peakTension=Math.max(s.peakTension,s.tension);
  }
  if(e.actor==='USER'&&e.type==='speech'&&s.playerStance==='seated'&&s.askedAt>=0&&!s.playerMove){s.playerMove='answer';s.playerMovedAt=g.world.elapsed;}
}
export function advanceSceneThree(g:Engine){
  const s=g.world.scene3;if(!s)return;
  const w=g.world,u=g.actor('USER');
  if(s.gazeCue&&w.elapsed>=s.gazeCue.due){
    const q=s.gazeCue,a=g.actor(q.actor),b=g.actor(q.target);
    if(a.active&&b.active&&!a.route.length&&!a.withdrawn){
      a.yaw=Math.atan2(b.x-a.x,b.z-a.z)*180/Math.PI;a.conversationTarget=b.id;a.facingUntil=w.elapsed+1.5;
      script(g,'movement',a.id,b.id,'gaze',sceneOneDisplayName(g,a.id)+'停了一下，又看向'+sceneOneDisplayName(g,b.id)+'。');
    }
    s.gazeCue=undefined;
  }
  for(const id of s.seatedActors){const a=g.actor(id);if(!a.route.length&&distance(a,TABLE)<2.5&&!a.withdrawn&&!['closing','scene4_ready'].includes(s.phase))a.posture='sit';}
  for(const a of w.actors)if(a.conversationTarget&&(a.facingUntil??0)<w.elapsed){
    const other=a.conversationTarget;a.conversationTarget='';a.facingUntil=0;
    const b=w.actors.find(x=>x.id===other);if(b&&b.conversationTarget===a.id){b.conversationTarget='';b.facingUntil=0;}
  }
  if(s.phase==='seating'){
    for(const id of s.seatedActors){const a=g.actor(id);if(a.active&&!a.withdrawn&&!a.route.length&&distance(a,TABLE)>2.2)g.go(a,g.location('main_table'),'main_table');}
    if(w.elapsed-s.stageAt>=6||s.seatedActors.every(id=>distance(g.actor(id),TABLE)<=2.4)){
      s.reader=chooseReader(g);s.phase='reader_chosen';s.stageAt=w.elapsed;
      script(g,'action',s.reader,'USER','tarot_take','有人伸手把牌拉了过来，开始洗牌。','tarot_deck');
    }
    return;
  }
  if(s.phase==='reader_chosen'&&w.elapsed-s.stageAt>=3){openRound(g);return;}
  if(['closing','scene4_ready'].includes(s.phase)){advanceSceneThreeClosing(g);return;}
  advanceSceneThreeRound(g);
}
// A round closes when the people this question pulled at have reacted, or when the table simply lets
// it pass. Silence closes a round exactly like an answer does.
function advanceSceneThreeRound(g:Engine){
  const s=g.world.scene3!,w=g.world;
  if(s.askedAt<0)return;
  const open=w.elapsed-s.askedAt;
  const pool=s.seatedActors.filter(id=>g.actor(id).active&&!g.actor(id).withdrawn&&id!==s.reader);
  const enough=s.responded.length>=Math.min(2,pool.length);
  if(['round_open','comedy'].includes(s.phase)&&s.responded.length&&s.phase!=='comedy')s.phase='answering';
  // A second voice picks the question up on its own; this is where one sentence starts meaning
  // different things in different relationships.
  if(open>=7&&s.responded.length&&s.responded.length<pool.length&&!w.jobs.some(j=>pool.includes(j.actor))){
    const next=pool.filter(id=>!s.responded.includes(id))
      .sort((a,b)=>(g.actor(b).initiative)-(g.actor(a).initiative))[0];
    const last=w.events.filter(e=>e.type==='speech'&&s.responded.includes(e.actor)).at(-1);
    if(next&&last&&g.random()<.75){recordGaze(g,next);w.jobs.push({actor:next,eventId:last.id,due:w.elapsed+1});}
  }
  // The player not answering is an event. Each agent reads it differently; none of them is told.
  if(open>=26&&!s.playerMove&&s.playerStance==='seated'&&distance(g.actor('USER'),TABLE)<3.2&&w.events.find(e=>e.id===s.questionEventId)?.perceptions.some(p=>p.actor==='USER'&&p.level==='full')&&!(w.replies||[]).some(r=>['error','running'].includes(r.status)&&(r.eventId===s.questionEventId||w.events.some(e=>e.id===r.eventId&&e.actor==='USER'&&e.time>=s.askedAt)))){
    s.playerMove='silence';s.playerMovedAt=w.elapsed;s.silences++;
    script(g,'action','USER',s.reader||'BARTENDER','silence','你没有回答。桌上安静了一下，然后有人把话接了过去。');
  }
  if(open>=30&&(enough||open>=44)){closeRound(g);}
}
function closeRound(g:Engine){
  const s=g.world.scene3!,w=g.world;
  s.history.push({cardId:s.cardId,question:s.question,firstResponder:s.firstResponder,playerMove:s.playerMove||'none'});
  s.askedAt=-1;s.question='';s.cardId='';s.tags=[];s.stageAt=w.elapsed;
  // Comedy break: after the room has been squeezed, one joker lets everybody off the hook.
  if(s.boundaryHits>0||s.seatedActors.some(id=>g.actor(id).withdrawn)){s.phase='closing';return;}
  if(!s.jokerUsed&&s.history.length<5&&(s.history.length===4||s.history.length>=3&&s.peakTension>=.45)){openRound(g,true);return;}
  if(shouldCloseScene(g)){s.phase='closing';return;}
  s.phase='round_open';openRound(g);
}
// 3–5 cards, or any single moment that has already done the work: a boundary, a walkout, a named
// answer. The scene does not insist on finishing the deck.
function shouldCloseScene(g:Engine){
  const s=g.world.scene3!;
  if(s.history.length>=5)return true;
  if(s.boundaryHits>0||s.seatedActors.some(id=>g.actor(id).withdrawn))return true;
  if(s.history.length<3)return false;
  return s.peakTension>=.6||s.boundaryHits>0||s.nameCalled>0||
    s.seatedActors.some(id=>g.actor(id).withdrawn);
}
// Someone picks up her glass, pauses, stands. Who leaves is whoever this round cost the most; who
// follows is whoever has the most unsaid toward her. Neither is scripted to a fixed character.
function advanceSceneThreeClosing(g:Engine){
  const s=g.world.scene3!,w=g.world;
  if(!s.leaver){
    if(w.elapsed-s.stageAt<4)return;
    const pool=s.seatedActors.filter(id=>g.actor(id).active);
    const cost=(id:string)=>{
      const a=g.actor(id),rel=Object.values(a.relations);
      const spoke=w.events.filter(e=>e.actor===id&&e.type==='speech').length;
      return Math.max(...rel.map(r=>r.tension))*1.5+(a.withdrawn?1:0)+
        (s.responded.includes(id)?.3:0)+(spoke?0:.2);
    };
    s.leaver=pool.sort((a,b)=>cost(b)-cost(a))[0]||'';
    if(!s.leaver){s.phase='scene4_ready';return;}
    const leaver=g.actor(s.leaver);
    s.leftAt=w.elapsed;
    script(g,'action',s.leaver,'USER','step_out',sceneOneDisplayName(g,s.leaver)+'拿起杯子，停了一下，站了起来。');
    w.jobs.push({actor:s.leaver,eventId:w.events.at(-1)!.id,due:w.elapsed+1});
    g.go(leaver,g.location('corridor'),'corridor');
    return;
  }
  // The agent with the most unresolved communication toward her may go out on her own.
  if(!s.follower&&w.elapsed-s.leftAt>=5){
    const pool=s.seatedActors.filter(id=>id!==s.leaver&&g.actor(id).active&&!g.actor(id).withdrawn);
    const pull=(id:string)=>{
      const r=g.actor(id).relations[s.leaver];
      const talked=w.events.some(e=>e.actor===id&&e.target===s.leaver&&e.type==='speech');
      return (r?.tension??0)+(r?.uncertainty??0)+(r?.closeness??0)*.6-(talked?.35:0);
    };
    const best=pool.sort((a,b)=>pull(b)-pull(a))[0];
    if(best&&pull(best)>.75){
      s.follower=best;
      script(g,'action',best,s.leaver,'follow_out',sceneOneDisplayName(g,best)+'看了一眼门口，也跟了出去。');
      g.go(g.actor(best),g.location('corridor'),'corridor');
    }else s.follower='none';
  }
  if(s.follower&&w.elapsed-s.leftAt>=9&&s.phase!=='scene4_ready'){
    s.phase='scene4_ready';
    script(g,'system','OWNER','USER','scene4','夜深了。你可以去走廊透透气，也可以继续留在桌边。');
  }
}
// Answer / Skip / Deflect / Ask Back / Observe / Joke are all valid social moves. Skip and Observe
// are recorded, not discarded: refusing to answer is information the table can read.
export function sceneThreeCommand(g:Engine,c:Command):boolean{
  const s=g.world.scene3;if(!s)return false;
  const w=g.world,u=g.actor('USER');
  if(c.type==='tarot_seat'){
    const stance=c.text==='watch'?'watching':c.text==='decline'?'declined':'seated';
    if(s.playerStance!=='undecided'){
      if(s.playerStance===stance)return true;
      throw new Error('这一局的参与方式已经选择，不能重复改动');
    }
    if(stance==='seated'&&(distance(u,TABLE)>3.2||!g.navigation.visible(u,TABLE)))throw new Error('请先走到主桌旁');
    s.playerStance=stance;s.playerSeated=stance==='seated';u.posture=stance==='seated'?'sit':'stand';
    if(stance==='seated'){
      if(distance(u,TABLE)>3.2)throw new Error('请先走到主桌旁');
      u.route=[];u.animation='sit';
      g.emit('action','USER',s.reader||'BARTENDER','sit','你在桌边坐下，把手放在牌旁边。','','normal','','player','tarot_deck');
    }else g.emit('action','USER',s.reader||'BARTENDER',stance==='watching'?'observe':'boundary',
      stance==='watching'?'你没有坐下，只站在旁边看这一轮。':'你摆了摆手，这一局不参加。','','normal','','player','tarot_deck');
    return true;
  }
  if(!['tarot_move','tarot_answer'].includes(c.type))return false;
  if(s.askedAt<0)throw new Error('现在没有正在进行的问题');
  if(s.playerStance==='declined')throw new Error('你已经退出这一局');
  if(s.playerMove&&s.playerMove!=='silence')throw new Error('这一轮你已经表态了');
  const move=c.type==='tarot_answer'?'answer':(c.intent||'observe');
  const targetId=move==='ask_back'?(c.target||''):(c.target||s.reader||'BARTENDER');
  if(!['answer','skip','deflect','ask_back','observe','joke'].includes(move))throw new Error('不支持的表态');
  if(distance(u,TABLE)>3.2||!g.navigation.visible(u,TABLE))throw new Error('请先走近桌边');
  if(!w.events.find(e=>e.id===s.questionEventId)?.perceptions.some(p=>p.actor==='USER'&&p.level==='full'))throw new Error('你没有看清这一轮的问题，可以观察下一张牌');
  if(['answer','ask_back','joke'].includes(move)){const text=(c.text||'').trim(),target=w.actors.find(a=>a.id===targetId&&a.active);if(!text||[...text].length>200)throw new Error('请输入 1–200 字');if(!target||target.id==='USER'||distance(u,target)>4.5||!g.navigation.visible(u,target))throw new Error('请选择桌边可见的人');}
  s.playerMove=move;s.playerMovedAt=w.elapsed;
  if(move==='skip'||move==='observe'){
    if(s.playerStance==='seated')s.silences++;
    g.emit('action','USER',s.reader||'BARTENDER',move==='skip'?'boundary':'observe',
      move==='skip'?'你明确说这一题不答。':'你没有回答，只看着桌上其他人的反应。','','normal','','player');
    return true;
  }
  if(move==='deflect'){
    g.emit('action','USER',s.reader||'BARTENDER','deflect','你把这题让给别人先答。','','normal','','player');
    const next=s.seatedActors.filter(id=>g.actor(id).active&&!g.actor(id).withdrawn&&!s.responded.includes(id))[0];
    if(next){recordGaze(g,next);w.jobs.push({actor:next,eventId:w.events.at(-1)!.id,due:w.elapsed+1.5});}
    return true;
  }
  // Ask Back and Answer both need words, and being named in public is something the target remembers.
  const text=(c.text||'').trim();
  if(!text||[...text].length>200)throw new Error('请输入 1–200 字');
  const target=targetId;
  const actor=w.actors.find(a=>a.id===target&&a.active);
  if(!actor||target==='USER')throw new Error('请选择在场的人');
  if(distance(u,actor)>4.5||!g.navigation.visible(u,actor))throw new Error('请先走近桌边');
  if(move==='ask_back'){s.nameCalled++;s.tension=Math.min(1,s.tension+.12);s.peakTension=Math.max(s.peakTension,s.tension);}
  if(move==='joke')s.tension=Math.max(0,s.tension-.15);
  facePair(g,'USER',target);
  const e=g.emit('speech','USER',target,move==='ask_back'?'ask_back':move==='joke'?'joke':'answer',text,'','normal','','player','tarot_card');
  e.tone=c.tone||'natural';
  if(!w.jobs.some(j=>j.actor===target))w.jobs.push({actor:target,eventId:e.id,due:w.elapsed+1});
  return true;
}
// Scene 3 output. Impressions and observable facts only; no truth about anyone's history.
export function sceneThreeHandoff(g:Engine){
  const s=g.world.scene3;if(!s)return null;
  return {
    reader:s.reader,rounds:s.history.length,
    questionHistory:s.history.map(h=>({card:h.cardId,question:h.question,firstResponder:h.firstResponder,playerMove:h.playerMove})),
    gazeEvents:s.gazes.slice(-12),
    playerSocialMoves:s.history.reduce<Record<string,number>>((acc,h)=>{acc[h.playerMove]=(acc[h.playerMove]??0)+1;return acc;},{}),
    tension:Number(s.tension.toFixed(3)),peakTension:Number(s.peakTension.toFixed(3)),
    silences:s.silences,namedAnswers:s.nameCalled,boundaries:s.boundaryHits,
    jokerUsed:s.jokerUsed,leaver:s.leaver,follower:s.follower==='none'?'':s.follower,playerFollow:s.playerFollow
  };
}
