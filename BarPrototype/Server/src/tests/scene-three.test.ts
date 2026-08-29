import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {Engine} from '../engine.js';
import {loadScenario} from '../config.js';
import {Navigator,distance} from '../navigation.js';
import {applyReadyReplies} from '../reply-runtime.js';
import {DECK,JOKERS,TABLE} from '../scene-three.js';
const scenario=loadScenario('scenarios/last_call.json');
const nav=new Navigator(JSON.parse(readFileSync('scenarios/navigation.json','utf8')));
function game(online=false){const g=new Engine(scenario,{playerId:'scene-three-test',story:'scene1_v1',online},undefined,nav);g.advance(0);return g;}
// Mirrors the server loop: move routes like the client, drain due jobs through the offline rules.
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
function approach(g:Engine,id:string){
  const t=g.actor(id);
  Object.assign(g.actor('USER'),g.navigation.nearest({x:t.x-.7,z:t.z}));
  if(distance(g.actor('USER'),t)>2.6||!g.navigation.visible(g.actor('USER'),t))
    Object.assign(g.actor('USER'),g.navigation.nearest({x:t.x,z:t.z-.7}));
}
// Stop exactly at a condition instead of ticking a fixed span, so a test that means "the first round"
// does not silently land three rounds later.
function tickUntil(g:Engine,ready:()=>boolean,maxSeconds:number){
  for(let n=0;n<maxSeconds&&!ready();n++)tick(g,1);
  return ready();
}
// Play the whole line: Scene 1 completes, Scene 2 runs its montage, the deck lands, Scene 3 opens.
// Returns the moment Scene 3 exists, before any card is drawn.
function reachSceneThree(g:Engine){
  tick(g,130);g.command({id:'observe',type:'observe'});tick(g,14);
  tick(g,30);
  g.command({id:'watch-social',type:'observe'});
  assert.ok(tickUntil(g,()=>!!g.world.scene3,400),'Scene 3 should open once the deck is on the table');
  Object.assign(g.actor('USER'),g.navigation.nearest({x:TABLE.x,z:TABLE.z-1.1}));
  return g.world.scene3!;
}
// Advance to the next open question and park the player next to the table.
function reachOpenQuestion(g:Engine){
  assert.ok(tickUntil(g,()=>g.world.scene3!.askedAt>=0,90),'a question should open');
  Object.assign(g.actor('USER'),g.navigation.nearest({x:TABLE.x,z:TABLE.z-1.1}));
  return g.world.scene3!;
}

test('Scene 3 opens from the Scene 2 deck and picks a reader that is not fixed to the bartender',()=>{
  const g=game();reachSceneThree(g);tick(g,12);
  const s=g.world.scene3!;
  assert.ok(s.reader,'a reader is chosen');
  assert.ok(['A','B','C','D','BARTENDER'].includes(s.reader));
  assert.ok(g.world.events.some(e=>e.intent==='tarot_take'),'someone pulls the deck over');
  // Weighting must be able to land on a guest; the bartender is only the fallback.
  const readers=new Set<string>();
  for(let seed=1;seed<40;seed++){
    const h=game();h.world.seed=seed;h.world.rng=seed;
    reachSceneThree(h);tick(h,12);
    readers.add(h.world.scene3!.reader);
  }
  assert.ok([...readers].some(id=>id!=='BARTENDER'),'a guest can host, got '+[...readers].join(','));
});

test('the first round flips a card, asks a real question and names a first responder',()=>{
  const g=game();reachSceneThree(g);
  const s=reachOpenQuestion(g);
  assert.equal(s.round,1,'this is the opening round');
  assert.ok(s.question,'a question is on the table');
  assert.ok(DECK.some(c=>c.question===s.question),'the question comes from the deck');
  assert.ok(g.world.events.some(e=>e.intent==='tarot_flip'&&e.objectTarget==='tarot_card'));
  assert.ok(g.world.events.some(e=>e.text===s.question),'the question is spoken aloud');
  assert.ok(s.firstResponder,'someone is pulled to answer first');
  assert.notEqual(s.firstResponder,s.reader,'the reader does not answer her own draw first');
});

