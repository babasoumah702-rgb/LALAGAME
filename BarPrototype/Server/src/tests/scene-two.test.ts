import {initializeSceneTwo} from '../scene-two.js';
import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {Engine} from '../engine.js';
import {loadScenario} from '../config.js';
import {Navigator,distance} from '../navigation.js';
import {applyReadyReplies} from '../reply-runtime.js';
const scenario=loadScenario('scenarios/last_call.json');
const nav=new Navigator(JSON.parse(readFileSync('scenarios/navigation.json','utf8')));
function game(online=false){const g=new Engine(scenario,{playerId:'scene-two-test',story:'scene1_v1',online},undefined,nav);g.advance(0);return g;}
// Mirrors the server loop: walk routes like the client, then drain due jobs through the offline rules
// and apply them. Without the drain the queue backs up and nothing in the room would ever answer.
function tick(g:Engine,seconds:number){for(let n=0;n<seconds*4;n++){
  for(const a of g.world.actors){if(!a.active||!a.route.length)continue;const p=a.route[0],d=distance(a,p),step=Math.min(d,.5);
    g.command({id:'pos'+n+a.id,type:'position',actor:a.id,x:a.x+(p.x-a.x)*step/Math.max(d,.001),z:a.z+(p.z-a.z)*step/Math.max(d,.001),yaw:a.yaw});}
  g.advance(.25);
  for(const job of g.dueJobs(2)){
    g.world.replies??=[];
    const id=job.actor+':'+job.eventId;
    if(g.world.replies.some(r=>r.id===id))continue;
    g.world.replies.push({id,actor:job.actor,eventId:job.eventId,status:'ready',decision:g.rule(job.actor,job.eventId)});
  }
  applyReadyReplies(g);
}}
// Walk the player to the target instead of snapping once: in Scene 2 everyone keeps moving, so a
// stale snapshot would leave the player out of speaking range by the time the command lands.
function approach(g:Engine,id:string){
  const t=g.actor(id);
  t.route=[];t.destination='';
  Object.assign(g.actor('USER'),g.near(g.actor('USER'),t));
  if(distance(g.actor('USER'),t)>2.6||!g.navigation.visible(g.actor('USER'),t))
    Object.assign(g.actor('USER'),g.navigation.nearest({x:t.x,z:t.z-.7}));
}
function nearby(g:Engine,id='B'){approach(g,id);}
// Reach Scene 2 the way a player does: Scene 1 completes, D arrives, the chapter opens itself.
function reachSceneTwo(g:Engine){
  tick(g,130);g.command({id:'observe',type:'observe'});tick(g,14);
  assert.equal(g.world.scene1!.phase,'scene2_ready');
  assert.ok(g.world.scene2,'Scene 2 should open from the completed Scene 1');
  return g.world.scene2!;
}

test('Scene 2 opens from the finished Scene 1 without replaying D arrival',()=>{
  const g=game();const s=reachSceneTwo(g);
  assert.equal(s.phase,'cross_intro');
  assert.equal(g.actor('D').active,true);
  // The staged arrival itself must not replay. Replies that answer it inherit the intent, so the
  // check is scoped to the scripted action rather than to every event carrying that intent.
  assert.equal(g.world.events.filter(e=>e.actor==='D'&&e.intent==='arrival'&&e.generationSource==='script').length,1);
});

test('cross introduction is a model cue; unheard names never unlock',()=>{
  const g=game();g.actor('D').active=true;initializeSceneTwo(g);g.world.story!.chapter=2;Object.assign(g.actor('USER'),{x:-7,z:-3.2,y:0,area:'bar'});for(let i=0;i<4;i++)g.advance(2);
  const s=g.world.scene2!;assert.ok(s.crossIntroEventId);const e=g.world.events.find(e=>e.id===s.crossIntroEventId)!;
  assert.equal(e.type,'action');assert.equal(e.generationSource,'script');assert.equal(g.world.scene1!.knownNames.D,undefined);
  const context=g.context(e.actor,e.id) as any;assert.equal(context.scene.introduction.name,g.actor('D').name);
  assert.equal(context.scene.introduction.occupation,g.scenario.identityPacks[g.world.identityPack].actors.D.publicRole);
  nearby(g,'B');g.emit('speech','B','USER','introduce','这位是'+g.actor('D').name+'。','','normal','','ai');
  assert.ok(g.world.scene1!.knownNames.D,'only the actually heard speech unlocks the name');
});

