using System;
using System.Linq;
using UnityEngine;
using UnityEngine.UI;

namespace LastCall
{
    public sealed partial class LastCallInterface
    {
        private int entryPage,entryRole=-2,entryIntent=-2,entryStyle=-2;
        private string entrySettingsSection="",entryModeOverride="",entryBackground="";
        private string entryApiBase="",entryApiModel="",entryApiKey="",entryApiStatus="";
        private bool entryApiSaving;
        private string choiceDomain="skip",choiceCareer="skip",choiceDensity="skip";
        private readonly string[] entryModes={"solo","friend_invited","event_guest"};
        private readonly string[] entryModeNames={"独自来","朋友邀约","活动参与"};

        private void BuildEntry()
        {
            Clear(root);entryVisible=true;introUIVisible=false;
            entryPanel=Box("Entry",root,0,0,Width,Height);Fill(entryPanel);
            var plate=OtomeArt.Bar();if(plate){var photo=Box("Painting",entryPanel,0,0,Width,Height);Fill(photo);var image=photo.gameObject.AddComponent<RawImage>();image.texture=plate;image.raycastTarget=false;}
            Fill(Panel("Veil",entryPanel,0,0,Width,Height,new Color(.035f,.03f,.04f,.72f)));
            var form=Panel("Arrival",entryPanel,(Width-900)/2,(Height-650)/2,900,650,ink);
            Panel("Brass",form,0,0,3,650,new Color(.64f,.45f,.28f));Label(form,"LA LA LAND",34,22,820,46,32);
            var config=Client.Bootstrap;
            if(config==null){Label(form,"今晚见。",34,88,780,40,25);Label(form,Client.Status,34,210,780,80,20);ActionButton(form,"退出",34,535,180,44,Quit);return;}
            if(entryPage==0){BuildEntryLanding(form,config);return;}
            if(entryPage>=1&&entryPage<=3){BuildEntryQuestion(form,config);return;}
            BuildEntrySettings(form,config);
        }

        private void BuildEntryLanding(Transform form,BootstrapDto config)
        {
            Label(form,"今晚见。",34,82,780,42,25);Label(form,"新游戏只需回答三个问题；每页选择一次，点「下一步」继续。",34,130,780,34,16,muted);
            ActionButton(form,"开始新的夜晚",34,205,520,58,()=>{entryPage=1;BuildEntry();},true);
            var save=config.sessions?.FirstOrDefault();if(save!=null)ActionButton(form,"继续上次的夜晚",34,280,520,52,()=>Client.OpenSession(new SessionRequest{mode="resume",sessionId=save.id}));
            ActionButton(form,"更多设置",34,349,250,46,()=>{entryPage=4;entrySettingsSection="";BuildEntry();});
            ActionButton(form,online?"在线模型 · 开":"离线规则模式",304,349,250,46,()=>{online=!online;BuildEntry();});
            ActionButton(form,config.modelConfigured?"模型 API · 已配置":"填写模型 API",574,349,250,46,OpenModelSettings);
            Label(form,config.modelConfigured?"模型："+config.model+" · 密钥只保存在本机，不会显示。":"未配置密钥时可选择离线规则模式；也可以先填写自己的模型 API。",34,418,790,44,14,muted);ActionButton(form,"退出",34,540,180,44,Quit);
        }

        private void BuildEntryQuestion(Transform form,BootstrapDto config)
        {
            string title=entryPage==1?"今晚以什么身份来？":entryPage==2?"今晚想做什么？":"喜欢什么样的聊天风格？";
            Label(form,"新游戏  "+entryPage+" / 3",34,86,300,28,15,gold);var titleLabel=Label(form,title,0,126,900,50,29);titleLabel.alignment=TextAnchor.MiddleCenter;
            Label(form,"选择一项后，点击右下角「下一步」。",34,178,540,30,14,muted);
            var labels=entryPage==1?config.roles.Select(x=>x.name).ToArray():entryPage==2?config.intents.Select(x=>x.name).ToArray():config.styles.Select(x=>x.name).ToArray();
            int current=entryPage==1?entryRole:entryPage==2?entryIntent:entryStyle;
            // 竖排选项列，水平居中于表单：三页共用同一套布局，样式零差异。
            const float sx=307,sy=230,sw=286,sh=46,gap=20;
            for(int i=0;i<labels.Length;i++)
            {
                int index=i;var b=ActionButton(form,labels[i],sx,sy+i*(sh+gap),sw,sh,()=>SelectEntryAnswer(entryPage,index));
                b.name="Entry answer "+entryPage+" "+index;SetChoiceState(b,current==index,current!=index);
            }
            ActionButton(form,"返回",530,570,160,48,()=>{entryPage=entryPage==1?0:entryPage-1;BuildEntry();});
            var next=ActionButton(form,"下一步",706,570,160,48,()=>AdvanceEntry(),true);
            next.name="Entry next "+entryPage;next.interactable=current>=0;
        }

