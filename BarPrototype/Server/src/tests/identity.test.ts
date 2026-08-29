import test from 'node:test';
import assert from 'node:assert/strict';
import {dirname} from 'node:path';
import {fileURLToPath} from 'node:url';
import {Engine} from '../engine.js';
import {loadScenario} from '../config.js';
import {buildProfile,selectPack,identityBrief,DEFAULT_PACK} from '../identity.js';
const root=dirname(dirname(dirname(fileURLToPath(import.meta.url))));
const scenario=loadScenario(root+'/scenarios/last_call.json');
const packs=scenario.identityPacks;

test('same answers, free text and seed always resolve to the same pack',()=>{
  const answers={domain:'investment',career_stage:'working',preferred_topic_density:'work'};
  const a=buildProfile(answers,'我在看消费和 AI 项目。');
  const b=buildProfile(answers,'我在看消费和 AI 项目。');
  assert.deepEqual(a,b);
  assert.equal(selectPack(a,packs,821).packId,selectPack(b,packs,821).packId);
  assert.equal(selectPack(a,packs,7).packId,'investment_market');
});

test('an explicit choice overrides free text and is not overridden by it',()=>{
  const profile=buildProfile({domain:'creative',career_stage:'skip',preferred_topic_density:'skip'},'我做私募股权');
  assert.equal(profile.domain,'creative');
  assert.equal(selectPack(profile,packs,821).packId,'creative_professional');
});

test('ambiguous or missing domain falls back to the default pack',()=>{
  const empty=buildProfile({},undefined);
  assert.equal(empty.domain,'');
  assert.equal(selectPack(empty,packs,821).packId,DEFAULT_PACK);
  const mixed=buildProfile({},'我是做私募的，也在创业');
  assert.equal(mixed.domain,''); // two domains: do not guess a specific desk
  assert.equal(selectPack(mixed,packs,821).packId,DEFAULT_PACK);
});

test('profile extracts only explicit content and never infers protected attributes',()=>{
  const p=buildProfile({},'我公司卖掉后 gap 了半年');
  assert.equal(p.career_stage,'gap');
  assert.equal(p.current_transition,'recent_exit_or_gap');
  assert.equal(p.consent_scope,'context_only');
  // Nothing about wealth, orientation, family or mental state is ever derived.
  for(const v of Object.values(p))assert.equal(typeof v==='string'&&/性取向|同性|家庭|抑郁|焦虑|身家|有钱|结婚|出轨/.test(v),false);
});

test('every pack covers A/B/C/D and only skins real beats',()=>{
  const beatIds=scenario.beats.map(b=>b.id);
  for(const [id,pack] of Object.entries(packs)){
    for(const c of ['A','B','C','D'])assert.ok(pack.actors[c],`${id} missing ${c}`);
    for(const key of Object.keys(pack.sceneSkin))assert.ok(beatIds.includes(key),`${id} skins ${key}`);
    assert.ok(pack.maxNewConcepts>0&&pack.maxNewConcepts<=4);
  }
});

test('identity brief is compressed and carries no raw player input',()=>{
  const brief=identityBrief(scenario,'investment_market','A');
  assert.ok(brief);
  assert.equal(brief.name,'kiko');
  assert.ok(brief.corpus.length<=2);
  assert.equal(JSON.stringify(brief).includes('我做私募'),false);
  assert.equal(JSON.stringify(brief).includes('消费'),false);
});

test('agent context never serializes raw player background',()=>{
  const g=new Engine(structuredClone(scenario),{playerId:'p',choices:{domain:'investment'},entryContext:'PRIVATE_BG_99 我做私募股权',online:false});
  g.actor('B').active=true;
  const event=g.emit('message','USER','B','probe','你好');
  const text=JSON.stringify(g.context('B',event.id));
  assert.equal(text.includes('PRIVATE_BG_99'),false);
  assert.equal(text.includes('我做私募'),false);
  assert.equal(text.includes('entryContext'),false);
});

test('mid-game revision defers to the next beat boundary and keeps state',()=>{
  const g=new Engine(structuredClone(scenario),{playerId:'p',role:'passerby',online:false});
  const b=g.actor('B');
  b.relations.USER.trust=0.9; // arbitrary established state
  // Seed one memory so we can prove a pack revision does not clear it.
  const seedEvent=g.emit('message','USER','B','probe','今晚想见你。');
  const memoryIdsBefore=b.memory.map(m=>m.eventId);
  assert.ok(memoryIdsBefore.includes(seedEvent.id));
  assert.equal(g.world.identityPack,DEFAULT_PACK);
  g.command({id:'r1',type:'revise_context',choices:{domain:'investment',career_stage:'skip',preferred_topic_density:'skip'}});
  assert.ok(g.world.pendingPackRevision);
  assert.equal(g.world.identityPack,DEFAULT_PACK); // not applied yet
  assert.equal(b.relations.USER.trust,0.9); // relations untouched
  g.world.elapsed=400;g.advance(.2); // a later beat boundary consumes the revision
  assert.equal(g.world.pendingPackRevision,undefined);
  assert.equal(g.world.identityPack,'investment_market');
  assert.equal(b.relations.USER.trust,0.9);
  for(const id of memoryIdsBefore)assert.ok(b.memory.some(m=>m.eventId===id),'revision must not clear memory');
});

test('no background means the default pack and a runnable night',()=>{
  const g=new Engine(structuredClone(scenario),{playerId:'p',online:false});
  assert.equal(g.world.identityPack,DEFAULT_PACK);
  assert.equal(g.world.contextProfile.domain,'');
  g.command({id:'leave',type:'leave'});
  assert.equal(g.view().status,'ended');
});
