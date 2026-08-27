import type {Actor,Event,Perception,Scenario,World} from './types.js';
import {actor,zone} from './world.js';
import {distance,Navigator} from './navigation.js';
export function perceive(s:Scenario,w:World,nav:Navigator,a:Actor,e:Event):Perception|undefined {
  if(!a.active)return;
  const result=(source:string,level:string,text:string,confidence:number):Perception=>({actor:a.id,source,level,text,confidence});
  if(e.type==='system')return result('public','full',e.text,1);
  const speaker=actor(w,e.actor),d=distance(speaker,a),loc=zone(s,speaker),same=zone(s,a).id===loc.id;
  if(a.id===e.actor||(a.id===e.target&&(e.type==='message'||(d<=s.rules.fullHear&&nav.visible(speaker,a)))))return result(e.type==='message'?'shared':'direct','full',e.text,1);
  if(e.type==='message')return;
  const visible=nav.visible(speaker,a),dx=speaker.x-a.x,dz=speaker.z-a.z;
  const looking=d<.6||(dx*Math.sin(a.yaw*Math.PI/180)+dz*Math.cos(a.yaw*Math.PI/180))/Math.max(d,.01)>-.35;
  if(e.privacy==='private')return visible&&looking&&d<4?result('observed','gesture',`${speaker.name} 递出一张纸条；你不知道内容。`,1):undefined;
  const hearingScale=(same?1:1-loc.privacy*.8)*(visible?1:.3);
  if(d<=s.rules.fullHear*hearingScale)return result('overheard','full',e.text,.9);
  if(d<=s.rules.partialHear*hearingScale)return result('overheard','partial',`${speaker.name} 提到「${e.text.slice(0,5)}……」，后面没有听清。`,.4);
  if(e.type==='movement'&&visible&&looking&&d<s.rules.sight)return result('observed','gesture',e.text,1);
}
