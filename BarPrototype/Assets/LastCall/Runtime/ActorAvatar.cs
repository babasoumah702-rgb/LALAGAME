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
            }
        }
        public void Place(ActorDto state)
        {
            State = state;
            controller.enabled = false;
            transform.position = new Vector3(state.x, .09f, state.z);
            controller.enabled = true;
        }
        public void Tick(bool blocked)
        {
            if (State == null) return;
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
            motor.InputBlocked = blocked || auto;
            if (blocked) { pose.externalSpeed = 0; return; }
            if (auto)
            {
                var target = new Vector3(route[index].x, transform.position.y, route[index].z);
                var direction = Vector3.ClampMagnitude((target - transform.position) / Mathf.Max(Time.deltaTime * 2.1f, .01f), 1);
                if (IsPlayer) motor.MoveWorld(direction, Time.deltaTime);
                else
                {
                    if (controller.isGrounded) vertical = -2;
                    vertical -= 20 * Time.deltaTime;
                    controller.Move((direction * 2.1f + Vector3.up * vertical) * Time.deltaTime);
                    if (direction.sqrMagnitude > .01f)
                        visual.rotation = Quaternion.RotateTowards(visual.rotation, Quaternion.LookRotation(direction), Time.deltaTime * 360);
                }
            }
            else if (!IsPlayer)
            {
                if (controller.isGrounded) vertical = -2;
                vertical -= 20 * Time.deltaTime;
                controller.Move(Vector3.up * vertical * Time.deltaTime);
                visual.rotation = Quaternion.RotateTowards(visual.rotation, Quaternion.Euler(0, State.yaw, 0), Time.deltaTime * 120);
            }
            pose.externalSpeed = Vector3.Distance(before, transform.position) / Mathf.Max(Time.deltaTime, .001f);
            pose.presentation = State.animation;
        }
        public CommandDto Report()
        {
            return new CommandDto { type = "position", actor = ActorId, x = transform.position.x, z = transform.position.z, yaw = visual.eulerAngles.y };
        }
        public void Tint(Color tint)
        {
            foreach (var renderer in GetComponentsInChildren<Renderer>())
                if (renderer.gameObject.name.Contains("Jacket") || renderer.gameObject.name.Contains("Sleeve"))
                    renderer.material.color = tint;
        }
    }
}
