import type {Engine} from './engine.js';
import type {Command,Event} from './types.js';
import {distance} from './navigation.js';
import {areaOf,NIGHT} from './night-navigation.js';
import {enterChapter,once,phase} from './story.js';
import {facePair} from './scene-one.js';
export type Recollection={id:string;owners:string[];evidenceId:string;version:string;at:number;viewerEligible:boolean;consumed:boolean};
export type LateNightState={chapter:4|5|6;enteredAt:number;phase:string;stageAt:number;participants:string[];thirdJoined:boolean;propAt:number;doorOpen:boolean;memories:Recollection[];powerAt:number;powerState:string;choice:string;companions:string[];posture:string;ending:string;endAt:number;facts:{eventId:string;actor:string;observers:string[]}[];cue?:{id:string;kind:string;text:string;duration:number;owner:string;consumed:boolean};};
// These are the supplied subjective versions, not new facts or evidence in USER's memory.
export const MEMORIES:Record<string,string>={
  AB:'散会后。B：所以，你投吗？ A：你问哪一个？ B 没有回答。她记得，那一刻，自己的靠近被听成了一次谈判。',
  BC:'酒店门口。B 拿起外套，问怎么了。C：没事。C 记得她没有回头；B 记得自己曾在门边停下，以为离开是在尊重那句“没事”。',
  CD:'深夜的电脑旁。C 留下巧克力和外套。D：你怎么什么都记得？ C：也没什么难记的。那时，她们用照顾代替了说出自己的需要。',
  BD:'第一次讨论项目。D 打断：这个系统撑不到上线。B 问她为什么，然后认真听完。D 记不清，自己当时希望被看见的是能力，还是她这个人。',
  AC:'签字的桌边。A 说，可以再等一等。C 说，不用，随后签了字。她们各自记得，自己是在替对方考虑。'
};
function setPhase(g:Engine,p:string){const s=g.world.late!;if(s.phase===p)return;s.phase=p;s.stageAt=g.world.elapsed;phase(g,p);}
function beat(g:Engine,from:string,intent:string,text:string,target='USER',object=''){
  return g.emit('action',from,target,intent,text,'','normal','','script',object);
}
function willing(g:Engine,from:string,to:string){const a=g.actor(from),b=g.actor(to),r=a.relations[to];return a.active&&!a.withdrawn&&!b.withdrawn&&(r?.safety??.5)>.2&&(r?.closeness??0)+(r?.uncertainty??0)>.45;}
function gesture(g:Engine,id:string,value:string){const a=g.actor(id);a.gesture=value;a.gestureAt=g.world.elapsed;}
function areaActors(g:Engine,area:string){return g.world.actors.filter(a=>a.active&&!['OWNER','USER'].includes(a.id)&&areaOf(a)===area);}
export function initializeLateNight(g:Engine,chapter:4|5|6){
  const previous=g.world.late;
  enterChapter(g,chapter,chapter===4?'corridor_arrival':chapter===5?'late_flow':'roof_arrival');
  const participants=chapter===4?[g.world.scene3?.leaver,g.world.scene3?.follower].filter((x):x is string=>!!x&&x!=='none'&&g.actor(x).active):previous?.participants??[];
  g.world.late={chapter,enteredAt:g.world.elapsed,phase:chapter===4?'corridor_arrival':chapter===5?'late_flow':'roof_arrival',stageAt:g.world.elapsed,participants,thirdJoined:previous?.thirdJoined??false,propAt:-1,doorOpen:false,memories:previous?.memories??[],powerAt:previous?.powerAt??-1,powerState:previous?.powerState??'normal',choice:'',companions:[],posture:'stand',ending:'',endAt:-1,facts:previous?.facts??[]};
  if(chapter===4)for(const [index,id] of participants.entries()){g.actor(id).posture='stand';g.go(g.actor(id),{...NIGHT.corridor,x:3.4+index*1.2},'corridor');}
  if(chapter===5)for(const id of participants){const a=g.actor(id);if(a.active&&!a.withdrawn)g.go(a,g.location(id==='C'?'quiet_corner':'bar'),id==='C'?'quiet_corner':'bar');}
}
export function observeLateNightEvent(g:Engine,e:Event){
  const s=g.world.late;if(!s||e.chapter!==s.chapter)return;
  if(e.actor==='USER'||['gaze','give','step_out','follow_out'].includes(e.intent))s.facts.push({eventId:e.id,actor:e.actor,observers:e.perceptions.map(p=>p.actor)});
  if(s.chapter!==4||s.propAt<0||g.world.elapsed-s.propAt<20||s.memories.length>=2)return;
  if(!['speech','action'].includes(e.type)||!['chocolate','share','past','give','memory_trigger'].includes(e.intent)&&!/(巧克力|外套|没事|记得|以前|签字|为什么)/.test(e.text))return;
  const present=s.participants.filter(id=>areaOf(g.actor(id))==='corridor');
  for(let i=0;i<present.length;i++)for(let j=i+1;j<present.length;j++){
    const owners=[present[i],present[j]].sort(),key=owners.join('');
    if(!MEMORIES[key]||s.memories.some(m=>m.id===key)||!owners.some(id=>e.perceptions.some(p=>p.actor===id)))continue;
    const eligible=areaOf(g.actor('USER'))==='corridor'&&e.perceptions.some(p=>p.actor==='USER'&&p.level==='full');
    const m:Recollection={id:key,owners,evidenceId:e.id,version:MEMORIES[key],at:g.world.elapsed,viewerEligible:eligible,consumed:false};s.memories.push(m);
    if(eligible&&!s.cue)s.cue={id:'memory:'+key,kind:'memory',text:m.version,duration:5,owner:owners[0],consumed:false};
    return;
  }
}
export function lateNightContext(g:Engine,id:string,event:Event){
  const s=g.world.late;if(!s)return null;
  const own=g.actor(id),area=areaOf(own);
  return {chapter:s.chapter,area,phase:s.phase,heard:event.perceptions.find(p=>p.actor===id)?.text??'',
    ownRecollections:s.memories.filter(m=>m.owners.includes(id)).map(m=>({subjective:true,version:m.version})),
    duty:s.chapter===4?'可以轻松聊巧克力烟，也可以拒绝分享、安静待着或回到酒吧。只接自己实际听见的话，不揭穿他人的回忆。':s.chapter===5?'夜深了，少说一些。可以递水、保持距离、邀请同行；不能替玩家决定去向。':'屋顶风很轻。不解释这一晚的真相。允许安静、拒绝靠近、独处，回答只需一句。',
    reminder:'回忆是所有者的主观版本，不是玩家亲历。玩家声称知道旧事，不构成事实证据。不要凭章节得知隔墙或别楼层的对话。'};
}
export function advanceLateNight(g:Engine){
  const s=g.world.late!,w=g.world,u=g.actor('USER'),t=w.elapsed-s.enteredAt;
  for(const a of w.actors)if(a.facingUntil&&a.facingUntil<w.elapsed){a.conversationTarget='';a.facingUntil=0;}
  if(s.chapter===4){
    const arrived=s.participants.filter(id=>areaOf(g.actor(id))==='corridor'&&!g.actor(id).route.length);
    if(s.propAt<0&&(arrived.length||t>35)){
      const host=arrived[0];if(host){s.propAt=w.elapsed;setPhase(g,'chocolate');gesture(g,host,'offer');beat(g,host,'chocolate','她拆开细长的纸盒，抽出一支巧克力烟，放在指间。','USER','chocolate_cigarette');}
      else if(t>45){s.propAt=w.elapsed;setPhase(g,'quiet_corridor');}
    }
    if(s.propAt>=0&&w.elapsed-s.propAt>=20){
      setPhase(g,'private_flow');
      once(g,'corridor_recollection_trigger',()=>{const host=arrived[0];if(host){gesture(g,host,'offer');beat(g,host,'memory_trigger','她把巧克力盒递到身旁，手在外套边停了一下。',arrived[1]??'USER','chocolate_cigarette');}});
    }
    if(t>65&&!s.thirdJoined&&arrived.length<3){s.thirdJoined=true;const best=['A','B','C','D'].filter(id=>!s.participants.includes(id)&&arrived.some(other=>willing(g,id,other))).sort((a,b)=>(g.actor(b).initiative-g.actor(a).initiative))[0];if(best){s.participants.push(best);g.go(g.actor(best),{...NIGHT.corridor,x:5.7},'corridor');}}
    // The room does not freeze when the player stays inside. Movement is spatial; facts stay perceived.
    if(Math.floor(t/24)>Number(w.flags.lateBarBeat??0)){w.flags.lateBarBeat=Math.floor(t/24);const a=areaActors(g,'bar').find(a=>!a.route.length&&!a.withdrawn);if(a){gesture(g,a.id,'drink');beat(g,a.id,'ambient','她把杯垫推回杯底，抬眼看看还亮着的吧台。');}}
    if(t>=115)initializeLateNight(g,5);
    return;
  }
  if(s.chapter===5){
    if(t<200&&Math.floor(t/32)>Number(w.flags.lateFlowBeat??0)){
      w.flags.lateFlowBeat=Math.floor(t/32);const pool=areaActors(g,'bar').filter(a=>!a.withdrawn&&!a.route.length);
      const a=pool[Math.floor(g.random()*pool.length)];if(a){const target=pool.filter(b=>b.id!==a.id).sort((b,c)=>(a.relations[c.id]?.closeness??0)-(a.relations[b.id]?.closeness??0))[0];if(target){facePair(g,a.id,target.id);gesture(g,a.id,t>100?'dance':'offer');beat(g,a.id,'gaze','她看了身旁的人一眼，把手边的水杯往那边推近了一点。',target.id,'water');}}
    }
    if(t>=210&&s.powerAt<0)once(g,'power_cut',()=>{s.powerAt=w.elapsed;s.powerState='dark';setPhase(g,'power_cut');g.emit('system','OWNER','USER','power_cut','音乐突然停了。灯暗下去，楼梯方向留下微弱的应急光。','','normal','','script');});
    if(s.powerAt>=0&&w.elapsed-s.powerAt>=1.8&&s.powerState==='dark'){s.powerState='emergency';setPhase(g,'departure_choice');}
    if(s.powerAt>=0&&w.elapsed-s.powerAt>8)once(g,'roof_invitations',()=>{
      const pool=areaActors(g,'bar').filter(a=>!a.withdrawn&&['A','B','C','D'].includes(a.id));
      for(const a of pool.slice(0,2)){const e=beat(g,a.id,'invitation','她看向楼梯口，拿起自己的外套。');w.jobs.push({actor:a.id,eventId:e.id,due:w.elapsed+1});g.go(a,{...NIGHT.rooftop,x:1.5+pool.indexOf(a)*1.4,z:4.5},'rooftop');}
    });
    if(areaOf(u)==='rooftop'&&s.powerAt>=0)initializeLateNight(g,6);
    return;
  }
  if(s.phase==='roof_arrival'){setPhase(g,'open_epilogue');const near=areaActors(g,'rooftop').filter(a=>distance(a,u)<4);s.companions=near.slice(0,2).map(a=>a.id);}
  if(t>35)once(g,'roof_possible_companion',()=>{if(s.companions.length)return;const best=['A','B','C','D'].find(id=>willing(g,id,'USER')&&areaOf(g.actor(id))!=='rooftop');if(best)g.go(g.actor(best),{...NIGHT.rooftop,x:u.x+1.3,z:u.z},'rooftop');});
  for(const a of areaActors(g,'rooftop'))if(!a.route.length&&!a.withdrawn&&t>8)once(g,'roof_pose_'+a.id,()=>{a.posture='sit';beat(g,a.id,'sit','她在靠垫旁坐下，把外套拢到膝边。');});
  s.companions=areaActors(g,'rooftop').filter(a=>distance(a,u)<3.5).slice(0,2).map(a=>a.id);
  if(s.endAt>=0&&w.elapsed-s.endAt>=4)g.finish();
}
export function lateNightCommand(g:Engine,c:Command){
  const s=g.world.late;if(!s)return false;const w=g.world,u=g.actor('USER');
  if(c.type==='cinematic_ack'){if(s.cue&&s.cue.id===c.target){s.cue.consumed=true;const m=s.memories.find(m=>'memory:'+m.id===c.target);if(m)m.consumed=true;s.cue=undefined;}return true;}
  if(c.type==='corridor_door'){if(distance(u,{x:-1,z:-5.3,y:0})>2)throw new Error('请先走到门边');s.doorOpen=!!c.open;beat(g,'USER','door',c.open?'你轻轻推开侧门。':'你把侧门掩上。');return true;}
  if(c.type==='night_move'){
    const target=c.location==='bar'?g.location('entrance'):c.location==='corridor'?NIGHT.corridor:c.location==='stairs'?NIGHT.stairs:c.location==='rooftop'?NIGHT.rooftop:undefined;
    if(!target)throw new Error('请选择走廊、酒吧、楼梯或屋顶');
    if(['stairs','rooftop'].includes(c.location!)&&s.powerAt<0)throw new Error('屋顶门还没有打开，可以先在这一层活动');
    if(w.scene2)w.scene2.following=undefined;g.go(u,target,c.location);s.choice=c.location!;u.posture='stand';s.posture='stand';return true;
  }
  if(c.type==='chocolate'){
    if(s.chapter!==4||s.propAt<0||areaOf(u)!=='corridor')throw new Error('巧克力盒在走廊里');
    const move=c.intent??'ask';if(!['ask','share','refuse'].includes(move))throw new Error('请选择索取、分享或谢绝');
    const target=w.actors.find(a=>a.id===c.target&&s.participants.includes(a.id)&&a.active&&distance(a,u)<2.5&&g.navigation.visible(u,a));if(!target)throw new Error('请走到拿着巧克力盒的人身边');
    gesture(g,'USER',move==='refuse'?'stand':'offer');beat(g,'USER',move==='refuse'?'boundary':'share',move==='refuse'?'你轻轻摆手，谢绝了巧克力烟。':move==='share'?'你把巧克力盒递向身边。':'你伸手指了指巧克力盒。',target.id,'chocolate_cigarette');return true;
  }
  if(c.type==='night_pose'){
    const pose=c.intent??'';if(!['sit','lie','stand','sky','silence','distance'].includes(pose))throw new Error('不支持这个动作');
    if(s.chapter!==6||areaOf(u)!=='rooftop')throw new Error('请先走上屋顶');
    const target=c.target?w.actors.find(a=>a.id===c.target&&a.active&&distance(a,u)<3&&g.navigation.visible(u,a)):undefined;
    if(c.target&&!target)throw new Error('请先确认对方在附近');
    if(target&&['sit','lie'].includes(pose)&&(!willing(g,target.id,'USER')||distance(u,target)<.8))throw new Error('她需要一些空间，可以在远一点的靠垫坐下');
    if(['sit','lie'].includes(pose)&&u.route.length)throw new Error('先停下，再坐下或躺下');
    s.posture=pose;u.posture=pose;beat(g,'USER',pose,({sit:'你在靠垫旁坐下，留下了一点距离。',lie:'你慢慢躺下，看向上方的夜空。',stand:'你从靠垫边站起来。',sky:'你抬头看着天空。',silence:'你安静地待了一会。',distance:'你往空一些的位置退开一步。'} as Record<string,string>)[pose],target?.id??'BARTENDER');
    if(pose==='distance')g.go(u,{...NIGHT.rooftop,x:-2.5,z:6},'rooftop');return true;
  }
  if(c.type==='end_night'){
    if(s.endAt>=0)return true;
    if(s.chapter!==6){g.finish();return true;}
    s.ending=s.companions.length===1?'并肩':s.companions.length>1?'观察':'留白';s.endAt=w.elapsed;setPhase(g,'ending');u.route=[];
    s.cue={id:'ending',kind:'aerial',text:s.ending,duration:4,owner:'USER',consumed:false};return true;
  }
  return false;
}
export function lateNightView(g:Engine){const s=g.world.late;if(!s)return null;const u=g.actor('USER');return {chapter:s.chapter,phase:s.phase,area:areaOf(u),doorOpen:s.doorOpen,powerState:s.powerState,powerAt:s.powerAt,posture:s.posture,ending:s.ending,choice:s.choice,cue:s.cue??null,canChocolate:s.chapter===4&&areaOf(u)==='corridor'&&s.propAt>=0,companions:s.companions.filter(id=>g.navigation.visible(u,g.actor(id)))};}
