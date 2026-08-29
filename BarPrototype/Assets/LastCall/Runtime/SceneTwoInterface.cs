using System.Linq;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;

namespace LastCall
{
    // Scene 2 keeps the same minimal frame as Scene 1 and only adds the verbs the chapter introduces:
    // following someone, listening without joining, and the light warm-up game. Deliberately no
    // relationship graph and no "you discovered X and Y have history" hint: the design requires the
    // player to reach that feeling from observation alone.
    public sealed partial class LastCallInterface
    {
        public bool SceneTwoMode=>Client?.State?.scene2!=null&&Client?.State?.scene3==null&&Client?.State?.late==null&&!cardsExpanded;
        private Text sceneTwoRoom,sceneTwoGame;
        private Button sceneTwoTable;
        private void BuildSceneTwo()
        {
            string draft=expression?expression.text:"";
            Clear(root);expression=null;rightPanel=null;pausePanel=null;notesPanel=null;
            size=new Vector2(Width,Height);Camera.main.rect=new Rect(0,0,1,1);
            Label(root,"LA LA LAND",24,16,310,38,27);
            Label(root,"酒局热场 · 人终于到齐",26,54,300,24,15,muted);
            clockText=Label(root,"",Width-240,24,210,32,20);
            modeText=Label(root,"",24,83,Width-48,28,13,muted);
            sceneTwoRoom=Label(root,"",24,111,Width-48,26,13,muted);
            targetText=Label(root,"",24,Height-240,Width-48,28,18);
            sceneTwoGame=Label(root,"",24,Height-302,Width-48,29,15,gold);
            var people=Client.State.characters.Where(a=>a.id!="USER"&&a.id!="OWNER").ToArray();
            lastActors=string.Join(",",people.Select(a=>a.id+":"+a.name));
            for(int i=0;i<people.Length;i++){
                string id=people[i].id;
                ActionButton(root,people[i].name,24+i*146,Height-202,138,32,()=>Select(id));
            }
            ActionButton(root,"观察",24,Height-159,80,34,()=>Client.Send(new CommandDto{type="observe"}));
            ActionButton(root,"靠近",112,Height-159,80,34,()=>Client.Send(new CommandDto{type="approach_target",target=selected}));
            ActionButton(root,"跟着走",200,Height-159,88,34,()=>Client.Send(new CommandDto{type="follow_target",target=selected}));
            ActionButton(root,"旁听",296,Height-159,80,34,()=>Client.Send(new CommandDto{type="listen_in"}));
            ActionButton(root,"来一局",384,Height-159,88,34,()=>Client.Send(new CommandDto{type="join_game"}));
            ActionButton(root,"去哪里",480,Height-159,88,34,ShowLocations);
            ActionButton(root,"线索册",576,Height-159,88,34,ShowNotes);
            ActionButton(root,"卡牌",672,Height-159,76,34,()=>{cardsExpanded=true;BuildWorld();});
            sceneTwoTable=ActionButton(root,"回主桌",756,Height-159,88,34,()=>Client.Send(new CommandDto{type="move_to",location="main_table"}));
            sceneTwoTable.name="Go to tarot table";
            ActionButton(root,"暂停 / 存档",Width-163,Height-159,139,34,()=>Pause(true));
            expression=InputBox(root,24,Height-112,Width-229,64);expression.text=draft;
            sendButton=ActionButton(root,"交流",Width-188,Height-112,164,64,SubmitConversation,true);
            sendButton.name="Speak to character";
            replyStatus=Label(root,"",24,Height-42,Width-237,32,13,muted);
            retryReply=ActionButton(root,"重试这条回复",Width-188,Height-41,164,31,()=>{
                var r=Client.State.replies?.LastOrDefault(r=>r.actor==selected&&r.status=="error");
                if(r!=null)Client.Send(new CommandDto{type="retry_reply",requestId=r.id});
            });
            toastText=Label(root,"",24,Height-275,Width-48,29,15,gold);
            RefreshSceneTwo();
        }
        private void RefreshSceneTwo()
        {
            if(!targetText||!replyStatus||!sceneTwoRoom)return;
            var s=Client.State;var two=s.scene2;
            var people=s.characters.Where(a=>a.id!="USER"&&a.id!="OWNER").ToArray();
            if(lastActors!=string.Join(",",people.Select(a=>a.id+":"+a.name))&&!Blocking){BuildSceneTwo();return;}
            var target=people.FirstOrDefault(a=>a.id==selected);
            if(target==null){target=people.FirstOrDefault();selected=target?.id??"B";}
            targetText.text="对 "+(target?.name??"附近的人")+" 说话";
            clockText.text=two.deckPlaced?"最后一轮？":s.clock;
            modeText.text=(s.mode=="online"?"在线 AI":"离线规则")+" · 本章 "+(s.story?.budgetCalls??s.calls)+" / 80 次调用";
            // Time is shown as the room, never as a progress bar.
            sceneTwoRoom.text="杯子 "+Mathf.RoundToInt(two.drinkLevel*100)+"%  ·  杯垫 "+two.coasters+
                "  ·  还在的客人 "+two.guests+(two.rainStopped?"  ·  雨停了":"  ·  外面还在下雨");
            string next=two.deckPlaced?"塔罗牌已经放到主桌；下一章会自动开始。":
                two.phase=="cross_intro"?"先听完刚到者的简短介绍。":
                two.phase=="freeflow"?"自由交流中：可跟随、旁听或玩轻游戏；夜深后会自动收桌。":
                two.phase=="montage"?"客人正在散去：继续交流或旁观，随后回到主桌。":
                two.phase=="gathering"?"调酒师正在收桌：请走向主桌，塔罗牌马上会留下。":
                "";
            sceneTwoGame.text=string.IsNullOrEmpty(two.gamePrompt)?next:two.gamePrompt+"  ·  "+next;
            if(sceneTwoTable)sceneTwoTable.GetComponentInChildren<Text>().text=two.deckPlaced?"走向塔罗":"回主桌";
            var request=s.replies?.LastOrDefault(r=>r.actor==selected&&r.status=="error")??s.replies?.LastOrDefault(r=>r.actor==selected);
            bool failed=request?.status=="error",waiting=request?.status=="running"||request?.status=="queued";
            replyStatus.text=failed?request.error:waiting?"对方正在回应；你可以继续走动和观察。":
                "大家都在走动 · 可以跟着谁，也可以只站着听";
            retryReply.gameObject.SetActive(failed);
            sendButton.GetComponentInChildren<Text>().text=queuedCard!=null?"走近中 · 取消":sentCard!=null?"发送中…":"交流";
            sendButton.interactable=sentCard==null&&s.status=="playing";
        }
    }
}
