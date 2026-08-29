using System.Linq;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;

namespace LastCall
{
    public sealed partial class LastCallInterface
    {
        private Text[] recommendedLabels;
        private Text storyText;
        private void BuildWorld()
        {
            if(Client.State==null)return;
            if(Client.State.intro?.phase=="elevator"){BuildIntroUI();return;}
            if(Client.State.interaction!=null&&!cardsExpanded){BuildUnifiedInteraction();return;}
            // Later chapters claim the frame first; each falls back to the previous one when absent.
            if(NightMode){BuildNight();return;}
            if(SceneThreeMode){BuildSceneThree();return;}
            if(SceneTwoMode){BuildSceneTwo();return;}
            if(SceneOneMode){BuildSceneOne();return;}
            Clear(root);
            expression=null;pausePanel=null;notesPanel=null;
            size=new Vector2(Width,Height);
            Camera.main.rect=new Rect(0,0,(Width-330)/Width,1);
            Label(root,"LA LA LAND",24,14,300,40,30);
            Label(root,"闭店前最后一局 · 虚构关系体验",26,57,430,24,14,muted);
            clockText=Label(root,"",Width-535,20,180,34,25,gold);
            modeText=Label(root,"",24,85,Width-385,25,13,muted);
            var feed=Panel("What you perceive",root,22,Height-151,Width-372,93,new Color(.08f,.08f,.1f,.92f));
            feedText=Label(feed,"",12,5,Width-398,82,15);
            feedText.alignment=TextAnchor.UpperLeft;
            ActionButton(root,"观察",22,Height-47,88,34,()=>Client.Send(new CommandDto{type="observe"}));
            ActionButton(root,"去哪里",118,Height-47,95,34,ShowLocations);
            ActionButton(root,"线索册",221,Height-47,95,34,ShowNotes);
            ActionButton(root,"牌局",324,Height-47,90,34,ShowParty);
            ActionButton(root,"暂停 / 存档",422,Height-47,135,34,()=>Pause(true));
            Label(root,"Q/C 左右转头   R/F 上下   按住右键也可转   WASD 移动   E 互动",575,Height-46,Width-935,34,12,muted);
            toastText=Label(root,"",24,Height-188,Width-380,32,16,gold);
            rightPanel=Panel("Social moves",root,Width-330,0,330,Height,ink);
            Panel("Edge",rightPanel,0,0,2,Height,gold);
            // 剧情描述模块：锁定右侧顶部，复用现有场景文案（人物 + 时间），不新增服务端字段。
            var storyPanel=Panel("Story",rightPanel,0,0,330,148,new Color(.11f,.11f,.14f,.9f));
            Label(storyPanel,"这一夜",14,10,302,22,15,gold);
            storyText=Label(storyPanel,"",14,34,302,106,14,cream);
            storyText.alignment=TextAnchor.UpperLeft;
            targetText=Label(rightPanel,"",18,156,298,36,22);
            var actors=Client.State.characters.Where(a=>a.id!="USER").ToArray();
            lastActors=string.Join(",",actors.Select(a=>a.id+":"+a.name));
            for(int i=0;i<actors.Length;i++)
            {
                string id=actors[i].id;
                ActionButton(rightPanel,actors[i].name,18+(i%3)*99,198+(i/3)*34,92,30,()=>Select(id));
            }
            ActionButton(rightPanel,"走近她",18,268,143,32,()=>Client.Send(new CommandDto{type="approach_target",target=selected}),true);
            ActionButton(rightPanel,"前往主桌",174,268,139,32,()=>Client.Send(new CommandDto{type="move_to",location="main_table"}));
            Label(rightPanel,"先选择意图，再选择表达",18,306,295,22,15,gold);
            var cards=Client.State.cards;
            cardButtons=new Button[cards.Length];
            for(int i=0;i<cards.Length;i++)
            {
                string id=cards[i].id;
                cardButtons[i]=ActionButton(rightPanel,cards[i].name,18+(i%3)*99,332+(i/3)*38,92,32,()=>SelectCard(id));
            }
            cardText=Label(rightPanel,"",18,410,294,38,13,muted);
            recommendedLabels=new[]{
                ActionButton(rightPanel,"",18,454,294,38,()=>ChooseExpression(0)).GetComponentInChildren<Text>(),
                ActionButton(rightPanel,"",18,492,294,38,()=>ChooseExpression(1)).GetComponentInChildren<Text>()
            };
            foreach(var label in recommendedLabels)label.fontSize=13;
            expression=InputBox(rightPanel,18,536,294,58);
            sendButton=ActionButton(rightPanel,"出牌",18,600,294,40,Submit,true);
            sendButton.gameObject.name="Play selected card";
            ActionButton(rightPanel,"退出",18,666,294,36,()=>Pause(true));
            if(Client.State.scene1!=null)ActionButton(root,"收起卡牌",Width-480,120,130,34,()=>{cardsExpanded=false;BuildWorld();});
            RefreshWorld();
        }
        private void RefreshWorld()
        {
            var state=Client.State;
            if(state==null)return;
            if(state.interaction!=null&&!cardsExpanded){RefreshUnifiedInteraction();return;}
            if(NightMode){RefreshNight();return;}
            if(SceneThreeMode){RefreshSceneThree();return;}
            if(SceneTwoMode){RefreshSceneTwo();return;}
            if(SceneOneMode){RefreshSceneOne();return;}
            if(!rightPanel)return;
            var actors=state.characters.Where(a=>a.id!="USER").ToArray();
            if(lastActors!=string.Join(",",actors.Select(a=>a.id+":"+a.name))&&!Blocking){BuildWorld();return;}
            var target=state.characters.FirstOrDefault(a=>a.id==selected);
            if(target==null){selected=actors.FirstOrDefault()?.id??"B";target=actors.FirstOrDefault();}
            targetText.text=target==null?"选择一个人":target.name+"  /  "+target.location;
            clockText.text=state.clock+"  ·  第 "+state.night+" 夜";
            modeText.text=(state.busy?"她正在想如何回应…  |  ":"")+state.modeReason+"  ·  "+state.calls+" 次调用";
            if(storyText)storyText.text=StoryText(state);
            var card=state.cards.FirstOrDefault(c=>c.id==cardId)??state.cards[0];
            cardText.text=card.text+(string.IsNullOrEmpty(card.lockReason)?"":"\n"+card.lockReason);
            for(int i=0;i<recommendedLabels.Length;i++)recommendedLabels[i].text=card.expressions[System.Math.Min(i,card.expressions.Length-1)];
            for(int i=0;i<cardButtons.Length;i++){
                cardButtons[i].interactable=state.status=="playing";
                cardButtons[i].image.color=state.cards[i].id==cardId?new Color(.44f,.36f,.24f):new Color(.16f,.16f,.18f);
                cardButtons[i].GetComponentInChildren<Text>().color=state.cards[i].ready?cream:muted;
            }
            RefreshPlayButton(card,target);
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
        private string StoryText(StateDto state)
        {
            var others=state.characters.Where(a=>a.id!="USER").ToArray();
            string present=others.Length==0?"这里空无一人":string.Join("、",others.Select(a=>a.name));
            return "第 "+state.night+" 夜 · "+state.clock+"\n"+present+" 都在店里。";
        }
        private void ChooseExpression(int index)
        {
            var card=Client.State.cards.First(c=>c.id==cardId);
            expression.text=card.expressions[System.Math.Min(index,card.expressions.Length-1)];
            Toast("已选好表达，点击下方「出牌」。");
        }
    }
}
