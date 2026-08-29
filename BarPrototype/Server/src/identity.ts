import type {ContextProfile,IdentityPack,Persona,Scenario,SharedPersona} from './types.js';

// Identity adaptation lives here as pure, deterministic functions. It never calls the model:
// the pack selection must be reproducible (same input + same seed -> same pack) and must not
// spend the per-night model budget. The raw player input never leaves this module except as a
// sanitised ContextProfile; it is never handed to a Character Agent.

export const DEFAULT_PACK='urban_capital_default';

const DOMAIN_PACK:Record<string,string>={investment:'investment_market',founder:'founder_life_transition',technology:'technology_product',creative:'creative_professional'};

const DOMAIN_KEYWORDS:Record<string,string[]> = {
  investment:['私募','PE','VC','基金','资管','二级市场','投行','FA','LP','GP','投研','并购','家办','尽调','估值','融资','投资'],
  founder:['创业','创始人','联合创始人','找钱','公司出售','离职','gap','休息','空窗','转型','自由职业'],
  technology:['产品经理','工程师','开发者','程序员','技术','产品','研发','架构','代码','设计','上线','PM'],
  creative:['创意','媒体','内容','写作','编剧','导演','音乐','艺术','策划','品牌','广告','博主']
};

// Extract only what the player explicitly expressed. Never infer wealth, orientation, family,
// romantic history, mental state or real-world relationships from a career prompt.
export function buildProfile(answers:Record<string,string>|undefined,freeText:string|undefined):ContextProfile{
  const profile:ContextProfile={domain:'',career_stage:'',capital_role:'',work_life_balance:'',current_transition:'',preferred_topic_density:'',confidence:0,consent_scope:'context_only'};
  const text=(freeText||'').slice(0,200);
  const domainTag=(answers?.domain==='skip'?'':answers?.domain||'');
  if(DOMAIN_PACK[domainTag]){profile.domain=domainTag;profile.confidence=Math.max(profile.confidence,.9);}
  const stageTag=answers?.career_stage==='skip'?'':answers?.career_stage||'';
  if(stageTag){profile.career_stage=stageTag;profile.confidence=Math.max(profile.confidence,.8);}
  const densityTag=answers?.preferred_topic_density==='skip'?'':answers?.preferred_topic_density||'';
  if(densityTag){profile.preferred_topic_density=densityTag;profile.confidence=Math.max(profile.confidence,.6);}

  // Free text only fills gaps the choices left open; it cannot override an explicit choice.
  if(!profile.domain){
    const hits=Object.entries(DOMAIN_KEYWORDS)
      .map(([domain,words])=>[domain,words.filter(w=>text.includes(w)).length] as const)
      .filter(([,n])=>n>0);
    if(hits.length===1)profile.domain=hits[0][0];
    else if(hits.length>1)profile.domain=''; // ambiguous: leave unknown, do not guess a specific desk
    if(hits.length===1)profile.confidence=Math.max(profile.confidence,.55);
  }
  if(!profile.career_stage){
    if(/离职|出售|退出|空窗|gap|休息/.test(text)){profile.career_stage='gap';profile.current_transition='recent_exit_or_gap';profile.work_life_balance='transition';}
    else if(/刚起步|找方向|第一轮|找钱|融资/.test(text)){profile.career_stage='starting';profile.current_transition='seeking_resources';}
  }
  if(/私募|基金|LP|GP|募资|投研/.test(text))profile.capital_role='fund_side';
  return profile;
}

export function selectPack(profile:ContextProfile,packs:Record<string,IdentityPack>,seed:number=821){
  void seed; // selection is deterministic; seed is accepted for a stable signature and future tie-breaking.
  const target=DOMAIN_PACK[profile.domain];
  if(target&&packs[target])return {packId:target,confidence:profile.confidence};
  // Unknown or ambiguous domain: keep the default identity, adjust topic density only.
  const fallback=packs[DEFAULT_PACK]?DEFAULT_PACK:Object.keys(packs)[0];
  return {packId:fallback,confidence:profile.confidence};
}

// Compressed per-decision brief for the model. Never the full persona or full corpus: it carries
// the invariant core, the voice essentials, the current occupational shell and at most
// maxNewConcepts corpus terms. It provides no conclusions and does not override Voice.
export function identityBrief(scenario:Scenario,packId:string,actorId:string,topicPreference?:string){
  const pack=scenario.identityPacks[packId]||scenario.identityPacks[DEFAULT_PACK];
  const persona=scenario.personas[actorId] as Persona|undefined;
  const packActor=pack.actors[actorId];
  if(!persona)return null;
  return {
    name:persona.name,
    title:persona.title,
    oneLine:persona.oneLine,
    userDefault:persona.userDefault,
    everyday:{life:packActor?.life||[],closeSelf:persona.voice.closeSelf,contrast:persona.voice.contrast,breaking:persona.voice.breaking},
    humanity:(scenario.personas.shared as SharedPersona)?.humanity,
    invariant:persona.invariant,
    self:persona.voice.formalSelf+' '+(actorId==='D'&&persona.emojiRule?persona.emojiRule:''),
    userAddress:persona.voice.userAddress,
    rhythm:persona.voice.rhythm,
    publicRole:packActor?.publicRole||'',
    position:packActor?.position||'',
    corpus:(packActor?.corpus||[]).slice(0,pack.maxNewConcepts),
    topicMix:topicPreference==='life'?{professional:.15,life:.85}:topicPreference==='work'?{professional:.65,life:.35}:pack.topicMix,
    packLabel:pack.label
  };
}
