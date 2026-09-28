import type {Engine} from './engine.js';
import {ACTIVITIES,DRINKS,SONGS,drinkLabel} from './first-night-content.js';
import {distance} from './navigation.js';

export type InteractionOption={
  id:string;label:string;selected:boolean;replaceable:boolean;targetRequired:boolean;
  enabled:boolean;disabledReason:string;
};
export type InteractionGroup={id:'observe'|'move'|'interact';label:string;options:InteractionOption[]};
export type InteractionView={
  contextId:string;nextTitle:string;nextHint:string;nextGroup:'observe'|'move'|'interact';nextActionId:string;
  primaryActionId:string;primaryLabel:string;primaryTargetRequired:boolean;suggestions:InteractionOption[];
  groups:InteractionGroup[];
};

const option=(id:string,label:string,values:Partial<InteractionOption>={}):InteractionOption=>({
  id,label,selected:false,replaceable:false,targetRequired:false,enabled:true,disabledReason:'',...values
});

export function interactionView(g:Engine):InteractionView|null{
  const w=g.world;if(!w.story)return null;
  if(w.firstNight)return firstNightInteraction(g);
  const user=g.actor('USER'),stage=w.story.stageAt;
  const selected=(intent:string,object='')=>w.events.some(e=>e.actor==='USER'&&e.time>=stage&&e.intent===intent&&(!object||e.objectTarget===object));
  const commonObserve=[option('observe_room','观察周围',{selected:selected('observe')}),option('observe_target','观察所选人物',{targetRequired:true})];
  const commonMove=[option('approach','靠近所选人物',{targetRequired:true,selected:user.route.length>0&&!!w.scene1?.pendingApproach}),option('locations','前往其他位置',{replaceable:true})];
  const commonInteract=[option('talk','文字交流',{targetRequired:true,replaceable:true}),option('legacy_cards','情境卡牌',{replaceable:true})];
  let contextId='',nextTitle='',nextHint='',nextGroup:'observe'|'move'|'interact'='observe',nextActionId='';
  let observe=[...commonObserve],move=[...commonMove],interact=[...commonInteract];

  if(w.late){
    const s=w.late;contextId=`scene${s.chapter}.${s.phase}`;
    observe=[option('observe_room','观察周围',{selected:selected('observe')}),option('observe_target','观察所选人物',{targetRequired:true})];
    move=[option('approach','靠近所选人物',{targetRequired:true}),option('follow','跟随所选人物',{targetRequired:true,selected:!!w.scene2?.following}),option('move_bar','回到酒吧',{selected:s.choice==='bar'}),option('move_corridor','前往走廊',{selected:s.choice==='corridor'})];
    interact=[option('talk','文字交流',{targetRequired:true,replaceable:true})];
    if(s.chapter===4){
      nextTitle='选择是否跟去走廊';nextHint='可以跟随离席者、留在酒吧，或稍后返回。';nextGroup='move';
      move.push(option('stay','留在这里',{selected:s.choice==='stay'}));
      if(s.propAt>=0)interact.push(option('chocolate_ask','索取一支',{targetRequired:true,selected:selected('share','chocolate_cigarette')}),option('chocolate_share','递给身边的人',{targetRequired:true,selected:w.events.some(e=>e.actor==='USER'&&e.time>=stage&&e.objectTarget==='chocolate_cigarette'&&e.text.includes('递向'))}),option('chocolate_refuse','摆手谢绝',{targetRequired:true,selected:selected('boundary','chocolate_cigarette')}));
    }else if(s.chapter===5){
      if(s.powerState==='normal'){nextTitle='夜色还在流动';nextHint='可以继续观察和交流；断电后会出现明确的散场选择。';nextGroup='observe';nextActionId='observe_room';}
      else{nextTitle='断电后的去向';nextHint='上楼、留下或离开，选择不会替你自动移动。';nextGroup='move';}
      move.push(option('move_rooftop','沿楼梯上楼',{selected:s.choice==='rooftop',enabled:s.powerState!=='normal',disabledReason:s.powerState==='normal'?'楼梯尚未开放':''}),option('stay','留在这里',{selected:s.choice==='stay'}),option('leave','离开酒吧'));
    }else{
      nextTitle='决定何时结束这一晚';nextHint='可以继续交流、调整姿态，或确认结束。';nextGroup='interact';nextActionId='end_night';
      interact.push(...[
        ['pose_sit','坐在靠垫旁','sit'],['pose_lie','躺下看夜空','lie'],['pose_stand','站起来','stand'],
        ['pose_sky','抬头看天空','sky'],['pose_silence','安静待着','silence'],['pose_distance','留一点距离','distance']
      ].map(([id,label,pose])=>option(id,label,{selected:s.posture===pose,replaceable:true})));
      interact.push(option('end_night','结束这一晚'));
      move.push(option('leave','离开'));
    }
  }else if(w.scene3){
    const s=w.scene3;contextId=`scene3.${s.phase}.round${s.round}`;
    observe.push(option('observe_tarot_deck','观察桌上的牌',{selected:selected('observe','tarot_deck')}));
    move.push(option('move_main','前往主桌',{selected:user.destination==='main_table'}));
    interact=[option('talk','文字交流',{targetRequired:true,replaceable:true})];
    if(s.playerStance==='undecided'){
      nextTitle='先选择怎样参与';nextHint='坐下、旁观或不参加，三种选择都能继续剧情。';nextGroup='interact';
      interact=[option('tarot_sit','坐下'),option('tarot_watch','旁观'),option('tarot_decline','不参加')];
    }else if(s.askedAt>=0&&!s.playerMove){
      nextTitle='这一轮轮到你选择';nextHint='只需选择一种回应；选择后本轮不会重复提交。';nextGroup='interact';
      interact=[option('tarot_answer','回答'),option('tarot_skip','跳过'),option('tarot_deflect','让别人先'),option('tarot_ask_back','反问她',{targetRequired:true}),option('tarot_observe','只看着'),option('tarot_joke','开个玩笑')];
    }else{
      nextTitle=s.phase==='scene4_ready'?'有人离开了牌桌':'等待下一张牌';nextHint=s.phase==='scene4_ready'?'可以跟去走廊，也可以继续留在桌边。':'你这一轮已经表态，牌局会自动继续。';nextGroup=s.phase==='scene4_ready'?'move':'observe';
    }
    for(const item of interact){
      const moveId=item.id.replace('tarot_','');
      if(item.id==='tarot_sit')item.selected=s.playerStance==='seated';
      else if(item.id==='tarot_watch')item.selected=s.playerStance==='watching';
      else if(item.id==='tarot_decline')item.selected=s.playerStance==='declined';
      else if(item.id.startsWith('tarot_'))item.selected=s.playerMove===moveId;
    }
  }else if(w.scene2){
    const s=w.scene2;contextId=`scene2.${s.phase}`;
    observe.push(option('listen','旁听附近谈话',{selected:selected('listen')}));
    move.push(option('follow','跟随所选人物',{targetRequired:true,selected:!!s.following}),option('move_main','回到主桌',{selected:user.destination==='main_table'}));
    interact.push(option('light_game','参加轻游戏',{selected:s.gameAskedAt>=0&&w.elapsed-s.gameAskedAt<40}));
    if(s.phase==='cross_intro'){nextTitle='听完简短介绍';nextHint='介绍结束后会进入自由交流，不需要猜对任何台词。';nextGroup='observe';}
    else if(['freeflow','montage'].includes(s.phase)){nextTitle='自由交流，等待夜深';nextHint='可以交流、旁听或只观察；夜深后调酒师会自动收桌。';nextGroup='interact';}
    else if(s.phase==='gathering'&&s.deckAt<0){nextTitle='回到主桌';nextHint='调酒师正在收桌，塔罗牌马上会留下。';nextGroup='move';nextActionId='move_main';}
    else{nextTitle='塔罗牌已经到桌上';nextHint='下一章会自动开始，不需要继续尝试其他按钮。';nextGroup='observe';}
  }else if(w.scene1){
    const s=w.scene1;contextId=`scene1.${s.phase}`;
    observe.push(option('observe_third','观察第三杯',{selected:selected('observe','third_drink'),enabled:!!s.drinkEventId,disabledReason:s.drinkEventId?'':'第三杯尚未落桌'}),option('observe_seat','观察空椅',{selected:selected('observe','reserved_seat')}));
    move.push(option('move_main','前往主桌',{selected:user.destination==='main_table'}));
    interact.push(option('sit_reserved','坐空椅',{selected:s.seated}));
    if(!selected('observe')){nextTitle='先观察酒吧';nextHint='看看在场人物和主桌，第一步不需要说话。';nextGroup='observe';nextActionId='observe_room';}
    else if(!s.drinkEventId){nextTitle='留意调酒师';nextHint='第三杯会在接近主桌后，或入场约 45 秒后自然落桌。';nextGroup='observe';}
    else if(s.lightInteractions<1){nextTitle='完成一次轻互动';nextHint='观察、靠近、交流或坐下任意一种即可。';nextGroup='interact';}
    else if(s.phase==='d_arrival'){nextTitle='来客正在入场';nextHint='你已经完成本段选择，剧情正在自然衔接。';nextGroup='observe';}
    else{nextTitle='等待最后一位来客';nextHint='不需要继续重复操作；到场后会自动进入动态社交。';nextGroup='observe';}
  }else return null;

  return {contextId,nextTitle,nextHint,nextGroup,nextActionId,primaryActionId:nextActionId,primaryLabel:nextTitle,primaryTargetRequired:false,suggestions:[],groups:[
    {id:'observe',label:'观察',options:observe},{id:'move',label:'移动',options:move},{id:'interact',label:'互动',options:interact}
  ]};
}

