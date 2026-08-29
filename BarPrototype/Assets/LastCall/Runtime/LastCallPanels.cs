using System;
using System.Linq;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;

namespace LastCall
{
    public sealed partial class LastCallInterface
    {
        private RectTransform Modal(string title,float width=740,float height=540)
        {
            var shade=Panel(title,root,0,0,Width,Height,new Color(.015f,.03f,.035f,.91f));
            Fill(shade);
            var body=Panel("Body",shade,(Width-width)/2,(Height-height)/2,width,height,ink);
            Panel("Brass line",body,0,0,width,3,gold);
            Label(body,title,28,20,width-56,46,26);
            return body;
        }
        private void ShowPause()
        {
            if(pausePanel)Destroy(pausePanel.gameObject);
            var body=Modal("休息片刻",520,420);
            pausePanel=body.parent.GetComponent<RectTransform>();
            Label(body,"世界暂停。你的选择和角色记忆会保存在本机。",30,82,460,48,16,muted);
            ActionButton(body,"继续今晚",30,145,460,45,()=>Pause(false),true);
            ActionButton(body,"保存进度",30,202,220,42,()=>{Client.Save();Toast("正在保存到本机。");});
            ActionButton(body,Client.State.mode=="online"?"切换离线规则":"尝试在线模型",266,202,224,42,()=>{
                Client.Send(new CommandDto{type="mode",online=Client.State.mode!="online"});
                ShowPause();
            });
            if(Client.State.intro?.phase!="elevator")ActionButton(body,"离场并回看",30,258,460,44,()=>{
                Pause(false);
                Client.Send(new CommandDto{type="leave"});
            });
            ActionButton(body,"保存并退出",30,322,460,45,Quit);
        }
        private void OpenNotes(string title,Action<RectTransform> build)
        {
            if(notesPanel)Destroy(notesPanel.gameObject);
            notesVisible=true;
            EventSystem.current.SetSelectedGameObject(null);
            Client.Send(new CommandDto{type="pause",paused=true});
            var body=Modal(title);
            notesPanel=body.parent.GetComponent<RectTransform>();
            build(body);
            ActionButton(body,"回到今晚",28,475,684,42,CloseNotes,true);
        }
        private void CloseNotes()
        {
            if(Client.State.status=="ended")return;
            notesVisible=false;
            if(notesPanel)Destroy(notesPanel.gameObject);
            Client.Send(new CommandDto{type="pause",paused=pauseVisible});
        }
        private void ScrollText(Transform body,string value,float y=84,float height=367)
        {
            var viewport=Panel("Scroll",body,28,y,684,height,new Color(.06f,.06f,.08f));
            viewport.gameObject.AddComponent<RectMask2D>();
            var scroll=viewport.gameObject.AddComponent<ScrollRect>();
            var content=Box("Content",viewport,0,0,658,100);
            var text=Label(content,value,12,0,630,100,17);
            text.alignment=TextAnchor.UpperLeft;
            text.verticalOverflow=VerticalWrapMode.Overflow;
            Canvas.ForceUpdateCanvases();
            float textHeight=Mathf.Max(height,text.preferredHeight+30);
            text.rectTransform.sizeDelta=new Vector2(630,textHeight);
            content.sizeDelta=new Vector2(658,textHeight);
            scroll.content=content;scroll.viewport=viewport;
            scroll.horizontal=false;scroll.vertical=true;
            scroll.movementType=ScrollRect.MovementType.Clamped;
            scroll.scrollSensitivity=35;
        }
        private void ShowNotes()
        {
            OpenNotes("你知道的，和你还不知道的",body=>{
                string text=string.Join("\n\n",Client.State.events.Select(e=>e.time+"  "+e.name+" / "+SourceLabel(e.source)+" · "+GenerationLabel(e.generationSource)+"\n"+e.text));
                ScrollText(body,text.Length>0?text:"这里会记录你亲见、旁听和被告知的事。它不是全知日志。");
            });
        }
        private void ShowLocations()
        {
            OpenNotes("空间也是一种选择",body=>{
                var places=Client.State.locations;
                for(int i=0;i<places.Length;i++)
                {
                    var place=places[i];
                    ActionButton(body,place.name,28+(i%2)*350,90+(i/2)*76,334,60,()=>{
                        CloseNotes();
                        Client.Send(new CommandDto{type="move_to",location=place.id});
                    },place.privacy>.5f);
                }
                Label(body,"深色与安静区域会改变谁能听见；不是隐身或读心。",30,418,674,35,15,muted);
            });
        }
        private void ShowParty()
        {
            OpenNotes("最后一局 · 可以参加，也可以拒绝",body=>{
                partyStatus=Label(body,Client.State.cardsJoined?"你已加入牌局。六张情境牌现在可用；「最后一次表达」在 Last Call 时开放。":
                    Client.State.cardsOffered?"老板娘已经发出邀请。加入后即可使用六张情境牌。":"不用等营业计时：可以现在请老板娘开局并加入。五种入口都可以参加。",30,90,680,100,20);
                Label(body,"玩法：选人物 → 选牌 → 选一句表达或自己输入 → 出牌。\n距离太远会先走近。没有强制回答、胜负或扣分。",30,340,680,90,17,muted);
                if(Client.State.cardsJoined)
                {
                    ActionButton(body,"开始选牌",30,236,330,60,()=>{CloseNotes();SelectCard("truth");},true);
                    ActionButton(body,"退出牌局，继续聊天",380,236,330,60,()=>{CloseNotes();Client.Send(new CommandDto{type="decline"});});
                }
                else{
                    ActionButton(body,Client.State.cardsOffered?"加入牌局":"请老板娘开局并加入",30,236,330,60,()=>StartCoroutine(StartPartyWhenReady()),true);
                    ActionButton(body,"先不参加",380,236,330,60,()=>{CloseNotes();if(Client.State.cardsOffered)Client.Send(new CommandDto{type="decline"});});
                }
            });
        }
        private void ShowReflection()
        {
            if(pausePanel)Destroy(pausePanel.gameObject);
            if(notesPanel)Destroy(notesPanel.gameObject);
            var report=Client.State.reflection;
            if(report==null)return;
            var body=Modal(report.title,740,620);
            notesPanel=body.parent.GetComponent<RectTransform>();
            string text="关系快照\n"+string.Join("\n",report.trends)+"\n\n三个关键片段\n"+string.Join("\n\n",report.events)+
                "\n\n一条可见的关系涟漪\n"+string.Join("\n↓\n",report.chain)+"\n\n你的行动\n"+report.behavior+"\n\n"+report.ending;
            ScrollText(body,text,80,415);
            string previous=Client.State.sessionId;
            ActionButton(body,"带着记忆，下次再来",28,520,328,46,()=>{
                notesVisible=false;
                Client.OpenSession(new SessionRequest{mode="next",sessionId=previous});
            },true);
            ActionButton(body,"换个入口，独立新开局",372,520,340,46,()=>{
                notesVisible=false;entryVisible=true;entryPage=0;BuildEntry();
            });
            ActionButton(body,"保存并退出",28,576,684,32,Quit);
        }
    }
}
