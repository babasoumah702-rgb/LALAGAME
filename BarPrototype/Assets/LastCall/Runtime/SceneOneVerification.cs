using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using UnityEngine.InputSystem;
using UnityEngine.InputSystem.LowLevel;
using BarPrototype;

namespace LastCall
{
    public sealed class SceneOneVerification:MonoBehaviour
    {
        public static bool Running {get;private set;}
        [Serializable] private class Check {public string name;public bool passed;}
        [Serializable] private class Report {public bool passed;public int width,height;public List<Check> checks=new List<Check>();public List<string> errors=new List<string>();}
        private readonly Report report=new Report();private LastCallGame game;private string output;
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        private static void Boot(){if(Environment.GetCommandLineArgs().Contains("-sceneOneVerify"))new GameObject("Scene One verification").AddComponent<SceneOneVerification>();}
        private void CheckResult(string name,bool result){report.checks.Add(new Check{name=name,passed=result});Debug.Log("SCENE1_TEST "+name+" "+result);}
        private IEnumerator Until(Func<bool> predicate,float seconds=25){float until=Time.realtimeSinceStartup+seconds;while(!predicate()&&Time.realtimeSinceStartup<until)yield return null;}
        private IEnumerator UntilWorld(Func<bool> predicate,float seconds){float until=Time.realtimeSinceStartup+seconds;while(!predicate()&&Time.realtimeSinceStartup<until){if(game?.Client?.State?.paused==true)game.Interface.Pause(false);yield return null;}}
        private bool TryClick(string title){var b=FindObjectsOfType<Button>().FirstOrDefault(b=>b.gameObject.activeInHierarchy&&b.interactable&&(b.name==title||b.GetComponentInChildren<Text>()?.text==title));if(!b)return false;ExecuteEvents.Execute(b.gameObject,new PointerEventData(EventSystem.current),ExecuteEvents.pointerClickHandler);return true;}
        private bool Click(string title){if(TryClick(title))return true;CheckResult("button_"+title,false);return false;}
        private bool Visible(string title){return FindObjectsOfType<Button>().Any(b=>b.gameObject.activeInHierarchy&&b.GetComponentInChildren<Text>()?.text==title);}
        private bool Disabled(string title){return FindObjectsOfType<Button>().Any(b=>b.gameObject.activeInHierarchy&&!b.interactable&&b.GetComponentInChildren<Text>()?.text==title);}
        private bool ClickAction(string group,string option){if(TryClick(option))return true;TryClick(group);return TryClick(option);}
        private IEnumerator Shot(string name){yield return new WaitForEndOfFrame();var t=ScreenCapture.CaptureScreenshotAsTexture();File.WriteAllBytes(Path.Combine(output,name+".png"),t.EncodeToPNG());var pixels=t.GetPixels32();CheckResult("rendered_"+name,pixels.Any(p=>p.r>80&&p.g>80));Destroy(t);}
        private IEnumerator Say(string text){ClickAction("互动","文字交流");var field=FindObjectsOfType<InputField>().FirstOrDefault(f=>f.gameObject.activeInHierarchy);if(field)field.text=text;Click("Submit unified text");yield return null;}
        private Rect BubbleBounds(string actor){var r=GameObject.Find("Bubble "+actor)?.GetComponent<RectTransform>();if(!r)return new Rect();var corners=new Vector3[4];r.GetWorldCorners(corners);return new Rect(corners[0],corners[2]-corners[0]);}
        private void CheckHumanoid(string id){var a=game.Avatars[id];var rig=a.GetComponent<HumanoidCastAnimator>();CheckResult("humanoid_"+id,rig&&rig.IsHumanoid&&rig.Head&&rig.RightHand&&rig.LeftHand);}
        // Deliberately synthetic, presentation-only fixtures. These never enter the world, saves, or model history.
        private IEnumerator BubbleFixture(){
            var client=game.Client;client.Send(new CommandDto{type="release_facing"});client.Send(new CommandDto{type="pause",paused=true});yield return Until(()=>client.State.paused);
            var lens=game.GetComponent<DirectorLens>();var old=Camera.main.transform.eulerAngles;
            var bAvatar=game.Avatars["B"];var cAvatar=game.Avatars["C"];
            var bPosition=bAvatar.transform.position;var cPosition=cAvatar.transform.position;
            var bVisual=bAvatar.GetComponent<PlayerMotor>().VisualRoot;var cVisual=cAvatar.GetComponent<PlayerMotor>().VisualRoot;
            var bRotation=bVisual.rotation;var cRotation=cVisual.rotation;
            client.enabled=false;
            var userPosition=game.Avatars["USER"].transform.position;
            bAvatar.transform.position=userPosition+new Vector3(-.45f,0,-1.6f);cAvatar.transform.position=userPosition+new Vector3(.65f,0,-2.0f);
            bVisual.rotation=cVisual.rotation=Quaternion.identity;
            var component=game.GetComponent<DialogueBubbles>();var add=typeof(DialogueBubbles).GetMethod("Add",System.Reflection.BindingFlags.NonPublic|System.Reflection.BindingFlags.Instance);
            string longText="这是中文长句排版测试：窗外下着雨，桌上放着一杯尚未有人碰过的酒。我们可以先聊聊今晚的音乐，再慢慢说那些还没说清楚的话，不需要急着给彼此下定义。第二段只用于检查自动分段和气泡之间的留白。";
            add.Invoke(component,new object[]{new EventDto{actor="B",name="排版测试 B",text=longText,type="speech",generationSource="script",level="full"}});
            add.Invoke(component,new object[]{new EventDto{actor="C",name="排版测试 C",text="这是一段多人气泡的测试文字，只检查显示位置。",type="speech",generationSource="script",level="full"}});
            var b=game.Avatars["B"].HeadAnchor;var c=game.Avatars["C"].HeadAnchor;
            var rotation=Quaternion.LookRotation((b+c)/2-Camera.main.transform.position).eulerAngles;lens.TakePose(rotation.y,rotation.x);yield return new WaitForSecondsRealtime(.5f);
            var bb=BubbleBounds("B");var cb=BubbleBounds("C");
            CheckResult("fixture_long_chinese_paginated",GameObject.Find("Bubble B")?.GetComponentInChildren<Text>().text.Contains("1/2")==true);
            CheckResult("fixture_multiple_nonoverlap",bb.width>0&&cb.width>0&&!bb.Overlaps(cb));yield return Shot("bubble-fixture");
            lens.TakePose(rotation.y+180,0);yield return new WaitForSecondsRealtime(.3f);CheckResult("offscreen_bubbles_hidden",BubbleBounds("B").width==0&&BubbleBounds("C").width==0);
            var blocker=GameObject.CreatePrimitive(PrimitiveType.Cube);blocker.name="Verification occlusion wall";blocker.transform.position=Vector3.Lerp(Camera.main.transform.position,b,.5f);blocker.transform.localScale=new Vector3(.55f,3,.55f);Physics.SyncTransforms();
            var bLook=Quaternion.LookRotation(b-Camera.main.transform.position).eulerAngles;
            lens.TakePose(bLook.y,bLook.x);yield return new WaitForSecondsRealtime(.3f);CheckResult("occluded_bubble_hidden",BubbleBounds("B").width==0);Destroy(blocker);
            bAvatar.transform.position=bPosition;cAvatar.transform.position=cPosition;bVisual.rotation=bRotation;cVisual.rotation=cRotation;
            client.enabled=true;lens.TakePose(old.y,old.x);EventSystem.current?.SetSelectedGameObject(null);
            game.Interface.Pause(false);yield return Until(()=>!client.State.paused);CheckResult("fixture_restores_world_time",!client.State.paused);
        }
        private IEnumerator Start(){
            Running=true;
            var args=Environment.GetCommandLineArgs();int ix=Array.IndexOf(args,"-sceneOneOutput");output=ix>=0?args[ix+1]:Path.Combine(Application.persistentDataPath,"SceneOneVerification");Directory.CreateDirectory(output);
            report.width=Screen.width;report.height=Screen.height;Application.logMessageReceived+=(m,t,k)=>{if(k==LogType.Error||k==LogType.Exception)report.errors.Add(m);};
            yield return Until(()=>FindObjectOfType<LastCallGame>()?.Client?.Ready==true,35);game=FindObjectOfType<LastCallGame>();var c=game.Client;
            CheckResult("service_ready",c.Ready);if(!c.Ready){Finish();yield break;}
            yield return Until(()=>Application.isFocused,12);CheckResult("visible_window_focus",Application.isFocused);yield return Shot("entry");
            string modelButton=Visible("模型 API · 已配置")?"模型 API · 已配置":"填写模型 API";
            CheckResult("entry_model_api_button",Visible(modelButton));Click(modelButton);yield return null;
            CheckResult("model_api_form",Visible("保存并启用在线模型")&&FindObjectsOfType<InputField>().Any(f=>f.gameObject.activeInHierarchy&&f.contentType==InputField.ContentType.Password));
            Click("返回更多设置");yield return null;Click("返回首页");yield return null;
            if(Click("在线模型 · 开"))yield return null;
            CheckResult("entry_landing_has_three_step_path",Visible("开始新的夜晚")&&Visible("更多设置"));
            Click("开始新的夜晚");yield return Until(()=>Visible("临时路过"),3);
            CheckResult("entry_page1_all_answers",c.Bootstrap.roles.All(x=>Visible(x.name))&&Visible("下一步")&&Visible("返回"));
            Click(c.Bootstrap.roles[0].name);yield return null;Click("下一步");yield return Until(()=>Visible(c.Bootstrap.intents[0].name),3);Click("返回");yield return Until(()=>Visible(c.Bootstrap.roles[0].name),3);
            CheckResult("entry_back_keeps_role",Disabled(c.Bootstrap.roles[0].name));
            Click(c.Bootstrap.roles[1].name);yield return null;Click("下一步");yield return Until(()=>Visible(c.Bootstrap.intents[0].name),3);
            CheckResult("entry_page2_all_answers",c.Bootstrap.intents.All(x=>Visible(x.name))&&Visible("下一步")&&Visible("返回"));
            Click(c.Bootstrap.intents[0].name);yield return null;Click("下一步");yield return Until(()=>Visible(c.Bootstrap.styles[0].name),3);Click("返回");yield return Until(()=>Visible(c.Bootstrap.intents[0].name),3);
            CheckResult("entry_back_keeps_intent",Disabled(c.Bootstrap.intents[0].name));
            Click(c.Bootstrap.intents[1].name);yield return null;Click("下一步");yield return Until(()=>Visible(c.Bootstrap.styles[0].name),3);
            CheckResult("entry_page3_all_answers",c.Bootstrap.styles.All(x=>Visible(x.name))&&Visible("下一步")&&Visible("返回"));
            Click(c.Bootstrap.styles[0].name);yield return null;Click("下一步");
            yield return Until(()=>c.State?.intro?.phase=="bar",25);CheckResult("scene0_handoff",c.State?.scene1!=null&&c.State.intro.phase=="bar");
            if(c.State?.scene1==null){Finish();yield break;}
            yield return new WaitForSecondsRealtime(.6f);CheckResult("light_ui",game.Interface.SceneOneMode);foreach(var castId in new[]{"A","B","C"})CheckHumanoid(castId);CheckResult("a_seated_pose",(game.Avatars["A"].GetComponent<CastActionAdapter>()?.IsSeated==true||game.Avatars["A"].GetComponent<SceneOneSeatedPose>()?.IsSeated==true));yield return Shot("first-meeting");
            var aDir=game.Avatars["A"].HeadAnchor-Camera.main.transform.position;var aAngles=Quaternion.LookRotation(aDir).eulerAngles;game.GetComponent<DirectorLens>().TakePose(aAngles.y,aAngles.x);yield return new WaitForSecondsRealtime(.3f);yield return Shot("a-seated");
            ClickAction("移动","靠近所选人物");yield return Until(()=>c.State.characters.First(a=>a.id=="USER").route.Length>0,5);
            yield return Until(()=>c.State.characters.First(a=>a.id=="USER").route.Length==0,30);
            yield return Say("你好，你叫什么名字？");yield return Until(()=>c.State.events.Any(e=>e.actor=="B"&&e.type=="speech"&&e.text.Contains("X")),25);
            CheckResult("legal_name_from_reply",c.State.characters.First(a=>a.id=="B").name=="X");
            yield return new WaitForSecondsRealtime(.8f);CheckResult("b_uses_talk_animation",game.Avatars["B"].GetComponent<HumanoidCastAnimator>()?.CurrentState=="TalkB");CheckResult("bubble_visible",game.GetComponent<DialogueBubbles>().VisibleCount>0);yield return Shot("conversation");
            var caption=BubbleBounds("USER");CheckResult("player_caption_below_face",caption.width>0&&caption.yMax<Camera.main.WorldToScreenPoint(game.Avatars["B"].HeadAnchor-Vector3.up*.12f).y);
            var b=game.Avatars["B"];var u=game.Avatars["USER"];var direction=(u.transform.position-b.transform.position).normalized;
            CheckResult("npc_faces_player",Vector3.Dot(b.GetComponent<PlayerMotor>().VisualRoot.forward,direction)>.8f);
            yield return Until(()=>c.State.scene1.drinkPlaced,60);CheckResult("third_drink_delivered",c.State.scene1.drinkPlaced);yield return Shot("third-drink");
            yield return Say("这杯酒是留给谁的？");yield return Until(()=>c.State.events.Any(e=>e.actor=="USER"&&e.objectTarget=="third_drink"),20);
            CheckResult("free_text_object_target",c.State.events.Any(e=>e.actor=="USER"&&e.objectTarget=="third_drink"));
            var before=u.transform.position;if(Keyboard.current!=null){InputSystem.QueueStateEvent(Keyboard.current,new KeyboardState(Key.W));yield return new WaitForSecondsRealtime(.35f);InputSystem.QueueStateEvent(Keyboard.current,new KeyboardState());}
            CheckResult("keyboard_movement",Vector3.Distance(before,u.transform.position)>.1f);
            yield return BubbleFixture();
            if(args.Contains("-sceneOneQuick")){Finish();yield break;}
            ClickAction("观察","观察周围");
            yield return UntilWorld(()=>c.State.characters.Any(a=>a.id=="D"),150);
            if(game.Avatars.TryGetValue("D",out var arrival)){
                var look=Quaternion.LookRotation(arrival.HeadAnchor-Camera.main.transform.position).eulerAngles;game.GetComponent<DirectorLens>().TakePose(look.y,look.x);
                yield return new WaitForSecondsRealtime(.3f);CheckHumanoid("D");var dState=arrival.GetComponent<HumanoidCastAnimator>()?.CurrentState;CheckResult("d_has_live_animation",dState=="Walk"||dState=="IdleD"||dState=="Phone");CheckResult("d_phone_on_entry",GameObject.Find("D phone | screen hidden")!=null);yield return Shot("d-entry");
            }
            yield return UntilWorld(()=>c.State.scene1.phase=="scene2_ready",35);CheckResult("d_arrival_and_chapter_end",c.State.scene1.phase=="scene2_ready"&&c.State.characters.Any(a=>a.id=="D"));
            if(game.Avatars.TryGetValue("D",out var arrived)){var look=Quaternion.LookRotation(arrived.HeadAnchor-Camera.main.transform.position).eulerAngles;game.GetComponent<DirectorLens>().TakePose(look.y,look.x);}
            yield return new WaitForSecondsRealtime(.3f);yield return Shot("d-arrival");c.Save();yield return new WaitForSecondsRealtime(.5f);string id=c.State.sessionId;
            c.OpenSession(new SessionRequest{mode="resume",sessionId=id});yield return new WaitForSecondsRealtime(1);
            CheckResult("resume_keeps_chapter",c.State.sessionId==id&&c.State.scene1.phase=="scene2_ready");
            CheckResult("resume_does_not_replay_bubbles",game.GetComponent<DialogueBubbles>().VisibleCount==0);
            Finish();
        }
        private void Finish(){Running=false;report.passed=report.checks.Count>8&&report.checks.All(c=>c.passed)&&report.errors.Count==0;File.WriteAllText(Path.Combine(output,"report.json"),JsonUtility.ToJson(report,true));Debug.Log("SCENE1_VERIFY_COMPLETE "+report.passed);Application.Quit(report.passed?0:2);}
    }
}
