using System.Linq;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;

namespace LastCall
{
    public sealed partial class LastCallInterface
    {
        private Text[] recommendedLabels;
        private void BuildWorld()
        {
            if(Client.State==null)return;
            Clear(root);
            expression=null;pausePanel=null;notesPanel=null;
            size=new Vector2(Width,Height);
            Camera.main.rect=new Rect(0,.18f,(Width-330)/Width,.72f);
            Label(root,"LAST CALL",24,14,300,40,30);
            Label(root,"闭店前最后一局 · 虚构关系体验",26,57,430,24,14,muted);
            clockText=Label(root,"",Width-535,20,180,34,25,gold);
            modeText=Label(root,"",24,85,Width-385,25,13,muted);
            var feed=Panel("What you perceive",root,22,Height-151,Width-372,93,new Color(.035f,.065f,.068f,.93f));
            feedText=Label(feed,"",12,5,Width-398,82,15);
            feedText.alignment=TextAnchor.UpperLeft;
            ActionButton(root,"观察",22,Height-47,88,34,()=>Client.Send(new CommandDto{type="observe"}));
            ActionButton(root,"去哪里",118,Height-47,95,34,ShowLocations);
            ActionButton(root,"线索册",221,Height-47,95,34,ShowNotes);
            ActionButton(root,"牌局",324,Height-47,90,34,ShowParty);
            ActionButton(root,"暂停 / 存档",422,Height-47,135,34,()=>Pause(true));
            Label(root,"WASD / 方向键  移动   SHIFT  快走   E  互动",575,Height-46,Width-935,34,12,muted);
            toastText=Label(root,"",24,Height-188,Width-380,32,16,gold);
            rightPanel=Panel("Social moves",root,Width-330,0,330,Height,ink);
            Panel("Edge",rightPanel,0,0,2,Height,gold);
            targetText=Label(rightPanel,"",18,18,298,42,23);
            var actors=Client.State.characters.Where(a=>a.id!="USER").ToArray();
            lastActors=string.Join(",",actors.Select(a=>a.id));
            for(int i=0;i<actors.Length;i++)
            {
                string id=actors[i].id;
                ActionButton(rightPanel,actors[i].name,18+(i%3)*99,72+(i/3)*38,92,32,()=>Select(id));
            }
            ActionButton(rightPanel,"走近她",18,155,143,34,()=>Client.Send(new CommandDto{type="approach_target",target=selected}),true);
            ActionButton(rightPanel,"前往主桌",174,155,139,34,()=>Client.Send(new CommandDto{type="move_to",location="main_table"}));
            Label(rightPanel,"先选择意图，再选择表达",18,202,295,26,15,gold);
            var cards=Client.State.cards;
            cardButtons=new Button[cards.Length];
            for(int i=0;i<cards.Length;i++)
            {
                string id=cards[i].id;
                cardButtons[i]=ActionButton(rightPanel,cards[i].name,18+(i%3)*99,237+(i/3)*41,92,35,()=>{cardId=id;RefreshWorld();});
            }
            cardText=Label(rightPanel,"",18,404,294,44,14,muted);
            recommendedLabels=new[]{
                ActionButton(rightPanel,"",18,450,294,43,()=>ChooseExpression(0)).GetComponentInChildren<Text>(),
                ActionButton(rightPanel,"",18,498,294,43,()=>ChooseExpression(1)).GetComponentInChildren<Text>()
            };
            foreach(var label in recommendedLabels)label.fontSize=13;
            expression=InputBox(rightPanel,18,551,294,67);
            sendButton=ActionButton(rightPanel,"说给她听",18,628,294,42,Submit,true);
            Label(rightPanel,"输入时暂停世界 · 最多 200 字",18,680,294,25,12,muted);
            RefreshWorld();
        }
        private void RefreshWorld()
        {
            var state=Client.State;
            if(state==null||!rightPanel)return;
            var actors=state.characters.Where(a=>a.id!="USER").ToArray();
            if(lastActors!=string.Join(",",actors.Select(a=>a.id))&&!Blocking){BuildWorld();return;}
            var target=state.characters.FirstOrDefault(a=>a.id==selected);
            if(target==null){selected=actors.FirstOrDefault()?.id??"B";target=actors.FirstOrDefault();}
            targetText.text=target==null?"选择一个人":target.name+"  /  "+target.location;
            clockText.text=state.clock+"  ·  第 "+state.night+" 夜";
            modeText.text=(state.busy?"她正在想如何回应…  |  ":"")+state.modeReason+"  ·  "+state.calls+" 次调用";
            var card=state.cards.FirstOrDefault(c=>c.id==cardId)??state.cards[0];
            cardText.text=card.text;
            for(int i=0;i<recommendedLabels.Length;i++)recommendedLabels[i].text=card.expressions[System.Math.Min(i,card.expressions.Length-1)];
            for(int i=0;i<cardButtons.Length;i++)
                cardButtons[i].interactable=state.cards[i].ready&&!state.busy;
            sendButton.interactable=target!=null&&card.ready&&!state.busy;
            if(lastEvent!=state.cursor)
            {
                lastEvent=state.cursor;
                feedText.text=string.Join("\n",state.events.Skip(System.Math.Max(0,state.events.Length-2)).Select(e=>
                    e.time+"  "+e.name+" · "+SourceLabel(e.source)+"  "+e.text));
            }
        }
        private string SourceLabel(string value)
        {
            return value=="direct"?"亲见":value=="overheard"?"旁听":value=="observed"?"观察":value=="shared"?"告知":"公开";
        }
        private void ChooseExpression(int index)
        {
            var card=Client.State.cards.First(c=>c.id==cardId);
            expression.text=card.expressions[System.Math.Min(index,card.expressions.Length-1)];
            Toast(expression.text);
        }
        private void Submit()
        {
            EventSystem.current.SetSelectedGameObject(null);
            editing=false;
            Client.Send(new CommandDto{type="pause",paused=false});
            Client.Send(new CommandDto{type="card",target=selected,card=cardId,text=expression.text});
            expression.text="";
        }
    }
}