test('the arc runs light before high tension rather than pure random',()=>{
  const g=game();reachSceneThree(g);
  tick(g,240);
  const s=g.world.scene3!;
  assert.ok(s.history.length>=3,'several rounds played, got '+s.history.length);
  const tagsOf=(id:string)=>DECK.concat(JOKERS).find(c=>c.id===id)!.tags;
  const firstTags=tagsOf(s.history[0].cardId);
  assert.ok(firstTags.includes('light')||firstTags.includes('ambiguous'),
    'the opening round stays light, got '+firstTags.join(','));
  const ids=s.history.map(h=>h.cardId);
  assert.equal(new Set(ids).size,ids.length,'no question repeats');
});

test('the gaze before an answer is recorded and only seen by whoever was looking',()=>{
  const g=game();reachSceneThree(g);
  const s=reachOpenQuestion(g);
  assert.ok(s.gazes.length>=1,'gaze order is recorded');
  const gaze=s.gazes[0];
  assert.ok(gaze.order.length>=1);
  assert.ok(gaze.pauseMs>=300&&gaze.pauseMs<=1700,'a real pause is recorded, got '+gaze.pauseMs);
  assert.ok(gaze.gesture);
  // Turn the player away: a gaze is not backfilled for someone who was not watching.
  Object.assign(g.actor('USER'),{x:-7,z:-3.2,yaw:180});
  const before=g.world.events.filter(e=>e.intent==='gaze'&&e.perceptions.some(p=>p.actor==='USER')).length;
  tick(g,60);
  const missed=g.world.events.filter(e=>e.intent==='gaze'&&!e.perceptions.some(p=>p.actor==='USER'));
  assert.ok(missed.length>=1,'a gaze can be missed entirely');
  assert.equal(g.world.events.filter(e=>e.intent==='gaze'&&e.perceptions.some(p=>p.actor==='USER')).length,before);
});

test('the player can answer, skip, deflect, ask back, observe and joke',()=>{
  for(const move of ['skip','observe','deflect','joke','ask_back'] as const){
    const g=game();reachSceneThree(g);
    const s=reachOpenQuestion(g);
    const target=s.seatedActors.find(id=>id!==s.reader)??'B';
    // Ask Back and Joke carry words, so the player has to be within speaking range of the person
    // she is addressing; Skip, Observe and Deflect are table-level moves.
    if(move==='ask_back')approach(g,target);
    if(move==='joke')approach(g,s.reader==='BARTENDER'?target:s.reader);
    // Distinct command id per move: ids are deduplicated for the whole session, and reaching the
    // table already spent a command called "observe".
    g.command({id:'tarot-move-'+move,type:'tarot_move',intent:move,target:move==='ask_back'?target:undefined,
      text:['joke','ask_back'].includes(move)?'这题该问她吧。':undefined});
    assert.equal(g.world.scene3!.playerMove,move,move+' should register');
  }
  const g=game();reachSceneThree(g);
  const s=reachOpenQuestion(g);
  approach(g,s.reader==='BARTENDER'?(s.seatedActors[0]??'B'):s.reader);
  g.command({id:'ans',type:'tarot_answer',text:'有。但我不说是谁。'});
  assert.equal(g.world.scene3!.playerMove,'answer');
  assert.equal(g.world.events.at(-1)!.intent,'answer');
});

test('silence is an event, not a null input',()=>{
  const g=game();reachSceneThree(g);g.command({id:'sit-for-question',type:'tarot_seat'});tick(g,20);
  assert.ok(g.world.scene3!.askedAt>=0);
  tick(g,32);
  const s=g.world.scene3!;
  const silence=g.world.events.find(e=>e.intent==='silence'&&e.actor==='USER');
  assert.ok(silence,'not answering produces a world event '+JSON.stringify({stance:s.playerStance,move:s.playerMove,asked:s.askedAt,now:g.world.elapsed,errors:g.world.replies?.filter(r=>r.status==='error'),question:g.world.events.find(e=>e.id===s.questionEventId)?.perceptions}));
  assert.ok(s.silences>=1);
  // Other actors can perceive the silence and react to it in their own terms.
  assert.ok(silence!.perceptions.some(p=>!['USER','OWNER'].includes(p.actor)),'the table registers it');
});

