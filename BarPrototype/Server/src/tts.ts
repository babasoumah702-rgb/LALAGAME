import {readFileSync,existsSync} from 'node:fs';
import {homedir} from 'node:os';
import {modelConfigFile} from './model.js';
import type {Event} from './types.js';

const VOICEABLE=['A','B','C','D','BARTENDER','OWNER'];

export function ttsConfig(env:NodeJS.ProcessEnv=process.env,userHome=homedir()){
  const file=modelConfigFile(env,userHome);
  const values:Record<string,string>={};
  if(existsSync(file)){
    for(const line of readFileSync(file,'utf8').split(/\r?\n/)){
      const index=line.indexOf('=');
      if(index>0)values[line.slice(0,index).trim()]=line.slice(index+1).trim();
    }
  }
  const pick=(key:string)=>env[key]||values[key];
  // Per-actor voice ids: LASTCALL_TTS_VOICE_A / _B / _C / _D / _BARTENDER / _OWNER.
  // An actor is voiced only when a voice id is configured for them.
  const voices:Record<string,string>={};
  for(const id of VOICEABLE){
    const v=pick('LASTCALL_TTS_VOICE_'+id);
    if(v)voices[id]=v;
  }
  // Legacy single-voice key applies to A when no per-actor override is present.
  if(!voices['A']&&pick('LASTCALL_TTS_VOICE'))voices['A']=pick('LASTCALL_TTS_VOICE')!;
  return {
    base:pick('LASTCALL_TTS_BASE')||'https://dashscope.aliyuncs.com/api/v1/services/audio/tts/SpeechSynthesizer',
    model:pick('LASTCALL_TTS_MODEL')||'cosyvoice-v3.5-plus',
    key:pick('LASTCALL_TTS_API_KEY')||'',
    voices,
    format:pick('LASTCALL_TTS_FORMAT')||'wav',
    sampleRate:Number(pick('LASTCALL_TTS_SAMPLE_RATE')||22050)
  };
}

export class TtsAdapter {
  config=ttsConfig();
  private cache=new Map<string,Buffer>();
  reload(){this.config=ttsConfig();return this.config;}
  audio(id:string){return this.cache.get(id);}
  // Best-effort voice: a failed or pending line stays silent and never blocks dialogue.
  // The event's `audio` path appears on the next broadcast once synthesis lands.
  async speak(event:Event){
    if(event.audio)return;
    const voiceId=this.config.voices[event.actor];
    const text=event.text.trim();
    if(!voiceId||!text||!this.config.key)return;
    try{
      const response=await fetch(this.config.base,{
        method:'POST',
        headers:{'Content-Type':'application/json','Authorization':'Bearer '+this.config.key},
        body:JSON.stringify({model:this.config.model,input:{text,voice:voiceId,format:this.config.format,sample_rate:this.config.sampleRate}}),
        signal:AbortSignal.timeout(15000)
      });
      if(!response.ok)throw new Error('TTS_GATEWAY');
      const result=await response.json() as any;
      const url=result?.output?.audio?.url;
      if(!url)throw new Error('TTS_NO_URL');
      const audioResponse=await fetch(url,{signal:AbortSignal.timeout(15000)});
      if(!audioResponse.ok)throw new Error('TTS_FETCH');
      const bytes=Buffer.from(await audioResponse.arrayBuffer());
      if(!bytes.length)throw new Error('TTS_EMPTY');
      this.cache.set(event.id,bytes);
      event.audio='/api/audio/'+event.id;
    }catch{/* silence */ }
  }
}
