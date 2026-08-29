using System.Collections;
using System.Linq;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using UnityEngine.InputSystem;

namespace LastCall
{
    public sealed partial class LastCallInterface
    {
        private CommandDto queuedCard,sentCard;
        private string approachId;
        private float travelTime,retryAt;
        private bool partyStarting;
        private Text partyStatus;

        private void SelectCard(string id)
        {
            CancelQueuedCard();cardId=id;
            if(expression)expression.text="";
            RefreshWorld();
        }
        private void RefreshPlayButton(CardDto card,ActorDto target)
        {
            bool needsParty=card.type=="situation"&&card.id!="last_call"&&!Client.State.cardsJoined;
            string title=sentCard!=null?"正在确认出牌…":queuedCard!=null?"正在走近 · 点击取消":
                needsParty?"开局 / 加入牌局":!card.ready?card.cooldownRemaining>0?"冷却 "+Mathf.CeilToInt(card.cooldownRemaining)+" 秒":"Last Call 时开放":
                target!=null&&!target.interactable?"走近并出牌":"出牌 · "+card.name;
            sendButton.GetComponentInChildren<Text>().text=title;
            sendButton.interactable=Client.State.status=="playing"&&sentCard==null&&
                (queuedCard!=null||needsParty||(target!=null&&card.ready&&!Client.State.busy));
        }
        private void Submit()
        {
            if(queuedCard!=null){CancelQueuedCard();Toast("已取消，表达仍然保留。");return;}
            if(sentCard!=null)return;
            var card=Client.State.cards.FirstOrDefault(c=>c.id==cardId);
            if(card==null)return;
            if(card.type=="situation"&&card.id!="last_call"&&!Client.State.cardsJoined){ShowParty();return;}
            if(!card.ready){Toast(card.lockReason);return;}
            EventSystem.current?.SetSelectedGameObject(null);editing=false;
            Client.Send(new CommandDto{type="pause",paused=false});
            queuedCard=new CommandDto{type="card",target=selected,card=cardId,
                text=string.IsNullOrWhiteSpace(expression.text)?card.expressions[0]:expression.text};
            travelTime=retryAt=0;refresh=true;
            Toast("准备出牌。距离太远时会先走近，按 WASD 或再次点击可取消。");
        }
        private void TickCardFlow()
        {
            if(queuedCard==null)return;
            if(Client.State.status!="playing"){queuedCard=null;return;}
            if(Client.State.paused||Blocking)return;
            var keys=Keyboard.current;
            if(keys!=null&&(keys.wKey.isPressed||keys.aKey.isPressed||keys.sKey.isPressed||keys.dKey.isPressed||
                keys.upArrowKey.isPressed||keys.downArrowKey.isPressed||keys.leftArrowKey.isPressed||keys.rightArrowKey.isPressed)){
                CancelQueuedCard();Toast("已取消自动出牌，表达仍然保留。");return;
            }
            var target=Client.State.characters.FirstOrDefault(a=>a.id==queuedCard.target);
            if(target==null){CancelQueuedCard();Toast("对方已经离开，请选择另一个人。");return;}
            if(target.interactable){
                Client.Send(new CommandDto{type="cancel_move"});
                sentCard=queuedCard;queuedCard=null;Client.Send(sentCard);refresh=true;return;
            }
            travelTime+=Time.unscaledDeltaTime;
            if(travelTime>20){CancelQueuedCard();Toast("通道暂时受阻，请手动靠近后再出牌；表达已保留。");return;}
            var player=Client.State.characters.First(a=>a.id=="USER");
            if(travelTime>=retryAt&&(player.route==null||player.route.Length==0)){
                retryAt=travelTime+1;
                var approach=new CommandDto{type="approach_target",target=queuedCard.target};
                Client.Send(approach);approachId=approach.id;
            }
        }
        private void CancelQueuedCard()
        {
            if(queuedCard!=null&&Client.State?.status=="playing")Client.Send(new CommandDto{type="cancel_move"});
            queuedCard=null;approachId=null;refresh=true;
        }
        private void CardAcknowledged(string id)
        {
            UnifiedInteractionAcknowledged(id);
            if(sentCard==null||sentCard.id!=id)return;
            bool talk=sentCard.type=="talk";
            string name=Client.State.cards.FirstOrDefault(c=>c.id==sentCard.card)?.name??"卡牌";
            if(expression&&expression.text==sentCard.text)expression.text="";
            sentCard=null;refresh=true;Toast(talk?"已说出，等待对方回应。": "已出牌「"+name+"」，等待对方回应；对方也可以拒绝。");
            if(!pauseVisible&&!notesVisible&&!introInputVisible)Client.Send(new CommandDto{type="pause",paused=false});
        }
        private void CardRejected(string id,string reason)
        {
            UnifiedInteractionRejected(id);
            if(sentCard?.id==id){sentCard=null;refresh=true;if(!pauseVisible&&!notesVisible&&!introInputVisible)Client.Send(new CommandDto{type="pause",paused=false});}
            if(approachId==id){queuedCard=null;approachId=null;refresh=true;}
        }
        private IEnumerator StartPartyWhenReady()
        {
            if(partyStarting)yield break;
            partyStarting=true;string sessionId=Client.State.sessionId;
            float until=Time.realtimeSinceStartup+20;
            if(partyStatus)partyStatus.text="正在准备牌局；当前回复结束后加入。";
            while(notesVisible&&Client.State.sessionId==sessionId&&Client.State.busy&&Time.realtimeSinceStartup<until)yield return null;
            partyStarting=false;
            if(!notesVisible||Client.State.sessionId!=sessionId)yield break;
            if(Client.State.busy){if(partyStatus)partyStatus.text="当前回复尚未结束，请稍后重试，或在暂停菜单切换规则模式。";yield break;}
            CloseNotes();Client.Send(new CommandDto{type="start_party"});
            Toast("加入后选择情境牌和人物，再点击出牌。");
        }
    }
}
