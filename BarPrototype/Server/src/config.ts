import {readFileSync} from 'node:fs';
import type {Scenario} from './types.js';
export function loadScenario(path:string):Scenario {
  const s=JSON.parse(readFileSync(path,'utf8')) as Scenario;
  for(const key of ['roles','intents','styles','actors','cards','beats','locations'] as const){if(!Array.isArray(s[key])||!s[key].length)throw new Error(`Missing scenario list: ${key}`);const ids=s[key].map((x:any)=>x.id);if(new Set(ids).size!==ids.length)throw new Error(`Duplicate ${key} IDs`);}
  if(s.version!==1||!s.actors.find(a=>a.id==='USER')||!(s.duration>0))throw new Error('Invalid scenario version or player');
  const conditions=['always','b_available','past_at_seat','alone_at_seat'];
  const effects=['opening','signal','cards','enter_a','enter_c','enter_d','last_call','close'];
  for(const b of s.beats)if(!conditions.includes(b.condition)||!effects.includes(b.effect))throw new Error('Unsupported declarative beat');
  for(const a of s.actors){if(!s.locations.find(l=>l.id===a.home))throw new Error(`Unknown home for ${a.id}`);for(const f of a.knownFacts)if(!s.facts[f])throw new Error(`Undefined fact ${f}`);}
  return s;
}
