import type {Engine} from './engine.js';
import type {Command,Perception,Actor,Event,Scenario,World} from './types.js';
import {distance,Navigator} from './navigation.js';
import {runBeats} from './beats.js';
import {sceneOneDisplayName} from './scene-one.js';

export type IntroOptions={story?:string;opening?:string;entryMode?:string;entryContext?:string;choices?:Record<string,string>};
export type IntroState={
  version:number;phase:'elevator'|'bar';progress:number;checkpoint:number;ready:boolean;
  entryMode:string;declaredContext:string;playerText:string;attitude:string;intent:string;
  choiceAnswers:Record<string,string>;
  checkedMessage:boolean;phoneVisible:boolean;messageLocked:boolean;message:string;hint:string;
  messageSource:string;generationStatus:string;backgroundEventId:string;revealed:string[];
};
export const introActive=(w:World)=>w.intro?.phase==='elevator';
export function initializeIntro(g:Engine,options:IntroOptions){
  if(options.opening!=='scene0_v1')return;
  const modes=['solo','friend_invited','event_guest'];
  if(options.entryContext!==undefined&&(typeof options.entryContext!=='string'||[...options.entryContext].length>200))throw new Error('背景最多200字');
  const entryMode=modes.includes(options.entryMode??'')?options.entryMode!:'friend_invited';
  g.world.intro={
    version:1,phase:'elevator',progress:0,checkpoint:0,ready:false,entryMode,
    declaredContext:options.entryContext?.trim()??'',playerText:'',attitude:'observing',intent:'observe',
    choiceAnswers:options.choices??{},
    checkedMessage:false,phoneVisible:true,messageLocked:false,message:'今晚见。',
    hint:entryMode==='solo'?'到了就进来。':entryMode==='event_guest'?'就差你了。':'给你留了位置。',
    messageSource:'preset',generationStatus:g.world.modelMode==='online'?'pending':'规则模式 · 预设文案',
    backgroundEventId:'',revealed:[]
  };
  g.world.flags.scene0Route=true;
  const marks:Record<string,[number,number,number]>={
    USER:[-1,-8.65,0],B:[-2.2,0,0],BARTENDER:[-2.2,1.1,180],
    A:[-.2,-2.6,324],C:[3.4,1.2,260]
  };
  for(const [id,[x,z,yaw]] of Object.entries(marks)){
    const a=g.actor(id),p=id==='USER'?{x,z}:g.navigation.nearest({x,z});
    Object.assign(a,p,{active:true,yaw,route:[],animation:'idle'});
  }
  g.actor('D').active=false;
  g.world.beatIds.push('a_arrival','c_window');
}
export function backgroundIntro(g:Engine){
  const i=g.world.intro;if(!i||i.backgroundEventId)return;
  // This is only an expectation, not a claim that the newcomer or an already-present actor is the missing guest.
  const event=g.emit('preentry','B','BARTENDER','expectation','我等的那位，今晚会来吗？','','normal','','script');
  i.backgroundEventId=event.id;
  for(const id of ['A','C']){
    const p=event.perceptions.find(p=>p.actor===id);
    if(!p)continue;
    g.actor(id).beliefs.push({
      subject:id==='A'?'B_is_waiting_for_someone':'B_expected_guest_may_matter',
      confidence:id==='A'?.42:.28,sourceEventId:event.id,
      interpretation:id==='A'?'只听见等待的片段，尚不知道她在等谁。':'看见她朝调酒师询问并望向入口；也许在等人，无法确认。'
    });
  }
}
export function introPerception(s:Scenario,w:World,nav:Navigator,a:Actor,e:Event):Perception|undefined{
  if(!a.active||a.id==='USER')return;
  const speaker=w.actors.find(x=>x.id===e.actor)!;
  const d=distance(a,speaker),visible=nav.visible(a,speaker);
  const looking=d<.6||((speaker.x-a.x)*Math.sin(a.yaw*Math.PI/180)+(speaker.z-a.z)*Math.cos(a.yaw*Math.PI/180))/Math.max(.01,d)>-.35;
  const p=(source:string,level:string,text:string,confidence:number)=>({actor:a.id,source,level,text,confidence});
  if(a.id===e.actor||a.id===e.target&&d<=s.rules.fullHear&&visible)return p('direct','full',e.text,1);
  if(visible&&d<=s.rules.partialHear)return p('overheard','partial','有人问起「等的人……」，后面没有听清。',.42);
  if(visible&&looking&&d<s.rules.sight)return p('observed','gesture','吧台边的人朝调酒师询问，又看向入口，似乎在等人。',.28);
}
export function advanceIntro(g:Engine,dt:number){
  const i=g.world.intro!;
  if(!i.ready||g.world.paused)return;
  i.progress=Math.min(7,i.progress+Math.max(0,Math.min(.25,dt)));
  i.checkpoint=[0,1.3,2.3,4.2,5.4,7].filter(t=>i.progress>=t).length-1;
  if(i.progress>=2.3){
    i.messageLocked=true;
    if(i.phoneVisible)i.checkedMessage=true;
    if(i.generationStatus==='pending')i.generationStatus='本条使用预设文案';
  }
  if(i.progress>=.6)backgroundIntro(g);
}
export function introCommand(g:Engine,c:Command):boolean{
  const i=g.world.intro;
  if(!c.type.startsWith('intro_'))return false;
  if(!i)throw new Error('当前存档没有电梯开场');
  if(i.phase==='bar')return true;
  switch(c.type){
    case 'intro_ready':i.ready=true;break;
    case 'intro_phone':
      if(g.world.paused)throw new Error('请先继续');
      i.phoneVisible=!!c.open;
      if(c.open&&i.progress>=2.3)i.checkedMessage=true;
      break;
    case 'intro_text':{
      if(typeof c.text!=='string'||[...c.text].length>200)throw new Error('最多输入200字');
      i.playerText=c.text.trim();
      i.attitude=/担心|犹豫|紧张|小心|不安/.test(i.playerText)?'hesitant':/期待|好奇|看看|想知道/.test(i.playerText)?'curious':/直接|进去|准备好|主动/.test(i.playerText)?'direct':'observing';
      i.intent=i.attitude==='direct'?'approach':i.attitude==='hesitant'?'caution':'observe';
      break;
    }
    case 'intro_complete':
      if(g.world.paused||i.progress<6.9)throw new Error('电梯尚未到达');
      backgroundIntro(g);i.progress=7;i.checkpoint=5;i.phase='bar';i.messageLocked=true;
      Object.assign(g.actor('USER'),g.navigation.nearest(g.location('entrance')),{yaw:8});
      if(i.attitude==='direct')g.world.flags.entryWarm=true;
      g.world.elapsed=0;runBeats(g);break;
    default:throw new Error('不支持的开场命令');
  }
  return true;
}
export function displayName(g:Engine,id:string){
  if(g.world.scene1)return sceneOneDisplayName(g,id);
  const i=g.world.intro;
  if(!i||i.revealed.includes(id)||!['A','B','C'].includes(id))return g.actor(id).name;
  return ({A:'眼镜来客',B:'浅衣来客',C:'侧廊来客'} as Record<string,string>)[id];
}