function firstNightInteraction(g:Engine):InteractionView{
  const s=g.world.firstNight!,phase=s.phase,user=g.actor('USER');
  let nextTitle='今晚先按自己的节奏来',nextHint='看向人物或物件时，只显示一个当前动作。按 Enter 可自由输入。';
  let primaryActionId='',primaryLabel='',primaryTargetRequired=false;
  let suggestions:InteractionOption[]=[];
  const pages=Math.ceil(DRINKS.length/2);
  const drinkSuggestions=()=>{
    const page=Math.max(0,s.drinkMenuPage)%pages,menu=DRINKS.slice(page*2,page*2+2);
    nextTitle=`酒单 · ${page+1}/${pages}`;
    nextHint=menu.map(d=>`${d.name}：${d.recipe}／${d.flavor}`).join('；')+`。剩余 ${s.cash.USER} Cash。点单与真正喝下分开记录。`;
    return [...menu.map(d=>option('drink_'+d.id,drinkLabel(d,(s.vouchers.USER??0)>0?'特调券':`${d.price} Cash`),{enabled:(s.vouchers.USER??0)>0||s.cash.USER>=d.price,disabledReason:(s.vouchers.USER??0)>0||s.cash.USER>=d.price?'':`Cash 不足 · ${d.recipe}`})),option('drink_next','下一页')];
  };
  if(s.inviteFrom&&!s.invited&&phase==='meteor_window'){
    nextTitle=`${g.actor(s.inviteFrom).name} 邀请你一起上露台`;nextHint='浪漫同行、朋友同行或拒绝，都是有效答复。';
    suggestions=[option('npc_romance','答应浪漫同行'),option('npc_friend','以朋友身份一起去'),option('npc_decline','这次不去')];
  }else if(s.postGameStep==='songs'){
    nextTitle='选一首今晚的歌';nextHint='吧台会真的换上这首歌。';
    suggestions=SONGS.map(x=>option('song_'+x.id,x.label));
  }else if(s.postGameStep==='activities'){
    nextTitle='选一项轻松活动';nextHint='选择会立刻发生在酒吧里。';
    suggestions=ACTIVITIES.map(x=>option('activity_'+x.id,x.label));
  }else if(s.postGameStep==='question'){
    nextTitle='回答一个轻松问题';nextHint=s.question||'用文字回答即可。';
    primaryActionId='talk';primaryLabel='用文字回答';primaryTargetRequired=true;
    suggestions=[option('talk','自由回答',{targetRequired:true,replaceable:true}),option('open_drinks','去吧台'),option('observe_room','看看周围')];
  }else if(phase==='arrival'){
    nextTitle='一颗球滚到了你附近';nextHint='这是可以错过的小互动。你也可以先去找人说话，球会在视野边缘自行收走。';
    primaryActionId='ball_return';primaryLabel='捡起球';
    suggestions=[option('ball_return','递回万塞'),option('ball_try','问能否试投'),option('talk','去和眼前的人说话',{targetRequired:true,replaceable:true})];
  }else if(phase==='free_time'){
    nextTitle='自由认识这里的人';nextHint=`你有 ${s.cash.USER} Cash。可以聊天、看夜景或去吧台；工作人员稍后会组织活动。`;
    if(s.drinkMenuPage>=0)suggestions=drinkSuggestions();
    else{
      primaryActionId='talk';primaryLabel='和眼前的人交谈';primaryTargetRequired=true;
      const held=[...s.drinks].reverse().find(d=>d.owner==='USER'&&d.status==='served');
      if(s.openingBall==='rolling')suggestions=[option('ball_return','递回万塞'),option('talk','自由交谈',{targetRequired:true,replaceable:true}),option('open_drinks','查看酒单',{replaceable:true})];
      else if(held)suggestions=[option('consume_'+held.id,'喝下手边的饮品'),option('talk','自由交谈',{targetRequired:true,replaceable:true}),option('open_drinks','查看酒单',{replaceable:true})];
      else if(distance(user,g.actor('A'))<2.4&&!s.seatedNearKiko)suggestions=[option('sit_view','在观景位坐下'),option('talk','自由交谈',{targetRequired:true,replaceable:true}),option('open_drinks','查看酒单',{replaceable:true})];
      else if(!s.feigningDrunk.USER&&s.drinkStage.USER==='sober')suggestions=[option('talk','自由交谈',{targetRequired:true,replaceable:true}),option('open_drinks','查看酒单',{replaceable:true}),option('feign_drunk','装作有些醉')];
      else suggestions=[option('talk','自由交谈',{targetRequired:true,replaceable:true}),option('open_drinks','查看酒单',{replaceable:true}),option('observe_room','看看周围')];
    }
  }else if(['game_call','game_choice'].includes(phase)){
    nextTitle='工作人员正在组织弹球入杯';nextHint='规则和彩头已经说明，选择不会暗中判定输赢。';
    suggestions=[option('bounce_join','加入'),option('bounce_watch','先看一局'),option('bounce_decline','不参加')];
  }else if(phase==='game_round'){
    const turn=s.participants[s.turn%s.participants.length];
    nextTitle=turn==='USER'?'轮到你投球':'看清每个人真实的投球';nextHint=turn==='USER'?'调整方向和力度后出手。球必须先落桌再进杯。':`${g.actor(turn).name} 正在准备。`;
    if(s.pendingThrow){nextTitle=`${g.actor(s.pendingThrow.actor).name} 的球正在运动`;nextHint='结果只由场内物理碰撞决定。';suggestions=[];}
    else if(turn==='USER'&&s.pendingAim<0){primaryLabel='先选择落点';suggestions=[option('aim_left','偏左'),option('aim_center','正中'),option('aim_right','偏右')];}
    else if(turn==='USER'){primaryLabel='再选择力度';suggestions=[option('power_light','轻投'),option('power_medium','适中'),option('power_heavy','重投')];}
    else suggestions=[];
  }else if(phase==='post_game'){
    nextTitle=s.winner==='USER'?'你先命中了目标杯':'比赛已经有了结果';nextHint='输赢只打开新的相处机会，不直接决定谁喜欢谁。';
    if(s.drinkMenuPage>=0)suggestions=drinkSuggestions();
    else if(s.winner==='USER'&&!s.postGameChoice)suggestions=[option('reward_voucher','拿一张特调券'),option('reward_song','选择下一首歌'),option('reward_activity','选择下一项活动')];
    else if(s.lastPlace==='USER'&&!s.postGameChoice&&s.postGameMenu)suggestions=[option('penalty_mocktail','调一杯无酒精特饮'),option('penalty_question','回答一个轻松问题'),option('penalty_song','帮大家选歌')];
    else if(s.lastPlace==='USER'&&!s.postGameChoice)suggestions=[option('penalty_tasks','接受一项轻任务'),option('penalty_pay','支付 2 Cash',{enabled:s.cash.USER>=2,disabledReason:s.cash.USER>=2?'':'Cash 不足'})];
    else{primaryActionId='talk';primaryLabel='和眼前的人聊聊赛后';primaryTargetRequired=true;suggestions=[option('talk','自由交谈',{targetRequired:true,replaceable:true}),option('open_drinks','去吧台'),option('observe_room','看看大家的反应')];}
  }else if(phase==='meteor_window'){
    if(s.rooftopChosen){nextTitle='沿灯带找到楼梯';nextHint='亲自走上楼梯。抵达上层露台后，流星雨阶段才会开始。';suggestions=[option('end_first_night','留在楼下结束')];}
    else{nextTitle='流星雨快到了';nextHint=s.invited?'邀请已经发出，也可以独自上楼。':'邀请一位愿意同行的人，或者自己上露台。';
      primaryActionId=s.invited?'go_rooftop':'invite_rooftop';primaryLabel=s.invited?'沿楼梯上露台':'邀请眼前的人';primaryTargetRequired=!s.invited;
      suggestions=s.invited?[option('go_rooftop','上露台'),option('end_first_night','留在这里结束')]:[option('invite_rooftop','邀请同行',{targetRequired:true}),option('go_rooftop','独自上楼'),option('end_first_night','提前结束')];}
  }else if(phase==='rooftop'){
    nextTitle=s.roofCompanions.length?'一起看完这场流星雨':'一个人也能看完整个夜晚';nextHint='继续说话或安静待着，准备好后再结束。';
    primaryActionId='end_first_night';primaryLabel='结束这一晚';
    suggestions=[option('talk','轻声交谈',{targetRequired:true,replaceable:true}),option('end_first_night','结束这一晚')];
  }else {
    nextTitle='这一晚已经结算';nextHint=`${s.ending||'首夜结束'} · 剩余 ${s.cash.USER} Cash · ${s.winner?`${g.actor(s.winner).name} 先命中`:'本局无人先命中'}。攀岩馆与温泉尚未开放。`;
    suggestions=[option('station_later','留待下次'),option('station_climbing','室内攀岩馆（未开放）',{enabled:false,disabledReason:'尚未开放'}),option('station_hotspring','温泉（未开放）',{enabled:false,disabledReason:'尚未开放'})];
  }
  return {contextId:`first-night.${phase}`,nextTitle,nextHint,nextGroup:'interact',nextActionId:primaryActionId,primaryActionId,primaryLabel,primaryTargetRequired,suggestions,groups:[{id:'observe',label:'观察',options:[]},{id:'move',label:'移动',options:[]},{id:'interact',label:'互动',options:suggestions}]};
}
