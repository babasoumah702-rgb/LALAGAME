import type {Engine} from './engine.js';
import {distance} from './navigation.js';
export function runBeats(g:Engine){
  const w=g.world, u=g.actor('USER');
  for(const b of g.scenario.beats){
    let availableAt=b.at;
    if(b.effect==='cards'&&(w.role==='social_guest'||w.entryIntent==='meet_people'))availableAt-=45;
    if(b.effect==='signal'&&w.entryIntent==='romantic_open')availableAt-=30;
    if(b.effect==='cards'&&w.entryIntent==='low_energy')availableAt+=35;
    if(w.elapsed<availableAt||w.beatIds.includes(b.id))continue;
    let ready=b.condition==='always';
    if(b.condition==='b_available')ready=g.actor('B').active&&!g.actor('B').withdrawn;
    if(b.condition==='past_at_seat')ready=!!w.flags.pastDrink&&g.zone(u).id==='seat13';
    if(b.condition==='alone_at_seat')ready=g.zone(u).id==='seat13'&&[0,1,2].every(i=>typeof w.flags['withdrawAt'+i]==='number'&&w.elapsed-Number(w.flags['withdrawAt'+i])<=180)&&w.elapsed-Number(w.flags.lastApproach??-100)>90&&!w.actors.some(a=>a.active&&a.id!=='USER'&&distance(a,u)<1.5);
    if(!ready)continue;
    w.beatIds.push(b.id);
    switch(b.effect){
      case 'opening':g.emit('message','BARTENDER','USER','observe',w.night===1?b.text:'欢迎回来。昨晚没说完的，不必假装忘记。');break;
      case 'signal':g.emit('message','B','USER','probe',b.text);break;
      case 'cards':w.flags.cardsOffered=true;g.emit('system','OWNER','USER','invite',b.text);break;
      case 'enter_a':case 'enter_c':case 'enter_d':{
        const id=b.effect.slice(-1).toUpperCase(),a=g.actor(id);
        const p=g.navigation.nearest(g.location('entrance'));
        a.active=true;a.x=p.x;a.z=p.z;a.withdrawn=false;
        const entry=g.emit('system','OWNER',id,'arrival',b.text);g.go(a,g.location(a.home));
        if(id==='A')w.jobs.push({actor:id,eventId:entry.id,due:w.elapsed+12});
        if(id==='D')g.emit('message','D','USER','approach',b.text);
        break;
      }
      case 'last_call':{
        w.flags.lastCall=true;
        const candidates=w.actors.filter(a=>a.active&&!['USER','OWNER'].includes(a.id));
        candidates.sort((a,b)=>b.relations.USER.tension-a.relations.USER.tension);
        w.flags.lastTarget=candidates[0]?.id??'BARTENDER';
        g.emit('system','OWNER','USER','last_call',b.text);break;
      }
      case 'close':g.finish();break;
    }
  }
}
