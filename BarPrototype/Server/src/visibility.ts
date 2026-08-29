import {areaOf} from './night-navigation.js';
import type {Actor,Event,Perception,Scenario,World} from './types.js';
import {actor,zone} from './world.js';
import {distance,Navigator} from './navigation.js';
import {sceneOneAlias} from './scene-one.js';
import {introActive,introPerception} from './intro.js';
export function perceive(s:Scenario,w:World,nav:Navigator,a:Actor,e:Event):Perception|undefined {
  if(!a.active)return;
  if(e.type==='preentry')return introPerception(s,w,nav,a,e);
  if(a.id==='USER'&&introActive(w))return;
  const result=(source:string,level:string,text:string,confidence:number):Perception=>({actor:a.id,source,level,text,confidence});
  if(e.type==='system'){if(w.story&&areaOf(a)!=='bar'&&!['power_cut','close'].includes(e.intent))return;return result('public','full',e.text,1);}
  const speaker=actor(w,e.actor),d=distance(speaker,a),loc=zone(s,speaker),same=zone(s,a).id===loc.id;
  if(e.intent==='tarot_question'&&e.objectTarget==='tarot_card'){const table={x:1.65,z:-1.8,y:0,area:'bar'};return areaOf(a)==='bar'&&distance(a,table)<3.2&&nav.visible(a,table)?result('observed','full',e.text,1):undefined;}
  if(a.id===e.actor||(a.id===e.target&&(e.type==='message'||(!['action','movement'].includes(e.type)&&d<=s.rules.fullHear&&nav.visible(speaker,a)))))return result(e.type==='message'?'shared':'direct','full',e.text,1);
  if(e.type==='message')return;
  const speakerName=w.scene1&&a.id==='USER'&&!w.scene1.knownNames[speaker.id]?(sceneOneAlias(speaker.id)||speaker.name):speaker.name;
  const visible=nav.visible(speaker,a),dx=speaker.x-a.x,dz=speaker.z-a.z;
  const looking=d<.6||(dx*Math.sin(a.yaw*Math.PI/180)+dz*Math.cos(a.yaw*Math.PI/180))/Math.max(d,.01)>-.35;
  if(w.story&&Math.abs((speaker.y??0)-(a.y??0))>2.2)return;
  if(e.privacy==='private')return visible&&looking&&d<4?result('observed','gesture',`${speakerName} 递出一张纸条；你不知道内容。`,1):undefined;
  if(w.scene1&&e.type==='action')return visible&&looking&&d<s.rules.sight?result('observed','full',e.text,1):undefined;
  // A gaze is only information to whoever was watching. Missing it is normal, and never backfilled:
  // that asymmetry is the whole point of the tarot round.
  if(w.scene3&&e.type==='movement'&&e.intent==='gaze')return visible&&looking&&d<s.rules.sight?result('observed','gesture',e.text,.9):undefined;
  if(e.privacy==='private')return visible&&looking&&d<4?result('observed','gesture',`${speakerName} 递出一张纸条；你不知道内容。`,1):undefined;
  if(w.story&&areaOf(speaker)!==areaOf(a)&&(['rooftop','stairs'].includes(areaOf(speaker))||['rooftop','stairs'].includes(areaOf(a))))return;
  const doorScale=w.late&&!w.late.doorOpen&&areaOf(speaker)!==areaOf(a)?.2:1;
  const hearingScale=(same?1:1-loc.privacy*.8)*(visible?1:.3)*doorScale;
  if(d<=s.rules.fullHear*hearingScale)return result('overheard','full',e.text,.9);
  if(d<=s.rules.partialHear*hearingScale)return result('overheard','partial',`${speakerName} 提到「${e.text.slice(0,5)}……」，后面没有听清。`,.4);
  if(e.type==='movement'&&visible&&looking&&d<s.rules.sight)return result('observed','gesture',e.text,1);
}
