import {NightNavigator,areaOf,heightAt} from './night-navigation.js';
import {lateNightCommand} from './late-night.js';
import type {Engine} from './engine.js';
import type {Command} from './types.js';
import {distance} from './navigation.js';
import {runBeats} from './beats.js';
import {introActive,introCommand,displayName} from './intro.js';
import {buildProfile,selectPack} from './identity.js';
import {sceneOneCommand,releaseFacing,facePair} from './scene-one.js';
import {sceneTwoCommand} from './scene-two.js';
import {sceneThreeCommand} from './scene-three.js';
export function handleCommand(g:Engine,c:Command){
  const w=g.world;
  if(c.version!==undefined&&c.version!==1)throw new Error('协议版本不兼容');
  if(c.sessionId&&c.sessionId!==w.id)throw new Error('命令属于另一个夜晚');
  if(!c.id||!c.type)throw new Error('命令缺少标识');
  if(w.commandIds.includes(c.id))return;
  if(introCommand(g,c)){remember(g,c);return;}
  if(c.type==='pause'){w.paused=!!c.paused;remember(g,c);return;}
  if(c.type==='mode'){w.modelMode=c.online?'online':'offline';w.modelReason=c.online?'联网模式':'已选择规则模式';for(const r of w.replies||[])if(r.status==='error'){r.status='queued';r.error='';if(!w.jobs.some(j=>j.actor===r.actor&&j.eventId===r.eventId))w.jobs.push({actor:r.actor,eventId:r.eventId,due:w.elapsed});}remember(g,c);return;}
  if(c.type==='retry_reply'){const r=w.replies?.find(r=>r.id===c.requestId);if(!r)throw new Error('没有可重试的回复');if(r.status==='error'){r.status='queued';r.error='';if(!w.jobs.some(j=>j.actor===r.actor&&j.eventId===r.eventId))w.jobs.push({actor:r.actor,eventId:r.eventId,due:w.elapsed});}remember(g,c);return;}
  if(c.type==='release_facing'){releaseFacing(g,'USER');remember(g,c);return;}
  if(c.type==='revise_context'){
    const profile=buildProfile(c.choices,typeof c.text==='string'?c.text:undefined);
    const {packId}=selectPack(profile,g.scenario.identityPacks,w.seed);
    // Deferred to the next beat boundary: the occupational shell may change, never mid-sentence,
    // and established relationship state and memory stay untouched.
    w.pendingPackRevision={packId,contextProfile:profile};
    remember(g,c);return;
  }
  if(w.status!=='playing'){if(c.type==='position'||c.type==='positions')return;throw new Error('本局已经结束');}
  if(introActive(w)){if(c.type==='position'||c.type==='positions')return;throw new Error('请等待电梯到达');}
  if(c.type==='positions'){
    if(!Array.isArray(c.items)||c.items.length>8)throw new Error('Invalid position batch');
    for(const item of c.items)position(g,item);
    return;
  }
  if(c.type==='position'){position(g,c);return;}
  if(c.type==='leave'){g.finish();remember(g,c);return;}
  // Stopping an already requested walk is a control action, including while a reply pauses movement.
  if(c.type==='cancel_move'){g.actor('USER').route=[];g.actor('USER').destination='';if(w.scene2)w.scene2.following=undefined;releaseFacing(g,'USER');if(w.scene1){w.scene1.pendingApproach=undefined;w.scene1.seated=false;}g.actor('USER').posture='stand';if(w.late){w.late.posture='stand';if(c.intent==='stay')w.late.choice='stay';}remember(g,c);return;}
  if(w.paused||g.busy)throw new Error('请稍等当前回复完成，或继续游戏');
  // Later chapters claim their own verbs first; anything they do not own falls through to Scene 1 and
  // then to the legacy night, so the shared verbs (talk, approach, observe) keep working throughout.
  if(lateNightCommand(g,c)){remember(g,c);return;}
  if(!w.late&&sceneThreeCommand(g,c)){remember(g,c);return;}
  if((c.type==='follow_target'||!w.late&&(!w.scene3||c.type==='observe_object'))&&sceneTwoCommand(g,c)){remember(g,c);return;}
  if(sceneOneCommand(g,c)){remember(g,c);return;}
  const u=g.actor('USER');
  switch(c.type){
    case 'start_party':
      if(w.flags.cardsJoined)break;
      if(!w.flags.cardsOffered){
        w.flags.cardsOffered=true;
        g.emit('system','OWNER','USER','invite','你问起牌局。老板娘拿出情境牌：想参加就来，不想回答可以跳过。');
      }
      w.flags.cardsJoined=true;w.flags.cardsDeclined=false;
      g.emit('message','OWNER','USER','party_join','你加入了牌局。选一张牌、一个人，再选择表达；回答与拒绝都算一次交流。');
      break;
    case 'move_to':{
      const l=g.location(c.location??'entrance');
      if(l.id==='service'&&!['staff'].includes(w.role))throw new Error('这个入口不能进入服务区');
      if(w.actors.filter(a=>a.active&&a.id!=='USER'&&g.zone(a).id===l.id).length>=l.capacity)throw new Error('这里暂时没有空位');
      if(w.scene2)w.scene2.following=undefined;
      g.go(u,l,l.id);break;
    }
    case 'approach_target':{
      const t=g.actor(c.target??'B');
      if(!t.active||!g.navigation.visible(u,t))throw new Error('请先找到可见的目标');
      if(t.withdrawn)throw new Error('对方正准备离开，请先给她一点空间');
      // In Scene 2 roaming guests acknowledge a visible approach and pause at their next natural
      // beat. Do not chase a moving target forever, and never stop somebody who is actually leaving.
      if(w.scene2&&!w.scene3){
        t.nextAction=Math.max(t.nextAction,w.elapsed+18);
        if(t.route.length&&!['corridor','outside'].includes(t.destination)){
          t.route=[];t.routeVersion=(t.routeVersion??0)+1;t.destination='';t.animation='idle';
        }
      }
      g.go(u,g.near(u,t));if(w.scene1)w.scene1.pendingApproach=t.id;break;
    }
    case 'observe':{
      w.moves.observe=(w.moves.observe??0)+1;
      recordWithdrawal(g);
      const nearby=w.actors.filter(a=>a.active&&a.id!=='USER'&&distance(a,u)<5&&g.navigation.visible(a,u));
      if(w.scene1)g.emit('action','USER','BARTENDER','observe','你停下来观察身边的客人与桌上的摆设。','','normal','','player');
      g.emit('message','OWNER','USER','observe',nearby.length?'你看见 '+nearby.map(a=>displayName(g,a.id)).join('、')+' 在附近。动作可以看见，内心不能。':'这里很安静，暂时没有新的对话。');break;
    }
    case 'join':case 'decline':
      if(!w.flags.cardsOffered)throw new Error('目前还没有牌局邀请');
      w.flags.cardsJoined=c.type==='join';w.flags.cardsDeclined=c.type==='decline';
      g.emit('message','OWNER','USER','boundary',c.type==='join'?'来，给你留了个位置。':'当然，你可以继续聊天，或者只是看看。');break;
    case 'invite_game':
      if(!['staff','event_guest'].includes(w.role))throw new Error('当前入口不能自己组织牌局');
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
  const p={x:c.x!,z:c.z!,y:c.y??a.y??0,area:c.area??a.area};
  if(g.navigation instanceof NightNavigator&&!g.navigation.acceptsPosition(a,p))return;
  if(!g.navigation.walkable(p)||distance(a,p)>1.2)return;
  if(a.id==='USER'&&!['staff'].includes(g.world.role)&&g.zone(p).id==='service')return;
  a.x=p.x;if(g.world.story){a.area=areaOf(p);a.y=heightAt(p);}
  a.z=p.z;
  if((a.id==='USER'||a.route.length>0)&&(!a.conversationTarget||(a.facingUntil??0)<g.world.elapsed))a.yaw=c.yaw??a.yaw;
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
  if(!selected)throw new Error('请先选择一张牌');
  if(!target.active||target.id==='USER')throw new Error('对方已不在场，请重新选择');
  const range=distance(player,target);
  if(range>3||!g.navigation.visible(player,target)){throw new Error('请先走近对方，并确认中间没有遮挡');}
  if(selected.type==='situation'&&!state.flags.cardsJoined&&selected.id!=='last_call')throw new Error('请先加入牌局');
  if(selected.id==='last_call'&&!state.flags.lastCall)throw new Error('最后一次表达在 Last Call 时开放');
  if((state.cooldowns[selected.id]??-1)>state.elapsed)throw new Error('这张牌正在冷却，请稍候或换一张');
  const text=(c.text||selected.expressions[0]).trim();
  if(text.length===0||[...text].length>200)throw new Error('Enter 1-200 characters');
  if(state.intro&&!state.scene1&&!state.intro.revealed.includes(target.id))state.intro.revealed.push(target.id);
  state.cooldowns[selected.id]=state.elapsed+selected.cooldown;
  state.moves[selected.intent]=(state.moves[selected.intent]||0)+1;
  if(selected.intent==='boundary')recordWithdrawal(g);
  if(['approach','reveal'].includes(selected.intent))state.flags.lastApproach=state.elapsed;
  if(selected.effect==='past_drink')state.flags.pastDrink=true;
  const privacy=selected.effect==='private_note'?'private':'normal';
  facePair(g,'USER',target.id);
  const event=g.emit('speech','USER',target.id,selected.intent,text,'',privacy,'','player',selected.effect==='photo'?'photo_request':'');
  if(selected.effect==='outside')g.go(player,g.location('outside'),'outside');
  if(selected.effect==='invite_table')state.flags.tableInvitation=target.id;
  if(selected.effect==='photo'&&target.privatePhoto&&state.modelMode!=='online'){
    state.jobs=state.jobs.filter(job=>job.actor!==target.id||job.eventId!==event.id);
    g.apply(target.id,{generationSource:'rules',action:'speak',target:'USER',intent:'boundary',expression:'谢谢你先问。我不想拍照，也不希望公开我的照片。',interpretation:'Protect privacy',evidenceIds:[event.id],signal:'boundary',confidence:1},event.id);
  }
}
function recordWithdrawal(g:Engine){
  const count=Number(g.world.flags.withdrawCount??0);
  g.world.flags['withdrawAt'+(count%3)]=g.world.elapsed;
  g.world.flags.withdrawCount=count+1;
}
