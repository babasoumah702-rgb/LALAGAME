using System;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;

namespace LastCall
{
    public sealed partial class LastCallInterface
    {
        private string unifiedContext="",activeInteractionGroup="observe",activeInteractionOption="";
        private string pendingInteractionId="",pendingInteractionOption="";
        private float pendingInteractionRelease;
        private Text unifiedNextTitle,unifiedNextHint,unifiedStatus;
        private Button unifiedNextButton;
        private readonly Dictionary<string,Button> unifiedPrimary=new Dictionary<string,Button>();
        private readonly Dictionary<string,Button> unifiedOptions=new Dictionary<string,Button>();

        private void BuildUnifiedInteraction()
        {
            var interaction=Client.State?.interaction;if(interaction==null)return;
            string draft=expression?expression.text:"";
            Clear(root);expression=null;rightPanel=null;pausePanel=null;notesPanel=null;
            size=new Vector2(Width,Height);Camera.main.rect=new Rect(0,0,1,1);
            unifiedPrimary.Clear();unifiedOptions.Clear();unifiedContext=interaction.contextId;
            if((interaction.groups??Array.Empty<InteractionGroupDto>()).All(g=>g.id!=activeInteractionGroup))activeInteractionGroup=interaction.nextGroup??"observe";
            Label(root,"LA LA LAND",24,16,310,38,27);
            Label(root,UnifiedChapterTitle(),26,54,420,24,15,muted);
            clockText=Label(root,"",Width-240,24,210,32,20);
            modeText=Label(root,"",24,83,Width-390,25,13,muted);
            ActionButton(root,"线索册",Width-304,72,126,34,ShowNotes);
            ActionButton(root,"暂停 / 存档",Width-166,72,142,34,()=>Pause(true));

            var next=Panel("Current objective",root,24,116,Width-48,88,new Color(.12f,.105f,.09f,.96f));
            Panel("Objective edge",next,0,0,4,88,new Color(.72f,.58f,.31f));
            unifiedNextTitle=Label(next,"",18,8,Width-340,30,20,gold);
            unifiedNextHint=Label(next,"",18,39,Width-340,41,14,cream);
            unifiedNextButton=ActionButton(next,"查看下一步",Width-302,19,254,50,OpenNextStep,true);

            targetText=Label(root,"",24,Height-316,Width-48,28,17);
            unifiedStatus=Label(root,"",24,Height-286,Width-48,25,13,muted);
            toastText=Label(root,"",24,Height-348,Width-48,28,15,gold);

            float x=24;
            foreach(var group in interaction.groups??Array.Empty<InteractionGroupDto>())
            {
                string id=group.id;
                var button=ActionButton(root,group.label,x,Height-246,124,38,()=>SelectInteractionGroup(id));
                button.name="Primary interaction "+id;unifiedPrimary[id]=button;x+=136;
            }
            BuildInteractionOptions(interaction,draft);
            RefreshUnifiedInteraction();
        }

        private void BuildInteractionOptions(InteractionDto interaction,string draft)
        {
            var group=interaction.groups?.FirstOrDefault(g=>g.id==activeInteractionGroup);
            if(group==null)return;
            var visible=group.options??Array.Empty<InteractionOptionDto>();
            bool targetNeeded=visible.Any(o=>o.targetRequired);
            if(targetNeeded)
            {
                var people=Client.State.characters.Where(a=>a.id!="USER"&&a.id!="OWNER").ToArray();
                lastActors=string.Join(",",people.Select(a=>a.id+":"+a.name));
                for(int i=0;i<people.Length;i++)
                {
                    string id=people[i].id;
                    var b=ActionButton(root,people[i].name,24+i*142,Height-278,134,28,()=>Select(id));
                    b.name="Interaction target "+id;
                }
            }
            float width=126,gap=8,startY=Height-198;
            for(int i=0;i<visible.Length;i++)
            {
                var item=visible[i];string id=item.id;
                float bx=24+(i%7)*(width+gap),by=startY+(i/7)*40;
                var button=ActionButton(root,item.label,bx,by,width,34,()=>ExecuteInteractionOption(id));
                button.name="Interaction option "+id;unifiedOptions[id]=button;
            }
            if(NeedsTextInput(activeInteractionOption))
            {
                expression=InputBox(root,24,Height-112,Width-229,64);expression.text=draft;
                sendButton=ActionButton(root,TextSubmitLabel(),Width-188,Height-112,164,64,SubmitUnifiedText,true);
                sendButton.name="Submit unified text";
                replyStatus=Label(root,"",24,Height-42,Width-237,32,13,muted);
            }else Label(root,"选择一个二级动作；已完成的选择会变灰，重复点击不会再次提交。",24,Height-105,Width-48,32,13,muted);
        }

