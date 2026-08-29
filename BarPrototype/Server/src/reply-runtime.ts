import {currentRequest} from './story.js';
import {canActorReplyToEvent,type Engine} from './engine.js';
import type {Store} from './store.js';
import type {ModelAdapter} from './model.js';
import type {Job} from './types.js';

export async function generateReply(g:Engine,adapter:ModelAdapter,job:Job){
  try{await adapter.decide(g,job);}catch{/* Adapter records only a safe failure category/message; never inject a fallback. */}
}
export function applyReadyReplies(g:Engine,store?:Store){
  if(g.world.paused||g.world.status!=='playing')return;
  for(const reply of g.world.replies||[]){
    if(reply.status!=='ready'||!reply.decision)continue;
    const event=g.world.events.find(e=>e.id===reply.eventId);
    if(event&&!canActorReplyToEvent(event,reply.actor)){
      reply.status='complete';reply.errorCode='SUPPRESSED';reply.error='';reply.decision=undefined;continue;
    }
    if(!currentRequest(g.world,reply.eventId)){reply.status='complete';reply.errorCode='EXPIRED';reply.error='回复属于已结束章节，没有触发当前章节动作。';continue;}
    const accepted=g.apply(reply.actor,reply.decision,reply.eventId);
    store?.recordDecision(g.world,{actor:reply.actor,eventId:reply.eventId,due:g.world.elapsed},reply.decision,accepted);
    if(accepted){reply.status='complete';reply.error='';}
    else{reply.status='error';reply.errorCode='INVALID';reply.error='回复未通过当前场景校验，请重试。';reply.decision=undefined;}
  }
}
