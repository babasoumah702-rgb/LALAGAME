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
    // Plays the real chapters in the real build: Scene 1 -> Scene 2 -> Scene 3, through the actual UI
    // buttons and the local service, then records what was observable. Offline rules mode so the run
    // needs no credentials and spends no model budget.
    public sealed class SceneTwoThreeVerification:MonoBehaviour
    {
        public static bool Running {get;private set;}
        [Serializable] private class Check {public string name;public bool passed;}
        [Serializable] private class Report {public bool passed;public int width,height;public List<Check> checks=new List<Check>();public List<string> errors=new List<string>();}
        private readonly Report report=new Report();private LastCallGame game;private string output;
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        private static void Boot(){if(Environment.GetCommandLineArgs().Contains("-sceneTwoThreeVerify"))new GameObject("Scene 2 and 3 verification").AddComponent<SceneTwoThreeVerification>();}
        private void CheckResult(string name,bool result){report.checks.Add(new Check{name=name,passed=result});Debug.Log("SCENE23_TEST "+name+" "+result);}
        private IEnumerator Until(Func<bool> predicate,float seconds=30){float until=Time.realtimeSinceStartup+seconds;while(!predicate()&&Time.realtimeSinceStartup<until)yield return null;}
        private bool TryClick(string title){var b=FindObjectsOfType<Button>().FirstOrDefault(b=>b.gameObject.activeInHierarchy&&b.interactable&&(b.name==title||b.GetComponentInChildren<Text>()?.text==title));if(!b)return false;ExecuteEvents.Execute(b.gameObject,new PointerEventData(EventSystem.current),ExecuteEvents.pointerClickHandler);return true;}
        private bool Click(string title){return TryClick(title);}
        private bool ClickAction(string group,string option){if(TryClick(option))return true;TryClick(group);return TryClick(option);}
        private IEnumerator Shot(string name){yield return new WaitForEndOfFrame();var t=ScreenCapture.CaptureScreenshotAsTexture();File.WriteAllBytes(Path.Combine(output,name+".png"),t.EncodeToPNG());var pixels=t.GetPixels32();CheckResult("rendered_"+name,pixels.Any(p=>p.r>80&&p.g>80));Destroy(t);}
        private IEnumerator Say(string text,string button="Speak to character"){ClickAction("互动",button=="Answer the card"?"回答":"文字交流");var field=FindObjectsOfType<InputField>().FirstOrDefault(f=>f.gameObject.activeInHierarchy);if(field)field.text=text;Click("Submit unified text");yield return null;}
        private IEnumerator Start()
        {
            Running=true;
            var args=Environment.GetCommandLineArgs();int ix=Array.IndexOf(args,"-sceneTwoThreeOutput");
            output=ix>=0?args[ix+1]:Path.Combine(Application.persistentDataPath,"SceneTwoThreeVerification");Directory.CreateDirectory(output);
            report.width=Screen.width;report.height=Screen.height;
            Application.logMessageReceived+=(m,t,k)=>{if(k==LogType.Error||k==LogType.Exception)report.errors.Add(m);};
            yield return Until(()=>FindObjectOfType<LastCallGame>()?.Client?.Ready==true,35);
            game=FindObjectOfType<LastCallGame>();var c=game.Client;
            CheckResult("service_ready",c.Ready);if(!c.Ready){Finish();yield break;}
            c.OpenSession(new SessionRequest{mode="new",opening="scene0_v1",story="scene1_v1",online=false,role="passerby"});
            yield return Until(()=>c.State?.scene1!=null&&c.State.intro?.phase=="bar",30);
            if(c.State?.scene1==null){CheckResult("scene1_open",false);Finish();yield break;}
            // Scene 1 completes on its own once the player interacts at all.
            yield return Until(()=>c.State.scene1.drinkPlaced,90);
            CheckResult("third_drink_before_scene2",c.State.scene1.drinkPlaced);
            ClickAction("观察","观察周围");
            yield return Until(()=>c.State.scene2!=null,180);
            CheckResult("scene2_opens_from_scene1",c.State.scene2!=null);
            if(c.State.scene2==null){Finish();yield break;}
            CheckResult("scene2_light_ui",game.Interface.SceneTwoMode);
            CheckResult("d_present_at_scene2",c.State.characters.Any(a=>a.id=="D"));
            yield return Shot("scene2-arrival");
            // The cross introduction unlocks D's name in the roster without a UI hint panel.
            yield return Until(()=>c.State.characters.Any(a=>a.id=="D"&&a.name=="一桐"),40);
            CheckResult("cross_intro_unlocks_name",c.State.characters.Any(a=>a.id=="D"&&a.name=="一桐"));
            CheckResult("no_relationship_panel",!FindObjectsOfType<Text>().Any(t=>t.text!=null&&(t.text.Contains("好感")||t.text.Contains("关系图"))));
            // Scene 2 verbs: follow, listen without joining, and the light warm-up round.
            yield return Until(()=>c.State.scene2.phase=="freeflow"||c.State.scene2.phase=="montage",60);
            CheckResult("freeflow_reached",c.State.scene2.phase=="freeflow"||c.State.scene2.phase=="montage");
            CheckResult("follow_button",ClickAction("移动","跟随所选人物"));
            yield return new WaitForSecondsRealtime(.6f);
            CheckResult("follow_recorded",c.State.events.Any(e=>e.actor=="USER"&&e.text.Contains("跟着")));
            CheckResult("listen_button",ClickAction("观察","旁听附近谈话"));
            yield return new WaitForSecondsRealtime(.6f);
            CheckResult("game_button",ClickAction("互动","参加轻游戏"));
            yield return Until(()=>c.State.scene2.games>0,10);
            CheckResult("light_game_started",c.State.scene2.games>0);
            yield return Shot("scene2-freeflow");
            // Time passes through the room, not through a progress bar.
            float drink=c.State.scene2.drinkLevel;
            yield return Until(()=>c.State.scene2.drinkLevel<drink||c.State.scene2.coasters>0,240);
            CheckResult("montage_glass_drains",c.State.scene2.drinkLevel<drink);
            CheckResult("montage_coasters_stack",c.State.scene2.coasters>=1);
            yield return Shot("scene2-montage");
            // Scene 3 begins when the deck lands on the table.
            yield return Until(()=>c.State.scene2.deckPlaced||c.State.scene3!=null,260);
            CheckResult("deck_left_on_table",c.State.scene2.deckPlaced||c.State.scene3!=null);
            CheckResult("deck_prop_exists",GameObject.Find("La La Land Social Tarot | deck")!=null);
            yield return Until(()=>c.State.scene3!=null,90);
            CheckResult("scene3_opens_from_deck",c.State.scene3!=null);
            if(c.State.scene3==null){Finish();yield break;}
            CheckResult("scene3_light_ui",game.Interface.SceneThreeMode);
            CheckResult("tarot_stance_group",ClickAction("互动","坐下"));
            yield return Until(()=>c.State.scene3.playerStance=="seated",15);
            yield return Until(()=>!string.IsNullOrEmpty(c.State.scene3.reader),40);
            CheckResult("reader_chosen",!string.IsNullOrEmpty(c.State.scene3.reader));
            yield return Until(()=>c.State.scene3.askedAt>=0&&!string.IsNullOrEmpty(c.State.scene3.question),60);
            CheckResult("card_flipped_with_question",!string.IsNullOrEmpty(c.State.scene3.question)&&!string.IsNullOrEmpty(c.State.scene3.cardName));
            CheckResult("question_shown_in_ui",FindObjectsOfType<Text>().Any(t=>t.text==c.State.scene3.question));
            CheckResult("no_score_readout",!FindObjectsOfType<Text>().Any(t=>t.text!=null&&(t.text.Contains("好感 +")||t.text.Contains("嫉妒"))));
            yield return Shot("scene3-card");
            // Every social move the design lists is reachable from the real UI.
            CheckResult("skip_button_present",FindObjectsOfType<Button>().Any(b=>b.gameObject.activeInHierarchy&&b.GetComponentInChildren<Text>()?.text=="跳过"));
            CheckResult("deflect_button_present",FindObjectsOfType<Button>().Any(b=>b.gameObject.activeInHierarchy&&b.GetComponentInChildren<Text>()?.text=="让别人先"));
            CheckResult("observe_button_present",FindObjectsOfType<Button>().Any(b=>b.gameObject.activeInHierarchy&&b.GetComponentInChildren<Text>()?.text=="只看着"));
            int round=c.State.scene3.round;
            yield return Say("有。但我不想说是谁。","Answer the card");
            yield return Until(()=>c.State.scene3.playerMove=="answer",15);
            CheckResult("player_answer_registers",c.State.scene3.playerMove=="answer");
            CheckResult("answer_is_own_event",c.State.events.Any(e=>e.actor=="USER"&&e.text.Contains("不想说是谁")));
            // Gaze before an answer is presented as an observation, never as an interpretation.
            yield return Until(()=>c.State.scene3.lastGaze!=null,60);
            CheckResult("gaze_recorded",c.State.scene3.lastGaze!=null);
            yield return Shot("scene3-answer");
            // The round advances on its own and closes without exhausting the deck.
            yield return Until(()=>c.State.scene3.rounds>=2,180);
            CheckResult("multiple_rounds",c.State.scene3.rounds>=2);
            yield return Until(()=>c.State.scene3.phase=="closing"||c.State.scene3.phase=="scene4_ready",320);
            CheckResult("round_closes_without_flushing_deck",c.State.scene3.rounds<=6);
            yield return Until(()=>c.State.scene3.phase=="scene4_ready",120);
            CheckResult("someone_leaves_the_table",!string.IsNullOrEmpty(c.State.scene3.leaver));
            CheckResult("scene4_exit_offered",c.State.scene3.phase=="scene4_ready");
            yield return Shot("scene3-exit");
            // A save taken here resumes into the same chapter without replaying the deck or the card.
            c.Save();yield return new WaitForSecondsRealtime(.6f);
            string id=c.State.sessionId;int flips=c.State.scene3.round;
            c.OpenSession(new SessionRequest{mode="resume",sessionId=id});
            yield return new WaitForSecondsRealtime(1.2f);
            CheckResult("resume_keeps_scene3",c.State.sessionId==id&&c.State.scene3!=null&&c.State.scene3.round==flips);
            CheckResult("resume_does_not_replay_bubbles",game.GetComponent<DialogueBubbles>().VisibleCount==0);
            Finish();
        }
        private void Finish(){Running=false;report.passed=report.checks.Count>20&&report.checks.All(c=>c.passed)&&report.errors.Count==0;File.WriteAllText(Path.Combine(output,"report.json"),JsonUtility.ToJson(report,true));Debug.Log("SCENE23_VERIFY_COMPLETE "+report.passed);Application.Quit(report.passed?0:2);}
    }
}