        private void RefreshUnifiedInteraction()
        {
            var state=Client.State;var interaction=state?.interaction;if(interaction==null)return;
            if(!clockText||!modeText||!unifiedNextTitle||!unifiedNextHint||!unifiedNextButton||!targetText||!unifiedStatus)
            {
                if(!Blocking)BuildUnifiedInteraction();
                return;
            }
            if(interaction.contextId!=unifiedContext){activeInteractionOption="";activeInteractionGroup=interaction.nextGroup??"observe";BuildUnifiedInteraction();return;}
            clockText.text=state.clock;
            modeText.text=(state.mode=="online"?"在线 AI":"离线规则")+" · 本章 "+(state.story?.budgetCalls??state.calls)+" / 80 次调用";
            unifiedNextTitle.text="下一步 · "+interaction.nextTitle;unifiedNextHint.text=interaction.nextHint;
            unifiedNextButton.GetComponentInChildren<Text>().text=string.IsNullOrEmpty(interaction.nextActionId)?"展开相关选择":"执行下一步";
            foreach(var pair in unifiedPrimary)SetChoiceState(pair.Value,pair.Key==activeInteractionGroup,pair.Key!=activeInteractionGroup);
            foreach(var group in interaction.groups??Array.Empty<InteractionGroupDto>())foreach(var item in group.options??Array.Empty<InteractionOptionDto>())
            {
                if(!unifiedOptions.TryGetValue(item.id,out var button))continue;
                bool pending=item.id==pendingInteractionOption&&Time.unscaledTime<pendingInteractionRelease;
                SetChoiceState(button,item.selected||pending,!item.selected&&!pending&&item.enabled);
            }
            var target=state.characters.FirstOrDefault(a=>a.id==selected);
            bool targetNeeded=(interaction.groups?.FirstOrDefault(g=>g.id==activeInteractionGroup)?.options??Array.Empty<InteractionOptionDto>()).Any(o=>o.enabled&&o.targetRequired);
            targetText.text=targetNeeded?"当前人物 · "+(target?.name??"请选择人物"):"当前分类 · "+(interaction.groups?.FirstOrDefault(g=>g.id==activeInteractionGroup)?.label??"");
            if(replyStatus)
            {
                var request=state.replies?.LastOrDefault(r=>r.actor==selected&&r.status=="error")??state.replies?.LastOrDefault(r=>r.actor==selected);
                replyStatus.text=request?.status=="error"?request.error:request?.status=="queued"||request?.status=="running"?"对方正在回应；剧情和其他操作仍会继续。":"输入后只发送一次。";
            }
            var unavailable=interaction.groups?.FirstOrDefault(g=>g.id==activeInteractionGroup)?.options?.FirstOrDefault(o=>!o.enabled&&!string.IsNullOrEmpty(o.disabledReason));
            unifiedStatus.text=unavailable!=null?"暂不可用 · "+unavailable.disabledReason:"一级：观察 / 移动 / 互动   ·   当前上下文 "+interaction.contextId;
        }

        private void SetChoiceState(Button button,bool selectedState,bool enabled)
        {
            if(!button)return;button.interactable=enabled;
            button.image.color=selectedState?new Color(.27f,.27f,.29f):new Color(.16f,.16f,.18f);
            var text=button.GetComponentInChildren<Text>();if(text)text.color=selectedState?muted:cream;
        }
        private void SelectInteractionGroup(string id){if(activeInteractionGroup==id)return;activeInteractionGroup=id;activeInteractionOption="";BuildUnifiedInteraction();}
        private void OpenNextStep(){var i=Client.State?.interaction;if(i==null)return;activeInteractionGroup=i.nextGroup??"observe";if(!string.IsNullOrEmpty(i.nextActionId))ExecuteInteractionOption(i.nextActionId);else BuildUnifiedInteraction();}

