import type {Engine} from './engine.js';

export type InteractionOption={
  id:string;label:string;selected:boolean;replaceable:boolean;targetRequired:boolean;
  enabled:boolean;disabledReason:string;
};
export type InteractionGroup={id:'observe'|'move'|'interact';label:string;options:InteractionOption[]};
export type InteractionView={
  contextId:string;nextTitle:string;nextHint:string;nextGroup:'observe'|'move'|'interact';nextActionId:string;
  groups:InteractionGroup[];
};

const option=(id:string,label:string,values:Partial<InteractionOption>={}):InteractionOption=>({
  id,label,selected:false,replaceable:false,targetRequired:false,enabled:true,disabledReason:'',...values
});

export function interactionView(g:Engine):InteractionView|null{
  const w=g.world;if(!w.story)return null;
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

  return {contextId,nextTitle,nextHint,nextGroup,nextActionId,groups:[
    {id:'observe',label:'观察',options:observe},{id:'move',label:'移动',options:move},{id:'interact',label:'互动',options:interact}
  ]};
}