test('freeflow moves everyone and records position as a social act',()=>{
  const g=game();reachSceneTwo(g);tick(g,30);
  assert.equal(g.world.scene2!.phase,'freeflow');
  const before=['A','B','C','D'].map(id=>({id,...g.navigation.nearest(g.actor(id)),routeVersion:g.actor(id).routeVersion??0}));
  tick(g,90);
  const moved=before.filter(p=>distance(p,g.actor(p.id))>.6||(g.actor(p.id).routeVersion??0)>p.routeVersion);
  assert.ok(moved.length>=2,'at least two actors relocate during freeflow');
});

test('follow, listen and the light game are all valid moves and reach the agents',()=>{
  const g=game();reachSceneTwo(g);tick(g,30);
  nearby(g,'D');
  g.command({id:'follow',type:'follow_target',target:'D'});
  assert.ok(g.world.scene2!.followed.includes('D'));
  g.command({id:'listen',type:'listen_in'});
  assert.equal(g.world.events.at(-1)!.intent,'listen');
  g.command({id:'game',type:'join_game'});
  const s=g.world.scene2!;
  assert.equal(s.games,1);assert.ok(s.gamePrompt);
  assert.ok(g.world.events.some(e=>e.text===s.gamePrompt),'the prompt is spoken in the room');
  assert.throws(()=>g.command({id:'game2',type:'join_game'}),/还没结束/);
});

test('A2A ripple happens with no player input and respects who could perceive it',()=>{
  const g=game();reachSceneTwo(g);tick(g,40);
  Object.assign(g.actor('USER'),{x:-7,z:-3.2,yaw:180});
  const before=g.world.events.length;
  tick(g,120);
  const turns=g.world.events.filter(e=>e.intent==='turn_to');
  assert.ok(turns.length>=1,'NPCs start exchanges on their own');
  const unseen=turns.filter(e=>!e.perceptions.some(p=>p.actor==='USER'));
  assert.ok(unseen.length>=1,'some exchanges happen out of the player view');
  assert.ok(g.world.events.length>before);
});

test('a direct player line queues only its selected target while witnesses only remember it',()=>{
  const g=game();reachSceneTwo(g);tick(g,30);
  for(const [i,id] of ['A','B','C','D'].entries())Object.assign(g.actor(id),{x:1.1+i*.35,z:-1.8,area:'bar',route:[],nextAction:g.world.elapsed+30});
  Object.assign(g.actor('USER'),{x:1.4,z:-2.5,area:'bar'});g.world.jobs=[];g.world.replies=[];
  const e=g.emit('speech','USER','B','chat','我只是在和你说这句话。','','normal','','player');
  assert.ok(e.perceptions.some(p=>p.actor==='C'),'nearby characters may still overhear the line');
  assert.deepEqual(g.world.jobs.filter(j=>j.eventId===e.id).map(j=>j.actor),['B']);
  g.world.replies.push({id:'C:'+e.id,actor:'C',eventId:e.id,status:'ready',decision:{...g.rule('C',e.id),target:'USER',action:'speak',expression:'不该出现的插话。'}});
  applyReadyReplies(g);
  assert.equal(g.world.events.some(x=>x.text==='不该出现的插话。'),false,'a restored stale bystander reply is suppressed');
  assert.equal(g.world.replies.at(-1)!.errorCode,'SUPPRESSED');
});

test('approaching a roaming Scene 2 target gives the conversation a stable pause',()=>{
  const g=game();reachSceneTwo(g);tick(g,30);const b=g.actor('B'),u=g.actor('USER');
  Object.assign(u,{x:b.x-2.5,z:b.z,area:b.area});b.route=[g.navigation.nearest({x:b.x+2,z:b.z})];b.destination='bar';
  g.command({id:'approach-stable',type:'approach_target',target:'B'});
  assert.equal(b.route.length,0);assert.equal(b.destination,'');assert.ok(b.nextAction>=g.world.elapsed+17);
  assert.ok(u.route.length>0,'the player still walks through navigation rather than teleporting');
});

