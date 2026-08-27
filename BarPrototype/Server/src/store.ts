import {DatabaseSync} from 'node:sqlite';
import type {World,Decision,Job} from './types.js';
export class Store {
  db:DatabaseSync;
  constructor(path:string){this.db=new DatabaseSync(path);this.db.exec('PRAGMA journal_mode=WAL; PRAGMA foreign_keys=ON; CREATE TABLE IF NOT EXISTS sessions(id TEXT PRIMARY KEY, player_id TEXT NOT NULL, updated_at TEXT NOT NULL, snapshot TEXT NOT NULL); CREATE TABLE IF NOT EXISTS event_log(world_id TEXT NOT NULL, seq INTEGER NOT NULL, payload TEXT NOT NULL, PRIMARY KEY(world_id,seq));');}
  save(w:World){this.db.exec('BEGIN IMMEDIATE');try{this.db.prepare('INSERT INTO sessions VALUES(?,?,?,?) ON CONFLICT(id) DO UPDATE SET updated_at=excluded.updated_at,snapshot=excluded.snapshot').run(w.id,w.playerId,w.updatedAt,JSON.stringify(w));const put=this.db.prepare('INSERT OR IGNORE INTO event_log VALUES(?,?,?)');for(const e of w.events)put.run(w.id,e.seq,JSON.stringify(e));this.db.exec('COMMIT');}catch(e){this.db.exec('ROLLBACK');throw e;}}
  load(id:string,playerId:string):World|undefined{const row=this.db.prepare('SELECT snapshot FROM sessions WHERE id=? AND player_id=?').get(id,playerId) as {snapshot:string}|undefined;return row?JSON.parse(row.snapshot):undefined;}
  list(playerId:string){return this.db.prepare('SELECT id,snapshot FROM sessions WHERE player_id=? ORDER BY updated_at DESC LIMIT 20').all(playerId).map((r:any)=>{const s=JSON.parse(r.snapshot);return{id:r.id,role:s.role,status:s.status,night:s.night,elapsed:s.elapsed,updatedAt:s.updatedAt};});}
  recordDecision(w:World,job:Job,decision:Decision,accepted:boolean){
    this.db.exec('CREATE TABLE IF NOT EXISTS decision_log(world_id TEXT, ordinal INTEGER PRIMARY KEY AUTOINCREMENT, event_id TEXT, actor TEXT, mode TEXT, elapsed REAL, payload TEXT, accepted INTEGER)');
    this.db.prepare('INSERT INTO decision_log(world_id,event_id,actor,mode,elapsed,payload,accepted) VALUES(?,?,?,?,?,?,?)')
      .run(w.id,job.eventId,job.actor,w.modelMode,w.elapsed,JSON.stringify(decision),accepted?1:0);
  }
  decisions(id:string,playerId:string){
    if(!this.load(id,playerId))return [];
    this.db.exec('CREATE TABLE IF NOT EXISTS decision_log(world_id TEXT, ordinal INTEGER PRIMARY KEY AUTOINCREMENT, event_id TEXT, actor TEXT, mode TEXT, elapsed REAL, payload TEXT, accepted INTEGER)');
    return this.db.prepare('SELECT event_id,actor,mode,elapsed,payload,accepted FROM decision_log WHERE world_id=? ORDER BY ordinal').all(id);
  }
  close(){this.db.close();}
}