        private void ExecuteInteractionOption(string id)
        {
            if(id==pendingInteractionOption&&Time.unscaledTime<pendingInteractionRelease)return;
            var current=Client.State?.interaction?.groups?.SelectMany(g=>g.options??Array.Empty<InteractionOptionDto>()).FirstOrDefault(o=>o.id==id);
            if(current?.selected==true)return;
            if(id=="talk"||id=="tarot_answer"||id=="tarot_ask_back"||id=="tarot_joke")
            {activeInteractionOption=id;BuildUnifiedInteraction();return;}
            if(id=="locations"){ShowLocations();return;}
            if(id=="legacy_cards"){cardsExpanded=true;BuildWorld();return;}
            if(id=="leave"||id=="end_night"){ConfirmNightEnd();return;}
            CommandDto command=null;
            switch(id)
            {
                case "observe_room":case "observe_target":command=new CommandDto{type="observe",target=id=="observe_target"?selected:null};break;
                case "observe_third":command=new CommandDto{type="observe_object",objectTarget="third_drink"};break;
                case "observe_seat":command=new CommandDto{type="observe_object",objectTarget="reserved_seat"};break;
                case "observe_tarot_deck":command=new CommandDto{type="observe_object",objectTarget="tarot_deck"};break;
                case "listen":command=new CommandDto{type="listen_in"};break;
                case "approach":command=new CommandDto{type="approach_target",target=selected};break;
                case "follow":command=new CommandDto{type="follow_target",target=selected};break;
                case "move_main":command=new CommandDto{type="move_to",location="main_table"};break;
                case "move_bar":command=new CommandDto{type="night_move",location="bar"};break;
                case "move_corridor":command=new CommandDto{type="night_move",location="corridor"};break;
                case "move_rooftop":command=new CommandDto{type="night_move",location="rooftop"};break;
                case "stay":command=new CommandDto{type="cancel_move",intent="stay"};break;
                case "sit_reserved":command=new CommandDto{type="sit_reserved"};break;
                case "light_game":command=new CommandDto{type="join_game"};break;
                case "tarot_sit":command=new CommandDto{type="tarot_seat"};break;
                case "tarot_watch":command=new CommandDto{type="tarot_seat",text="watch"};break;
                case "tarot_decline":command=new CommandDto{type="tarot_seat",text="decline"};break;
                case "tarot_skip":command=new CommandDto{type="tarot_move",intent="skip"};break;
                case "tarot_deflect":command=new CommandDto{type="tarot_move",intent="deflect"};break;
                case "tarot_observe":command=Client.State.scene3!=null&&Client.State.scene3.askedAt>=0?new CommandDto{type="tarot_move",intent="observe"}:new CommandDto{type="observe_object",objectTarget="tarot_deck"};break;
                case "chocolate_ask":command=new CommandDto{type="chocolate",intent="ask",target=selected};break;
                case "chocolate_share":command=new CommandDto{type="chocolate",intent="share",target=selected};break;
                case "chocolate_refuse":command=new CommandDto{type="chocolate",intent="refuse",target=selected};break;
                default:
                    if(id.StartsWith("pose_"))command=new CommandDto{type="night_pose",intent=id.Substring(5),target=selected};
                    break;
            }
            if(command==null)return;
            Client.Send(command);
            pendingInteractionId=command.id;pendingInteractionOption=id;pendingInteractionRelease=Time.unscaledTime+30;
            refresh=true;RefreshUnifiedInteraction();
        }

        private bool NeedsTextInput(string id)=>id=="talk"||id=="tarot_answer"||id=="tarot_ask_back"||id=="tarot_joke";
        private string TextSubmitLabel()=>activeInteractionOption=="talk"?"交流":activeInteractionOption=="tarot_answer"?"回答":activeInteractionOption=="tarot_ask_back"?"反问":"说个玩笑";
        private void SubmitUnifiedText()
        {
            if(activeInteractionOption=="talk")
            {
                string line=expression?expression.text.Trim():"";
                var latest=Client.State?.events?.LastOrDefault(e=>e.actor=="USER"&&e.type=="speech"&&e.target==selected);
                bool waiting=Client.State?.replies?.Any(r=>r.actor==selected&&(r.status=="queued"||r.status=="running"))==true;
                if(waiting&&latest!=null&&latest.text==line){Toast("这句话已经发出，正在等对方回应。");return;}
                SubmitConversation();return;
            }
            if(activeInteractionOption==pendingInteractionOption&&Time.unscaledTime<pendingInteractionRelease)return;
            string text=expression?expression.text.Trim():"";
            if(string.IsNullOrEmpty(text)){Toast(activeInteractionOption=="tarot_answer"?"先写下你的回答，或者选择跳过。":"先写一句想说的话。");return;}
            string type=activeInteractionOption=="tarot_answer"?"tarot_answer":"tarot_move";
            string intent=activeInteractionOption=="tarot_ask_back"?"ask_back":activeInteractionOption=="tarot_joke"?"joke":null;
            var command=new CommandDto{type=type,intent=intent,target=activeInteractionOption=="tarot_joke"?null:selected,text=text,tone="natural"};
            EventSystem.current?.SetSelectedGameObject(null);editing=false;
            Client.Send(new CommandDto{type="pause",paused=false});Client.Send(command);
            pendingInteractionId=command.id;pendingInteractionOption=activeInteractionOption;pendingInteractionRelease=Time.unscaledTime+30;
            expression.text="";refresh=true;RefreshUnifiedInteraction();
        }
        private string UnifiedChapterTitle()
        {
            var s=Client.State;return s.late?.chapter==4?"走廊 · 巧克力烟":s.late?.chapter==5?"夜深 · 流动的夜色":s.late?.chapter==6?"屋顶 · 留白":s.scene3!=null?"闭店前最后一局 · 塔罗":s.scene2!=null?"酒局热场 · 人终于到齐":"第三杯";
        }
        private void UnifiedInteractionAcknowledged(string id){if(id!=pendingInteractionId)return;pendingInteractionRelease=Time.unscaledTime+.35f;refresh=true;}
        private void UnifiedInteractionRejected(string id){if(id!=pendingInteractionId)return;pendingInteractionId=pendingInteractionOption="";pendingInteractionRelease=0;refresh=true;}
    }
}
