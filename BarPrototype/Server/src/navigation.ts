import type {Navigation,Point} from './types.js';
export const distance=(a:Point,b:Point)=>Math.hypot(a.x-b.x,a.z-b.z,(a.y??0)-(b.y??0));
export class Navigator {
  blocked:Set<number>;
  constructor(public data:Navigation) { this.blocked=new Set(data.blocked); }
  index(p:Point) { return Math.floor((p.z-this.data.minZ)/this.data.cell)*this.data.width+Math.floor((p.x-this.data.minX)/this.data.cell); }
  point(i:number):Point { return {x:this.data.minX+(i%this.data.width+.5)*this.data.cell,z:this.data.minZ+(Math.floor(i/this.data.width)+.5)*this.data.cell}; }
  walkable(p:Point) { const x=Math.floor((p.x-this.data.minX)/this.data.cell), z=Math.floor((p.z-this.data.minZ)/this.data.cell);return x>=0&&z>=0&&x<this.data.width&&z<this.data.height&&!this.blocked.has(z*this.data.width+x); }
  nearest(p:Point) { if(this.walkable(p))return this.point(this.index(p));let best:Point|undefined,d=Infinity;for(let i=0;i<this.data.width*this.data.height;i++){if(this.blocked.has(i))continue;const q=this.point(i),n=distance(p,q);if(n<d){d=n;best=q;}}return best??p; }
  path(from:Point,to:Point):Point[] {
    const start=this.index(this.nearest(from)), goal=this.index(this.nearest(to));
    const open=new Set([start]), cost=new Map([[start,0]]), parents=new Map<number,number>();
    while(open.size){let cur=-1,score=Infinity;for(const i of open){const s=(cost.get(i)??Infinity)+distance(this.point(i),this.point(goal));if(s<score){score=s;cur=i;}}
      if(cur===goal){const result:Point[]=[];while(cur!==start){result.unshift(this.point(cur));cur=parents.get(cur)!;}return result;}
      open.delete(cur);const x=cur%this.data.width,z=Math.floor(cur/this.data.width);
      for(const [dx,dz] of [[1,0],[-1,0],[0,1],[0,-1],[1,1],[1,-1],[-1,1],[-1,-1]]){
        const nx=x+dx,nz=z+dz,n=nz*this.data.width+nx;
        if(nx<0||nz<0||nx>=this.data.width||nz>=this.data.height||this.blocked.has(n))continue;
        if(dx&&dz&&(this.blocked.has(z*this.data.width+nx)||this.blocked.has(nz*this.data.width+x)))continue;
        const c=cost.get(cur)!+Math.hypot(dx,dz)*this.data.cell;
        if(c<(cost.get(n)??Infinity)){cost.set(n,c);parents.set(n,cur);open.add(n);}
      }
    }return [];
  }
  visible(a:Point,b:Point) { const steps=Math.ceil(distance(a,b)/.12);for(let i=1;i<steps;i++){const x=a.x+(b.x-a.x)*i/steps,z=a.z+(b.z-a.z)*i/steps;if(this.data.walls.some(w=>Math.abs(x-w.x)<w.w/2&&Math.abs(z-w.z)<w.h/2))return false;}return true; }
}
export const emptyNavigation:Navigation={cell:.25,minX:-8.5,minZ:-5,width:60,height:40,blocked:[],walls:[]};
