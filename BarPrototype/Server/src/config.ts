import {readFileSync} from 'node:fs';
import type {Scenario} from './types.js';
const VOICE_KINDS=['warm','probe','boundary','share','relay'];
const CAST=['A','B','C','D'];
const DENSITIES=['work','life','mix'];
export function loadScenario(path:string):Scenario {
  const s=JSON.parse(readFileSync(path,'utf8')) as Scenario;
  for(const key of ['roles','intents','styles','actors','cards','beats','locations'] as const){if(!Array.isArray(s[key])||!s[key].length)throw new Error(`Missing scenario list: ${key}`);const ids=s[key].map((x:any)=>x.id);if(new Set(ids).size!==ids.length)throw new Error(`Duplicate ${key} IDs`);}
  if(s.version!==1||!s.actors.find(a=>a.id==='USER')||!(s.duration>0))throw new Error('Invalid scenario version or player');
  const conditions=['always','b_available','past_at_seat','alone_at_seat'];
  const effects=['opening','signal','cards','enter_a','enter_c','enter_d','last_call','close'];
  for(const b of s.beats)if(!conditions.includes(b.condition)||!effects.includes(b.effect))throw new Error('Unsupported declarative beat');
  for(const a of s.actors){if(!s.locations.find(l=>l.id===a.home))throw new Error(`Unknown home for ${a.id}`);for(const f of a.knownFacts)if(!s.facts[f])throw new Error(`Undefined fact ${f}`);}
  // Personas must cover the four cast members; a missing persona would silently degrade the brief.
  for(const id of CAST)if(!s.personas?.[id])throw new Error(`Missing persona for ${id}`);
  // Every identity pack must cover the full cast and only skin existing beats.
  const beatIds=s.beats.map(b=>b.id);
  const packEntries=Object.entries(s.identityPacks||{});
  if(!packEntries.length)throw new Error('Missing identity packs');
  for(const [id,pack] of packEntries){
    for(const c of CAST)if(!pack.actors?.[c])throw new Error(`Identity pack ${id} missing actor ${c}`);
    if(!(pack.maxNewConcepts>0&&pack.maxNewConcepts<=4))throw new Error(`Identity pack ${id} has invalid maxNewConcepts`);
    for(const key of Object.keys(pack.sceneSkin||{}))if(!beatIds.includes(key))throw new Error(`Identity pack ${id} skins unknown beat ${key}`);
  }
  // Every choice must carry an escape hatch and valid density labels.
  for(const c of s.choices||[]){
    if(!c.options?.some(o=>o.value==='skip'))throw new Error(`Choice ${c.id} missing skip option`);
    if(c.id==='preferred_topic_density')for(const o of c.options)if(o.value!=='skip'&&!DENSITIES.includes(o.value))throw new Error(`Unknown density ${o.value}`);
  }
  // Voices: a missing actor falls back to BARTENDER and a missing key falls back to probe, both
  // silently. Require the full grid so offline lines never say the wrong character's words.
  for(const a of s.actors)if(a.id!=='USER'&&a.id!=='OWNER'){
    const v=s.voices[a.id];if(!v)throw new Error(`Missing voice for ${a.id}`);
    for(const k of VOICE_KINDS)if(!v[k]?.length)throw new Error(`Voice ${a.id} missing ${k}`);
  }
  // Initial relations reference real actors; a typo otherwise throws at world creation or writes junk.
  const ids=s.actors.map(a=>a.id);
  for(const r of s.initialRelations||[])if(!ids.includes(r.from)||!ids.includes(r.to))throw new Error(`Initial relation references unknown actor ${r.from}->${r.to}`);
  return s;
}