        private void SelectEntryAnswer(int page,int index)
        {
            if(page==1)entryRole=index;else if(page==2)entryIntent=index;else entryStyle=index;
            BuildEntry();
        }
        private void AdvanceEntry()
        {
            if(entryPage<3){entryPage=entryPage+1;BuildEntry();}else StartNewNight();
        }
        private void StartNewNight()
        {
            var config=Client.Bootstrap;if(config==null)return;
            string role=entryRole>=0&&entryRole<config.roles.Length?config.roles[entryRole].id:"passerby";
            string intent=entryIntent>=0&&entryIntent<config.intents.Length?config.intents[entryIntent].id:"observe_only";
            string style=entryStyle>=0&&entryStyle<config.styles.Length?config.styles[entryStyle].id:"natural";
            string mode=!string.IsNullOrEmpty(entryModeOverride)?entryModeOverride:role=="event_guest"||role=="staff"?"event_guest":"solo";
            Client.OpenSession(new SessionRequest{role=role,entryIntent=intent,style=style,online=online,mode="new",opening="scene0_v1",story="scene1_v1",entryMode=mode,entryContext=entryBackground,
                choices=new ChoiceAnswersDto{domain=choiceDomain,career_stage=choiceCareer,preferred_topic_density=choiceDensity}});
        }

