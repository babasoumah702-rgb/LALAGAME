import type {Engine} from './engine.js';
import type {Event} from './types.js';
import {clock} from './world.js';
import {distance} from './navigation.js';
export function reflection(game:Engine){
  const world=game.world;
  const visible=world.events.filter(e=>e.perceptions.some(p=>p.actor==='USER'));
  const important=visible.filter(e=>e.type==='speech'||e.intent==='arrival').slice(-3);
  const ripple=visible.filter(e=>e.parentId).sort((a,b)=>b.depth-a.depth)[0];
  const chain:Event[]=[];
  let current:Event|undefined=ripple;
  while(current){
    chain.unshift(current);
    const parentId:string=current.parentId;
    current=world.events.find(e=>e.id===parentId);
  }
  const seenChain=chain.map(e=>{
    const perception=e.perceptions.find(p=>p.actor==='USER');
    return perception?game.actor(e.actor).name+'：'+perception.text:'中间有一段你未能感知的交流；来源暂时未知。';
  });
  const trends=world.actors.filter(a=>!['USER','OWNER'].includes(a.id)&&(a.active||visible.some(e=>e.actor===a.id))).map(a=>{
    const initial=a.id==='A'?'长期关系，有些话未说':a.id==='B'?'未定义的吸引':a.id==='C'?'过去的重要关系':a.id==='D'?'尚未认识':'认识的调酒师';
    const last=visible.filter(e=>e.actor===a.id).at(-1);
    const final=last?last.intent==='boundary'?'她表达了需要空间':['approach','reveal'].includes(last.intent)?'出现了进一步交流的信号':'还有一些话没有说清楚':'没有足够的可观察信息';
    return a.name+'｜'+initial+' → '+final;
  });
  const names:Record<string,string>={approach:'靠近',probe:'试探',reveal:'表达',boundary:'边界',connect:'连接',observe:'观察'};
  const counts=Object.entries(world.moves).map(([key,value])=>(names[key]||key)+' '+value+' 次').join('，');
  return {
    title:'这一晚，如何走到这里',
    trends,
    events:important.map(e=>clock(game.scenario,e.time)+' '+game.actor(e.actor).name+'：'+e.perceptions.find(p=>p.actor==='USER')!.text),
    chain:seenChain.length?seenChain:['本局尚未形成你能追踪的传播链。未知不等于没有发生。'],
    behavior:counts?'本局行动：'+counts+'。这不是人格判断。':'本局没有直接表达。这只是行动记录。',
    ending:'今晚结束了。那些还没说清楚的话，可以留给下一次。'
  };
}
export function viewState(game:Engine,fullHistory=false){
  const w=game.world,user=game.actor('USER');
  return {
    version:1,sessionId:w.id,cursor:w.sequence,clock:clock(game.scenario,w.elapsed),
    elapsed:w.elapsed,night:w.night,status:w.status,paused:w.paused,busy:game.busy,
    mode:w.modelMode,modeReason:w.modelReason,role:w.role,calls:w.calls,tokens:w.tokens,
    cardsOffered:!!w.flags.cardsOffered,cardsJoined:!!w.flags.cardsJoined,
    lastCall:!!w.flags.lastCall,pastDrink:!!w.flags.pastDrink,
    lastTarget:String(w.flags.lastTarget||''),
    characters:w.actors.filter(a=>a.active).map(a=>({
      id:a.id,name:a.name,color:a.color,x:a.x,z:a.z,yaw:a.yaw,animation:a.animation,
      destination:a.destination,route:a.route,routeVersion:a.routeVersion??0,
      interactable:a.id!=='USER'&&distance(a,user)<=3,location:game.zone(a).name
    })),
    events:w.events.filter(e=>e.perceptions.some(p=>p.actor==='USER')).slice(fullHistory?0:-45).map(e=>{
      const p=e.perceptions.find(p=>p.actor==='USER')!;
      return {id:e.id,seq:e.seq,time:clock(game.scenario,e.time),actor:e.actor,name:game.actor(e.actor).name,text:p.text,source:p.source,level:p.level,hasParent:!!e.parentId};
    }),
    cards:game.scenario.cards.map(c=>{
      const unlocked=c.type==='social'||(c.id==='last_call'?!!w.flags.lastCall:!!w.flags.cardsJoined);
      const style=game.scenario.styles.find(item=>item.id===w.style) as {expressions?:Record<string,string[]>}|undefined;
      return {...c,expressions:style?.expressions?.[c.id]||c.expressions,ready:unlocked&&(w.cooldowns[c.id]||0)<=w.elapsed};
    }),
    locations:game.scenario.locations.filter(l=>l.id!=='service'||['staff','owner_bartender'].includes(w.role)),
    reflection:w.status==='ended'?reflection(game):null
  };
}
