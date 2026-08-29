using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnityEngine;
using UnityEngine.InputSystem;
using UnityEngine.InputSystem.LowLevel;

namespace LastCall
{
    public sealed class SceneZeroVerification : MonoBehaviour
    {
        [Serializable] private class Check { public string name,detail;public bool passed; }
        [Serializable] private class Report {
            public string mode,device,messageSource;public int width,height,calls;
            public float introSeconds,averageFps;public bool passed;
            public List<Check> checks=new List<Check>();public List<string> errors=new List<string>();
        }
        private Report report=new Report();
        private LastCallGame game;
        private string output;
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        private static void Boot(){
            if(Environment.GetCommandLineArgs().Contains("-scene0Verify"))new GameObject("Scene0 verification").AddComponent<SceneZeroVerification>();
        }
        private void CheckResult(string name,bool passed,string detail=""){report.checks.Add(new Check{name=name,passed=passed,detail=detail});Debug.Log("SCENE0_TEST "+name+" "+passed+" "+detail);}
        private IEnumerator Until(Func<bool> done,float seconds=20){
            float limit=Time.realtimeSinceStartup+seconds;while(!done()&&Time.realtimeSinceStartup<limit)yield return null;
        }
        private IEnumerator Shot(string name){
            yield return new WaitForEndOfFrame();var image=ScreenCapture.CaptureScreenshotAsTexture();
            File.WriteAllBytes(Path.Combine(output,name+".png"),image.EncodeToPNG());Destroy(image);
        }
        private IEnumerator Start(){
            var args=Environment.GetCommandLineArgs();
            output=Arg(args,"-scene0Output",Path.Combine(Application.persistentDataPath,"scene0-verification"));
            Directory.CreateDirectory(output);
            report.mode=args.Contains("-scene0Online")?"online":"offline";report.width=Screen.width;report.height=Screen.height;report.device=SystemInfo.graphicsDeviceName;
            Application.logMessageReceived+=(message,trace,type)=>{if(type==LogType.Exception||type==LogType.Error)report.errors.Add(message);};
            yield return Until(()=>FindObjectOfType<LastCallGame>()?.Client?.Ready==true,35);
            game=FindObjectOfType<LastCallGame>();
            CheckResult("backend_ready",game&&game.Client.Ready);
            if(!game||!game.Client.Ready){Finish();yield break;}
            var c=game.Client;
            yield return Shot("entry");
            c.OpenSession(new SessionRequest{opening="scene0_v1",mode="new",online=report.mode=="online",entryMode="friend_invited",role="passerby",entryIntent="observe_only",style="natural"});
            yield return Until(()=>c.State?.intro?.phase=="elevator");
            CheckResult("elevator_started",c.State?.intro?.phase=="elevator");
            if(c.State?.intro?.phase!="elevator"){Finish();yield break;}
            float start=Time.realtimeSinceStartup;
            yield return Until(()=>game.Intro.Progress>=.6f);yield return Shot("elevator");
            yield return Until(()=>game.Intro.Progress>=2.7f);yield return Shot("phone");
            CheckResult("night_clock_held",c.State.elapsed==0&&c.State.events.Length==0);
            CheckResult("cast_and_privacy",new[]{"A","B","C"}.All(id=>c.State.characters.Any(a=>a.id==id))&&!c.State.characters.Any(a=>a.id=="D")&&c.State.cards.Length==0);
            yield return Until(()=>game.Intro.Progress>=6.25f);yield return Shot("doors");
            yield return Until(()=>game.Intro.Progress>=6.8f);yield return Shot("threshold");
            yield return Until(()=>c.State?.intro?.phase=="bar");
            report.introSeconds=Time.realtimeSinceStartup-start;report.calls=c.State.calls;report.messageSource=c.State.intro.messageSource;
            CheckResult("automatic_seven_second_handoff",report.introSeconds>=6&&report.introSeconds<=8f,report.introSeconds.ToString("F3")+" seconds");
            yield return new WaitForSecondsRealtime(.4f);yield return Shot("bar");
            CheckResult("all_models_packaged",game.Avatars.Where(p=>p.Key!="USER").All(p=>p.Value.GetComponentsInChildren<Transform>(true).Any(t=>t.name=="Cast mesh")));
            var player=game.Avatars["USER"];var before=player.transform.position;
            if(Keyboard.current!=null){
                InputSystem.QueueStateEvent(Keyboard.current,new KeyboardState(Key.W));
                yield return new WaitForSecondsRealtime(.3f);
                InputSystem.QueueStateEvent(Keyboard.current,new KeyboardState());
            }
            CheckResult("bar_movement_restored",Vector3.Distance(before,player.transform.position)>.2f);
            int frames=0;float sample=Time.realtimeSinceStartup;while(Time.realtimeSinceStartup-sample<5){frames++;yield return null;}
            report.averageFps=frames/(Time.realtimeSinceStartup-sample);
            c.OpenSession(new SessionRequest{opening="scene0_v1",mode="new",online=false,role="passerby",entryMode="solo"});
            string previous=c.State.sessionId;
            yield return Until(()=>c.State.sessionId!=previous&&c.State.intro?.phase=="elevator");
            yield return Until(()=>game.Intro.Progress>=2.5f);
            game.Interface.OpenIntroInput();yield return Until(()=>c.State.paused);
            float frozen=c.State.intro.progress;before=game.Avatars["USER"].transform.position;
            var input=FindObjectsOfType<UnityEngine.UI.InputField>().FirstOrDefault(f=>f.gameObject.activeInHierarchy);
            if(input)input.text="有一点好奇，也有一点紧张。";
            if(Keyboard.current!=null)InputSystem.QueueStateEvent(Keyboard.current,new KeyboardState(Key.W));
            yield return new WaitForSecondsRealtime(.65f);
            if(Keyboard.current!=null)InputSystem.QueueStateEvent(Keyboard.current,new KeyboardState());
            CheckResult("chinese_input_freezes_scene",Mathf.Abs(c.State.intro.progress-frozen)<.01f&&Vector3.Distance(before,game.Avatars["USER"].transform.position)<.01f);
            yield return Shot("text-input");
            if(input)input.text="有一点好奇，也有一点紧张。";
            game.Interface.CloseIntroInput(true);
            yield return Until(()=>!c.State.paused);CheckResult("private_text_stored",c.State.intro.playerText.Contains("好奇")&&c.State.events.Length==0);
            game.Interface.Pause(true);yield return Until(()=>c.State.paused);frozen=c.State.intro.progress;
            yield return new WaitForSecondsRealtime(.3f);CheckResult("pause_freezes_progress",Mathf.Abs(c.State.intro.progress-frozen)<.01f);
            c.Save();yield return new WaitForSecondsRealtime(.3f);
            string id=c.State.sessionId;c.OpenSession(new SessionRequest{mode="resume",sessionId=id});
            yield return new WaitForSecondsRealtime(.8f);CheckResult("resume_preserves_checkpoint",c.State.sessionId==id&&Mathf.Abs(c.State.intro.progress-frozen)<.1f);
            game.Interface.Pause(false);yield return Until(()=>!c.State.paused);
            game.SendMessage("OnApplicationFocus",false);yield return Until(()=>c.State.paused);
            CheckResult("focus_loss_pauses",c.State.paused&&game.Interface.IsPaused);
            game.Interface.Pause(false);
            float resumeLimit=Time.realtimeSinceStartup+20;
            while(c.State.intro.phase!="bar"&&Time.realtimeSinceStartup<resumeLimit){
                // Test runner explicitly resumes if another desktop app steals focus; production still pauses.
                if(c.State.paused&&game.Interface.IsPaused&&!Application.isFocused)game.Interface.Pause(false);
                yield return null;
            }
            CheckResult("resume_handoff",c.State.intro.phase=="bar","progress="+c.State.intro.progress+", paused="+c.State.paused+", localFrozen="+game.Intro.LocallyFrozen);
            Finish();
        }
        private static string Arg(string[] args,string name,string fallback){int index=Array.IndexOf(args,name);return index>=0&&index+1<args.Length?args[index+1]:fallback;}
        private void Finish(){
            report.passed=report.checks.All(c=>c.passed)&&report.errors.Count==0;
            File.WriteAllText(Path.Combine(output,"report.json"),JsonUtility.ToJson(report,true));
            Debug.Log("SCENE0_VERIFICATION_COMPLETE "+report.passed);
            if(game)game.Client.Save();Application.Quit(report.passed?0:2);
        }
    }
}
