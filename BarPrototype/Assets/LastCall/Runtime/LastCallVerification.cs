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

namespace LastCall
{
    // Explicit opt-in. Normal players never run this or write verification artifacts.
    public sealed class LastCallVerification : MonoBehaviour
    {
        [Serializable] private class Check { public string name,detail; public bool passed; }
        [Serializable] private class Report
        {
            public string version,device,mode;
            public int width,height,calls;
            public float averageFps;
            public bool passed,resumed;
            public List<Check> checks=new List<Check>();
            public List<string> errors=new List<string>();
        }
        private readonly Report report=new Report();
        private LastCallGame game;
        private LocalServiceClient client;
        private string output;
        private bool finished;
        private float deadline;
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        private static void Boot()
        {
            if(Environment.GetCommandLineArgs().Contains("-lastCallVerify"))
                new GameObject("Last Call verification").AddComponent<LastCallVerification>();
        }
        private void CheckResult(string name,bool pass,string detail="")
        {
            report.checks.Add(new Check{name=name,passed=pass,detail=detail});
            Debug.Log("LASTCALL_TEST "+(pass?"PASS ":"FAIL ")+name+" "+detail);
        }
        private IEnumerator Until(Func<bool> predicate,float seconds=15)
        {
            float end=Time.realtimeSinceStartup+seconds;
            while(!predicate()&&Time.realtimeSinceStartup<end)yield return null;
        }
        private IEnumerator Travel(string location,string expected)
        {
            float timeout=Time.realtimeSinceStartup+35;
            float retry=0;
            while(Time.realtimeSinceStartup<timeout)
            {
                var player=client.State.characters.First(a=>a.id=="USER");
                if(player.location==expected&&player.route.Length==0)yield break;
                if(player.route.Length==0&&Time.realtimeSinceStartup>retry)
                {
                    retry=Time.realtimeSinceStartup+2;
                    game.Interface.Pause(false);
                    yield return Until(()=>!client.State.paused&&!client.State.busy,10);
                    client.Send(new CommandDto{type="move_to",location=location});
                }
                yield return new WaitForSecondsRealtime(.2f);
            }
        }
        private void Update(){if(!finished&&Time.realtimeSinceStartup>deadline)Finish();}
        private IEnumerator Start()
        {
            var args=Environment.GetCommandLineArgs();
            int index=Array.IndexOf(args,"-lastCallArtifacts");
            output=index>=0&&index+1<args.Length?args[index+1]:Path.Combine(Application.persistentDataPath,"Verification");
            Directory.CreateDirectory(output);
            deadline=Time.realtimeSinceStartup+(args.Contains("-lastCallFullNight")?1500:210);
            report.version=Application.unityVersion;report.device=SystemInfo.graphicsDeviceName;
            report.width=Screen.width;report.height=Screen.height;
            Application.logMessageReceived+=Log;
            game=FindObjectOfType<LastCallGame>();client=game.Client;
            yield return Until(()=>client.Ready,30);
            CheckResult("backend_bootstrap",client.Ready);
            if(!client.Ready){Finish();yield break;}
            yield return new WaitForSecondsRealtime(1);
            ScreenCapture.CaptureScreenshot(Path.Combine(output,"entry.png"));
            var save=args.Contains("-lastCallResume")?client.Bootstrap.sessions.FirstOrDefault(s=>s.status=="playing"):null;
            report.resumed=save!=null;
            client.OpenSession(new SessionRequest{mode=save==null?"new":"resume",sessionId=save?.id,role="social_guest",online=args.Contains("-lastCallOnline"),seed=821});
            yield return Until(()=>client.State!=null&&game.Avatars.ContainsKey("USER"));
            CheckResult("session_created",client.State!=null&&game.Avatars.ContainsKey("USER"));
            if(client.State==null){Finish();yield break;}
            yield return new WaitForSecondsRealtime(1);
            game.Interface.Pause(false);
            yield return new WaitForSecondsRealtime(.4f);
            CheckResult("opening_within_ten_seconds",client.State.events.Any(e=>e.text.Contains("替你点")));
            client.Send(new CommandDto{type="approach_target",target="B"});
            yield return Until(()=>client.State.characters.First(a=>a.id=="B").interactable,20);
            yield return new WaitForSecondsRealtime(.6f);
            CheckResult("physical_approach",client.State.characters.First(a=>a.id=="B").interactable);
            client.Send(new CommandDto{type="cancel_move"});
            client.Send(new CommandDto{type="card",target="B",card="reveal",text="今晚我其实是为了见你。"});
            yield return Until(()=>client.State.events.Any(e=>e.name=="B"&&e.hasParent),40);
            CheckResult("independent_reply",client.State.events.Any(e=>e.name=="B"&&e.hasParent));
            yield return Until(()=>!client.State.busy,30);
            game.Interface.Pause(true);
            yield return Until(()=>client.State.paused,5);
            var elapsed=client.State.elapsed;
            yield return new WaitForSecondsRealtime(1);
            CheckResult("pause_freezes_world",client.State.paused&&Mathf.Abs(client.State.elapsed-elapsed)<.05f,
                "paused="+client.State.paused+" delta="+(client.State.elapsed-elapsed).ToString("F3"));
            game.Interface.Pause(false);
            yield return new WaitForSecondsRealtime(.5f);
            var input=game.Interface.GetComponentInChildren<InputField>();
            input.ActivateInputField();
            yield return new WaitForSecondsRealtime(.4f);
            input.text="这个问题我现在不想回答。";
            var before=game.Avatars["USER"].transform.position;
            if(Keyboard.current!=null)InputSystem.QueueStateEvent(Keyboard.current,new KeyboardState(Key.W));
            yield return new WaitForSecondsRealtime(.5f);
            if(Keyboard.current!=null)InputSystem.QueueStateEvent(Keyboard.current,new KeyboardState());
            CheckResult("chinese_edit_blocks_movement",input.isFocused&&Vector3.Distance(before,game.Avatars["USER"].transform.position)<.04f);
            EventSystem.current.SetSelectedGameObject(null);
            input.text="";
            yield return new WaitForSecondsRealtime(.5f);
            ScreenCapture.CaptureScreenshot(Path.Combine(output,"scene.png"));
            float start=Time.realtimeSinceStartup;int frames=0;
            while(Time.realtimeSinceStartup-start<5){frames++;yield return null;}
            report.averageFps=frames/(Time.realtimeSinceStartup-start);
            CheckResult("performance_sample",report.averageFps>=50,report.averageFps.ToString("F1")+" FPS (5 seconds)");
            client.Save();
            yield return new WaitForSecondsRealtime(.4f);
            if(args.Contains("-lastCallFullNight"))
            {
                yield return Travel("outside","门外透气区");
                CheckResult("terrace_navigation",client.State.characters.First(a=>a.id=="USER").location=="门外透气区");
                yield return Travel("bar","吧台");
                CheckResult("return_from_terrace",client.State.characters.First(a=>a.id=="USER").location=="吧台");
                bool declined=false;
                while(client.State.status=="playing")
                {
                    if(client.State.cardsOffered&&!declined){client.Send(new CommandDto{type="decline"});declined=true;}
                    yield return new WaitForSecondsRealtime(1);
                }
                CheckResult("natural_full_night",client.State.elapsed>=720&&declined);
            }
            else client.Send(new CommandDto{type="leave"});
            yield return Until(()=>client.State.status=="ended");
            CheckResult("leave_and_reflection",client.State.status=="ended"&&client.State.reflection!=null);
            report.mode=client.State.mode;report.calls=client.State.calls;
            yield return new WaitForSecondsRealtime(.5f);
            ScreenCapture.CaptureScreenshot(Path.Combine(output,"reflection.png"));
            yield return new WaitForSecondsRealtime(.5f);
            Finish();
        }
        private void Log(string text,string trace,LogType type)
        {
            if(type==LogType.Exception||type==LogType.Error)report.errors.Add(text);
        }
        private void Finish()
        {
            if(finished)return;finished=true;
            report.passed=report.errors.Count==0&&report.checks.Count>=9&&report.checks.All(c=>c.passed);
            if(!string.IsNullOrEmpty(output))File.WriteAllText(Path.Combine(output,"report.json"),JsonUtility.ToJson(report,true));
            Application.logMessageReceived-=Log;
            Application.Quit(report.passed?0:1);
        }
    }
}
