import {readdirSync,statSync,openSync,readSync,closeSync,writeFileSync} from 'node:fs';
import {join,relative,resolve,dirname} from 'node:path';
import {fileURLToPath} from 'node:url';
import {modelConfig} from '../BarPrototype/Server/dist/model.js';
const project=resolve(dirname(fileURLToPath(import.meta.url)),'../BarPrototype');
const root=resolve(process.argv[2]||'');
if(!root.startsWith(join(project,'Builds')+'/')&&!root.startsWith(join(project,'Builds')+'\\'))throw Error('Expected a runtime inside project Builds');
const key=Buffer.from(modelConfig().key||'');const issues=[];let files=0,bytes=0;
function scan(dir){for(const item of readdirSync(dir,{withFileTypes:true})){
  const path=join(dir,item.name),name=relative(root,path).replaceAll('\\','/');
  if(item.isSymbolicLink()){issues.push('Unexpected symlink: '+name);continue;}
  if(/DoNotShip/i.test(name))issues.push('Debug symbol directory: '+name);
  if(item.isDirectory()){if(item.name==='private')issues.push('Private directory: '+name);scan(path);continue;}
  if(/(^|\/)(\.env|model\.env)$|\.(db|sqlite|sqlite3|log)(-|$)/i.test(name))issues.push('Private artifact: '+name);
  files++;bytes+=statSync(path).size;
  if(key.length){const fd=openSync(path,'r'),buffer=Buffer.alloc(128*1024);let carry=Buffer.alloc(0),length;
    try{while((length=readSync(fd,buffer,0,buffer.length,null))>0){const chunk=Buffer.concat([carry,buffer.subarray(0,length)]);if(chunk.includes(key)){issues.push('Credential found in: '+name);break;}carry=Buffer.from(chunk.subarray(Math.max(0,chunk.length-key.length)));}}finally{closeSync(fd);}
  }
}}
scan(root);const report={passed:issues.length===0,files,bytes,keyConfigured:key.length>0,issues,note:'Checks all staged runtime and evidence files for configured credentials, private config, saves, logs and debug artifacts. Only file names and counts are recorded.'};
writeFileSync(join(project,'Verification/scene1-package-audit.json'),JSON.stringify(report,null,2));console.log(JSON.stringify(report));if(issues.length)process.exitCode=1;
