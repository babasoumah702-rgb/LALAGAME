// Run via Explorer ShellExecute, not the development app's child-process environment.
import {writeFileSync,existsSync} from 'node:fs';
import {join,dirname} from 'node:path';
import {fileURLToPath} from 'node:url';
import {homedir} from 'node:os';
import {modelConfig} from '../BarPrototype/Builds/Scene0-Windows/Server/dist/model.js';
const project=join(dirname(fileURLToPath(import.meta.url)),'../BarPrototype');
const config=modelConfig();
const report={pid:process.pid,configured:!!config.key,model:config.model,base:config.base,
  legacyConfigVisible:existsSync(join(process.env.LOCALAPPDATA,'LALAGAME/private/model.env')),
  stableConfigVisible:existsSync(join(homedir(),'.lalagame/private/model.env'))};
writeFileSync(join(project,'Verification/normal-launch-config.json'),JSON.stringify(report,null,2));
