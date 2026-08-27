import type {Engine} from './engine.js';
import type {Command} from './types.js';
import {distance} from './navigation.js';
import {runBeats} from './beats.js';
export function handleCommand(g:Engine,c:Command){
  const w=g.world;
  if(c.version!==undefined&&c.version!==1)throw new Error('协议版本不兼容');
  if(c.sessionId&&c.sessionId!==w.id)throw new Error('命令属于另一个夜晚');
  if(!c.id||!c.type)throw new Error('命令缺少标识');
  if(w.commandIds.includes(c.id))return;
  if(c.type==='pause'){w.paused=!!c.paused;remember(g,c);return;}
  if(c.type==='mode'){w.modelMode=c.online?'online':'offline';w.modelReason=c.online?'联网模式':'已选择规则模式';remember(g,c);return;}
  if(w.status!=='playing'){if(c.type==='position'||c.type==='positions')return;throw new Error('本局已经结束');}
  if(c.type==='positions'){
    if(!Array.isArray(c.items)||c.items.length>8)throw new Error('Invalid position batch');
    for(const item of c.items)position(g,item);
    return;
  }
  if(c.type==='position'){position(g,c);return;}
  if(c.type==='leave'){g.finish();remember(g,c);return;}
  if(w.paused||g.busy)throw new Error('请稍等当前回复完成，或继续游戏');
  const u=g.actor('USER');
  switch(c.type){
    case 'cancel_move':u.route=[];u.destination='';break;
    case 'move_to':{
      const l=g.location(c.location??'entrance');
      if(l.id==='service'&&!['staff','owner_bartender'].includes(w.role))throw new Error('这个入口不能进入服务区');
      if(w.actors.filter(a=>a.active&&a.id!=='USER'&&g.zone(a).id===l.id).length>=l.capacity)throw new Error('这里暂时没有空位');
      g.go(u,l,l.id);break;
    }
    case 'approach_target':{
      const t=g.actor(c.target??'B');
      if(!t.active)throw new Error('目标不在场');
      g.go(u,g.near(u,t));break;
    }
    case 'observe':{
      w.moves.observe=(w.moves.observe??0)+1;
      recordWithdrawal(g);
      const nearby=w.actors.filter(a=>a.active&&a.id!=='USER'&&distance(a,u)<5&&g.navigation.visible(a,u));
      g.emit('message','OWNER','USER','observe',nearby.length?'你看见 '+nearby.map(a=>a.name).join('、')+' 在附近。动作可以看见，内心不能。':'这里很安静，暂时没有新的对话。');break;
    }
    case 'join':case 'decline':
      if(!w.flags.cardsOffered)throw new Error('目前还没有牌局邀请');
      w.flags.cardsJoined=c.type==='join';w.flags.cardsDeclined=c.type==='decline';
      g.emit('message','OWNER','USER','boundary',c.type==='join'?'来，给你留了个位置。':'当然，你可以继续聊天，或者只是看看。');break;
    case 'invite_game':
      if(!['staff','owner_bartender','social_guest'].includes(w.role))throw new Error('当前入口不能自己组织牌局');
      w.flags.cardsOffered=true;
      g.emit('system','USER','OWNER','connect','要不要一起玩一局？不参加也没关系。');break;
    case 'interact':case 'card':playCard(g,c);break;
    case 'leave':g.finish();break;
    default:throw new Error('不支持的命令');
  }
  remember(g,c);runBeats(g);
}
function remember(g:Engine,c:Command){
  g.world.commandIds.push(c.id);
  g.world.updatedAt=new Date().toISOString();
}
function position(g:Engine,c:Command){
  const a=g.actor(c.actor??'USER');
  if(!a.active||g.world.paused||g.busy||!Number.isFinite(c.x)||!Number.isFinite(c.z)||!Number.isFinite(c.yaw??0))return;
  const p={x:c.x!,z:c.z!};
  if(!g.navigation.walkable(p)||distance(a,p)>1.2)return;
  if(a.id==='USER'&&!['staff','owner_bartender'].includes(g.world.role)&&g.zone(p).id==='service')return;
  a.x=p.x;
  a.z=p.z;
  a.yaw=c.yaw??a.yaw;
  while(a.route.length&&distance(a,a.route[0])<.3)a.route.shift();
  if(!a.route.length){
    if(g.world.elapsed-a.lastSpoke<2.5)a.animation='speak';
    else if(g.zone(a).id==='bar'&&Math.floor(g.world.elapsed/6)%4===0)a.animation='drink';
    else a.animation='idle';
    a.destination='';
    if(a.pending){
      const d=a.pending,parent=a.pendingParent??'';
      a.pending=undefined;
      a.pendingParent=undefined;
      g.apply(a.id,d,parent);
    }
  }
}
function playCard(g:Engine,c:Command){
  const state=g.world;
  const player=g.actor('USER');
  const target=g.actor(c.target||'B');
  const selected=g.scenario.cards.find(item=>item.id===(c.card||c.intent));
  if(!selected)throw new Error('Select a card first');
  if(!target.active||target.id==='USER')throw new Error('Target unavailable');
  const range=distance(player,target);
  if(range>3||!g.navigation.visible(player,target)){throw new Error('Move closer to the character');}
  if(selected.type==='situation'&&!state.flags.cardsJoined&&selected.id!=='last_call')throw new Error('Join the card game first');
  if(selected.id==='last_call'&&!state.flags.lastCall)throw new Error('Last Call is not available');
  if((state.cooldowns[selected.id]??-1)>state.elapsed)throw new Error('Card cooldown');
  const text=(c.text||selected.expressions[0]).trim();
  if(text.length===0||[...text].length>200)throw new Error('Enter 1-200 characters');
  state.cooldowns[selected.id]=state.elapsed+selected.cooldown;
  state.moves[selected.intent]=(state.moves[selected.intent]||0)+1;
  if(selected.intent==='boundary')recordWithdrawal(g);
  if(['approach','reveal'].includes(selected.intent))state.flags.lastApproach=state.elapsed;
  if(selected.effect==='past_drink')state.flags.pastDrink=true;
  const privacy=selected.effect==='private_note'?'private':'normal';
  const event=g.emit('speech','USER',target.id,selected.intent,text,'',privacy);
  if(selected.effect==='outside')g.go(player,g.location('outside'),'outside');
  if(selected.effect==='invite_table')state.flags.tableInvitation=target.id;
  if(selected.effect==='photo'&&target.privatePhoto){
    state.jobs=state.jobs.filter(job=>job.actor!==target.id||job.eventId!==event.id);
    g.apply(target.id,{action:'speak',target:'USER',intent:'boundary',expression:'谢谢你先问。我不想拍照，也不希望公开我的照片。',interpretation:'Protect privacy',evidenceIds:[event.id],signal:'boundary',confidence:1},event.id);
  }
}
function recordWithdrawal(g:Engine){
  const count=Number(g.world.flags.withdrawCount??0);
  g.world.flags['withdrawAt'+(count%3)]=g.world.elapsed;
  g.world.flags.withdrawCount=count+1;
}
