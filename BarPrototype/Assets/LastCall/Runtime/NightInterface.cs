using System.Linq;
using UnityEngine;
using UnityEngine.UI;
namespace LastCall
{
    public sealed partial class LastCallInterface
    {
        public bool NightMode=>Client?.State?.late!=null&&!cardsExpanded;
        private Text nightHint;private Button chocolateButton,roofButton;
        private void BuildNight(){string draft=expression?expression.text:"";Clear(root);expression=null;rightPanel=null;pausePanel=null;notesPanel=null;size=new Vector2(Width,Height);Camera.main.rect=new Rect(0,0,1,1);
            var s=Client.State;var n=s.late;
            Label(root,"LA LA LAND",24,16,310,38,27);Label(root,n.chapter==4?"走廊 · 巧克力烟":n.chapter==5?"夜深 · 流动的夜色":"屋顶 · 留白",26,54,360,24,15,muted);
            clockText=Label(root,"",Width-230,24,206,32,20);modeText=Label(root,"",24,83,Width-48,28,13,muted);nightHint=Label(root,"",24,115,Width-48,50,15,muted);
            targetText=Label(root,"",24,Height-242,Width-48,28,18);
            var people=s.characters.Where(a=>a.id!="USER"&&a.id!="OWNER").ToArray();lastActors=string.Join(",",people.Select(a=>a.id+":"+a.name));
            for(int i=0;i<people.Length;i++){string id=people[i].id;ActionButton(root,people[i].name,24+i*142,Height-204,134,30,()=>Select(id));}
            ActionButton(root,"观察",24,Height-164,72,34,()=>Client.Send(new CommandDto{type="observe"}));ActionButton(root,"靠近",103,Height-164,72,34,()=>Client.Send(new CommandDto{type="approach_target",target=selected}));
            ActionButton(root,n.chapter==4?"走廊":"回吧台",182,Height-164,90,34,()=>NightMove(n.chapter==4?"corridor":"bar"));
            roofButton=ActionButton(root,"沿楼梯上楼",280,Height-164,116,34,()=>NightMove("rooftop"));
            if(n.chapter==4){chocolateButton=ActionButton(root,"巧克力烟",404,Height-164,104,34,ChocolateMenu);ActionButton(root,"回酒吧",516,Height-164,88,34,()=>NightMove("bar"));}
            else if(n.chapter==6){ActionButton(root,"坐 / 躺 / 留白",404,Height-164,132,34,RoofPoseMenu);ActionButton(root,"结束这一晚",544,Height-164,116,34,ConfirmNightEnd);}
            else ActionButton(root,"留在这里",404,Height-164,110,34,()=>{Client.Send(new CommandDto{type="cancel_move"});Toast("你留下了。可以继续交流，也可以稍后再上楼。");});
            ActionButton(root,"线索册",680,Height-164,90,34,ShowNotes);ActionButton(root,"离开",778,Height-164,72,34,ConfirmNightEnd);ActionButton(root,"跟随",858,Height-164,72,34,()=>Client.Send(new CommandDto{type="follow_target",target=selected}));ActionButton(root,"暂停 / 存档",Width-163,Height-164,139,34,()=>Pause(true));
            expression=InputBox(root,24,Height-112,Width-229,64);expression.text=draft;sendButton=ActionButton(root,"轻声交流",Width-188,Height-112,164,64,SubmitConversation,true);
            replyStatus=Label(root,"",24,Height-42,Width-237,32,13,muted);retryReply=ActionButton(root,"重试这条回复",Width-188,Height-41,164,31,()=>{var r=Client.State.replies?.LastOrDefault(r=>r.actor==selected&&r.status=="error");if(r!=null)Client.Send(new CommandDto{type="retry_reply",requestId=r.id});});
            toastText=Label(root,"",24,Height-280,Width-48,30,15,gold);RefreshNight();
        }
        private void NightMove(string area){Client.Send(new CommandDto{type="night_move",location=area});}
        private void RefreshNight(){if(!nightHint)return;var s=Client.State;var n=s.late;if(n==null)return;var target=s.characters.FirstOrDefault(a=>a.id==selected);targetText.text="对 "+(target?.name??"附近的人")+" 说话";clockText.text=s.clock;modeText.text=(s.mode=="online"?"在线 AI":"离线规则")+" · 本章 "+(s.story?.budgetCalls??s.calls)+" / 80 次调用";
            nightHint.text=n.chapter==4?"走廊里的动静隔着门变闷了。可以跟出去，也可以留在酒吧。":n.chapter==5?(n.powerState=="normal"?"杯子少了，音乐也轻了。人们还在各自选择距离。":"音乐停了。应急灯照着楼梯；你可以上楼、留下，或离开。"):"夜风、城市远声。可以并肩坐，也可以保持距离。结束由你决定。";
            if(roofButton)roofButton.interactable=n.powerState!="normal";if(chocolateButton)chocolateButton.interactable=n.canChocolate;
            var r=s.replies?.LastOrDefault(r=>r.actor==selected&&r.status=="error")??s.replies?.LastOrDefault(r=>r.actor==selected);replyStatus.text=r?.status=="error"?r.error:r?.status=="running"||r?.status=="queued"?"对方正在回应，你可以继续走动。":"WASD 移动 · 右键转头 · 回忆可按空格跳过";retryReply.gameObject.SetActive(r?.status=="error");sendButton.interactable=s.status=="playing"&&sentCard==null;
        }
        private void ChocolateMenu(){var p=Panel("Chocolate choices",root,Width/2-210,180,420,215,new Color(.07f,.07f,.09f,.97f));Label(p,"巧克力烟",20,12,380,30,21);var words=new[]{"索取一支","递给身边的人","摆手谢绝"};var intents=new[]{"ask","share","refuse"};for(int i=0;i<3;i++){string intent=intents[i];ActionButton(p,words[i],20,52+i*45,380,36,()=>{Client.Send(new CommandDto{type="chocolate",intent=intent,target=selected});Destroy(p.gameObject);});}}
        private void RoofPoseMenu(){var p=Panel("Roof choices",root,Width/2-220,150,440,334,new Color(.07f,.07f,.09f,.97f));Label(p,"留下什么姿态",20,10,400,30,21);var words=new[]{"坐在靠垫旁","躺下看夜空","站起来","抬头看天空","安静待着","留一点距离"};var intents=new[]{"sit","lie","stand","sky","silence","distance"};for(int i=0;i<6;i++){string intent=intents[i];ActionButton(p,words[i],20,49+i*44,400,35,()=>{Client.Send(new CommandDto{type="night_pose",intent=intent});Destroy(p.gameObject);});}}
        private void ConfirmNightEnd(){var p=Panel("Confirm end",root,Width/2-220,220,440,190,new Color(.07f,.07f,.09f,.98f));Label(p,"把这一晚留在这里？",24,20,392,42,23);Label(p,"回顾只记录你实际看见和听见的片段。",24,65,392,35,15,muted);ActionButton(p,"再待一会",24,121,184,42,()=>Destroy(p.gameObject));ActionButton(p,"确认结束",232,121,184,42,()=>{Client.Send(new CommandDto{type="end_night"});Destroy(p.gameObject);},true);}
    }
}
