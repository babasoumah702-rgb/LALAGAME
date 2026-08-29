import type {Engine} from './engine.js';
import {Navigator,distance} from './navigation.js';
import {NightNavigator,areaOf} from './night-navigation.js';
import type {Point} from './types.js';
const progress=new WeakMap<Engine,Map<string,{point:Point;at:number}>>();
// Replan a stalled walk around real character footprints. The client still walks every segment
// through its CharacterController; no actor positions or destinations are teleported.
export function advanceCrowd(g:Engine){
 if(!g.world.story||!(g.navigation instanceof NightNavigator))return;
 let known=progress.get(g);if(!known){known=new Map();progress.set(g,known);}
 const now=g.world.elapsed;
 for(const a of g.world.actors){
  const before=known.get(a.id);
  if(!before||!a.route.length||distance(a,before.point)>.16){known.set(a.id,{point:{x:a.x,z:a.z,y:a.y,area:a.area},at:now});continue;}
  if(now-before.at<2||areaOf(a)!=='bar')continue;
  before.at=now;
  const others=g.world.actors.filter(o=>o.active&&o.id!==a.id&&areaOf(o)==='bar');
  if(!a.route.slice(0,7).some(p=>others.some(o=>distance(p,o)<.58)))continue;
  const base=g.navigation.base,dynamic=new Navigator(base.data);dynamic.blocked=new Set(base.blocked);
  for(const o of others)for(let i=0;i<base.data.width*base.data.height;i++)if(distance(base.point(i),o)<.53)dynamic.blocked.add(i);
  dynamic.blocked.delete(base.index(a));
  const route=new NightNavigator(dynamic).path(a,a.route.at(-1)!);
  if(route.length){a.route=route;a.routeVersion=(a.routeVersion??0)+1;}
 }
}
