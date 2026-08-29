using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnityEngine;
using UnityEngine.UI;
namespace LastCall
{
    // Opt-in visible player verification. Only isolated synthetic saves; no chapter injection.
    public sealed class FullNightVerification:MonoBehaviour
    {
        public static bool Running { get; private set; }
        [Serializable] private class Check{public string name,detail;public bool passed;}
        [Serializable] private class Result{public bool passed;public string kind="visible native story flow",clockScale="1";public List<Check> checks=new List<Check>();public List<string> errors=new List<string>();}
        private readonly Result result=new Result();private LastCallGame game;private string output;private bool finished,memoryShot;private float start,resumeAt;
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]private static void Boot(){if(Environment.GetCommandLineArgs().Contains("-fullNightVerify")){Running=true;new GameObject("Full night visible verification").AddComponent<FullNightVerification>();}}
        private void CheckIt(string name,bool ok,string detail=""){result.checks.Add(new Check{name=name,passed=ok,detail=detail});Debug.Log("NIGHT_TEST "+name+" "+ok+" "+detail);}
        private IEnumerator Until(Func<bool> ready,float seconds){float end=Time.realtimeSinceStartup+seconds;while(!ready()&&Time.realtimeSinceStartup<end)yield return null;}
        private void Send(string type,string intent=null,string location=null,string target=null,string text=null){game.Client.Send(new CommandDto{type=type,intent=intent,location=location,target=target,text=text});}
        private IEnumerator Shot(string name){yield return new WaitForEndOfFrame();var image=ScreenCapture.CaptureScreenshotAsTexture();File.WriteAllBytes(Path.Combine(output,name+".png"),image.EncodeToPNG());var pixels=image.GetPixels32();double mean=pixels.Average(p=>(p.r+p.g+p.b)/3.0);int lit=pixels.Count(p=>p.r>25||p.g>25||p.b>25),peak=pixels.Max(p=>Mathf.Max(p.r,Mathf.Max(p.g,p.b)));CheckIt("render_"+name,mean>8&&peak>100&&lit>image.width*image.height*.035f,"mean="+mean.ToString("F1"));Destroy(image);}
        private IEnumerator Resolutions(string name){foreach(var size in new[]{new Vector2Int(1280,720),new Vector2Int(1920,1080),new Vector2Int(1280,800)}){Screen.SetResolution(size.x,size.y,false);yield return new WaitForSecondsRealtime(2f);game.Interface.Pause(false);yield return new WaitForSecondsRealtime(.3f);yield return Shot(name+"-"+size.x+"x"+size.y);CheckIt("resolution_"+name+size,Screen.width==size.x&&Screen.height==size.y);}Screen.SetResolution(1280,720,false);yield return new WaitForSecondsRealtime(.5f);}
        private IEnumerator Start(){Application.runInBackground=true;start=Time.realtimeSinceStartup;var args=Environment.GetCommandLineArgs();int ix=Array.IndexOf(args,"-fullNightOutput");output=ix>=0?args[ix+1]:Path.Combine(Application.persistentDataPath,"FullNightVerification");Directory.CreateDirectory(output);int scale=Array.IndexOf(args,"-fullNightClock");if(scale>=0)result.clockScale=args[scale+1];Application.logMessageReceived+=(m,t,k)=>{if(k==LogType.Exception||k==LogType.Error)result.errors.Add(m);};
            yield return Until(()=>FindObjectOfType<LastCallGame>()?.Client?.Ready==true,40);game=FindObjectOfType<LastCallGame>();CheckIt("service_ready",game&&game.Client.Ready);if(!game||!game.Client.Ready){Finish();yield break;}
            var c=game.Client;if(args.Contains("-nightTravelOnly")){result.kind="visible native resumed travel regression";c.OpenSession(new SessionRequest{mode="resume",sessionId=c.Bootstrap.sessions[0].id});yield return Until(()=>c.State?.late!=null,20);game.Interface.Pause(false);yield return new WaitForSecondsRealtime(1);yield return RoofTravel();yield break;}c.OpenSession(new SessionRequest{mode="new",opening="scene0_v1",story="scene1_v1",online=false,role="passerby",seed=821});yield return Until(()=>c.State?.scene1!=null&&c.State.intro?.phase=="bar",40);CheckIt("elevator_to_bar",c.State?.intro?.phase=="bar");if(c.State?.scene1==null){Finish();yield break;}
            Send("observe");game.GetComponent<DirectorLens>().TakePose(90,0);yield return Resolutions("scene1-first-person");
            yield return Until(()=>c.State.scene1.drinkPlaced,90);CheckIt("third_drink_placed",c.State.scene1.drinkPlaced);CheckIt("original_cast_adapted",game.Avatars["A"].GetComponent<CastActionAdapter>()!=null);yield return Shot("third-drink");
            yield return Until(()=>c.State.scene2!=null,180);CheckIt("scene2_from_single_arrival",c.State.scene2!=null&&c.State.characters.Any(a=>a.id=="D"));if(c.State.scene2==null){Finish();yield break;}
            Send("move_to",location:"main_table");yield return new WaitForSecondsRealtime(8);yield return Shot("scene2-social");
            yield return Until(()=>c.State.scene3!=null,340);CheckIt("passive_social_reaches_tarot",c.State.scene3!=null);if(c.State.scene3==null){Finish();yield break;}
            Send("move_to",location:"main_table");yield return new WaitForSecondsRealtime(4);Send("tarot_seat",text:"watch");yield return Until(()=>c.State.scene3==null||!string.IsNullOrEmpty(c.State.scene3.question),55);CheckIt("perceived_card_text",c.State.scene3!=null&&!string.IsNullOrEmpty(c.State.scene3.question));yield return Resolutions("scene3-card");
            Send("tarot_seat",text:"decline");yield return new WaitForSecondsRealtime(.4f);
            CheckIt("tarot_stance_locked_after_watch",c.State.scene3!=null&&c.State.scene3.playerStance=="watching"&&!FindObjectsOfType<Button>().Any(b=>b.GetComponentInChildren<Text>()?.text=="不参加"));
            yield return Until(()=>c.State.late?.chapter==4,320);CheckIt("scene4_after_tarot",c.State.late?.chapter==4);if(c.State.late==null){Finish();yield break;}
            Send("night_move",location:"corridor");yield return Until(()=>c.State.late.area=="corridor",40);bool reachedCorridor=c.State.late.area=="corridor";CheckIt("walked_into_corridor",reachedCorridor);if(!reachedCorridor){DumpTravel();File.WriteAllText(Path.Combine(output,"stuck-corridor-state.json"),JsonUtility.ToJson(c.State,true));}game.GetComponent<DirectorLens>().TakePose(90,0);yield return Shot("scene4-corridor");yield return Until(()=>memoryShot,55);CheckIt("observed_subjective_memory_played",memoryShot);if(args.Contains("-fullNightCorridorOnly")){Finish();yield break;}
            c.Save();yield return new WaitForSecondsRealtime(.7f);string save=c.State.sessionId;c.OpenSession(new SessionRequest{mode="resume",sessionId=save});yield return new WaitForSecondsRealtime(1.5f);CheckIt("corridor_save_resumes",c.State.sessionId==save&&c.State.late!=null);CheckIt("resume_no_bubble_replay",game.GetComponent<DialogueBubbles>().VisibleCount==0);game.Interface.Pause(false);
            yield return Until(()=>c.State.late.chapter==5,160);CheckIt("scene5_even_without_more_dialogue",c.State.late.chapter==5);Send("night_move",location:"bar");yield return Until(()=>c.State.late.area=="bar",40);CheckIt("returned_through_corridor_door",c.State.late.area=="bar");yield return Shot("scene5-return");
            yield return Until(()=>c.State.late.powerState=="emergency",270);CheckIt("powercut_emergency",c.State.late.powerState=="emergency");yield return Resolutions("powercut");
            yield return RoofTravel();

        }
        private IEnumerator RoofTravel(){var c=game.Client;yield return new WaitForSecondsRealtime(1);game.Interface.Pause(false);yield return new WaitForSecondsRealtime(.7f);CheckIt("travel_controls_unpaused",!c.State.paused&&!game.Interface.Blocking);
            Send("night_move",location:"rooftop");yield return Until(()=>c.State.characters.First(a=>a.id=="USER").area=="stairs",45);CheckIt("real_stairs_entered",c.State.characters.First(a=>a.id=="USER").area=="stairs");yield return Shot("stairs");
            yield return Until(()=>c.State.late.chapter==6,60);DumpTravel();CheckIt("roof_reached_without_teleport",c.State.late.chapter==6&&game.Avatars["USER"].transform.position.y>4);if(c.State.late.chapter!=6){File.WriteAllText(Path.Combine(output,"stuck-state.json"),JsonUtility.ToJson(c.State,true));Finish();yield break;}
            yield return Until(()=>c.State.characters.First(a=>a.id=="USER").route.Length==0,15);game.GetComponent<DirectorLens>().TakePose(320,-4);yield return Resolutions("scene6-roof");
            Send("night_pose","sit");yield return new WaitForSecondsRealtime(1.2f);CheckIt("roof_sit",c.State.late.posture=="sit");yield return Shot("roof-sit");Send("night_pose","lie");yield return new WaitForSecondsRealtime(1.2f);CheckIt("roof_lie",c.State.late.posture=="lie");yield return Shot("roof-lie");Send("night_pose","stand");yield return new WaitForSecondsRealtime(1);
            CheckIt("waits_for_end_confirmation",c.State.status=="playing");Send("end_night");yield return new WaitForSecondsRealtime(.5f);yield return Shot("ending-aerial");yield return Until(()=>c.State.status=="ended",12);yield return Until(()=>!NightPresentation.CinematicActive,8);yield return new WaitForSecondsRealtime(.5f);CheckIt("perception_based_recap",c.State.reflection!=null);yield return Shot("recap");Finish();
        }
        private void DumpTravel(){Debug.Log("TRAVEL_GATE paused "+game.Client.State.paused+" interface "+game.Interface.Blocking+" cinematic "+NightPresentation.CinematicActive+" intro "+(game.Intro&&game.Intro.Active));foreach(var a in game.Avatars.Values){if(a.State?.route==null||a.State.route.Length==0)continue;var p=a.transform.position;var goal=a.ActiveWaypoint??a.State.route[0];var direction=new Vector3(goal.x-p.x,0,goal.z-p.z).normalized;var hits=Physics.SphereCastAll(p+Vector3.up*.6f,.24f,direction,1.4f);Debug.Log("TRAVEL_DIAG "+a.ActorId+" blocked "+a.LastBlocked+" local "+p+" target "+goal.x+","+goal.y+","+goal.z+" state "+a.State.x+","+a.State.y+","+a.State.z+" blockers "+string.Join(" | ",hits.Select(h=>h.collider.name+" "+h.collider.bounds+" parent "+h.collider.transform.parent?.name)));}}
        private void Update(){
            // Test automation keeps its visible window running when the tool host briefly takes focus.
            // This component is never installed in an ordinary player session.
            if(!finished&&game&&game.Client.State?.status=="playing"&&(game.Client.State.paused||game.Interface.IsPaused||game.Interface.Talking)&&Time.realtimeSinceStartup>resumeAt){resumeAt=Time.realtimeSinceStartup+1;UnityEngine.EventSystems.EventSystem.current?.SetSelectedGameObject(null);game.Interface.Pause(false);}
            if(!finished&&game&&game.Client.State?.late?.chapter==4&&NightPresentation.CinematicActive&&!memoryShot){memoryShot=true;StartCoroutine(Shot("scene4-subjective-memory"));}if(!finished&&start>0&&Time.realtimeSinceStartup-start>1400){CheckIt("watchdog",false);Finish();}}
        private void Finish(){if(finished)return;finished=true;result.passed=result.checks.Count>10&&result.checks.All(x=>x.passed)&&result.errors.Count==0;File.WriteAllText(Path.Combine(output,"report.json"),JsonUtility.ToJson(result,true));Debug.Log("NIGHT_VERIFY_COMPLETE "+result.passed);Application.Quit(result.passed?0:2);}
        private void OnDestroy(){Running=false;}
    }
}
