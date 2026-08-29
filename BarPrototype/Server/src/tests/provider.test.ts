import test from 'node:test';
import assert from 'node:assert/strict';
import {providerOptions,ModelAdapter} from '../model.js';
import {generateIntro} from '../intro-model.js';
import {Engine} from '../engine.js';
import {loadScenario} from '../config.js';
const scenario=loadScenario('scenarios/last_call.json');
test('DeepSeek uses non-thinking mode without leaking vendor parameters to other gateways',()=>{
  assert.deepEqual(providerOptions({base:'https://api.deepseek.com'}),{thinking:{type:'disabled'}});
  assert.deepEqual(providerOptions({base:'https://api.deepseek.com/v1'}),{thinking:{type:'disabled'}});
  assert.deepEqual(providerOptions({base:'https://another-gateway.test/v1'}),{});
});
test('both intro and character requests carry the configured DeepSeek model and JSON mode',async t=>{
  const requests:any[]=[];
  t.mock.method(globalThis,'fetch',async(_url:any,options:any)=>{
    requests.push(JSON.parse(options.body));
    return Response.json({choices:[{message:{content:JSON.stringify(requests.length===1?
      {message:'今晚见。',hint:'路上小心。',attitude:'observing'}:
      {action:'wait',target:'USER',intent:'observe',expression:'',interpretation:'等待',evidenceIds:[],signal:'neutral',confidence:.5})}}]});
  });
  const config={base:'https://api.deepseek.com',model:'deepseek-v4-flash',key:'unit-test-only'};
  const intro=new Engine(scenario,{playerId:'provider-test',online:true,opening:'scene0_v1'});
  await generateIntro(intro,config);assert.equal(intro.world.intro!.messageSource,'model');
  const game=new Engine(scenario,{playerId:'provider-test',online:true});
  const event=game.emit('speech','USER','B','approach','你好');
  const adapter=new ModelAdapter();adapter.config=config;
  await adapter.decide(game,{actor:'B',eventId:event.id,due:0});
  assert.equal(requests.length,2);
  for(const body of requests){assert.equal(body.model,'deepseek-v4-flash');assert.deepEqual(body.thinking,{type:'disabled'});assert.equal(body.response_format.type,'json_object');}
});
