using System.Linq;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;

namespace LastCall
{
    public sealed partial class LastCallInterface
    {
        private bool cardsExpanded;
        private Text replyStatus;
        private Button retryReply;
        public bool SceneOneMode=>Client?.State?.scene1!=null&&Client?.State?.late==null&&!cardsExpanded;
        public bool OverlayOpen=>entryVisible||pauseVisible||notesVisible||introInputVisible;
        public static string GenerationLabel(string source)=>source=="ai"?"AI":source=="rules"?"离线规则":source=="script"?"场景事件":source=="player"?"你的表达":"历史来源未知";
        private void BuildSceneOne()
        {
            string draft=expression?expression.text:"";
            Clear(root);expression=null;rightPanel=null;pausePanel=null;notesPanel=null;
            size=new Vector2(Width,Height);Camera.main.rect=new Rect(0,0,1,1);
            Label(root,"LA LA LAND",24,16,310,38,27);
            Label(root,"第三杯",26,54,260,24,15,muted);
            clockText=Label(root,"",Width-240,24,210,32,20);
            modeText=Label(root,"",24,83,Width-48,28,13,muted);
            targetText=Label(root,"",24,Height-240,Width-48,28,18);
            var people=Client.State.characters.Where(a=>a.id!="USER"&&a.id!="OWNER").ToArray();
            lastActors=string.Join(",",people.Select(a=>a.id+":"+a.name));
            for(int i=0;i<people.Length;i++){
                string id=people[i].id;
                ActionButton(root,people[i].name,24+i*146,Height-202,138,32,()=>Select(id));
            }
            ActionButton(root,"观察",24,Height-159,84,34,()=>Client.Send(new CommandDto{type="observe"}));
            ActionButton(root,"靠近",116,Height-159,84,34,()=>Client.Send(new CommandDto{type="approach_target",target=selected}));
            ActionButton(root,"看第三杯",208,Height-159,105,34,()=>Client.Send(new CommandDto{type="observe_object",objectTarget="third_drink"}));
            ActionButton(root,"坐空椅",321,Height-159,94,34,()=>Client.Send(new CommandDto{type="sit_reserved"}));
            ActionButton(root,"去哪里",423,Height-159,90,34,ShowLocations);
            ActionButton(root,"线索册",521,Height-159,90,34,ShowNotes);
            ActionButton(root,"卡牌",619,Height-159,78,34,()=>{cardsExpanded=true;BuildWorld();});
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
            RefreshSceneOne();
        }
        private void RefreshSceneOne()
        {
            if(!targetText||!replyStatus)return;
            var s=Client.State;var people=s.characters.Where(a=>a.id!="USER"&&a.id!="OWNER").ToArray();
            if(lastActors!=string.Join(",",people.Select(a=>a.id+":"+a.name))&&!Blocking){BuildSceneOne();return;}
            var target=people.FirstOrDefault(a=>a.id==selected);
            if(target==null){target=people.FirstOrDefault();selected=target?.id??"B";}
            targetText.text="对 "+(target?.name??"附近的人")+" 说话";
            clockText.text=s.scene1.phase=="scene2_ready"?"人到齐了 · 本段完成":s.clock;
            modeText.text=(s.mode=="online"?"在线 AI":"离线规则")+" · 本章 "+(s.story?.budgetCalls??s.calls)+" / 80 次调用";
            var request=s.replies?.LastOrDefault(r=>r.actor==selected&&r.status=="error")??s.replies?.LastOrDefault(r=>r.actor==selected);
            bool failed=request?.status=="error",waiting=request?.status=="running"||request?.status=="queued";
            replyStatus.text=failed?request.error:waiting?"对方正在回应；你可以继续观察。":"WASD 移动 · 右键转头 · 输入时暂停 · 你的话只发送一次";
            retryReply.gameObject.SetActive(failed);
            sendButton.GetComponentInChildren<Text>().text=queuedCard!=null?"走近中 · 取消":sentCard!=null?"发送中…":"交流";
            sendButton.interactable=sentCard==null&&s.status=="playing";
        }
        private void SubmitConversation()
        {
            if(queuedCard!=null){CancelQueuedCard();return;}
            if(sentCard!=null)return;
            var text=expression.text.Trim();if(string.IsNullOrEmpty(text)){Toast("先写一句想说的话。");return;}
            expression.DeactivateInputField();EventSystem.current?.SetSelectedGameObject(null);editing=false;
            Client.Send(new CommandDto{type="pause",paused=false});
            queuedCard=new CommandDto{type="talk",target=selected,text=text,tone="natural",movement="approach_if_needed"};
            travelTime=retryAt=0;refresh=true;
        }
    }
}
