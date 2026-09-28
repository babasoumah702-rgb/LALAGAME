import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {Engine} from '../engine.js';
import {loadScenario} from '../config.js';
import {Navigator} from '../navigation.js';
import {interactionView} from '../interaction.js';
import {DRINKS,FIRST_NIGHT_CONTENT_VERSION} from '../first-night-content.js';
import {queuePeerCue} from '../first-night.js';

const scenario=loadScenario('scenarios/last_call.json');
const nav=new Navigator(JSON.parse(readFileSync('scenarios/navigation.json','utf8')));
function game(){
  const g=new Engine(scenario,{playerId:'first-night-v2-test',opening:'first_night_v2',story:'first_night_v2',online:false},undefined,nav);
  g.world.intro!.phase='bar';g.world.intro!.progress=7;
  return g;
}
function tick(g:Engine,seconds:number){for(let i=0;i<seconds*2;i++)g.advance(.5);}

test('new handoff is the authoritative first-night content and all four characters are present',()=>{
  const g=game(),s=g.world.firstNight!;
  assert.equal(s.contentVersion,FIRST_NIGHT_CONTENT_VERSION);
  assert.deepEqual(['A','B','C','D'].map(id=>[id,g.actor(id).name,g.actor(id).active]),[['A','Kiko',true],['B','X',true],['C','万塞',true],['D','一桐',true]]);
  assert.equal(g.world.scene1,undefined);assert.equal(g.world.scene2,undefined);assert.equal(g.world.scene3,undefined);
  assert.equal(g.world.intro!.phoneVisible,false);assert.equal(g.world.events.length,0);
  assert.ok(['A','B','C','D'].every(id=>g.actor(id).knownFacts.length===0));
  assert.ok(['A','B','C','D'].every(id=>g.actor(id).relations.USER.trust===.5));
  Object.assign(g.actor('USER'),{x:g.actor('A').x-.5,z:g.actor('A').z});
  const context=g.context('A',g.emit('speech','USER','A','chat','今晚想先看看夜景').id);
  assert.match(context.identity.fixedCard,/顶楼酒吧/);assert.doesNotMatch(context.identity.fixedCard,/共同项目必须处理/);
  assert.doesNotMatch(JSON.stringify(context),/ECHO|塔罗|旧恋情|旧版/);
});

test('minimal interaction exposes one contextual action, at most three suggestions, and free input remains a separate client affordance',()=>{
  const g=game();let view=interactionView(g)!;
  assert.equal(view.contextId,'first-night.arrival');assert.equal(view.primaryActionId,'ball_return');assert.ok(view.suggestions.length<=3);
  g.command({id:'ball',type:'opening_ball',intent:'return'});g.advance(.1);view=interactionView(g)!;
  assert.equal(g.world.firstNight!.phase,'free_time');assert.equal(view.primaryActionId,'talk');assert.ok(view.suggestions.length<=3);
});

test('all A-D can initiate a directed A2A exchange and only the named partner may answer',()=>{
  const g=game();
  for(const id of ['USER','A','B','C','D'])Object.assign(g.actor(id),{x:0,z:0,area:'bar',route:[]});
  for(const [from,target] of [['A','B'],['B','C'],['C','D'],['D','A']]){
    g.world.jobs=[];g.world.replies=[];
    const cue=queuePeerCue(g,from,target,'peer_table',`${g.actor(from).name} 想和 ${g.actor(target).name} 说一句眼前的事。`);
    assert.deepEqual(g.world.jobs.filter(j=>j.eventId===cue.id).map(j=>j.actor),[from]);
    const scene:any=g.context(from,cue.id).scene;
    assert.deepEqual(scene.peerExchange,{role:'initiate',partner:target,topic:cue.text});
    assert.ok(scene.availableActions.includes('speak'));
    assert.equal(g.apply(from,g.rule(from,cue.id),cue.id),true);
    const line=g.world.events.at(-1)!;
    assert.equal(line.type,'speech');assert.equal(line.actor,from);assert.equal(line.target,target);
    assert.deepEqual(g.world.jobs.filter(j=>j.eventId===line.id).map(j=>j.actor),[target]);
    for(const bystander of ['A','B','C','D'].filter(id=>id!==from&&id!==target))
      assert.ok(line.perceptions.some(p=>p.actor===bystander),'bystanders may perceive without joining');
  }
});

