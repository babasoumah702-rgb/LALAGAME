import test from 'node:test';
import assert from 'node:assert/strict';
import {mkdtempSync,mkdirSync,writeFileSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {modelConfig,modelConfigFile,saveModelConfig,writableModelConfigFile} from '../model.js';
function fixture(){
  const home=mkdtempSync(join(tmpdir(),'lastcall-config-path-'));
  const local=join(home,'redirected-appdata');
  return {home,local,stable:join(home,'.lalagame/private'),legacy:join(local,'LALAGAME/private')};
}
function config(dir:string,value:string){mkdirSync(dir,{recursive:true});writeFileSync(join(dir,'model.env'),value);}
test('normal and redirected AppData processes read the same profile-private configuration',()=>{
  const f=fixture();config(f.stable,'LASTCALL_API_KEY=synthetic-shared-key\nLASTCALL_MODEL=deepseek-v4-flash');
  config(f.legacy,'LASTCALL_API_KEY=synthetic-old-key');
  for(const local of [f.local,join(f.home,'normal-appdata')]){
    assert.equal(modelConfig({LOCALAPPDATA:local},f.home).key,'synthetic-shared-key');
    assert.equal(modelConfigFile({LOCALAPPDATA:local},f.home),join(f.stable,'model.env'));
  }
});
test('explicit config directories remain isolated and never silently import a personal key',()=>{
  const f=fixture();config(f.stable,'LASTCALL_API_KEY=synthetic-personal-key');
  assert.equal(modelConfig({LASTCALL_CONFIG_DIR:join(f.home,'test-config')},f.home).key,'');
});
test('legacy config remains readable if no stable file exists; environment override is retained',()=>{
  const f=fixture();config(f.legacy,'LASTCALL_API_KEY=synthetic-legacy-key');
  assert.equal(modelConfig({LOCALAPPDATA:f.local},f.home).key,'synthetic-legacy-key');
  assert.equal(modelConfig({LOCALAPPDATA:f.local,LASTCALL_API_KEY:'synthetic-env-key'},f.home).key,'synthetic-env-key');
});
test('an intentionally empty stable key never falls back to a stale key from a different location',()=>{
  const f=fixture();config(f.stable,'LASTCALL_API_KEY=');config(f.legacy,'LASTCALL_API_KEY=synthetic-legacy-key');
  assert.equal(modelConfig({LOCALAPPDATA:f.local},f.home).key,'');
});
test('front-end model config writes only to the private profile and validates transport',()=>{
  const f=fixture(),env={LOCALAPPDATA:f.local};
  const saved=saveModelConfig({base:'https://gateway.example/v1/',model:'compatible-model',key:'synthetic-ui-key'},env,f.home);
  assert.equal(saved.base,'https://gateway.example/v1');assert.equal(saved.model,'compatible-model');assert.equal(saved.key,'synthetic-ui-key');
  assert.equal(writableModelConfigFile(env,f.home),join(f.home,'.lalagame/private/model.env'));
  const kept=saveModelConfig({base:'http://127.0.0.1:8080/v1',model:'local-model',keepKey:true},env,f.home);
  assert.equal(kept.key,'synthetic-ui-key');
  assert.throws(()=>saveModelConfig({base:'http://remote.example/v1',model:'unsafe',key:'x'},env,f.home),/HTTPS/);
  const cleared=saveModelConfig({base:'https://gateway.example/v1',model:'compatible-model',clearKey:true},env,f.home);
  assert.equal(cleared.key,'');
});
