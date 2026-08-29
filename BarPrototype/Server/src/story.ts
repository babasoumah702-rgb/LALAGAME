import type {Engine} from './engine.js';
import type {World} from './types.js';
export type StoryState={version:1;chapter:number;phase:string;stageAt:number;enteredAt:number;executed:string[];transitions:{from:number;to:number;at:number}[];budgets:Record<string,{calls:number;tokens:number}>};
export function initializeStory(w:World){
  if(w.scene3&&!w.scene3.questionEventId)w.scene3.questionEventId=w.events.findLast(e=>e.intent==='tarot_question'&&e.text===w.scene3!.question)?.id;
  if(w.story||!w.scene1)return;
  const chapter=w.scene3?3:w.scene2?2:1;
  w.story={version:1,chapter,phase:'restored',stageAt:w.elapsed,enteredAt:chapter===3?w.scene3!.enteredAt:chapter===2?w.scene2!.enteredAt:0,executed:[],transitions:[],budgets:{}};
  // Legacy counters are retained, not reset on migration. Unknown historic usage is charged to
  // the reached chapter; this is conservative and never creates extra budget by loading a save.
  for(let i=1;i<=6;i++)w.story.budgets[i]={calls:i===chapter?w.calls:0,tokens:i===chapter?w.tokens:0};
  for(const e of w.events)e.chapter??=w.scene3&&e.time>=w.scene3.enteredAt?3:w.scene2&&e.time>=w.scene2.enteredAt?2:1;
  for(const r of w.replies||[])r.chapter??=w.events.find(e=>e.id===r.eventId)?.chapter??chapter;
  for(const a of w.actors){a.y??=0;a.area??='bar';}
}
export function chapterOf(w:World,eventId?:string){return w.events.find(e=>e.id===eventId)?.chapter??w.story?.chapter??1;}
export function enterChapter(g:Engine,chapter:number,phase:string){
  const s=g.world.story;if(!s||s.chapter===chapter)return;
  s.transitions.push({from:s.chapter,to:chapter,at:g.world.elapsed});s.chapter=chapter;s.phase=phase;s.stageAt=s.enteredAt=g.world.elapsed;
  g.world.jobs=g.world.jobs.filter(j=>chapterOf(g.world,j.eventId)===chapter);
  for(const a of g.world.actors){a.pending=undefined;a.pendingParent=undefined;a.conversationTarget='';a.posture='stand';}
}
export function phase(g:Engine,value:string){const s=g.world.story;if(s&&s.phase!==value){s.phase=value;s.stageAt=g.world.elapsed;}}
export function once(g:Engine,key:string,action:()=>void){const s=g.world.story;if(!s||s.executed.includes(key))return false;s.executed.push(key);action();return true;}
export function reserveBudget(w:World,chapter:number,tokens:number){
  const b=w.story?.budgets[chapter]??w;
  // Call limit removed: keep the ledger counting for display/migration, never block generation.
  b.calls++;b.tokens+=tokens;if(b!==w){w.calls++;w.tokens+=tokens;}return true;
}
export function settleBudget(w:World,chapter:number,reserved:number,actual:number){
  if(!Number.isFinite(actual)||actual<0)return;
  const delta=actual-reserved,b=w.story?.budgets[chapter]??w;b.tokens+=delta;if(b!==w)w.tokens+=delta;
}
export function currentRequest(w:World,eventId:string){return !w.story||chapterOf(w,eventId)===w.story.chapter;}
