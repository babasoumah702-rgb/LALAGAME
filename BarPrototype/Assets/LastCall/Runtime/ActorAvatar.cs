using BarPrototype;
using System.Collections.Generic;
using UnityEngine;

namespace LastCall
{
    public sealed class ActorAvatar : MonoBehaviour
    {
        public string ActorId;
        public ActorDto State;
        public bool IsPlayer;
        private CharacterController controller;
        private PlayerMotor motor;
        private CharacterPose pose;
        private Transform visual;
        private float vertical;
        private int routeVersion=-1;
        private readonly HashSet<Vector2> reached=new HashSet<Vector2>();
        public PointDto ActiveWaypoint { get; private set; }
        public bool LastBlocked { get; private set; }
        public float MotionSpeed { get; private set; }
        public void Setup(string id, bool player)
        {
            ActorId = id;
            IsPlayer = player;
            controller = GetComponent<CharacterController>();
            motor = GetComponent<PlayerMotor>();
            pose = GetComponent<CharacterPose>();
            visual = motor.VisualRoot;
            if (!player)
            {
                motor.enabled = false;
                pose.useExternalMotion = true;
                controller.radius = .22f;
                if(HumanoidCastAnimator.Supports(id))
                {
                    controller.height=CastModel.HeightOf(id);
                    controller.center=new Vector3(0,controller.height*.5f+.01f,0);
                }
            }
        }
        public void Place(ActorDto state)
        {
            State = state;
            visual.rotation=Quaternion.Euler(0,state.yaw,0);
            controller.enabled = false;
            transform.position = new Vector3(state.x, state.y+.09f, state.z);
            controller.enabled = true;
        }
        public void Tick(bool blocked)
        {
            if (State == null) return;
            LastBlocked=blocked;ActiveWaypoint=null;
            var before = transform.position;
            var route = State.route;
            if(routeVersion!=State.routeVersion){routeVersion=State.routeVersion;reached.Clear();}
            int index=0;
            if(route!=null)
            {
                while(index<route.Length)
                {
                    var key=new Vector2(route[index].x,route[index].z);
                    var position=new Vector2(transform.position.x,transform.position.z);
                    if(!reached.Contains(key)&&Vector2.Distance(position,key)>.085f)break;
                    reached.Add(key);index++;
                }
            }
            var auto = route != null && index < route.Length;
            motor.InputBlocked = blocked || auto || State.posture=="sit" || State.posture=="lie";
            if (blocked) { MotionSpeed=0;pose.externalSpeed = 0; return; }
            if (auto)
            {
                ActiveWaypoint=route[index];
                var target = new Vector3(route[index].x, transform.position.y, route[index].z);
                // Cap displacement with the speed that will actually be used below. The player
                // walks at 2.8 while NPCs walk at 2.1; using the NPC divisor for both could make
                // a player overshoot a waypoint forever when a frame was slow.
                var speed=IsPlayer?motor.WalkSpeed:2.1f;
                var direction = Vector3.ClampMagnitude((target - transform.position) / Mathf.Max(Time.deltaTime * speed, .01f), 1);
                // A route is already expressed in world space. Driving the player route through
                // camera-relative input made unattended movement depend on focus and camera pose.
                // Direct CharacterController motion keeps the same world collision as NPC travel.
                if (controller.isGrounded) vertical = -2;
                vertical -= 20 * Time.deltaTime;
                controller.Move((direction * speed + Vector3.up * vertical) * Time.deltaTime);
                if (direction.sqrMagnitude > .01f)
                    visual.rotation = Quaternion.RotateTowards(visual.rotation, Quaternion.LookRotation(direction), Time.deltaTime * 360);
            }
            else if (!IsPlayer)
            {
                if (controller.isGrounded) vertical = -2;
                vertical -= 20 * Time.deltaTime;
                controller.Move(Vector3.up * vertical * Time.deltaTime);
                visual.rotation = Quaternion.RotateTowards(visual.rotation, Quaternion.Euler(0, State.yaw, 0), Time.deltaTime * 120);
            }
            MotionSpeed = Vector3.Distance(before, transform.position) / Mathf.Max(Time.deltaTime, .001f);
            pose.externalSpeed = MotionSpeed;
            pose.presentation = State.animation;
        }
        public CommandDto Report()
        {
            return new CommandDto { type = "position", actor = ActorId, x = transform.position.x,y=Mathf.Max(0,transform.position.y-.09f),area=NightStage.Area(transform.position), z = transform.position.z, yaw = visual.eulerAngles.y };
        }
        public Transform HeadAnchorTransform
        {
            get
            {
                var humanoid=GetComponent<HumanoidCastAnimator>();
                if(humanoid&&humanoid.Head)return humanoid.Head;
                var adapter=GetComponent<CastActionAdapter>();
                return adapter&&adapter.Head?adapter.Head:null;
            }
        }
        public Vector3 HeadAnchor=>HeadAnchorTransform?HeadAnchorTransform.position:transform.position+Vector3.up*(CastModel.HeightOf(ActorId)-(GetComponent<SceneOneSeatedPose>()?.IsSeated==true?.4f:0));
        public Transform HandAnchor=>GetComponent<HumanoidCastAnimator>()?.RightHand?GetComponent<HumanoidCastAnimator>().RightHand:GetComponent<CastActionAdapter>()?.RightHand;
        public Transform LeftHandAnchor=>GetComponent<HumanoidCastAnimator>()?.LeftHand;
        public void Tint(Color tint)
        {
            foreach (var renderer in GetComponentsInChildren<Renderer>())
                if (renderer.gameObject.name.Contains("Jacket") || renderer.gameObject.name.Contains("Sleeve"))
                    renderer.material.color = tint;
        }
    }
}
