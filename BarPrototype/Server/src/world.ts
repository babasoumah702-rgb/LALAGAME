import {randomUUID} from 'node:crypto';
import type {Actor,Point,Relation,Scenario,World} from './types.js';
import {distance,Navigator} from './navigation.js';
export const clamp=(n:number,lo=0,hi=1)=>Math.max(lo,Math.min(hi,n));
export const relation=():Relation=>({trust:.5,closeness:.3,attraction:.2,safety:.6,tension:.2,uncertainty:.5});
export const actor=(w:World,id:string)=>{const a=w.actors.find(a=>a.id===id);if(!a)throw new Error('Unknown actor');return a;};
export const location=(s:Scenario,id:string)=>{const l=s.locations.find(l=>l.id===id);if(!l)throw new Error('Unknown location');return l;};
export const zone=(s:Scenario,p:Point)=>s.locations.reduce((best,l)=>distance(p,l)/l.radius<distance(p,best)/best.radius?l:best);
export function random(w:World){let x=w.rng|0;x^=x<<13;x^=x>>>17;x^=x<<5;w.rng=x>>>0;return w.rng/4294967296;}
export function clock(s:Scenario,elapsed:number){const m=Math.floor(22*60+35+elapsed/s.duration*205)%1440;return `${Math.floor(m/60).toString().padStart(2,'0')}:${(m%60).toString().padStart(2,'0')}`;}
export function createWorld(s:Scenario,nav:Navigator,options:{playerId:string;role?:string;entryIntent?:string;style?:string;seed?:number;online?:boolean}):World {
  const role=s.roles.find(r=>r.id===options.role)??s.roles.find(r=>r.id==='friend_guest')!;
  const actors:Actor[]=s.actors.map(c=>{
    const p=nav.nearest(location(s,c.id==='USER'?role.spawn:c.home));
    return {...structuredClone(c),...p,yaw:180,active:c.initial,animation:'idle',destination:'',route:[],memory:[],beliefs:[],relations:{},nextAction:24+c.initiative*5,lastSpoke:-100,withdrawn:false};
  });
  for(const a of actors)for(const b of actors)if(a!==b)a.relations[b.id]=relation();
  for(const r of s.initialRelations)Object.assign(actors.find(a=>a.id===r.from)!.relations[r.to],r.values);
  const now=new Date().toISOString();
  return {version:1,scenarioId:s.id,id:randomUUID(),playerId:options.playerId,role:role.id,entryIntent:options.entryIntent??'observe_only',style:options.style??'natural',seed:options.seed??821,rng:options.seed??821,elapsed:0,night:1,status:'playing',paused:false,sequence:0,actors,events:[],jobs:[],beatIds:[],commandIds:[],flags:{},moves:{},cooldowns:{},initialRelations:Object.fromEntries(actors.filter(a=>a.id!=='USER').map(a=>[a.id,structuredClone(a.relations.USER)])),calls:0,tokens:0,modelMode:options.online?'online':'offline',modelReason:options.online?'联网推理':'规则模式 · 无模型调用',createdAt:now,updatedAt:now};
}
export function trimMemory(a:Actor){a.memory=[...a.memory.filter(m=>m.tier==='long').slice(-20),...a.memory.filter(m=>m.tier==='relationship').slice(-32),...a.memory.filter(m=>m.tier==='short').slice(-16)].sort((x,y)=>x.time-y.time);}