        private void BuildEntrySettings(Transform form,BootstrapDto config)
        {
            Label(form,"更多设置",34,84,600,44,26);
            if(string.IsNullOrEmpty(entrySettingsSection))
            {
                Label(form,"这些内容全部可跳过，不影响三步主路径。",34,132,700,30,15,muted);
                var entries=new[]{("arrival","赴约方式"),("background","补充今晚为什么来"),("domain","行业方向"),("career_stage","当前阶段"),("preferred_topic_density","今晚话题"),("model_api","模型 API 设置")};
                for(int i=0;i<entries.Length;i++){string id=entries[i].Item1;ActionButton(form,entries[i].Item2,34+(i%2)*395,190+(i/2)*66,370,50,()=>{if(id=="model_api")OpenModelSettings();else{entrySettingsSection=id;BuildEntry();}});}
                ActionButton(form,online?"在线模型 · 开":"规则模式 · 离线",34,408,370,50,()=>{online=!online;BuildEntry();});ActionButton(form,"返回首页",34,550,220,44,()=>{entryPage=0;BuildEntry();},true);return;
            }
            if(entrySettingsSection=="model_api")BuildModelSettings(form);
            else if(entrySettingsSection=="background")
            {
                Label(form,"补充今晚为什么来 · 可跳过",34,140,760,34,20,gold);var background=InputBox(form,34,200,765,150);
                ((Text)background.placeholder).text="不填写也可以直接开始。最多 200 字。";background.text=entryBackground;background.onValueChanged.AddListener(v=>entryBackground=v);
                var skip=ActionButton(form,"跳过 · 不补充背景",34,378,765,48,()=>{entryBackground="";BuildEntry();});
                SetChoiceState(skip,string.IsNullOrWhiteSpace(entryBackground),!string.IsNullOrWhiteSpace(entryBackground));
            }else if(entrySettingsSection=="arrival")
            {
                Label(form,"赴约方式 · 可跳过",34,140,760,34,20,gold);
                for(int i=0;i<entryModes.Length;i++){int index=i;var b=ActionButton(form,entryModeNames[i],34,200+i*60,765,48,()=>{entryModeOverride=entryModes[index];BuildEntry();});SetChoiceState(b,entryModeOverride==entryModes[i],entryModeOverride!=entryModes[i]);}
                var skip=ActionButton(form,"跳过 · 根据身份自动匹配",34,380,765,48,()=>{entryModeOverride="";BuildEntry();});SetChoiceState(skip,string.IsNullOrEmpty(entryModeOverride),!string.IsNullOrEmpty(entryModeOverride));
            }else BuildOptionalChoice(form,config,entrySettingsSection);
            ActionButton(form,"返回更多设置",34,550,260,44,()=>{entrySettingsSection="";BuildEntry();},true);
        }
        private void OpenModelSettings()
        {
            var config=Client.Bootstrap;
            entryApiBase=string.IsNullOrWhiteSpace(config?.modelBase)?"https://api.deepseek.com":config.modelBase;
            entryApiModel=string.IsNullOrWhiteSpace(config?.model)?"deepseek-v4-flash":config.model;
            entryApiKey="";
            entryApiStatus=config?.modelConfigured==true?"已经配置密钥；留空保存会保留原密钥。":"请填写 OpenAI 兼容接口。密钥不会回显。";
            entryPage=4;entrySettingsSection="model_api";BuildEntry();
        }
        private void BuildModelSettings(Transform form)
        {
            Label(form,"模型 API 设置",34,92,760,38,25);
            Label(form,"接口地址",34,150,155,44,15,muted);var apiBase=InputBox(form,190,150,610,44);
            apiBase.lineType=InputField.LineType.SingleLine;apiBase.characterLimit=500;apiBase.text=entryApiBase;((Text)apiBase.placeholder).text="https://api.deepseek.com";apiBase.onValueChanged.AddListener(v=>entryApiBase=v);
            Label(form,"模型名称",34,212,155,44,15,muted);var model=InputBox(form,190,212,610,44);
            model.lineType=InputField.LineType.SingleLine;model.characterLimit=120;model.text=entryApiModel;((Text)model.placeholder).text="例如 deepseek-v4-flash";model.onValueChanged.AddListener(v=>entryApiModel=v);
            Label(form,"API Key",34,274,155,44,15,muted);var key=InputBox(form,190,274,610,44);
            key.lineType=InputField.LineType.SingleLine;key.characterLimit=2000;key.contentType=InputField.ContentType.Password;key.asteriskChar='●';key.text=entryApiKey;
            ((Text)key.placeholder).text=Client.Bootstrap?.modelConfigured==true?"已配置；留空表示保留":"只保存在当前 Windows 用户目录";key.onValueChanged.AddListener(v=>entryApiKey=v);key.ForceLabelUpdate();
            Label(form,"仅允许 HTTPS；本机 localhost / 127.0.0.1 可使用 HTTP。配置写入用户私有目录，不进入游戏包、存档或日志。",34,330,766,54,14,muted);
            var save=ActionButton(form,entryApiSaving?"正在保存…":"保存并启用在线模型",34,402,370,48,()=>SubmitModelSettings(false),true);save.interactable=!entryApiSaving;
            var clear=ActionButton(form,"清除本机密钥",430,402,370,48,()=>SubmitModelSettings(true));clear.interactable=!entryApiSaving&&Client.Bootstrap?.modelConfigured==true;
            Label(form,entryApiStatus,34,462,766,58,14,entryApiStatus.Contains("失败")?gold:cream);
        }
        private void SubmitModelSettings(bool clear)
        {
            if(entryApiSaving)return;
            entryApiSaving=true;entryApiStatus=clear?"正在清除本机密钥…":"正在保存模型配置…";BuildEntry();
            Client.ConfigureModel(new ModelConfigRequestDto{@base=entryApiBase,model=entryApiModel,key=clear?"":entryApiKey,
                keepKey=!clear&&string.IsNullOrWhiteSpace(entryApiKey)&&Client.Bootstrap?.modelConfigured==true,clearKey=clear},(ok,message)=>{
                entryApiSaving=false;entryApiStatus=message;if(ok){entryApiKey="";online=Client.Bootstrap?.modelConfigured==true;}BuildEntry();
            });
        }
        private void BuildOptionalChoice(Transform form,BootstrapDto config,string id)
        {
            var choice=config.choices?.FirstOrDefault(c=>c.id==id);if(choice==null)return;Label(form,choice.label+" · 可跳过",34,140,760,34,20,gold);
            string current=id=="domain"?choiceDomain:id=="career_stage"?choiceCareer:choiceDensity;
            for(int i=0;i<choice.options.Length;i++)
            {
                var opt=choice.options[i];string value=opt.value;var b=ActionButton(form,opt.label,34+(i%2)*395,200+(i/2)*62,370,48,()=>{if(id=="domain")choiceDomain=value;else if(id=="career_stage")choiceCareer=value;else choiceDensity=value;BuildEntry();});
                SetChoiceState(b,current==value,current!=value);
            }
        }
        private void Quit()
        {
            Client.Save();
#if UNITY_EDITOR
            UnityEditor.EditorApplication.isPlaying=false;
#else
            Application.Quit();
#endif
        }
    }
}