test('the montage passes time through the room and never through a real hour',()=>{
  const g=game();reachSceneTwo(g);tick(g,30);
  approach(g,'B');g.command({id:'talk',type:'talk',target:'B',text:'今晚人挺多。'});
  approach(g,'C');g.command({id:'talk2',type:'talk',target:'C',text:'你也常来吗？'});
  const startClock=g.view().clock;
  tick(g,230);
  const s=g.world.scene2!;
  assert.ok(s.montageStage>=3,'montage advances through its stages');
  assert.ok(s.drinkLevel<.5,'the glass empties');
  assert.ok(s.coasters>=3,'coasters stack up');
  assert.ok(s.rainStopped,'the rain stops');
  assert.ok(s.musicLevel<.7,'music drops');
  assert.notEqual(g.view().clock,startClock);
  assert.ok(!JSON.stringify(g.view()).includes('visible'),'no relationship panel is exposed');
});

test('the table regathers and the bartender leaves the deck, which the player can look at',()=>{
  const g=game();reachSceneTwo(g);tick(g,30);
  approach(g,'B');g.command({id:'talk',type:'talk',target:'B',text:'今晚人挺多。'});
  approach(g,'C');g.command({id:'talk2',type:'talk',target:'C',text:'你也常来吗？'});
  tick(g,260);
  const s=g.world.scene2!;
  assert.ok(['gathering','tarot_ready'].includes(s.phase),'phase reached the gathering, got '+s.phase);
  assert.ok(s.deckAt>=0,'the deck is left on the table');
  assert.ok(g.world.events.some(e=>e.objectTarget==='tarot_deck'));
  assert.ok(g.world.events.some(e=>e.intent==='invite'&&e.generationSource==='script'),'the host receives an invitation cue, not a fixed online line');
  Object.assign(g.actor('USER'),g.navigation.nearest({x:1.65,z:-2.6}));
  g.command({id:'look',type:'observe_object',objectTarget:'tarot_deck'});
  assert.equal(g.world.events.at(-1)!.objectTarget,'tarot_deck');
});

test('the handoff carries impressions for Scene 3 and no verdicts',()=>{
  const g=game();reachSceneTwo(g);tick(g,30);
  nearby(g,'B');g.command({id:'talk',type:'talk',target:'B',text:'你和她们都很熟吧？'});
  tick(g,40);
  const h=g.handoff().scene2!;
  assert.ok(h.whoPlayerSpokeWith.includes('B'));
  assert.deepEqual(h.knownCharacters,['A','B','C','D'].filter(id=>g.world.scene1!.knownNames[id]));
  assert.equal(typeof h.visible.bd.familiarity,'number');
  assert.equal(typeof h.visible.ab.tension,'number');
  const text=JSON.stringify(h);
  assert.ok(!/过去|秘密|前女友|关系史/.test(text),'the handoff states no relationship history');
});

test('resume keeps the Scene 2 chapter and does not replay the introduction',()=>{
  const g=game();reachSceneTwo(g);tick(g,25);
  const introId=g.world.scene2!.crossIntroEventId;
  const restored=new Engine(scenario,{playerId:'scene-two-test'},g.world,nav);
  restored.command({id:'unpause',type:'pause',paused:false});tick(restored,20);
  assert.equal(restored.world.scene2!.crossIntroEventId,introId);
  assert.equal(restored.world.events.filter(e=>e.intent==='cross_introduce').length,1);
});

test('an old save never gains the new chapters',()=>{
  const legacy=new Engine(scenario,{playerId:'legacy'});
  assert.equal(legacy.world.scene1,undefined);
  legacy.advance(1);
  assert.equal(legacy.world.scene2,undefined);
  assert.equal(legacy.world.scene3,undefined);
});
