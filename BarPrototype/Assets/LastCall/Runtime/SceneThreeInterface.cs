using System.Linq;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;

namespace LastCall
{
    // Scene 3 shows the card face, the question and whose turn it is. Nothing else: no affinity
    // numbers, no jealousy meter, no "who she really meant". Relationship change is only ever visible
    // through what the characters do, which is why this panel stays this thin.
    public sealed partial class LastCallInterface
    {
        public bool SceneThreeMode=>Client?.State?.scene3!=null&&Client?.State?.late==null&&!cardsExpanded;
        private Text tarotCard,tarotQuestion,tarotTurn,tarotBeat;
        private Button tarotAnswer,tarotSkip,tarotDeflect,tarotAskBack,tarotObserve,tarotJoke;
        private void BuildSceneThree()
        {
            string draft=expression?expression.text:"";
            Clear(root);expression=null;rightPanel=null;pausePanel=null;notesPanel=null;
            size=new Vector2(Width,Height);Camera.main.rect=new Rect(0,0,1,1);
            Label(root,"LA LA LAND",24,16,310,38,27);
            Label(root,"闭店前最后一局 · 塔罗",26,54,320,24,15,muted);
            clockText=Label(root,"",Width-240,24,210,32,20);
            modeText=Label(root,"",24,83,Width-48,28,13,muted);
            // The card and its question are the whole HUD.
            float cardWidth=Mathf.Min(460,Width-48);
            var card=Panel("Tarot card",root,Width-cardWidth-24,116,cardWidth,126,new Color(.08f,.08f,.1f,.92f));
            tarotCard=Label(card,"",16,6,cardWidth-32,36,14,gold);
            tarotQuestion=Label(card,"",16,44,cardWidth-32,74,15);
            tarotQuestion.alignment=TextAnchor.UpperLeft;
            tarotTurn=Label(root,"",24,112,Width-cardWidth-64,26,14,muted);
            ActionButton(root,"坐下",Width-280,54,72,29,()=>Client.Send(new CommandDto{type="tarot_seat"}));ActionButton(root,"旁观",Width-202,54,72,29,()=>Client.Send(new CommandDto{type="tarot_seat",text="watch"}));ActionButton(root,"不参加",Width-124,54,100,29,()=>Client.Send(new CommandDto{type="tarot_seat",text="decline"}));
            tarotBeat=Label(root,"",24,144,Width-cardWidth-64,26,13,muted);
            targetText=Label(root,"",24,Height-240,Width-48,28,18);
            var people=Client.State.characters.Where(a=>a.id!="USER"&&a.id!="OWNER").ToArray();
            lastActors=string.Join(",",people.Select(a=>a.id+":"+a.name));
            for(int i=0;i<people.Length;i++){
                string id=people[i].id;
                ActionButton(root,people[i].name,24+i*146,Height-202,138,32,()=>Select(id));
            }
            // Answer / Skip / Deflect / Ask Back / Observe / Joke are all first-class social moves.
            tarotSkip=ActionButton(root,"跳过",24,Height-159,80,34,()=>SendTarotMove("skip"));
            tarotObserve=ActionButton(root,"只看着",112,Height-159,88,34,()=>SendTarotMove("observe"));
            tarotDeflect=ActionButton(root,"让别人先",208,Height-159,100,34,()=>SendTarotMove("deflect"));
            tarotAskBack=ActionButton(root,"反问她",316,Height-159,88,34,()=>SendTarotMove("ask_back"));
            tarotJoke=ActionButton(root,"开个玩笑",412,Height-159,100,34,()=>SendTarotMove("joke"));
            tarotObserve.name="Watch the table";
            ActionButton(root,"靠近",520,Height-159,80,34,()=>Client.Send(new CommandDto{type="approach_target",target=selected}));
            ActionButton(root,"线索册",608,Height-159,88,34,ShowNotes);
            ActionButton(root,"卡牌",704,Height-159,76,34,()=>{cardsExpanded=true;BuildWorld();});
            ActionButton(root,"暂停 / 存档",Width-163,Height-159,139,34,()=>Pause(true));
            expression=InputBox(root,24,Height-112,Width-229,64);expression.text=draft;
            tarotAnswer=ActionButton(root,"回答",Width-188,Height-112,164,64,SubmitTarotAnswer,true);
            tarotAnswer.name="Answer the card";
            sendButton=tarotAnswer;
            replyStatus=Label(root,"",24,Height-42,Width-237,32,13,muted);
            retryReply=ActionButton(root,"重试这条回复",Width-188,Height-41,164,31,()=>{
                var r=Client.State.replies?.LastOrDefault(r=>r.actor==selected&&r.status=="error");
                if(r!=null)Client.Send(new CommandDto{type="retry_reply",requestId=r.id});
            });
            toastText=Label(root,"",24,Height-275,Width-48,29,15,gold);
            RefreshSceneThree();
        }
        private void RefreshSceneThree()
        {
            if(!tarotCard||!replyStatus)return;
            var s=Client.State;var three=s.scene3;
            var people=s.characters.Where(a=>a.id!="USER"&&a.id!="OWNER").ToArray();
            if(lastActors!=string.Join(",",people.Select(a=>a.id+":"+a.name))&&!Blocking){BuildSceneThree();return;}
            var target=people.FirstOrDefault(a=>a.id==selected);
            if(target==null){target=people.FirstOrDefault();selected=target?.id??"B";}
            string ReaderName(string id)=>people.FirstOrDefault(a=>a.id==id)?.name??id;
            bool open=three.askedAt>=0&&!string.IsNullOrEmpty(three.question);
            targetText.text="对 "+(target?.name??"桌边的人")+" 说话";
            clockText.text=three.phase=="scene4_ready"?"有人出去透气了":s.clock;
            modeText.text=(s.mode=="online"?"在线 AI":"离线规则")+" · 本章 "+(s.story?.budgetCalls??s.calls)+" / 80 次调用";
            tarotCard.text=open?"牌面 · 场景内容  "+three.cardName+(three.isJoker?"   · 玩笑牌":""):
                three.phase=="seating"?"大家正在回到同一张桌子":
                three.phase=="reader_chosen"?"有人把牌拉了过来，正在洗牌":"这一轮结束了";
            tarotQuestion.text=open?three.question:"";
            tarotTurn.text=string.IsNullOrEmpty(three.reader)?"":
                "主持："+ReaderName(three.reader)+(open&&!string.IsNullOrEmpty(three.firstResponder)?
                "   ·   先答："+ReaderName(three.firstResponder):"")+"   ·   第 "+System.Math.Max(1,three.round)+" 轮";
            // The observable half of a gaze, with no interpretation attached to it.
            tarotBeat.text=three.lastGaze!=null&&three.lastGaze.order!=null&&three.lastGaze.order.Length>0
                ?"你注意到 "+ReaderName(three.lastGaze.actor)+" 回答前先看了 "+ReaderName(three.lastGaze.order[0])+
                 "（停顿约 "+(three.lastGaze.pauseMs/1000f).ToString("0.0")+" 秒 · "+three.lastGaze.gesture+"）"
                :"";
            bool acted=!string.IsNullOrEmpty(three.playerMove)&&three.playerMove!="silence";
            bool canAct=open&&!acted&&three.playerStance!="declined"&&s.status=="playing";
            foreach(var button in new[]{tarotSkip,tarotObserve,tarotDeflect,tarotAskBack,tarotJoke})
                if(button)button.interactable=canAct;
            if(tarotAnswer)tarotAnswer.interactable=canAct&&sentCard==null;
            var request=s.replies?.LastOrDefault(r=>r.actor==selected&&r.status=="error")??s.replies?.LastOrDefault(r=>r.actor==selected);
            bool failed=request?.status=="error",waiting=request?.status=="running"||request?.status=="queued";
            replyStatus.text=failed?request.error
                :three.playerStance=="declined"?"你已经退出这一局，仍然可以看着。"
                :acted?"这一轮你已经表态了。"
                :waiting?"她正在想怎么回答。"
                :open?"可以回答，也可以跳过、反问、让别人先说，或者只是看着。"
                :"牌还没翻开。";
            retryReply.gameObject.SetActive(failed);
        }
        private void SendTarotMove(string move)
        {
            var three=Client.State?.scene3;
            if(three==null)return;
            string text=expression?expression.text.Trim():"";
            if((move=="ask_back"||move=="joke")&&string.IsNullOrEmpty(text)){Toast("先写一句想说的话。");return;}
            EventSystem.current?.SetSelectedGameObject(null);editing=false;
            Client.Send(new CommandDto{type="pause",paused=false});
            Client.Send(new CommandDto{type="tarot_move",intent=move,target=move=="ask_back"?selected:null,
                text=move=="ask_back"||move=="joke"?text:null,tone="natural"});
            if(move=="ask_back"||move=="joke")expression.text="";
            refresh=true;
        }
        private void SubmitTarotAnswer()
        {
            var text=expression.text.Trim();
            if(string.IsNullOrEmpty(text)){Toast("先写下你的回答，或者选择跳过。");return;}
            EventSystem.current?.SetSelectedGameObject(null);editing=false;
            Client.Send(new CommandDto{type="pause",paused=false});
            Client.Send(new CommandDto{type="tarot_answer",text=text,target=selected,tone="natural"});
            expression.text="";
            refresh=true;
        }
    }
}
