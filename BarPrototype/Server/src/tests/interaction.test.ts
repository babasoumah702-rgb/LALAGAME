import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {Engine} from '../engine.js';
import {loadScenario} from '../config.js';
import {Navigator} from '../navigation.js';
import {interactionView} from '../interaction.js';
import {initializeSceneTwo} from '../scene-two.js';
import {initializeSceneThree} from '../scene-three.js';
import {initializeLateNight} from '../late-night.js';

const scenario=loadScenario('scenarios/last_call.json');
const nav=new Navigator(JSON.parse(readFileSync('scenarios/navigation.json','utf8')));
function game(){const g=new Engine(scenario,{playerId:'interaction-test',story:'scene1_v1',online:false},undefined,nav);g.advance(0);return g;}
function option(g:Engine,id:string){return interactionView(g)!.groups.flatMap(x=>x.options).find(x=>x.id===id);}

test('runtime interaction language always has exactly three stable primary groups',()=>{
  const g=game();
  assert.deepEqual((g.view() as any).interaction.groups.map((x:{id:string})=>x.id),['observe','move','interact'],'the DTO is exposed on the live state, not only inside reflection');
  for(const chapter of [1,2,3,4,5,6]){
    if(chapter===2)initializeSceneTwo(g);
    if(chapter===3)initializeSceneThree(g);
    if(chapter>=4)initializeLateNight(g,chapter as 4|5|6);
    const view=interactionView(g)!;
    assert.deepEqual(view.groups.map(x=>[x.id,x.label]),[['observe','观察'],['move','移动'],['interact','互动']]);
    const ids=view.groups.flatMap(x=>x.options.map(o=>o.id));
    assert.equal(new Set(ids).size,ids.length,'option IDs stay unique inside one interaction context');
  }
});

test('Scene 1 objective advances from observation to one light interaction without duplicating its state',()=>{
  const g=game();let view=interactionView(g)!;
  assert.equal(view.nextActionId,'observe_room');
  g.command({id:'observe-once',type:'observe'});
  view=interactionView(g)!;
  assert.equal(option(g,'observe_room')?.selected,true);
  g.command({id:'observe-once',type:'observe'});
  assert.equal(g.world.commandIds.filter(id=>id==='observe-once').length,1);
});

test('Scene 2 gathering gives one direct route to the main table and does not require dialogue',()=>{
  const g=game();initializeSceneTwo(g);g.world.scene2!.phase='gathering';g.world.scene2!.deckAt=-1;
  const view=interactionView(g)!;
  assert.equal(view.nextTitle,'回到主桌');assert.equal(view.nextGroup,'move');assert.equal(view.nextActionId,'move_main');
});

test('Scene 3 exposes one stance group and locks an answered turn',()=>{
  const g=game();initializeSceneThree(g);let view=interactionView(g)!;
  assert.equal(view.nextTitle,'先选择怎样参与');
  assert.deepEqual(view.groups.find(x=>x.id==='interact')!.options.filter(x=>x.id.startsWith('tarot_')).map(x=>x.label),['坐下','旁观','不参加']);
  g.command({id:'watch-once',type:'tarot_seat',text:'watch'});
  assert.throws(()=>g.command({id:'change-stance',type:'tarot_seat',text:'decline'}),/已经选择/);
  assert.equal(g.world.scene3!.playerStance,'watching');
  g.world.scene3!.playerStance='seated';g.world.scene3!.askedAt=g.world.elapsed;g.world.scene3!.playerMove='skip';
  view=interactionView(g)!;assert.equal(view.nextTitle,'等待下一张牌');
});

test('late chapters derive departure choices and replaceable roof postures from saved truth',()=>{
  const g=game();initializeLateNight(g,5);g.world.late!.powerAt=g.world.elapsed;g.world.late!.powerState='emergency';
  let view=interactionView(g)!;assert.equal(view.nextTitle,'断电后的去向');
  assert.deepEqual(['move_rooftop','stay','leave'].map(id=>option(g,id)?.enabled),[true,true,true]);
  initializeLateNight(g,6);g.world.late!.posture='lie';view=interactionView(g)!;
  assert.equal(option(g,'pose_lie')?.selected,true);assert.equal(option(g,'pose_lie')?.replaceable,true);
  assert.equal(view.nextActionId,'end_night');
});
