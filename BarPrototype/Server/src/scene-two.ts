import type {Engine} from './engine.js';
import type {Command,Event} from './types.js';
import {distance} from './navigation.js';
import {facePair,sceneOneDisplayName} from './scene-one.js';

// Scene 2 opens on the arrival D already made at the end of Scene 1, so the two scenes share that
// single boundary event instead of staging it twice. Scene 1 state stays live: alias display, known
// names and first impressions all continue to resolve through it.
export type SceneTwoState={
  version:1;following?:{target:string;x:number;z:number;y?:number;area?:string};
  phase:'cross_intro'|'freeflow'|'montage'|'gathering'|'tarot_ready';
  crossIntroEventId:string;professionHintEventId:string;
  spokeWith:string[];followed:string[];observed:string[];seenWith:string[];overheard:string[];
  pairs:Record<string,number>;interactions:number;enteredAt:number;
  games:number;gameId:string;gamePrompt:string;gameAskedAt:number;gameAnswers:string[];
  montageStage:number;stageAt:number;drinkLevel:number;coasters:number;guests:number;
  rainStopped:boolean;musicLevel:number;deckAt:number;rippleAt:number;
};
// Light and deliberately low-stakes: these exist so the table has already been playing something
// before the tarot deck arrives. The contrast is the point; they never grow into their own system.
const GAMES=[
  {id:'occupational_hazard',prompt:'轻一点的一轮：说一个在场某人的职业病，说不出来就喝一口。'},
  {id:'always_late',prompt:'轻一点的一轮：这一桌谁最可能迟到半小时，还理由充分？'},
  {id:'date_or_meeting',prompt:'轻一点的一轮：谁最像会在约会的时候聊工作？'},
  {id:'guess_the_drink',prompt:'猜酒：先别看杯子，猜猜旁边那位今晚喝的是什么。'}
];
const ROAM=['bar','main_table','quiet_corner','corridor','entrance'];
export const TABLE={x:1.65,z:-1.8};
export function initializeSceneTwo(g:Engine){
  g.world.scene2={version:1,phase:'cross_intro',crossIntroEventId:'',professionHintEventId:'',
    spokeWith:[],followed:[],observed:[],seenWith:[],overheard:[],pairs:{},interactions:0,
    enteredAt:g.world.elapsed,games:0,gameId:'',gamePrompt:'',gameAskedAt:-1,gameAnswers:[],
    montageStage:0,stageAt:g.world.elapsed,drinkLevel:1,coasters:0,guests:6,
    rainStopped:false,musicLevel:1,deckAt:-1,rippleAt:g.world.elapsed};
}
export function sceneTwoDuty(id:string){
  return ({
    B:'你是这个局的连接点，在不同小群体之间移动。你可以做一次极轻的交叉介绍，但不解释任何人之间的关系史。',
    A:'你偶尔离开原位和熟人短谈，但一直在观察。你只根据自己看见听见的部分反应，不替别人解释。',
    C:'你在主厅、吧台和露台边之间移动，有时加入，有时突然抽离。听到某些名字时你会停顿，但不说明原因。',
    D:'你刚从工作状态里出来，手机还没完全放下，正逐渐放松。你和这里的人本来就认识，但不主动讲过去。',
    BARTENDER:'你在正常营业：放酒、收杯、换音乐。夜深以后你开始收拾上一轮的游戏道具。'
  } as Record<string,string>)[id]||'';
}
export function sceneTwoContext(g:Engine,id:string,event:Event){
  const s=g.world.scene2;if(!s||g.world.late||g.world.scene3)return null;
  return {chapter:'酒局热场：人终于到齐',phase:s.phase,duty:sceneTwoDuty(id),
    introduction:event.intent==='cross_introduce'&&event.actor===id?{name:g.actor('D').name,occupation:g.scenario.identityPacks[g.world.identityPack]?.actors.D?.publicRole,instruction:'轻轻介绍她的名字；职业仅一句，不能解释关系史。'}:null,
    activity:event.perceptions.some(p=>p.actor===id&&p.level==='full')&&s.gameAskedAt>=0&&g.world.elapsed-s.gameAskedAt<45?s.gamePrompt:'',
    room:s.phase==='montage'||s.phase==='gathering'?'客人少了，音乐压低，说话更容易被听见。':'正常营业，人声和音乐都还热。',
    reminder:'这是轻松的社交场。可以闲聊、玩笑、短暂离开。不要解释别人的关系史，不要替别人认领秘密，不要把玩家当项目。'};
}
// Position and choice are the record: who the player talked to, followed, watched, and was seen with
// all become Scene 3 material, so they are captured from perception rather than from any UI score.
export function observeSceneTwoEvent(g:Engine,e:Event){
  const s=g.world.scene2;if(!s)return;
  const mark=(list:string[],id:string)=>{if(id&&!['USER','OWNER'].includes(id)&&!list.includes(id))list.push(id);};
  if(e.actor==='USER'){
    s.interactions++;
    if(e.type==='speech')mark(s.spokeWith,e.target);
    if(e.intent==='follow')mark(s.followed,e.target);
    if(e.intent==='observe'||e.intent==='listen')mark(s.observed,e.target);
    for(const p of e.perceptions)if(!['USER','OWNER'].includes(p.actor))mark(s.seenWith,p.actor);
  }else if(e.type==='speech'){
    const heard=e.perceptions.find(p=>p.actor==='USER');
    if(heard&&e.target!=='USER'&&heard.level!=='full')mark(s.overheard,e.actor);
    if(e.target!=='USER'&&!['USER','OWNER'].includes(e.target)){
      const key=[e.actor,e.target].sort().join('-');s.pairs[key]=(s.pairs[key]??0)+1;
    }
  }
}
function script(g:Engine,type:string,from:string,target:string,intent:string,text:string,object=''){
  return g.emit(type,from,target,intent,text,'','normal','','script',object);
}
export function advanceSceneTwo(g:Engine){
  const s=g.world.scene2;if(!s)return;
  const w=g.world,t=w.elapsed-s.stageAt;
  for(const a of w.actors)if(a.conversationTarget&&(a.facingUntil??0)<w.elapsed)releaseSceneTwoFacing(g,a.id);
  // One very light cross-introduction unlocks D's name; the profession stays a single vague line.
  if(s.phase==='cross_intro'){
    if(!s.crossIntroEventId&&t>=4){
      const host=g.actor('B').active&&!g.actor('B').withdrawn?'B':'BARTENDER';
      const e=script(g,'action',host,'USER','cross_introduce','她看了看刚到的人，又把目光转向桌边，准备做个简短介绍。');
      s.crossIntroEventId=e.id;s.professionHintEventId=e.id;
      w.jobs.push({actor:host,eventId:e.id,due:w.elapsed+.5});
    }
    if(s.professionHintEventId&&t>=13){s.phase='freeflow';s.stageAt=w.elapsed;}
  }
  // Everyone moves. Short pairs and trios keep forming and dissolving, which is the visual evidence
  // Scene 3 will reinterpret. Withdrawn actors are left alone.
  if(['freeflow','montage'].includes(s.phase)){
    for(const a of w.actors){
      if(['USER','OWNER','BARTENDER'].includes(a.id)||!a.active||a.withdrawn||a.route.length||a.nextAction>w.elapsed)continue;
      a.nextAction=w.elapsed+14+g.random()*16;
      const places=ROAM.filter(id=>id!=='outside');
      const target=places[Math.floor(g.random()*places.length)];
      const l=g.location(target);
      if(w.actors.filter(x=>x.active&&x.id!==a.id&&distance(x,l)<.85).length<l.capacity)g.go(a,l,l.id);
    }
    // A2A: two NPCs standing together with nobody pushing them still start talking, and the player
    // may or may not be in earshot. This is what makes the ripple visible without a UI panel.
    // Gated per participant rather than on the whole queue: a slow model backing up other replies
    // must not freeze the room, because autonomous movement is the point of this chapter.
    if(w.elapsed-s.rippleAt>=22){
      const pool=w.actors.filter(a=>!['USER','OWNER'].includes(a.id)&&a.active&&!a.withdrawn&&!a.route.length&&a.conversationTarget!=='USER');
      const busy=(id:string)=>w.jobs.some(j=>j.actor===id)||(w.replies||[]).some(r=>r.actor===id&&['queued','running'].includes(r.status));
      for(const a of pool){
        if(busy(a.id))continue;
        const near=pool.find(b=>b.id!==a.id&&!busy(b.id)&&distance(a,b)<2.4&&g.navigation.visible(a,b));
        if(!near)continue;
        s.rippleAt=w.elapsed;
        const seed=script(g,'action',a.id,near.id,'turn_to',sceneOneDisplayName(g,a.id)+'朝'+sceneOneDisplayName(g,near.id)+'那边偏了半步。');
        facePair(g,a.id,near.id);
        w.jobs.push({actor:near.id,eventId:seed.id,due:w.elapsed+1});
        break;
      }
    }
  }

  advanceSceneTwoStage(g);
}
function releaseSceneTwoFacing(g:Engine,id:string){
  const a=g.actor(id),other=a.conversationTarget;a.conversationTarget='';a.facingUntil=0;
  if(other){const b=g.actor(other);if(b.conversationTarget===id){b.conversationTarget='';b.facingUntil=0;}}
}
// Time passes through the space, not through a countdown: the glass empties, coasters stack, guests
// thin out, the rain stops and the music drops. The player never waits an hour of real time.
const MONTAGE=[
  {at:45,drink:.72,coasters:1,guests:6,music:1,text:'你手里的杯子已经下去一小半，桌上多了一张杯垫。'},
  {at:95,drink:.5,coasters:2,guests:5,music:.85,text:'又换了一首歌。有两位客人结账离开，桌边的人换过一轮座位。'},
  {at:150,drink:.34,coasters:3,guests:3,music:.65,text:'窗外的雨小了下去。店里安静了一点，说话不用再抬高声音。'},
  {at:205,drink:.2,coasters:4,guests:1,music:.45,text:'雨停了。剩下的客人不多，杯子碰到桌面的声音变得很清楚。'}
];
function advanceSceneTwoStage(g:Engine){
  const s=g.world.scene2!,w=g.world;
  if(['freeflow','montage'].includes(s.phase)){
    const since=w.elapsed-s.enteredAt;
    while(s.montageStage<MONTAGE.length&&since>=MONTAGE[s.montageStage].at){
      const step=MONTAGE[s.montageStage++];
      s.phase='montage';s.drinkLevel=step.drink;s.coasters=step.coasters;s.guests=step.guests;s.musicLevel=step.music;
      if(step.drink<=.34)s.rainStopped=true;
      script(g,'system','OWNER','USER','montage',step.text);
    }
    // The room only closes in once the player has actually been part of it for a while.
    if(s.montageStage>=MONTAGE.length&&since>=250){
      s.phase='gathering';s.stageAt=w.elapsed;
      script(g,'system','OWNER','USER','gathering','调酒师开始收空杯，把上一轮的骰子和卡片收回去。音乐又低了一点。');
      for(const id of ['A','B','C','D']){const a=g.actor(id);if(a.active&&!a.withdrawn)g.go(a,g.location('main_table'),'main_table');}
    }
  }
  if(s.phase==='gathering'&&w.elapsed-s.stageAt>=10&&s.deckAt<0){
    s.deckAt=w.elapsed;s.musicLevel=.3;
    script(g,'action','BARTENDER','USER','tarot_deck','调酒师没把所有东西都收走。她从旁边留下一副塔罗牌，推到桌子中央。','tarot_deck');
    const e=script(g,'action','BARTENDER','USER','invite','她把骰子收进盒里，手停在剩下的牌旁。');g.world.jobs.push({actor:'BARTENDER',eventId:e.id,due:w.elapsed+1});
  }
  if(s.deckAt>=0&&w.elapsed-s.deckAt>=4&&s.phase!=='tarot_ready'){s.phase='tarot_ready';s.stageAt=w.elapsed;}
}
// The player's own moves. None of these is a "route": following someone, listening in and simply
// watching are all recorded the same way a spoken line is, because position is a social act here.
export function sceneTwoCommand(g:Engine,c:Command):boolean{
  const s=g.world.scene2;if(!s)return false;
  const w=g.world,u=g.actor('USER');
  if(c.type==='follow_target'){
    const t=g.actor(c.target||'B');
    if(!t.active||!g.navigation.visible(u,t)||distance(u,t)>g.scenario.rules.sight)throw new Error('请先找到可见的目标');
    if(t.withdrawn)throw new Error('对方正需要一点空间');
    s.following={target:t.id,x:t.x,z:t.z,y:t.y,area:t.area};g.go(u,g.near(u,t));
    g.emit('action','USER',t.id,'follow','你跟着'+sceneOneDisplayName(g,t.id)+'走了过去。','','normal','','player');
    return true;
  }
  if(c.type==='listen_in'){
    const near=w.actors.filter(a=>a.active&&!['USER','OWNER'].includes(a.id)&&distance(a,u)<5&&g.navigation.visible(a,u));
    if(!near.length)throw new Error('附近没有可以旁听的谈话');
    g.emit('action','USER',near[0].id,'listen','你没有加入，只是站在旁边听着。','','normal','','player');
    return true;
  }
  if(c.type==='join_game'){
    if(!['freeflow','montage'].includes(s.phase))throw new Error('这一轮已经过去了');
    if(s.gameAskedAt>=0&&w.elapsed-s.gameAskedAt<40)throw new Error('这一轮还没结束');
    const pick=GAMES[Math.floor(g.random()*GAMES.length)];
    s.gameId=pick.id;s.gamePrompt=pick.prompt;s.gameAskedAt=w.elapsed;s.games++;s.gameAnswers=[];
    const host=g.actor('B').active&&!g.actor('B').withdrawn?'B':'BARTENDER';
    script(g,'speech',host,'USER','invite',pick.prompt);
    return true;
  }
  if(c.type==='observe_object'&&c.objectTarget==='tarot_deck'){
    if(s.deckAt<0)throw new Error('桌上还没有那副牌');
    if(distance(u,TABLE)>4||!g.navigation.visible(u,TABLE))throw new Error('请先走近主桌');
    g.emit('action','USER','BARTENDER','observe','你看了看推到桌子中央的那副塔罗牌。','','normal','','player','tarot_deck');
    return true;
  }
  return false;
}
// Handoff to Scene 3. Deliberately impressions, not truths: visible tension is what the player could
// have observed, never the relationship history behind it. Scene 3 reinterprets these, it never
// confirms them.
export function sceneTwoHandoff(g:Engine){
  const s=g.world.scene2;if(!s)return null;
  const pair=(a:string,b:string)=>{
    const x=g.actor(a).relations[b],y=g.actor(b).relations[a];
    const seen=s.pairs[[a,b].sort().join('-')]??0;
    return {observedExchanges:seen,tension:Number((((x?.tension??0)+(y?.tension??0))/2).toFixed(3)),
      familiarity:Number((((x?.closeness??0)+(y?.closeness??0))/2).toFixed(3))};
  };
  return {
    knownCharacters:['A','B','C','D'].filter(id=>g.world.scene1?.knownNames[id]),
    playerSocialAffinity:Object.fromEntries(['A','B','C','D'].filter(id=>g.actor(id).active)
      .map(id=>[id,Number((g.actor(id).relations.USER?.closeness??0).toFixed(3))])),
    whoPlayerSpokeWith:s.spokeWith,whoPlayerFollowed:s.followed,whoPlayerObserved:s.observed,
    whoPlayerWasSeenWith:s.seenWith,whatPlayerOverheard:s.overheard,
    visible:{ab:pair('A','B'),bc:pair('B','C'),cd:pair('C','D'),bd:pair('B','D')},
    lightGamesPlayed:s.games
  };
}

export function advanceFollowing(g:Engine){const s=g.world.scene2;if(!s)return;
  if(s.following){const f=s.following,a=g.actor(f.target),u=g.actor('USER');if(!a.active||a.withdrawn||!g.navigation.visible(u,a)){g.go(u,f);s.following=undefined;}else if(distance(u,a)>1.8&&distance(f,a)>.5){Object.assign(f,{x:a.x,z:a.z,y:a.y,area:a.area});g.go(u,g.near(u,a));}}
}