test('a joker breaks the room open after the tension peaks',()=>{
  const g=game();const s=reachSceneThree(g);
  s.peakTension=.7;s.tension=.7;
  tick(g,240);
  const after=g.world.scene3!;
  assert.ok(after.jokerUsed,'a joker is drawn after high tension');
  assert.ok(after.history.some(h=>JOKERS.some(j=>j.id===h.cardId)),'the joker is one of the pool');
  assert.equal(after.history.filter(h=>JOKERS.some(j=>j.id===h.cardId)).length,1,'at most one joker per game');
});

test('the round ends at 3 to 5 cards without exhausting the deck',()=>{
  const g=game();reachSceneThree(g);
  tick(g,600);
  const s=g.world.scene3!;
  assert.ok(s.history.length>=3&&s.history.length<=5,'played '+s.history.length+' cards');
  assert.ok(s.history.length<DECK.length,'the deck is never flushed');
  assert.ok(['closing','scene4_ready'].includes(s.phase),'reached the exit, got '+s.phase);
});

test('someone leaves the table and the exit to Scene 4 opens with the player free to choose',()=>{
  const g=game();reachSceneThree(g);
  tick(g,620);
  const s=g.world.scene3!;
  assert.ok(s.leaver,'an agent steps out');
  assert.ok(g.world.events.some(e=>e.intent==='step_out'));
  assert.equal(s.phase,'scene4_ready');
  assert.ok(g.world.events.some(e=>e.intent==='scene4'),'the player is offered the choice, not moved');
  const user=g.actor('USER');
  assert.ok(distance(user,g.location('corridor'))>1,'the player is never dragged to the corridor');
});

test('the view exposes the card and the turn but never tension numbers or hidden targets',()=>{
  const g=game();reachSceneThree(g);tick(g,25);
  const view=g.view() as any;
  assert.ok(view.scene3);
  assert.ok(view.scene3.cardName&&view.scene3.question);
  assert.equal(view.scene3.tension,undefined,'no tension score is shown');
  assert.equal(view.scene3.peakTension,undefined);
  const text=JSON.stringify(view);
  assert.ok(!text.includes('好感'),'no affinity readout');
  assert.ok(!text.includes('firstImpressions'));
  assert.ok(!text.includes('beliefs'),'private interpretations stay server-side');
});

test('the handoff reports observable moves and no verdict about anyone',()=>{
  const g=game();reachSceneThree(g);tick(g,240);
  const h=g.handoff().scene3!;
  assert.ok(h.rounds>=2);
  assert.ok(h.questionHistory.length===h.rounds);
  assert.equal(typeof h.tension,'number');
  assert.ok(Array.isArray(h.gazeEvents));
  const text=JSON.stringify(h);
  assert.ok(!/前女友|秘密|真相|其实是/.test(text),'no relationship truth is asserted');
});

test('resume keeps the tarot round and does not reflip the current card',()=>{
  const g=game();reachSceneThree(g);tick(g,25);
  const s=g.world.scene3!;
  const card=s.cardId,round=s.round;
  const flips=g.world.events.filter(e=>e.intent==='tarot_flip').length;
  const restored=new Engine(scenario,{playerId:'scene-three-test'},g.world,nav);
  restored.command({id:'unpause',type:'pause',paused:false});
  assert.equal(restored.world.scene3!.cardId,card);
  assert.equal(restored.world.scene3!.round,round);
  assert.equal(restored.world.events.filter(e=>e.intent==='tarot_flip').length,flips);
});

test('a declined player is never forced to answer and the table continues',()=>{
  const g=game();reachSceneThree(g);tick(g,20);
  g.command({id:'decline',type:'tarot_seat',text:'decline'});
  assert.equal(g.world.scene3!.playerStance,'declined');
  assert.throws(()=>g.command({id:'ans',type:'tarot_answer',text:'我说两句。'}),/退出/);
  const before=g.world.scene3!.history.length;
  tick(g,180);
  assert.ok(g.world.scene3!.history.length>before,'the round continues without the player');
});