test('Cash changes on purchase while intoxication changes only after actual consumption',()=>{
  const g=game();g.command({id:'ball',type:'opening_ball',intent:'ignore'});g.advance(.1);
  const drink=DRINKS.find(x=>x.id==='terrace_breeze')!;
  g.command({id:'order',type:'order_drink',objectTarget:drink.id});
  assert.equal(g.world.firstNight!.cash.USER,18-drink.price);assert.equal(g.world.firstNight!.intoxication.USER,0);
  g.command({id:'drink',type:'consume_drink',objectTarget:drink.id});
  assert.equal(g.world.firstNight!.intoxication.USER,drink.intoxication);assert.equal(g.world.firstNight!.drinkStage.USER,'sober');
});

test('a gifted drink charges Cash only after the addressed character accepts it',()=>{
  const g=game();g.command({id:'ball',type:'opening_ball',intent:'ignore'});g.advance(.1);
  Object.assign(g.actor('USER'),{x:g.actor('A').x-.5,z:g.actor('A').z});
  g.command({id:'offer',type:'order_drink',target:'A',objectTarget:'terrace_breeze'});
  const offerEvent=g.world.events.at(-1)!;
  assert.equal(g.world.firstNight!.cash.USER,18);assert.equal(offerEvent.intent,'drink_offer');
  const decision=g.rule('A',offerEvent.id);assert.equal(decision.intent,'accept_drink');
  assert.equal(g.apply('A',decision,offerEvent.id),true);
  assert.equal(g.world.firstNight!.cash.USER,12);
  assert.equal(g.world.firstNight!.drinks.at(-1)?.status,'accepted');
});

test('directed free text queues only the addressed character',()=>{
  const g=game();Object.assign(g.actor('USER'),{x:g.actor('A').x-.5,z:g.actor('A').z});g.world.jobs=[];
  g.command({id:'talk',type:'talk',target:'A',text:'今晚先不谈工作，可以聊聊这里吗？'});
  assert.deepEqual(g.world.jobs.map(j=>j.actor),['A']);
  const context=g.context('A',g.world.events.at(-1)!.id);
  assert.match(context.identity.fixedCard,/眼前的酒吧活动/);assert.match(context.identity.fixedCard,/不要把普通聊天转向项目/);
});

test('formal game result waits for the Unreal physics report and is then recorded as a world fact',()=>{
  const g=game();g.command({id:'ball',type:'opening_ball',intent:'try'});tick(g,80);
  assert.equal(g.world.firstNight!.phase,'game_choice');
  g.command({id:'join',type:'bounce_choice',intent:'join'});assert.equal(g.world.firstNight!.participants.length,5);
  g.command({id:'aim',type:'set_throw_aim',x:.67});g.command({id:'throw',type:'throw_ball',z:.58});
  assert.equal(g.world.firstNight!.throws.length,0);assert.equal(g.world.firstNight!.pendingThrow?.actor,'USER');
  const requestId=g.world.firstNight!.pendingThrow!.id;
  assert.throws(()=>g.command({id:'stale',type:'bounce_result',requestId:'other',open:true,x:1}),/过期/);
  g.command({id:'result',type:'bounce_result',requestId,open:true,x:1});
  assert.equal(g.world.firstNight!.throws[0].actor,'USER');assert.equal(g.world.firstNight!.throws[0].hit,true);
  assert.equal(g.world.events.some(e=>e.actor==='USER'&&e.intent==='bounce_hit'),true);
});

test('all five participants physically throw before anyone can be assigned last place',()=>{
  const g=game(),s=g.world.firstNight!;
  g.command({id:'ball-five',type:'opening_ball',intent:'try'});tick(g,80);
  g.command({id:'join-five',type:'bounce_choice',intent:'join'});
  let guard=0;
  while(s.phase==='game_round'&&guard++<80){
    if(s.pendingThrow){
      const pending=s.pendingThrow;
      // The first four land. D must still receive and resolve a real throw.
      const hit=pending.actor!=='D';
      g.command({id:`five-result-${guard}`,type:'bounce_result',requestId:pending.id,open:hit,x:hit?1:0});
    }else{
      const actor=s.participants[s.turn%s.participants.length];
      if(actor==='USER'){
        g.command({id:`five-aim-${guard}`,type:'set_throw_aim',x:.67});
        g.command({id:`five-throw-${guard}`,type:'throw_ball',z:.58});
      }else g.advance(.5);
    }
  }
  assert.equal(s.phase,'post_game');
  assert.equal(s.lastPlace,'D');
  assert.deepEqual(new Set(s.throws.map(item=>item.actor)),new Set(['USER','A','B','C','D']));
  assert.equal(s.throws.length,5);
});

