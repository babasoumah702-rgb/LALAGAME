import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {Engine} from '../engine.js';
import {loadScenario} from '../config.js';
import {Navigator,distance} from '../navigation.js';
import {NightNavigator,NIGHT,areaOf,heightAt} from '../night-navigation.js';
import {enterChapter,reserveBudget,settleBudget} from '../story.js';
import {initializeLateNight,MEMORIES} from '../late-night.js';
import {initializeSceneThree,openRound,TABLE} from '../scene-three.js';
import {applyReadyReplies} from '../reply-runtime.js';
import {advanceCrowd} from '../crowd-navigation.js';
import {ModelAdapter} from '../model.js';
const scenario=loadScenario('scenarios/last_call.json');
const nav=new Navigator(JSON.parse(readFileSync('scenarios/navigation.json','utf8')));
const game=()=>new Engine(scenario,{playerId:'full-night-synthetic',story:'scene1_v1',online:false},undefined,nav);
let seq=0;
function tick(g:Engine,seconds:number){for(let i=0;i<seconds*4;i++){
  for(const a of g.world.actors){if(!a.active||!a.route.length)continue;const p=a.route[0],d=distance(a,p),n=Math.min(1,.45/Math.max(d,.001));const to={x:a.x+(p.x-a.x)*n,z:a.z+(p.z-a.z)*n,area:p.area,y:(a.y??0)+((p.y??0)-(a.y??0))*n};g.command({id:'p'+seq++,type:'position',actor:a.id,...to,yaw:a.yaw});}
  g.advance(.25);
}}
function table(){const g=game();enterChapter(g,3,'seating');initializeSceneThree(g);const s=g.world.scene3!;s.reader='B';Object.assign(g.actor('B'),{x:2.4,z:-1.8,y:0,area:'bar',route:[]});Object.assign(g.actor('USER'),{x:1.4,z:-2.8,y:0,area:'bar',yaw:0});s.seatedActors=['A','B','C'];openRound(g);return g;}
test('each chapter keeps its own persistent call ledger, including Scene0 usage',()=>{const g=game();assert.ok(reserveBudget(g.world,1,1200));enterChapter(g,2,'freeflow');assert.ok(reserveBudget(g.world,1,300));assert.equal(g.world.story!.budgets[1].calls,2);assert.equal(g.world.story!.budgets[2].calls,0);for(let i=0;i<80;i++)assert.ok(reserveBudget(g.world,2,100));assert.equal(g.world.story!.budgets[2].calls,80);const h=new Engine(scenario,{playerId:'x'},g.world,nav);assert.ok(reserveBudget(h.world,3,120000));settleBudget(h.world,1,1200,400);assert.equal(h.world.story!.budgets[1].tokens,700);assert.equal(Object.keys(h.world.story!.budgets).length,6);});
test('chapter migration retains legacy usage and never adds chapters to classic saves',()=>{const g=game();g.world.calls=31;g.world.tokens=10002;delete g.world.story;const h=new Engine(scenario,{playerId:'x'},g.world,nav);assert.equal(h.world.story!.budgets[1].calls,31);const classic=new Engine(scenario,{playerId:'old'},undefined,nav);const restored=new Engine(scenario,{playerId:'old'},classic.world,nav);assert.equal(restored.world.story,undefined);});
test('late model result is charged to its origin chapter and causes no next-chapter action',async()=>{const g=game();g.world.modelMode='online';Object.assign(g.actor('USER'),{x:2,z:-2,y:0,area:'bar'});Object.assign(g.actor('B'),{x:2.6,z:-2,y:0,area:'bar'});const e=g.emit('speech','USER','B','chat','我想见你','','normal','','player');const adapter=new ModelAdapter();adapter.config={key:'synthetic-not-a-secret',model:'test',base:'http://invalid.local'};let release!:(v:Response)=>void;const old=globalThis.fetch;globalThis.fetch=()=>new Promise(resolve=>release=resolve);try{const pending=adapter.decide(g,{actor:'B',eventId:e.id,due:0});enterChapter(g,2,'freeflow');release(new Response(JSON.stringify({choices:[{message:{content:JSON.stringify({action:'speak',target:'USER',intent:'chat',expression:'那就坐一会。',interpretation:'reply',evidenceIds:[e.id],signal:'neutral',confidence:.8})}}],usage:{total_tokens:200}})));await pending;applyReadyReplies(g);assert.equal(g.world.story!.budgets[1].calls,1);assert.equal(g.world.story!.budgets[2].calls,0);assert.ok(!g.world.events.some(x=>x.text==='那就坐一会。'));assert.equal(g.world.replies!.at(-1)!.errorCode,'EXPIRED');}finally{globalThis.fetch=old;}});
test('invalid tarot commands do not consume stance or move and can be corrected',()=>{const g=table(),s=g.world.scene3!;assert.throws(()=>g.command({id:'bad',type:'tarot_answer',text:''}));assert.equal(s.playerMove,'');const before=s.playerStance;Object.assign(g.actor('USER'),{x:-7,z:-3});assert.throws(()=>g.command({id:'far',type:'tarot_seat'}));assert.equal(s.playerStance,before);Object.assign(g.actor('USER'),{x:1.4,z:-2.8});g.command({id:'good',type:'tarot_answer',text:'先让我想想。',target:'B'});assert.equal(s.playerMove,'answer');});
test('unseen questions and gazes do not leak through chapter DTO or NPC context',()=>{const g=table();Object.assign(g.actor('USER'),NIGHT.rooftop);openRound(g);assert.equal(g.view().scene3!.question,'');assert.equal(g.view().scene3!.lastGaze,null);const cause=g.emit('speech','USER','USER','claim','我听见了吗？','','normal','','player');assert.equal((g.context('USER',cause.id) as any).scene.currentCard,null);});
test('declined, absent and model-failed players are never automatically refusing',()=>{for(const mode of ['declined','absent','error']){const g=table(),s=g.world.scene3!;s.playerStance=mode==='declined'?'declined':'seated';if(mode==='absent')Object.assign(g.actor('USER'),NIGHT.rooftop);if(mode==='error')g.world.replies!.push({id:'failure',actor:'B',eventId:s.questionEventId!,status:'error'});tick(g,29);assert.equal(s.silences,0);}});
test('Joker counts toward max five, and a boundary can end before three',()=>{const g=table(),s=g.world.scene3!;s.boundaryHits=1;tick(g,45);assert.equal(s.history.length,1);assert.ok(['closing','scene4_ready'].includes(s.phase)||g.world.late);const h=table(),t=h.world.scene3!;t.history=Array.from({length:5},(_,i)=>({cardId:'Q0'+i,question:'',firstResponder:'',playerMove:''}));assert.equal(openRound(h,true),false);assert.equal(t.history.length,5);});
test('real portal route goes bar -> corridor -> stairs -> roof with small steps',()=>{const g=game(),n=g.navigation as NightNavigator;const path=n.path({x:-1,z:-4.3,area:'bar',y:0},NIGHT.rooftop);assert.ok(path.length>30);assert.ok(path.some(p=>p.area==='corridor'));assert.ok(path.some(p=>p.area==='stairs'));assert.equal(path.at(-1)!.y,4.2);for(let i=1;i<path.length;i++)assert.ok(Math.abs((path[i].y??0)-(path[i-1].y??0))<.2);assert.equal(n.acceptsPosition({x:5.8,z:4,y:0,area:'bar'},NIGHT.rooftop),false);});
test('cross-floor and closed-corridor conversations are not overheard through a flat distance',()=>{const g=game();initializeLateNight(g,4);Object.assign(g.actor('B'),{...NIGHT.rooftop,x:3,z:4});Object.assign(g.actor('USER'),{x:3,z:4,y:0,area:'bar'});let e=g.emit('speech','B','USER','chat','楼上的秘密。');assert.ok(!e.perceptions.some(p=>p.actor==='USER'));Object.assign(g.actor('B'),{...NIGHT.corridor,x:2});Object.assign(g.actor('USER'),{x:2,z:-4.7,y:0,area:'bar'});e=g.emit('speech','B','C','chat','隔着门的私聊。');assert.ok(!e.perceptions.some(p=>p.actor==='USER'&&p.level==='full'));});
test('all five supplied memories are subjective and never become USER factual memory',()=>{assert.deepEqual(Object.keys(MEMORIES).sort(),['AB','AC','BC','BD','CD']);for(const key of Object.keys(MEMORIES)){const g=game();g.world.scene3={leaver:key[0],follower:key[1]} as any;initializeLateNight(g,4);const s=g.world.late!;for(let i=0;i<2;i++)Object.assign(g.actor(key[i]),{...NIGHT.corridor,x:3.4+i*.8,route:[],active:true});g.world.late!.participants=key.split('');Object.assign(g.actor('USER'),{...NIGHT.corridor,x:3.8,yaw:90});s.propAt=0;g.world.elapsed=25;const e=g.emit('action',key[0],key[1],'memory_trigger','把巧克力盒递向身旁。','','normal','','script');assert.equal(s.memories[0]?.id,key);assert.equal(s.memories[0]?.evidenceId,e.id);assert.ok(s.cue);assert.ok(!g.actor('USER').memory.some(m=>m.summary===MEMORIES[key]));g.command({id:'ack',type:'cinematic_ack',target:s.cue!.id});const h=new Engine(scenario,{playerId:'x'},g.world,nav);assert.equal(h.world.late!.cue,undefined);assert.equal(h.world.late!.memories[0].consumed,true);}});
test('staying in the bar hides corridor flashbacks but does not stop either area',()=>{const g=game();g.world.scene3={leaver:'A',follower:'B'} as any;initializeLateNight(g,4);Object.assign(g.actor('A'),{...NIGHT.corridor,route:[]});Object.assign(g.actor('B'),{...NIGHT.corridor,x:4.5,route:[]});tick(g,40);assert.ok(g.world.late!.memories.length>0);assert.equal(g.world.late!.cue,undefined);assert.ok(g.world.events.some(e=>e.intent==='ambient'));tick(g,80);assert.equal(g.world.late!.chapter,5);});
test('powercut is once, pause stops time, save restores emergency light without replay',()=>{const g=game();initializeLateNight(g,5);tick(g,210);assert.equal(g.world.events.filter(e=>e.intent==='power_cut').length,1);g.world.paused=true;const at=g.world.elapsed;tick(g,40);assert.equal(g.world.elapsed,at);const h=new Engine(scenario,{playerId:'x'},g.world,nav);h.world.paused=false;tick(h,20);assert.equal(h.world.events.filter(e=>e.intent==='power_cut').length,1);assert.equal(h.world.late!.powerState,'emergency');assert.equal(areaOf(h.actor('USER')),'bar');});
test('unfollowed corridor, solo roof, sit/lie/stand and confirmed ending stay playable',()=>{const g=game();initializeLateNight(g,5);tick(g,220);for(const id of ['A','B','C','D']){g.actor(id).withdrawn=true;g.actor(id).route=[];}g.command({id:'roof',type:'night_move',location:'rooftop'});tick(g,70);assert.equal(g.world.late!.chapter,6);assert.equal(areaOf(g.actor('USER')),'rooftop');for(const pose of ['sit','lie','stand','silence']){g.command({id:pose,type:'night_pose',intent:pose});assert.equal(g.actor('USER').posture,pose);}assert.equal(g.world.status,'playing');g.command({id:'end',type:'end_night'});tick(g,5);assert.equal(g.world.status,'ended');assert.ok(g.reflection().ending.includes('留白'));});
test('early leaving never requires all chapters and recap excludes unperceived private history',()=>{const g=game();g.command({id:'leave',type:'leave'});assert.equal(g.world.status,'ended');const text=JSON.stringify(g.reflection());assert.ok(!Object.values(MEMORIES).some(m=>text.includes(m)));});
test('saving a flipped card preserves its question and does not open a second round',()=>{
  const g=table(),old=structuredClone(g.world),h=new Engine(scenario,{playerId:'x'},old,nav);
  assert.equal(h.world.scene3!.questionEventId,g.world.scene3!.questionEventId);
  assert.deepEqual(h.world.scene3!.history,g.world.scene3!.history);
  assert.equal(h.world.events.length,g.world.events.length);
});
for(const point of ['corridor','stairs','rooftop'] as const)test('save restores exact '+point+' height, route and chapter ledger',()=>{
  const g=game();initializeLateNight(g,point==='corridor'?4:point==='stairs'?5:6);
  Object.assign(g.actor('USER'),NIGHT[point]);g.go(g.actor('USER'),point==='corridor'?NIGHT.corridor:NIGHT.rooftop,point);
  reserveBudget(g.world,g.world.story!.chapter,350);g.world.paused=true;
  const before=structuredClone(g.world),h=new Engine(scenario,{playerId:'x'},before,nav);
  assert.deepEqual(h.actor('USER').route,g.actor('USER').route);assert.equal(h.actor('USER').y,g.actor('USER').y);
  assert.equal(h.world.story!.chapter,g.world.story!.chapter);assert.deepEqual(h.world.story!.budgets,g.world.story!.budgets);
  assert.equal(h.world.events.length,g.world.events.length);assert.equal(h.world.paused,true);
});
test('a late arrival cannot see a recollection triggered earlier without the player',()=>{
  const g=game();g.world.scene3={leaver:'B',follower:'C'} as any;initializeLateNight(g,4);
  Object.assign(g.actor('B'),{...NIGHT.corridor,route:[]});Object.assign(g.actor('C'),{...NIGHT.corridor,x:4.5,route:[]});tick(g,45);
  assert.ok(g.world.late!.memories.length>0);Object.assign(g.actor('USER'),NIGHT.corridor);
  assert.equal(g.view().late!.cue,null);assert.ok(!JSON.stringify(g.view().late).includes(MEMORIES.BC));
});
test('late phase time remains stable between actual transitions',()=>{
  const g=game();g.world.scene3={leaver:'B',follower:'C'} as any;initializeLateNight(g,4);
  Object.assign(g.actor('B'),{...NIGHT.corridor,route:[]});Object.assign(g.actor('C'),{...NIGHT.corridor,x:4.5,route:[]});tick(g,30);
  const at=g.world.late!.stageAt;tick(g,10);assert.equal(g.world.late!.stageAt,at);
});
test('recap does not introduce a character whose actions were never perceived',()=>{
  const g=game();g.world.events=[];g.actor('D').active=true;
  assert.deepEqual(g.reflection().trends,[]);
});
test('invalid ask-back target leaves the player turn available',()=>{
 const g=table(),s=g.world.scene3!;assert.throws(()=>g.command({id:'missing-target',type:'tarot_move',intent:'ask_back',text:'你呢？'}));assert.equal(s.playerMove,'');
 g.command({id:'corrected',type:'tarot_move',intent:'ask_back',target:'B',text:'你呢？'});assert.equal(s.playerMove,'ask_back');
});
test('watching a card does not count as a seated player refusing it',()=>{
 const g=table(),s=g.world.scene3!;s.playerStance='watching';g.command({id:'watch',type:'tarot_move',intent:'observe'});assert.equal(s.silences,0);
});
test('card context cannot reveal an unseen joker or future first speaker',()=>{
 const g=table(),s=g.world.scene3!;const seen=g.world.events.find(e=>e.id===s.questionEventId)!;
 assert.equal((g.context('USER',seen.id) as any).scene.currentCard.firstResponder,'');
 Object.assign(g.actor('USER'),NIGHT.rooftop);openRound(g,true);const own=g.emit('speech','USER','USER','claim','看不见桌面');
 assert.equal((g.context('USER',own.id) as any).scene.jokerRound,false);
});
test('stalled walkers replan around a standing character without moving either actor',()=>{
 const clear=new Navigator({minX:-5,minZ:-5,width:20,height:20,cell:.5,blocked:[],walls:[]});
 const g=new Engine(scenario,{playerId:'crowd-synthetic',story:'scene1_v1',online:false},undefined,clear);
 for(const a of g.world.actors)a.active=['USER','B'].includes(a.id);
 Object.assign(g.actor('USER'),{x:.25,z:.25,y:0,area:'bar',route:[]});Object.assign(g.actor('B'),{x:.25,z:1.25,y:0,area:'bar',route:[]});
 g.go(g.actor('USER'),{x:.25,z:3.25,y:0,area:'bar'});const before=structuredClone(g.actor('USER').route);
 advanceCrowd(g);g.world.elapsed+=3;advanceCrowd(g);
 assert.notDeepEqual(g.actor('USER').route,before);assert.ok(g.actor('USER').route.every(p=>distance(p,g.actor('B'))>=.53));
 assert.equal(g.actor('USER').x,.25);assert.equal(g.actor('B').z,1.25);
});
test('a blocked bar portal never fabricates a straight cross-area route',()=>{
 const data=structuredClone(nav.data),base=new Navigator(data),near=base.nearest({x:-1,z:-3.8});
 const start={...near,y:0,area:'bar'};base.blocked=new Set(Array.from({length:data.width*data.height},(_,i)=>i));base.blocked.delete(base.index(start));
 const n=new NightNavigator(base),route=n.path(start,NIGHT.corridor);assert.deepEqual(route,[]);
});
