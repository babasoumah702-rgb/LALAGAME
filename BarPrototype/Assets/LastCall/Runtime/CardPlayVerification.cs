using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;

namespace LastCall
{
    public sealed class CardPlayVerification:MonoBehaviour
    {
        [Serializable] private class Check {public string name;public bool passed;}
        [Serializable] private class Report {public bool passed;public List<Check> checks=new List<Check>();public List<string> errors=new List<string>();}
        private readonly Report report=new Report();private LastCallGame game;private string output;
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        private static void Boot(){if(Environment.GetCommandLineArgs().Contains("-cardPlayVerify"))new GameObject("Card play verification").AddComponent<CardPlayVerification>();}
        private void CheckResult(string name,bool value){report.checks.Add(new Check{name=name,passed=value});Debug.Log("CARD_TEST "+name+" "+value);}
        private IEnumerator Until(Func<bool> predicate,float seconds=25){float end=Time.realtimeSinceStartup+seconds;while(!predicate()&&Time.realtimeSinceStartup<end)yield return null;}
        private bool Click(string name){
            var button=FindObjectsOfType<Button>().FirstOrDefault(b=>b.gameObject.activeInHierarchy&&b.interactable&&(b.name==name||b.GetComponentInChildren<Text>()?.text==name));
            if(!button){CheckResult("button_exists_"+name,false);return false;}
            ExecuteEvents.Execute(button.gameObject,new PointerEventData(EventSystem.current){button=PointerEventData.InputButton.Left},ExecuteEvents.pointerClickHandler);return true;
        }
        private IEnumerator Shot(string name){yield return new WaitForEndOfFrame();var texture=ScreenCapture.CaptureScreenshotAsTexture();File.WriteAllBytes(Path.Combine(output,name+".png"),texture.EncodeToPNG());Destroy(texture);}
        private IEnumerator Start(){
            var args=Environment.GetCommandLineArgs();int index=Array.IndexOf(args,"-cardPlayOutput");
            output=index>=0?args[index+1]:Path.Combine(Application.persistentDataPath,"CardPlayVerification");Directory.CreateDirectory(output);
            Application.logMessageReceived+=(m,t,k)=>{if(k==LogType.Error||k==LogType.Exception)report.errors.Add(m);};
            yield return Until(()=>FindObjectOfType<LastCallGame>()?.Client?.Ready==true,35);
            game=FindObjectOfType<LastCallGame>();var c=game.Client;CheckResult("service_ready",c.Ready);
            if(!c.Ready){Finish();yield break;}
            c.OpenSession(new SessionRequest{mode="new",opening="scene0_v1",role="passerby",online=false});
            yield return Until(()=>c.State?.intro?.phase=="bar"&&!c.State.busy);
            if(c.State?.intro?.phase!="bar"){CheckResult("intro_finished",false);Finish();yield break;}
            yield return new WaitForSecondsRealtime(.3f);
            CheckResult("default_role_starts_locked",!c.State.cardsJoined&&!c.State.cards.First(x=>x.id=="truth").ready);
            Click("真心话");yield return null;
            CheckResult("locked_card_can_be_inspected",FindObjectsOfType<Text>().Any(t=>t.text.Contains("加入牌局后开放")));
            Click("Play selected card");yield return new WaitForSecondsRealtime(.25f);yield return Shot("party-entry");
            Click("请老板娘开局并加入");yield return Until(()=>c.State.cardsJoined);
            CheckResult("default_role_joins_without_three_minute_wait",c.State.cardsJoined&&c.State.elapsed<30);
            yield return Until(()=>!c.State.busy&&!c.State.paused);yield return new WaitForSecondsRealtime(.3f);
            var truth=c.State.cards.First(x=>x.id=="truth");
            Click(truth.expressions[0]);yield return null;
            var before=game.Avatars["USER"].transform.position;
            Click("Play selected card");
            yield return Until(()=>c.State.events.Any(e=>e.actor=="USER"&&e.text==truth.expressions[0]),30);
            CheckResult("ui_card_reaches_backend",c.State.events.Count(e=>e.actor=="USER"&&e.text==truth.expressions[0])==1);
            CheckResult("automatic_physical_approach",Vector3.Distance(before,game.Avatars["USER"].transform.position)>.5f);
            CheckResult("cooldown_explained",c.State.cards.First(x=>x.id=="truth").cooldownRemaining>0);
            var played=c.State.events.LastOrDefault(e=>e.actor=="USER"&&e.text==truth.expressions[0]);
            int playedSequence=played?.seq??int.MaxValue;
            yield return Until(()=>c.State.events.Any(e=>e.actor=="B"&&e.hasParent&&e.seq>playedSequence),25);
            CheckResult("independent_npc_response",c.State.events.Any(e=>e.actor=="B"&&e.hasParent&&e.seq>playedSequence));
            yield return new WaitForSecondsRealtime(.2f);
            yield return Shot("card-response");
            yield return Until(()=>!c.State.busy&&!c.State.paused);
            Click("点酒");yield return null;Click("来一杯旧时光吧。");yield return null;Click("Play selected card");
            yield return Until(()=>c.State.pastDrink);
            CheckResult("second_card_effect_applied",c.State.pastDrink);
            yield return Until(()=>!c.State.busy&&!c.State.paused);
            Click("牌局");yield return new WaitForSecondsRealtime(.2f);Click("退出牌局，继续聊天");
            yield return Until(()=>!c.State.cardsJoined);CheckResult("leave_party_without_ending_night",!c.State.cardsJoined&&c.State.status=="playing");
            Click("牌局");yield return new WaitForSecondsRealtime(.2f);Click("加入牌局");yield return Until(()=>c.State.cardsJoined);
            CheckResult("can_rejoin",c.State.cardsJoined);
            c.Send(new CommandDto{type="leave"});yield return Until(()=>c.State.status=="ended");
            CheckResult("reflection_still_works",c.State.reflection!=null);
            Finish();
        }
        private void Finish(){report.passed=report.checks.Count>=12&&report.checks.All(c=>c.passed)&&report.errors.Count==0;File.WriteAllText(Path.Combine(output,"report.json"),JsonUtility.ToJson(report,true));Debug.Log("CARD_VERIFICATION_COMPLETE "+report.passed);Application.Quit(report.passed?0:2);}
    }
}