test('winner reward is an actual single-use voucher rather than automatic affection',()=>{
  const g=game(),s=g.world.firstNight!;s.phase='post_game';s.winner='USER';s.gameEndedAt=g.world.elapsed;
  g.command({id:'reward',type:'post_game_choice',intent:'reward_voucher'});
  assert.equal(s.vouchers.USER,1);const cash=s.cash.USER;
  g.command({id:'menu',type:'drink_menu',intent:'open'});
  g.command({id:'voucher-drink',type:'order_drink',objectTarget:'old_city'});
  assert.equal(s.cash.USER,cash);assert.equal(s.vouchers.USER,0);
  assert.equal(g.actor('A').relations.USER.attraction,.2);
});

test('choosing the rooftop opens a physical route and does not teleport the story phase',()=>{
  const g=game(),s=g.world.firstNight!;s.phase='meteor_window';
  g.command({id:'roof-route',type:'go_rooftop'});
  assert.equal(s.rooftopChosen,true);assert.equal(s.phase,'meteor_window');
  g.actor('USER').area='rooftop';g.advance(.1);
  assert.equal(s.phase,'rooftop');assert.equal(g.world.events.at(-1)?.intent,'rooftop_arrival');
});

test('an accepted physical roof position completes arrival immediately',()=>{
  const g=game(),s=g.world.firstNight!,u=g.actor('USER');s.phase='meteor_window';s.rooftopChosen=true;
  Object.assign(u,{x:7.45,z:2.48,y:4.19,area:'stairs'});
  g.busy=true;
  g.command({id:'roof-position',type:'position',actor:'USER',x:7.45,z:2.55,y:4.2,area:'rooftop',yaw:90});
  assert.equal(u.area,'rooftop');
  assert.equal(s.phase,'rooftop');
  assert.equal(g.world.events.at(-1)?.intent,'rooftop_arrival');
});

test('live state exposes first-night cash, pending throws and settlement fields',()=>{
  const g=game();
  const live=g.view().firstNight!;
  assert.equal(live.availableCash,18);
  assert.equal(live.playerDrinkStage,'sober');
  assert.equal(live.pendingThrow.id,'');
  assert.equal(live.attitudes.length,4);
  assert.equal(live.evaluations.length,0);
});

test('declining the formal game returns to free time instead of assigning a hidden loss',()=>{
  const g=game();g.command({id:'ball',type:'opening_ball',intent:'ignore'});tick(g,80);
  assert.equal(g.world.firstNight!.phase,'game_choice');
  g.command({id:'skip',type:'bounce_choice',intent:'decline'});
  assert.equal(g.world.firstNight!.phase,'free_time');
  assert.equal(g.world.firstNight!.participants.length,0);
  assert.equal(g.world.firstNight!.winner,'');
});

test('an accepted gift is drunk later and only then changes that character\'s intoxication',()=>{
  const g=game();g.command({id:'ball',type:'opening_ball',intent:'ignore'});g.advance(.1);
  Object.assign(g.actor('USER'),{x:g.actor('A').x-.5,z:g.actor('A').z});
  g.command({id:'offer',type:'order_drink',target:'A',objectTarget:'terrace_breeze'});
  const decision=g.rule('A',g.world.events.at(-1)!.id);g.apply('A',decision,g.world.events.at(-1)!.id);
  assert.equal(g.world.firstNight!.intoxication.A,0);
  tick(g,9);
  assert.equal(g.world.firstNight!.drinks.at(-1)?.status,'consumed');
  assert.equal(g.world.firstNight!.intoxication.A,1);
});

