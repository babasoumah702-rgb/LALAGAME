using System.Collections.Generic;
using BarPrototype;
using UnityEngine;
using UnityEngine.InputSystem;
namespace LastCall
{
    [DefaultExecutionOrder(230)]
    public sealed class NightPresentation:MonoBehaviour
    {
        public LastCallGame Game;
        public static bool CinematicActive {get;private set;}
        private string lastPosture;private NightCueDto cue;private float cueTime;private Quaternion priorRotation;private Vector3 priorPosition;
        private GameObject memorySet,prop;
        private readonly HashSet<string> played=new HashSet<string>();
        private void Start(){
            prop=GameObject.CreatePrimitive(PrimitiveType.Cube);prop.name="Chocolate cigarette packet | hand held";prop.transform.localScale=new Vector3(.055f,.018f,.11f);Destroy(prop.GetComponent<Collider>());prop.GetComponent<Renderer>().material=OtomeArt.Cloth(new Color(.35f,.19f,.13f),.2f);prop.SetActive(false);
            var stick=GameObject.CreatePrimitive(PrimitiveType.Cylinder);stick.name="Chocolate cigarette";stick.transform.SetParent(prop.transform,false);stick.transform.localPosition=new Vector3(.2f,1.1f,0);stick.transform.localScale=new Vector3(.1f,2.5f,.05f);Destroy(stick.GetComponent<Collider>());stick.GetComponent<Renderer>().material=OtomeArt.Cloth(new Color(.74f,.63f,.47f),.1f);
        }
        private void Update(){var state=Game.Client?.State;var s=state?.late;if(s==null){prop?.SetActive(false);return;}
            if(s.posture!=lastPosture){lastPosture=s.posture;if(s.posture=="sky"||s.posture=="lie")Game.GetComponent<DirectorLens>()?.TakePose(Camera.main.transform.eulerAngles.y,s.posture=="lie"?-70:-45);}
            ActorAvatar holder=null;foreach(var a in Game.Avatars.Values)if(a.State?.area=="corridor"&&!a.IsPlayer&&a.ActorId!="OWNER"){holder=a;break;}
            prop.SetActive(s.chapter==4&&holder&&holder.HandAnchor);if(prop.activeSelf){prop.transform.position=holder.HandAnchor.position;prop.transform.rotation=holder.HandAnchor.rotation;}
            var next=s.cue;if(next!=null&&!next.consumed&&!played.Contains(state.sessionId+next.id)&&cue==null){played.Add(state.sessionId+next.id);Begin(next);Game.Client.Send(new CommandDto{type="cinematic_ack",target=next.id});}
            if(cue==null)return;if(!state.paused||state.status=="ended")cueTime+=Time.unscaledDeltaTime;
            if(cueTime>=cue.duration||Keyboard.current!=null&&(Keyboard.current.spaceKey.wasPressedThisFrame||Keyboard.current.escapeKey.wasPressedThisFrame))End();
        }
        private void Begin(NightCueDto value){cue=value;cueTime=0;CinematicActive=true;priorPosition=Camera.main.transform.position;priorRotation=Camera.main.transform.rotation;
            if(cue.kind=="memory"){
                memorySet=new GameObject("Subjective recollection | not world fact");var floor=GameObject.CreatePrimitive(PrimitiveType.Cube);floor.transform.SetParent(memorySet.transform,false);floor.transform.position=new Vector3(100,-.08f,1);floor.transform.localScale=new Vector3(7,.15f,7);floor.GetComponent<Renderer>().material=OtomeArt.Cloth(new Color(.21f,.18f,.16f),.2f);
                var desk=GameObject.CreatePrimitive(PrimitiveType.Cube);desk.transform.SetParent(memorySet.transform,false);desk.transform.position=new Vector3(100,.7f,1.6f);desk.transform.localScale=new Vector3(1.4f,.1f,.75f);desk.GetComponent<Renderer>().material=OtomeArt.Cloth(new Color(.34f,.28f,.21f),.25f);
                string pair=cue.id.Replace("memory:","");
                if(pair=="BC"){desk.SetActive(false);MemoryProp("Hotel doorway",new Vector3(101,1.25f,2),new Vector3(.12f,2.5f,1.5f),new Color(.25f,.22f,.19f));}
                if(pair=="CD"){MemoryProp("Laptop",new Vector3(100,1.05f,1.6f),new Vector3(.5f,.3f,.035f),new Color(.13f,.18f,.22f));MemoryProp("Chocolate",new Vector3(99.6f,.78f,1.6f),new Vector3(.13f,.02f,.1f),new Color(.35f,.18f,.12f));}
                if(pair=="AC")MemoryProp("Unsigned paper",new Vector3(100,.762f,1.6f),new Vector3(.23f,.004f,.3f),new Color(.76f,.71f,.6f));
                for(int i=0;i<pair.Length;i++)if(Game.Avatars.TryGetValue(pair[i].ToString(),out var original)){var copy=Instantiate(original.GetComponent<PlayerMotor>().VisualRoot.gameObject,memorySet.transform);copy.name="Recollection cast | "+pair[i];copy.transform.position=new Vector3(99.25f+i*1.5f,0,1.3f);copy.transform.rotation=Quaternion.Euler(0,i==0?65:295,0);foreach(var mesh in copy.GetComponentsInChildren<MeshFilter>())if(mesh.sharedMesh)mesh.sharedMesh=Instantiate(mesh.sharedMesh);}
                var lamp=new GameObject("Memory practical",typeof(Light)).GetComponent<Light>();lamp.transform.SetParent(memorySet.transform,false);lamp.transform.position=new Vector3(100,3,-.2f);lamp.intensity=3;lamp.range=8;lamp.color=new Color(1,.79f,.55f);
            }
        }
        private void MemoryProp(string name,Vector3 p,Vector3 scale,Color color){var o=GameObject.CreatePrimitive(PrimitiveType.Cube);o.name=name;o.transform.SetParent(memorySet.transform,false);o.transform.position=p;o.transform.localScale=scale;o.GetComponent<Renderer>().sharedMaterial=OtomeArt.Cloth(color,.2f);}
        private void LateUpdate(){if(cue==null||!Camera.main)return;var c=Camera.main;float progress=Mathf.Clamp01(cueTime/cue.duration);if(cue.kind=="memory"){c.transform.position=new Vector3(100+progress*.12f,1.5f,-1.2f);c.transform.LookAt(new Vector3(100,1.1f,1.4f));}else{c.transform.position=Vector3.Lerp(new Vector3(1.3f,9,-1),new Vector3(1.3f,12,1),progress);c.transform.LookAt(new Vector3(1.3f,4.2f,5.6f));}}
        private void OnGUI(){if(cue==null)return;var style=new GUIStyle(GUI.skin.label){fontSize=Mathf.RoundToInt(Screen.height/40f),font=Game.Interface.SharedFont,wordWrap=true,alignment=TextAnchor.MiddleCenter};style.normal.textColor=new Color(.98f,.92f,.79f);GUI.Box(new Rect(0,0,Screen.width,Screen.height*.13f),"");GUI.Label(new Rect(40,18,Screen.width-80,Screen.height*.1f),cue.kind=="memory"?"主观回忆 · 观众看见的片段，不进入玩家事实记忆":"这一晚 · "+cue.text,style);GUI.Box(new Rect(0,Screen.height*.73f,Screen.width,Screen.height*.27f),"");if(cueaway())return;GUI.Label(new Rect(50,Screen.height*.76f,Screen.width-100,Screen.height*.15f),cue.kind=="memory"?cue.text:"还有一些话，留在夜风里。",style);if(GUI.Button(new Rect(Screen.width-170,Screen.height-48,145,32),"跳过 · 空格"))End();}
        private bool cueaway()=>cue==null;
        private void End(){cue=null;CinematicActive=false;if(memorySet)Destroy(memorySet);if(Camera.main){Camera.main.transform.SetPositionAndRotation(priorPosition,priorRotation);Game.GetComponent<DirectorLens>()?.TakePose(priorRotation.eulerAngles.y,priorRotation.eulerAngles.x);}}
        private void OnDestroy(){End();if(prop)Destroy(prop);}
    }
}
