import type {Engine} from './engine.js';
import type {Command,Event,Point} from './types.js';
import {distance} from './navigation.js';

export type SceneOneState={
  version:1;phase:'first_meeting'|'drink_delivery'|'drink_placed'|'free_interaction'|'d_arrival'|'scene2_ready';
  started:boolean;drinkEventId:string;drinkPlacedAt:number;arrivalEventId:string;arrivalAt:number;
  phoneAt:number;firstTarget:string;firstSocialMove:string;askedAboutThirdDrink:boolean;occupiedReservedSeat:boolean;
  knownNames:Record<string,string>;firstImpressions:Record<string,{eventId:string;behavior:string}[]>;
  lightInteractions:number;pendingApproach?:string;seated:boolean;
};
export const DRINK:Point={x:1.73,z:-1.9};
export const SEAT:Point={x:.95,z:-1.7};
const DELIVERY:Point={x:1.05,z:-2.65};
export function initializeSceneOne(g:Engine){
  g.world.scene1={version:1,phase:'first_meeting',started:false,drinkEventId:'',drinkPlacedAt:-1,arrivalEventId:'',arrivalAt:-1,phoneAt:-1,
    firstTarget:'',firstSocialMove:'',askedAboutThirdDrink:false,occupiedReservedSeat:false,knownNames:{},firstImpressions:{},lightInteractions:0,seated:false};
  for(const [id,p,yaw]of [['A',{x:-3.9,z:-4.05},70],['B',{x:2.7,z:-2.2},260],['C',{x:3.4,z:1.2},230]] as const)Object.assign(g.actor(id),g.navigation.nearest(p),{active:true,yaw,route:[]});
}
export function facePair(g:Engine,from:string,to:string){
  const a=g.actor(from),b=g.actor(to);
  if(a.withdrawn||b.withdrawn||a.route.length||b.route.length||distance(a,b)>3||!g.navigation.visible(a,b))return;
  for(const [speaker,target]of [[a,b],[b,a]]){
    speaker.conversationTarget=target.id;speaker.facingUntil=g.world.elapsed+8;
    speaker.yaw=Math.atan2(target.x-speaker.x,target.z-speaker.z)*180/Math.PI;
  }
}
export function releaseFacing(g:Engine,id:string){
  const a=g.actor(id),other=a.conversationTarget;a.conversationTarget='';a.facingUntil=0;
  if(other){const b=g.actor(other);if(b.conversationTarget===id){b.conversationTarget='';b.facingUntil=0;}}
}
export function sceneOneAlias(id:string){return ({A:'眼镜来客',B:'浅衣来客',C:'露台边的来客',D:'刚到的来客'} as Record<string,string>)[id];}
export function sceneOneDisplayName(g:Engine,id:string){
  if(!g.world.scene1||g.world.scene1.knownNames[id]||!['A','B','C','D'].includes(id))return g.actor(id).name;
  return sceneOneAlias(id);
}
export function sceneOneContext(g:Engine,id:string,event:Event){
  const s=g.world.scene1;if(!s)return null;
  const duties:Record<string,string>={
    BARTENDER:'你提前给尚未到场的人准备了第三杯。杯主到场前不要说她是谁、不要解释关系。若有人问你叫什么，可以自然引见在场熟客。',
    B:'你知道第三杯给谁，熟悉迟到者的习惯。自然接话，不主动解释关系、不把新客当成项目。第三杯不是玩家的欢迎酒，不能递给玩家或说这杯请她；杯主到场前不提她名字。',
    A:'只根据自己看见或听见的事观察 B；可以轻轻试探她记得很清楚，也可以不说话。不解释关系、不演成质问。',
    C:'不负责讲解关系。根据看到的座位和第三杯反应，可以简短含蓄地提及座位安排，也可以保持沉默。',
    D:'刚从雨里进来，手上仍处理当前身份包的工作问题。回应当前事件，可用一句自然的工作短句，不解释秘密关系。'
  };
  return {chapter:'酒吧初见：第三杯',phase:s.phase,duty:duties[id]||'',object:event.objectTarget||'',
    namesYouKnow:Object.fromEntries(g.actor(id).knownActors.filter(k=>g.actor(k).active).map(k=>[k,g.actor(k).name])),
    ownName:g.actor(id).name,nameRequested:event.intent==='ask_name',
    reminder:'姓名只通过真实点名或自我介绍得知。先接住眼前这句话，不把职业词硬塞进私人闲聊。不根据玩家未公开的背景说话。'};
}
export function observeSceneOneEvent(g:Engine,e:Event){
  const s=g.world.scene1;if(!s)return;
  const userPerception=e.perceptions.find(p=>p.actor==='USER');
  if(e.type==='speech'&&e.actor!=='USER'&&userPerception?.level==='full'){
    for(const id of ['A','B','C','D'])if(g.actor(id).active&&userPerception.text.toLowerCase().includes(g.actor(id).name.toLowerCase())){
      s.knownNames[id]??=e.id;
      if(g.world.intro&&!g.world.intro.revealed.includes(id))g.world.intro.revealed.push(id);
    }
  }
  if(e.actor==='USER'&&['speech','action'].includes(e.type)){
    s.firstTarget||=e.objectTarget||e.target;s.firstSocialMove||=e.intent;s.lightInteractions++;
    for(const p of e.perceptions.filter(p=>!['USER','OWNER'].includes(p.actor))){
      const list=s.firstImpressions[p.actor]??=[];
      if(list.length<8)list.push({eventId:e.id,behavior:p.text});
    }
  }
}
function action(g:Engine,intent:string,text:string,target='BARTENDER',objectTarget=''){
  return g.emit('action','USER',target,intent,text,'','normal','','player',objectTarget);
}
export function advanceSceneOne(g:Engine){
  const s=g.world.scene1;if(!s||g.world.intro?.phase==='elevator')return;
  const w=g.world,u=g.actor('USER');
  if(!s.started){
    s.started=true;w.elapsed=0;
    for(const [id,p,yaw]of [['A',{x:-3.9,z:-4.05},70],['B',{x:2.7,z:-2.2},260],['C',{x:3.4,z:1.2},230]] as const){
      const a=g.actor(id);Object.assign(a,g.navigation.nearest(p),{active:true,yaw,route:[],animation:'idle'});
    }
    g.actor('D').active=false;
    g.emit('action','B','USER','welcome','浅衣来客朝主桌旁让了半步，留出一个可以站的位置。','','normal','','script');
  }
  for(const a of w.actors)if(a.conversationTarget&&(a.facingUntil??0)<w.elapsed)releaseFacing(g,a.id);
  if(s.pendingApproach){const t=w.actors.find(a=>a.id===s.pendingApproach);if(t&&!u.route.length&&distance(u,t)<=3){action(g,'approach','你走近了'+sceneOneDisplayName(g,t.id)+'。',t.id);facePair(g,'USER',t.id);s.pendingApproach=undefined;}}
  if(s.phase==='first_meeting'&&(distance(u,g.location('main_table'))<3&&w.elapsed>=8||w.elapsed>=45)){
    s.phase='drink_delivery';g.go(g.actor('BARTENDER'),DELIVERY);
    g.emit('action','BARTENDER','B','reserved_drink','调酒师端起一杯浅金色的酒，走向主桌旁的空椅。','','normal','','script','third_drink');
  }
  if(s.phase==='drink_delivery'){
    const bartender=g.actor('BARTENDER');
    if(!bartender.route.length&&distance(bartender,DELIVERY)<.85){
      s.phase='drink_placed';s.drinkPlacedAt=w.elapsed;
      const e=g.emit('action','BARTENDER','B','reserved_drink','调酒师把提前准备好的第三杯放到空椅前，杯底轻轻碰到木桌。','','normal','','script','third_drink');
      s.drinkEventId=e.id;
    }else if(!bartender.route.length)g.go(bartender,DELIVERY);
  }
  if(s.phase==='drink_placed'&&w.elapsed-s.drinkPlacedAt>=3){s.phase='free_interaction';g.go(g.actor('BARTENDER'),g.location('bar'));}
  if(s.drinkEventId&&s.phoneAt<0&&w.elapsed-s.drinkPlacedAt>=12){
    s.phoneAt=w.elapsed;g.actor('A').animation='phone';
    g.emit('action','A','A','phone_vibration','眼镜来客的手机短促震动了一下；她低头看了一眼，又将屏幕扣下。','','normal','','script');
  }
  if(s.phase==='free_interaction'&&w.elapsed>=120&&s.lightInteractions>0){
    s.phase='d_arrival';s.arrivalAt=w.elapsed;
    const d=g.actor('D');Object.assign(d,g.navigation.nearest(g.location('entrance')),{active:true,withdrawn:false});
    const e=g.emit('action','D','B','arrival','门再次打开，雨声短暂涌入。刚到的来客看着手机，在处理尚未结束的工作。','','normal','','script');
    s.arrivalEventId=e.id;w.jobs.push({actor:'D',eventId:e.id,due:w.elapsed+1});
    g.go(d,g.navigation.nearest({x:1.15,z:-.8}));
  }
  if(s.phase==='d_arrival'){
    const dt=w.elapsed-s.arrivalAt;
    for(const [id,delay]of [['B',0],['C',.6],['A',1.2],['BARTENDER',1.8]] as const){
      if(dt>=delay&&dt<7){const a=g.actor(id),d=g.actor('D');if(!a.active||a.withdrawn||a.route.length)continue;a.yaw=Math.atan2(d.x-a.x,d.z-a.z)*180/Math.PI;a.facingUntil=w.elapsed+1;a.conversationTarget='D';a.animation=id==='C'?'pause':'idle';}
    }
    if(dt>=8){s.phase='scene2_ready';g.emit('action','BARTENDER','D','reserved_drink','调酒师将刚才那杯酒朝新来的客人轻轻推近。','','normal','','script','third_drink');}
  }
}
export function sceneOneCommand(g:Engine,c:Command):boolean{
  const s=g.world.scene1;if(!s)return false;
  const u=g.actor('USER');
  if(c.type==='release_facing'){releaseFacing(g,'USER');return true;}
  if(c.type==='observe_object'||c.type==='sit_reserved'){
    const object=c.type==='sit_reserved'?'reserved_seat':c.objectTarget||'third_drink';
    if(!['third_drink','reserved_seat'].includes(object))throw new Error('这里没有这个可观察对象');
    const point=object==='third_drink'?DRINK:SEAT;
    if(distance(u,point)>4||!g.navigation.visible(u,point))throw new Error('请先走近，并确认没有遮挡');
    if(object==='third_drink'&&!s.drinkEventId)throw new Error('桌上还没有第三杯');
    if(c.type==='sit_reserved'){
      if(distance(u,SEAT)>1.25)throw new Error('请先走到空椅旁');
      if(s.occupiedReservedSeat&&s.seated)return true;
      s.occupiedReservedSeat=true;s.seated=true;u.route=[];u.animation='sit';
      action(g,'sit','你在空椅旁坐下。','B','reserved_seat');
    }else action(g,'observe',object==='third_drink'?'你靠近看了看那杯还没有人喝的浅金色酒。':'你看了看主桌旁留出的空椅。','BARTENDER',object);
    return true;
  }
  if(c.type!=='talk')return false;
  const text=(c.text||'').trim();if(!text||[...text].length>200)throw new Error('请输入 1–200 字');
  const object=c.objectTarget||(/这杯|第三杯|杯子|谁的酒/.test(text)?'third_drink':/空椅|空位|这个位置|这个座/.test(text)?'reserved_seat':'');
  if(object&&!['third_drink','reserved_seat'].includes(object))throw new Error('不支持的对象');
  if(object==='third_drink'&&!s.drinkEventId)throw new Error('第三杯还没放下');
  if(object){const p=object==='third_drink'?DRINK:SEAT;if(distance(u,p)>4||!g.navigation.visible(u,p))throw new Error('请先走近你想问的物品');}
  const id=c.target||((object==='third_drink')?'BARTENDER':'B'),target=g.world.actors.find(a=>a.id===id&&a.active);
  if(!target||id==='USER')throw new Error('请选择在场的人');
  if(distance(u,target)>3||!g.navigation.visible(u,target))throw new Error('请先走近对方，并确认中间没有遮挡');
  if(g.world.scene2&&!g.world.scene3&&!target.withdrawn){
    target.route=[];target.routeVersion=(target.routeVersion??0)+1;target.destination='';target.animation='idle';
    target.nextAction=Math.max(target.nextAction,g.world.elapsed+18);
  }
  const intent=/叫什么|怎么称呼|名字|你是[谁哪]/.test(text)?'ask_name':/别问|不想|不要|空间/.test(text)?'boundary':object?'probe':/想见|喜欢|在意|陪你/.test(text)?'reveal':'chat';
  if(object==='third_drink')s.askedAboutThirdDrink=true;
  facePair(g,'USER',id);
  const e=g.emit('speech','USER',id,intent,text,'','normal','','player',object);
  e.tone=c.tone||'natural';e.movement='none';
  return true;
}