test('rooftop invitation requires a witnessed conversation and does not flip after a refusal',()=>{
  const g=game(),s=g.world.firstNight!;s.phase='meteor_window';
  g.command({id:'cold',type:'invite_rooftop',target:'A'});
  assert.equal(s.invitationResult,'declined');
  assert.throws(()=>g.command({id:'again',type:'invite_rooftop',target:'A'}),/已经发出/);
  const h=game(),t=h.world.firstNight!;
  Object.assign(h.actor('USER'),{x:h.actor('B').x-.5,z:h.actor('B').z});
  h.command({id:'talk',type:'talk',target:'B',text:'今晚想先看看夜景，你呢？'});
  t.phase='meteor_window';
  h.command({id:'ask',type:'invite_rooftop',target:'B'});
  assert.equal(t.invitationResult,'friend');
});

test('settlement lists witnessed evaluations and one of the four ending types',()=>{
  const g=game(),s=g.world.firstNight!;
  Object.assign(g.actor('USER'),{x:g.actor('C').x-.5,z:g.actor('C').z});
  g.command({id:'talk',type:'talk',target:'C',text:'刚才那球你怎么控制力度的？'});
  s.phase='rooftop';s.invitationResult='';s.roofCompanions=[];
  g.command({id:'end',type:'end_first_night'});
  assert.equal(s.ending,'独自上楼');
  const view=g.view().firstNight!;
  assert.match(view.settlementSummary,/独自上楼/);
  assert.equal(view.evaluations.length,4);
  assert.match(view.evaluations.find(item=>item.id==='C')!.text,/玩过|说过|球/);
  assert.match(view.evaluations.find(item=>item.id==='A')!.text,/没怎么聊|看见你在场/);
});

test('four characters walk toward the throw line when the formal game starts',()=>{
  const g=game();g.command({id:'ball',type:'opening_ball',intent:'ignore'});tick(g,80);
  g.command({id:'join',type:'bounce_choice',intent:'join'});
  tick(g,6);
  for(const id of ['A','B','C','D'])assert.ok(g.actor(id).x<-1.5,id+' should be moving or standing at the throw line');
});

test('feigning drunk does not change intoxication and can be seen by Wan Sai',()=>{
  const g=game(),s=g.world.firstNight!;
  Object.assign(g.actor('USER'),{x:g.actor('C').x-.5,z:g.actor('C').z});
  const points=s.intoxication.USER;
  g.command({id:'feign',type:'feign_drunk'});
  assert.equal(s.feigningDrunk.USER,true);
  assert.equal(s.intoxication.USER,points);
  assert.equal(g.view().firstNight!.feigningDrunk,true);
  assert.ok(g.world.events.some(e=>e.intent==='feign_drunk'&&e.target==='C'));
});

test('drink preferences follow the handoff menu: Kiko refuses too-sweet, Yitong refuses old city, Wan Sai accepts mint lime',()=>{
  const g=game();g.command({id:'ball',type:'opening_ball',intent:'ignore'});
  Object.assign(g.actor('USER'),{x:g.actor('A').x-.5,z:g.actor('A').z});
  g.command({id:'sweet',type:'order_drink',target:'A',objectTarget:'berry_ginger_0'});
  const kiko=g.rule('A',g.world.events.at(-1)!.id);
  assert.equal(kiko.intent,'refuse_drink');assert.match(kiko.expression,/乌龙柚子/);
  Object.assign(g.actor('USER'),{x:g.actor('D').x-.5,z:g.actor('D').z});
  g.command({id:'bitter',type:'order_drink',target:'D',objectTarget:'old_city'});
  const yitong=g.rule('D',g.world.events.at(-1)!.id);
  assert.equal(yitong.intent,'refuse_drink');assert.match(yitong.expression,/苦橙旧城/);
  Object.assign(g.actor('USER'),{x:g.actor('C').x-.5,z:g.actor('C').z});
  g.command({id:'mint',type:'order_drink',target:'C',objectTarget:'mint_lime'});
  assert.equal(g.rule('C',g.world.events.at(-1)!.id).intent,'accept_drink');
});

