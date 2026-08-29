using UnityEngine;
using UnityEngine.InputSystem;
using UnityEngine.UI;

namespace LastCall
{
    [DefaultExecutionOrder(150)]
    public sealed class SceneZeroController : MonoBehaviour
    {
        public Transform leftDoor,rightDoor,phoneRig;
        public TextMesh floorDisplay;
        public Canvas phoneScreen;
        public Text phoneMessage,phoneHint,phoneSource;
        public Vector3 startEye=new Vector3(-1,1.64f,-8.65f);
        public Vector3 endEye=new Vector3(-1,1.64f,-4.25f);
        public float fieldOfView=62;
        public float Progress { get; private set; }
        public bool Active=>game&&game.Client?.State?.intro?.phase=="elevator";
        public bool LocallyFrozen=>game&&(game.Interface.IntroEditing||game.Interface.IsPaused);
        private LastCallGame game;
        private string session;
        private bool wasActive,completionSent;
        private Vector2 look;
        private Camera cam;
        public void Bind(LastCallGame owner){game=owner;cam=Camera.main;phoneScreen.GetComponent<Button>().onClick.AddListener(TogglePhone);}
        private void Update()
        {
            if(!game||game.Client.State==null)return;
            var state=game.Client.State;
            if(session!=state.sessionId)
            {
                session=state.sessionId;completionSent=false;look=Vector2.zero;
                Progress=state.intro?.progress??7;wasActive=false;
                if(Active)game.Client.Send(new CommandDto{type="intro_ready"});
                else SetDoors(1);
            }
            if(!Active)
            {
                if(wasActive)
                {
                    phoneRig.gameObject.SetActive(false);
                    SetDoors(1);
                    var lens=game.GetComponent<DirectorLens>();if(lens)lens.TakePose(8,3);
                }
                wasActive=false;return;
            }
            wasActive=true;
            bool frozen=state.paused||LocallyFrozen;
            if(!frozen)
            {
                Progress=Mathf.Min(7,Mathf.Max(Progress,state.intro.progress));
                Progress=Mathf.Min(state.intro.progress+.15f,Progress+Time.unscaledDeltaTime);
                if(Mouse.current!=null&&Mouse.current.rightButton.isPressed)
                {
                    var delta=Mouse.current.delta.ReadValue();
                    look.x=Mathf.Clamp(look.x+delta.x*.08f,-18,18);
                    look.y=Mathf.Clamp(look.y-delta.y*.08f,-12,12);
                }
                if(Keyboard.current?.eKey.wasPressedThisFrame==true)TogglePhone();
            }
            if(state.intro.progress>=7&&!frozen&&!completionSent)
            {completionSent=true;game.Client.Send(new CommandDto{type="intro_complete"});}
        }
        private void LateUpdate()
        {
            if(!Active||!cam)return;
            var i=game.Client.State.intro;
            float t=Progress;
            float lift=Mathf.SmoothStep(0,1,Mathf.InverseLerp(1.3f,2.3f,t));
            float lower=Mathf.SmoothStep(0,1,Mathf.InverseLerp(4.2f,5.4f,t));
            float door=Mathf.SmoothStep(0,1,Mathf.InverseLerp(5.4f,6.35f,t));
            float move=Mathf.SmoothStep(0,1,Mathf.InverseLerp(6.15f,7,t));
            cam.rect=new Rect(0,0,1,1);cam.orthographic=false;cam.fieldOfView=fieldOfView;
            cam.transform.position=Vector3.Lerp(startEye,endEye,move);
            cam.transform.rotation=Quaternion.Euler(Mathf.Lerp(-3,3,move)+30*lift*(1-lower)+look.y*(1-move),8*move+look.x*(1-move),0);
            SetDoors(door);
            floorDisplay.text=t<5.4f?"↑  "+Mathf.Min(33,28+Mathf.FloorToInt(t*1.05f)):"33";
            phoneRig.SetParent(cam.transform,false);
            phoneRig.gameObject.SetActive(i.phoneVisible&&t>=1.3f&&t<5.4f);
            phoneRig.localPosition=new Vector3(.035f,Mathf.Lerp(-.65f,-.10f,lift)*(1-lower)-.65f*lower,.61f);
            phoneRig.localRotation=Quaternion.Euler(-5,0,-6);
            phoneScreen.worldCamera=cam;
            if(game.Interface.SharedFont)foreach(var text in phoneScreen.GetComponentsInChildren<Text>())text.font=game.Interface.SharedFont;
            phoneMessage.text=t>=2.3f?i.message:"";
            phoneHint.text=t>=2.3f?i.hint:"";
            phoneSource.text=t>=2.3f?(i.messageSource=="model"?"在线生成":"预设文案"):"一条新消息";
            // The first turn is only a visible gesture, never a leak of the backstage exchange.
            if(t>5.7f&&game.Avatars.TryGetValue("B",out var b))
            {
                var root=b.GetComponent<BarPrototype.PlayerMotor>()?.VisualRoot;
                var direction=cam.transform.position-b.transform.position;direction.y=0;
                if(root&&direction.sqrMagnitude>.01f)root.rotation=Quaternion.Slerp(root.rotation,Quaternion.LookRotation(direction),Time.unscaledDeltaTime*2);
            }
        }
        public void TogglePhone()
        {
            if(!Active||LocallyFrozen||game.Client.State.paused)return;
            game.Client.Send(new CommandDto{type="intro_phone",open=!game.Client.State.intro.phoneVisible});
        }
        public void SetDoors(float openness)
        {
            leftDoor.localPosition=new Vector3(-.52f-1.05f*openness,1.4f,-6.6f);
            rightDoor.localPosition=new Vector3(.52f+1.05f*openness,1.4f,-6.6f);
        }
    }
}
