import {fileURLToPath} from 'node:url';
import {writeFileSync} from 'node:fs';
import {Engine} from './engine.js';
import {loadScenario} from './config.js';
import {generateIntro} from './intro-model.js';
const scenario=loadScenario(fileURLToPath(new URL('../scenarios/last_call.json',import.meta.url)));
const g=new Engine(scenario,{playerId:'scene0-synthetic-live-check',opening:'scene0_v1',online:true,entryMode:'friend_invited'});
const start=performance.now();
await generateIntro(g);
const result={
  calls:g.world.calls,tokens:g.world.tokens,durationMs:Math.round(performance.now()-start),
  messageSource:g.world.intro!.messageSource,status:g.world.intro!.generationStatus,
  accepted:g.world.intro!.messageSource==='model',
  note:'Synthetic private-free context. This checks gateway compatibility, not seven-second render timing.'
};
writeFileSync(fileURLToPath(new URL('../../Verification/scene0-gateway.json',import.meta.url)),JSON.stringify(result,null,2));
console.log(JSON.stringify(result));