test('throw order rotates after a full round and cup assist opens on later rounds',()=>{
  const g=game(),s=g.world.firstNight!;
  s.phase='game_round';s.gameChoice='join';s.participants=['USER','A','B','C','D'];s.round=1;s.turn=0;
  const order=s.participants.slice();
  g.command({id:'aim',type:'set_throw_aim',x:.67});g.command({id:'throw',type:'throw_ball',z:.58});
  g.command({id:'result',type:'bounce_result',requestId:s.pendingThrow!.id,open:true,x:1});
  assert.equal(s.participants[s.turn],'A');
  s.turn=0;s.round=3;s.participants=order.slice();s.winner='';s.lastPlace='';s.landed=[];
  g.command({id:'aim2',type:'set_throw_aim',x:.67});g.command({id:'throw2',type:'throw_ball',z:.58});
  g.command({id:'result2',type:'bounce_result',requestId:s.pendingThrow!.id,open:false,x:0});
  for(let i=0;i<4;i++){
    if(!s.pendingThrow){
      const actor=s.participants[s.turn];
      s.pendingThrow={id:`force:${i}`,actor,round:s.round,aim:.5,power:.5,startedAt:g.world.elapsed};
    }
    g.command({id:`r${i}`,type:'bounce_result',requestId:s.pendingThrow.id,open:false,x:0});
  }
  assert.equal(s.cupAssist,true);
  assert.equal(g.view().firstNight!.cupAssist,true);
  assert.notEqual(s.participants[0],'USER');
});

test('game call waits for an open player conversation before interrupting',()=>{
  const g=game();
  Object.assign(g.actor('USER'),{x:g.actor('A').x-.5,z:g.actor('A').z});
  g.command({id:'talk',type:'talk',target:'A',text:'今晚先不谈工作，可以聊聊这里吗？'});
  tick(g,76);
  assert.equal(g.world.firstNight!.phase,'free_time');
  g.world.jobs=[];
  if(g.world.replies)g.world.replies.length=0;
  tick(g,40);
  assert.ok(['game_call','game_choice'].includes(g.world.firstNight!.phase));
});

test('a character who talked with the player can invite them to the rooftop',()=>{
  const g=game(),s=g.world.firstNight!;
  Object.assign(g.actor('USER'),{x:g.actor('B').x-.5,z:g.actor('B').z});
  g.command({id:'talk',type:'talk',target:'B',text:'今晚想先看看夜景，你呢？'});
  s.phase='meteor_window';s.meteorAt=g.world.elapsed;s.stageAt=g.world.elapsed;
  g.world.jobs=[];tick(g,13);
  assert.equal(s.inviteFrom,'B');
  g.command({id:'yes',type:'npc_invite_reply',intent:'accept_friend'});
  assert.equal(s.invitationResult,'friend');assert.deepEqual(s.roofCompanions,['B']);
});

test('private speech does not leak into a far character evaluation',()=>{
  const g=game(),s=g.world.firstNight!;
  Object.assign(g.actor('USER'),{x:g.actor('B').x-.5,z:g.actor('B').z});
  g.command({id:'private',type:'talk',target:'B',text:'我想借你的人脉和资源。'});
  Object.assign(g.actor('D'),{x:8,z:8});
  s.phase='rooftop';s.invitationResult='';s.roofCompanions=[];
  g.command({id:'end',type:'end_first_night'});
  const text=g.view().firstNight!.evaluations.find(item=>item.id==='D')!.text;
  assert.doesNotMatch(text,/人脉|资源/);
  assert.match(text,/没交流|没怎么聊/);
});

test('reward song actually changes the bar music and locked stations stay closed',()=>{
  const g=game(),s=g.world.firstNight!;s.phase='post_game';s.winner='USER';s.gameEndedAt=g.world.elapsed;
  g.command({id:'song',type:'post_game_choice',intent:'reward_song'});
  g.command({id:'pick',type:'pick_song',objectTarget:'night_view'});
  assert.equal(s.song,'night_view');
  assert.ok(g.world.events.some(e=>e.intent==='play_song'));
  s.phase='settled';
  assert.throws(()=>g.command({id:'climb',type:'choose_next',intent:'climbing'}),/尚未开放/);
  g.command({id:'later',type:'choose_next',intent:'later'});
  assert.equal(s.nextStation,'later');
});

test('first night starts with the elevator already open so the player can walk into the bar',()=>{
  const g=new Engine(scenario,{playerId:'first-night-open-doors',opening:'first_night_v2',story:'first_night_v2',online:false},undefined,nav);
  assert.equal(g.world.intro!.phase,'bar');
  assert.equal(g.world.intro!.ready,true);
  const view=g.view();
  assert.equal(view.intro!.phase,'bar');
  assert.equal(view.intro!.ready,true);
  g.command({id:'ball',type:'opening_ball',intent:'ignore'});
  assert.equal(g.world.firstNight!.openingBall,'ignored');
});

