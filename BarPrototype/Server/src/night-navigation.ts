import {Navigator,distance} from './navigation.js';
import type {Point} from './types.js';
export const NIGHT={corridor:{x:3.8,z:-5.95,y:0,area:'corridor'},stairs:{x:7.45,z:-5.65,y:0,area:'stairs'},rooftop:{x:5.8,z:4,y:4.2,area:'rooftop'}};
const entry:Point={x:-1,z:-4.6,y:0,area:'bar'},hall:Point={x:-1,z:-5.95,y:0,area:'corridor'},bottom:Point={x:7.45,z:-5.7,y:0,area:'stairs'},top:Point={x:7.45,z:2.5,y:4.2,area:'rooftop'};
export function areaOf(p:Point){if(p.area)return p.area;if((p.y??0)>4&&p.z>=2.3)return 'rooftop';if(p.x>=6.8&&p.z>=-5.85&&p.z<2.5)return 'stairs';return p.z<-5.18?'corridor':'bar';}
export function heightAt(p:Point){const a=areaOf(p);return a==='rooftop'?4.2:a==='stairs'?Math.max(0,Math.min(4.2,(p.z+5.7)/8.2*4.2)):0;}
function point(p:Point):Point{const area=areaOf(p);return {...p,area,y:heightAt({...p,area})};}
function line(a:Point,b:Point){const n=Math.max(1,Math.ceil(distance(a,b)/.3));return Array.from({length:n},(_,i)=>point({x:a.x+(b.x-a.x)*(i+1)/n,z:a.z+(b.z-a.z)*(i+1)/n,y:(a.y??0)+((b.y??0)-(a.y??0))*(i+1)/n,area:areaOf(a)===areaOf(b)?b.area:undefined}));}
// A portal graph layered over the original bar A*. No modification to the classic navigation grid.
export class NightNavigator extends Navigator{
  constructor(public base:Navigator){super(base.data);}
  override walkable(p:Point){const a=areaOf(p);if(a==='bar')return this.base.walkable(p);if(a==='corridor')return p.z>=-6.55&&p.z<=-5.18&&p.x>=-2.25&&p.x<=8.1;if(a==='stairs')return p.x>=6.85&&p.x<=8.05&&p.z>=-5.85&&p.z<=2.55;return a==='rooftop'&&p.x>=-5.45&&p.x<=8.05&&p.z>=2.5&&p.z<=9;}
  override nearest(p:Point):Point{const a=areaOf(p);if(a==='bar')return {...this.base.nearest(p),area:'bar',y:0};if(this.walkable(p))return point(p);return point(a==='corridor'?NIGHT.corridor:a==='stairs'?bottom:NIGHT.rooftop);}
  override path(from:Point,to:Point):Point[]{
    const a=areaOf(from),b=areaOf(to),goal=this.nearest(to);
    if(a===b)return a==='bar'?this.base.path(from,goal).map(p=>({...p,area:'bar',y:0})):line(point(from),goal);
    const order=['bar','corridor','stairs','rooftop'],ia=order.indexOf(a),ib=order.indexOf(b);if(ia<0||ib<0)return [];
    let current=point(from);const out:Point[]=[];
    const append=(p:Point)=>{const segment=areaOf(current)==='bar'&&areaOf(p)==='bar'?this.base.path(current,p).map(q=>({...q,y:0,area:'bar'})):line(current,p);if(!segment.length&&distance(current,p)>.35)return false;out.push(...segment);current=p;return true;};
    for(let i=ia;i!==ib;i+=Math.sign(ib-ia)){
      if(i===0){if(!append(entry)||!append(hall))return [];}
      else if(i===1&&ib>i){if(!append({...bottom,z:-5.95,area:'corridor'})||!append(bottom))return [];}
      else if(i===2&&ib>i){if(!append({...top,z:2.48,area:'stairs'})||!append(top))return [];}
      else if(i===3){if(!append(top)||!append({...top,z:2.48,area:'stairs'}))return [];}
      else if(i===2){if(!append(bottom)||!append({...bottom,z:-5.95,area:'corridor'}))return [];}
      else {if(!append(hall)||!append(entry))return [];}
    }
    if(!append(goal))return [];return out;
  }
  override visible(a:Point,b:Point){
    const aa=areaOf(a),bb=areaOf(b);if(aa===bb)return aa==='bar'?this.base.visible(a,b):true;
    if([aa,bb].includes('rooftop'))return [aa,bb].includes('stairs')&&a.z>1.6&&b.z>1.6;
    if([aa,bb].includes('bar'))return [aa,bb].includes('corridor')&&Math.abs(a.x+1)<.8&&Math.abs(b.x+1)<.8&&distance(a,b)<2.5;
    return a.x>6.65&&b.x>6.65&&a.z<-4.9&&b.z<-4.9;
  }
  acceptsPosition(from:Point,to:Point){if(!this.walkable(to)||distance(from,to)>1.2||Math.abs((to.y??heightAt(to))-heightAt(to))>.42)return false;
    if(areaOf(from)!==areaOf(to)&&!this.visible(from,to))return false;
    return Math.abs(heightAt(from)-heightAt(to))<.8;
  }
}