test('a first-night save stuck in the elevator is released on load',()=>{
  const g=game();
  g.world.intro!.phase='elevator';
  g.world.intro!.progress=0;
  g.world.intro!.ready=false;
  const restored=new Engine(scenario,{playerId:'first-night-v2-test'},g.world,nav);
  assert.equal(restored.world.intro!.phase,'bar');
  restored.world.paused=false;
  restored.command({id:'ball',type:'opening_ball',intent:'return'});
  assert.equal(restored.world.firstNight!.openingBall,'returned');
});

test('intoxication recovers over time and served drinks stay unconsumed until sipped',()=>{
  const g=game(),s=g.world.firstNight!;
  s.intoxication.USER=3;s.drinkStage.USER='light';s.lastRecoveryAt=0;
  tick(g,91);
  assert.ok(s.intoxication.USER<3);
  g.command({id:'order',type:'order_drink',objectTarget:'terrace_breeze'});
  assert.equal(s.drinks.filter(d=>d.owner==='USER').at(-1)?.status,'served');
  assert.equal(s.drinks.filter(d=>d.owner==='USER'&&d.status==='consumed').length,0);
});

test('the complete first-night vertical slice reaches a witnessed shared-rooftop settlement',()=>{
  const g=game(),s=g.world.firstNight!;
  g.command({id:'slice-ball',type:'opening_ball',intent:'return'});
  Object.assign(g.actor('USER'),{x:g.actor('B').x-.5,z:g.actor('B').z,area:g.actor('B').area});
  for(let index=0;index<4;index++){
    g.command({id:`slice-talk-${index}`,type:'talk',target:'B',text:`今晚先看看眼前的夜景，第 ${index+1} 句。`});
    // Model execution belongs to the server loop. This state-machine test
    // clears its queued transport work after verifying the directed event.
    g.world.jobs=[];if(g.world.replies)g.world.replies.length=0;
  }
  g.command({id:'slice-order',type:'order_drink',objectTarget:'terrace_breeze'});
  g.command({id:'slice-sip',type:'consume_drink',objectTarget:'terrace_breeze'});
  tick(g,82);assert.equal(s.phase,'game_choice');
  g.command({id:'slice-join',type:'bounce_choice',intent:'join'});
  let guard=0;
  while((s.phase as string)==='game_round'&&guard++<100){
    if(s.pendingThrow){
      const pending=s.pendingThrow;const hit=pending.actor!=='D';
      g.command({id:`slice-result-${guard}`,type:'bounce_result',requestId:pending.id,open:hit,x:hit?1:0});
    }else if(s.participants[s.turn%s.participants.length]==='USER'){
      g.command({id:`slice-aim-${guard}`,type:'set_throw_aim',x:.67});
      g.command({id:`slice-throw-${guard}`,type:'throw_ball',z:.58});
    }else g.advance(.5);
  }
  assert.equal(s.throws.length,5);assert.equal(s.phase,'post_game');
  g.command({id:'slice-reward',type:'post_game_choice',intent:'reward_voucher'});
  tick(g,50);assert.equal(s.phase,'meteor_window');
  g.command({id:'slice-invite',type:'invite_rooftop',target:'B'});
  assert.equal(s.invitationResult,'friend');
  g.command({id:'slice-route',type:'go_rooftop'});
  Object.assign(g.actor('USER'),{area:'rooftop',x:1.3,y:4.2,z:5.75});g.advance(.5);
  assert.equal(s.phase,'rooftop');
  g.command({id:'slice-end',type:'end_first_night'});
  const view=g.view().firstNight!;
  assert.equal(s.phase,'settled');assert.equal(g.world.status,'ended');
  assert.equal(s.ending,'朋友同行');assert.equal(view.evaluations.length,4);
  assert.match(view.settlementSummary,/共同看流星雨/);
  assert.ok(view.keyActions.some(item=>item.includes('真正喝下')));
  assert.ok(view.keyActions.some(item=>item.includes('五人弹球')));
});
