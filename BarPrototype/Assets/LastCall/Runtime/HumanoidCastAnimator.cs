using System;
using System.Collections.Generic;
using BarPrototype;
using UnityEngine;

namespace LastCall
{
    /// <summary>
    /// Shared A-D body-language layer. Imported clips provide the character-specific base motion;
    /// this component adds collision-authoritative locomotion, posture, gestures and bounded gaze.
    /// </summary>
    [DefaultExecutionOrder(105)]
    public sealed class HumanoidCastAnimator : MonoBehaviour
    {
        private sealed class Style
        {
            public float stride, gesture, gaze, response, breath, nativeSpeed;
        }

        private static readonly Dictionary<string, Style> Styles = new Dictionary<string, Style>
        {
            { "A", new Style { stride=.82f, gesture=.55f, gaze=.55f, response=.72f, breath=.72f, nativeSpeed=.38f } },
            { "B", new Style { stride=1.02f, gesture=1.08f, gaze=1.0f, response=1.18f, breath=1.0f, nativeSpeed=.55f } },
            { "C", new Style { stride=.88f, gesture=.62f, gaze=.66f, response=.62f, breath=.8f, nativeSpeed=.34f } },
            { "D", new Style { stride=1.08f, gesture=.9f, gaze=.82f, response=1.25f, breath=.92f, nativeSpeed=.62f } }
        };

        private ActorAvatar actor;
        private LastCallGame game;
        private Transform model, hips, spine, chest, neck, head;
        private Transform leftUpperArm, rightUpperArm, leftForearm, rightForearm;
        private Transform leftHand, rightHand, leftUpperLeg, rightUpperLeg, leftLowerLeg, rightLowerLeg,leftFoot,rightFoot;
        private Animator animator;
        private Style style;
        private float speed, turn, phase, sit, lie, lean, gesture, talk, gazeYaw, gazePitch,footClearance;
        private Vector3 previousForward, firstPosition;
        private bool configured;
        private string currentState;

        public Transform Head => head;
        public Transform RightHand => rightHand;
        public Transform LeftHand => leftHand;
        public bool IsSeated => sit > .55f;
        public bool IsHumanoid => animator && animator.avatar && animator.avatar.isValid && animator.avatar.isHuman;
        public string CurrentState => currentState;

        public static bool Supports(string actorId) => Styles.ContainsKey(actorId);

        public void Configure(GameObject importedModel)
        {
            actor = GetComponent<ActorAvatar>();
            if (!actor || !Supports(actor.ActorId) || !importedModel) return;
            model = importedModel.transform;
            style = Styles[actor.ActorId];
            animator = importedModel.GetComponent<Animator>() ?? importedModel.GetComponentInChildren<Animator>();
            if (animator)
            {
                animator.applyRootMotion = false;
                animator.cullingMode = AnimatorCullingMode.AlwaysAnimate;
                animator.speed = style.nativeSpeed;
            }
            BindBones();
            configured = hips && head && rightHand && leftHand;
            previousForward = model.forward;
            firstPosition = transform.position;
            if(leftFoot&&rightFoot&&TryGround(out var ground))footClearance=Mathf.Clamp(Mathf.Min(leftFoot.position.y,rightFoot.position.y)-ground,.015f,.18f);
        }

        private void Start()
        {
            actor = actor ? actor : GetComponent<ActorAvatar>();
            game = FindObjectOfType<LastCallGame>();
            if (!configured && actor && Supports(actor.ActorId))
            {
                var visual = GetComponent<PlayerMotor>()?.VisualRoot;
                var cast = visual ? visual.Find("Cast mesh") : null;
                if (cast) Configure(cast.gameObject);
            }
        }

        private void BindBones()
        {
            hips = Bone(HumanBodyBones.Hips, "Pelvis", "Hip");
            spine = Bone(HumanBodyBones.Spine, "Spine01", "Spine");
            chest = Bone(HumanBodyBones.Chest, "Spine02", "Chest");
            neck = Bone(HumanBodyBones.Neck, "Neck");
            head = Bone(HumanBodyBones.Head, "Head");
            leftUpperArm = Bone(HumanBodyBones.LeftUpperArm, "L_Upperarm");
            rightUpperArm = Bone(HumanBodyBones.RightUpperArm, "R_Upperarm");
            leftForearm = Bone(HumanBodyBones.LeftLowerArm, "L_Forearm");
            rightForearm = Bone(HumanBodyBones.RightLowerArm, "R_Forearm");
            leftHand = Bone(HumanBodyBones.LeftHand, "L_Hand");
            rightHand = Bone(HumanBodyBones.RightHand, "R_Hand");
            leftUpperLeg = Bone(HumanBodyBones.LeftUpperLeg, "L_Thigh");
            rightUpperLeg = Bone(HumanBodyBones.RightUpperLeg, "R_Thigh");
            leftLowerLeg = Bone(HumanBodyBones.LeftLowerLeg, "L_Calf");
            rightLowerLeg = Bone(HumanBodyBones.RightLowerLeg, "R_Calf");
            leftFoot = Bone(HumanBodyBones.LeftFoot, "L_Foot");
            rightFoot = Bone(HumanBodyBones.RightFoot, "R_Foot");
        }

        private Transform Bone(HumanBodyBones human, params string[] names)
        {
            if (animator && animator.avatar && animator.avatar.isValid)
            {
                var found = animator.GetBoneTransform(human);
                if (found) return found;
            }
            foreach (var t in model.GetComponentsInChildren<Transform>(true))
                foreach (var name in names)
                    if (string.Equals(t.name, name, StringComparison.OrdinalIgnoreCase)) return t;
            return null;
        }

        private void LateUpdate()
        {
            if (!configured || actor?.State == null || !model || Time.timeScale == 0) return;
            if (!game) game = FindObjectOfType<LastCallGame>();
            var dt = Mathf.Max(Time.deltaTime, .0001f);
            var state = actor.State;
            var targetSpeed = Mathf.Clamp01(actor.MotionSpeed / 2.1f);
            speed = Mathf.MoveTowards(speed, targetSpeed, dt * (targetSpeed > speed ? 3.8f : 5.5f) * style.response);
            var signedTurn = Vector3.SignedAngle(previousForward, model.forward, model.up) / dt;
            previousForward = model.forward;
            turn = Mathf.MoveTowards(turn, Mathf.Clamp(signedTurn / 180f, -1, 1), dt * 4);
            phase += dt * Mathf.Lerp(1.35f, 8.8f, speed) * style.stride;

            var sceneOneSeat=actor.ActorId=="A"&&game?.Client?.State?.story?.chapter==1&&
                (state.route==null||state.route.Length==0)&&Vector3.Distance(firstPosition,transform.position)<.4f;
            var wantsSit = sceneOneSeat || state.posture == "sit" || state.animation == "sit";
            var wantsLie = state.posture == "lie";
            var wantsLean = state.posture == "lean";
            sit = Mathf.MoveTowards(sit, wantsSit ? 1 : 0, dt * 1.65f);
            lie = Mathf.MoveTowards(lie, wantsLie ? 1 : 0, dt * 1.2f);
            lean = Mathf.MoveTowards(lean, wantsLean ? 1 : 0, dt * 1.8f);
            talk = Mathf.MoveTowards(talk, state.animation == "speak" ? 1 : 0, dt * 3.2f);

            var elapsed = game?.Client?.State?.elapsed ?? 0;
            var gestureAge = elapsed - state.gestureAt;
            var activeGesture = gestureAge >= 0 && gestureAge < (state.gesture == "dance" ? 6 : 3.2f);
            gesture = Mathf.MoveTowards(gesture, activeGesture ? 1 : 0, dt * (activeGesture ? 4 : 3));

            if (animator)
            {
                animator.SetFloat("MotionSpeed", speed);
                animator.SetFloat("Speaking", talk);
                UpdateNativeState(state, activeGesture);
            }

            ApplyIdleAndWalk();
            ApplyPosture();
            ApplyConversationGaze();
            ApplyGesture(state.gesture, state.animation, gestureAge);
            KeepFeetOnGround();
        }

        private void ApplyIdleAndWalk()
        {
            var stride = Mathf.Sin(phase) * 28 * speed * style.stride;
            var liftLeft = Mathf.Max(0, Mathf.Sin(phase)) * 24 * speed;
            var liftRight = Mathf.Max(0, -Mathf.Sin(phase)) * 24 * speed;
            if(!IsHumanoid)
            {
                Pitch(leftUpperLeg, -stride);
                Pitch(rightUpperLeg, stride);
                Pitch(leftLowerLeg, liftLeft);
                Pitch(rightLowerLeg, liftRight);
                if (talk < .25f && gesture < .2f)
                {
                    Pitch(leftUpperArm, stride * .72f);
                    Pitch(rightUpperArm, -stride * .72f);
                }
            }

            var breath = Mathf.Sin(Time.time * (1.35f + style.breath * .35f) + Seed()) * (1-speed) * style.breath;
            Pitch(chest ? chest : spine, breath * 1.4f - speed * 2.2f);
            Roll(chest ? chest : spine, turn * -4.5f);
            if (hips)
            {
                hips.position += model.up * (Mathf.Abs(Mathf.Sin(phase)) * .018f * speed + breath * .0025f);
                hips.position += model.right * (Mathf.Sin(Time.time * .42f + Seed()) * .009f * (1-speed));
            }
        }

        private void UpdateNativeState(ActorDto state,bool activeGesture)
        {
            var wanted="Idle"+actor.ActorId;
            if(lie>.15f)wanted="Idle"+actor.ActorId;
            else if(sit>.12f)wanted="Sit";
            else if(speed>.08f)wanted="Walk";
            else if(state.animation=="phone")wanted="Phone";
            else if(activeGesture&&state.gesture=="dance")wanted="Dance";
            else if(state.animation=="speak"||(activeGesture&&(state.gesture=="offer"||state.gesture=="flip"||state.gesture=="drink")))wanted="Talk"+actor.ActorId;
            if(currentState!=wanted)
            {
                currentState=wanted;
                animator.CrossFade(wanted,wanted=="Walk"?.16f:.25f,0,0);
            }
            var info=animator.GetCurrentAnimatorStateInfo(0);
            if(wanted=="Sit"&&info.IsName("Sit")&&info.normalizedTime>.82f)animator.speed=0;
            else if(wanted=="Walk")animator.speed=Mathf.Clamp(actor.MotionSpeed/1.25f,.65f,1.65f);
            else if(wanted.StartsWith("Idle",StringComparison.Ordinal))animator.speed=style.nativeSpeed;
            else animator.speed=.92f*style.response;
        }

        private void ApplyPosture()
        {
            if (sit > 0)
            {
                if (hips) hips.position += -model.up * (.37f * sit) + model.forward * (.08f * sit);
                Pitch(leftUpperLeg, -72 * sit); Pitch(rightUpperLeg, -72 * sit);
                Pitch(leftLowerLeg, 76 * sit); Pitch(rightLowerLeg, 76 * sit);
                Pitch(spine, -4 * sit);
            }
            if (lie > 0)
            {
                if (hips) hips.position += -model.up * (.48f * lie) - model.forward * (.22f * lie);
                Pitch(hips, -82 * lie);
                Pitch(leftUpperLeg, 12 * lie); Pitch(rightUpperLeg, 8 * lie);
            }
            if (lean > 0) Pitch(spine, -9 * lean);
        }

        private void ApplyConversationGaze()
        {
            Transform target = null;
            if (!string.IsNullOrEmpty(actor.State.conversationTarget) && game &&
                game.Avatars.TryGetValue(actor.State.conversationTarget, out var other) && other.gameObject.activeSelf)
                target = other.HeadAnchorTransform;

            float wantedYaw, wantedPitch;
            if (target && Vector3.Distance(head.position, target.position) < 8)
            {
                var local = model.InverseTransformDirection((target.position - head.position).normalized);
                wantedYaw = Mathf.Clamp(Mathf.Atan2(local.x, local.z) * Mathf.Rad2Deg, -58, 58);
                wantedPitch = Mathf.Clamp(-Mathf.Atan2(local.y, new Vector2(local.x, local.z).magnitude) * Mathf.Rad2Deg, -22, 20);
            }
            else
            {
                wantedYaw = Mathf.Sin(Time.time * (.23f + style.gaze * .06f) + Seed()) * 8 * style.gaze;
                wantedPitch = Mathf.Sin(Time.time * .31f + Seed() * 1.7f) * 2.5f;
            }
            gazeYaw = Mathf.Lerp(gazeYaw, wantedYaw, 1-Mathf.Exp(-Time.deltaTime * (2.1f + style.response)));
            gazePitch = Mathf.Lerp(gazePitch, wantedPitch, 1-Mathf.Exp(-Time.deltaTime * 2.6f));
            Yaw(chest, gazeYaw * .18f); Yaw(neck, gazeYaw * .28f); Yaw(head, gazeYaw * .54f);
            Pitch(neck, gazePitch * .28f); Pitch(head, gazePitch * .72f);
            if (talk > .05f) Pitch(head, Mathf.Sin(Time.time * 3.1f + Seed()) * 2.2f * talk);
            else if (target) Pitch(head, Mathf.Sin(Time.time * 1.1f + Seed()) * .9f * style.gaze);
        }

        private void ApplyGesture(string value, string animation, float age)
        {
            var amount = Mathf.Max(gesture, talk * .72f) * style.gesture;
            if (amount <= .001f && animation != "phone") return;
            var wave = Mathf.Sin(Time.time * (2.4f + style.response));
            if (talk > .05f)
            {
                Pitch(rightUpperArm, (-24 + wave * 8) * amount);
                Roll(rightUpperArm, -10 * amount);
                Pitch(rightForearm, (-38 + wave * 6) * amount);
                if (actor.ActorId == "B") Pitch(leftForearm, (-20-wave*5) * amount);
            }
            if (value == "offer" || value == "flip")
            {
                Pitch(rightUpperArm, -52 * gesture * style.gesture);
                Pitch(rightForearm, -58 * gesture);
                Roll(rightUpperArm, -12 * gesture);
            }
            else if (value == "drink")
            {
                Pitch(rightUpperArm, -78 * gesture);
                Pitch(rightForearm, -92 * gesture);
                Roll(rightUpperArm, -18 * gesture);
            }
            else if (value == "dance")
            {
                Roll(spine, Mathf.Sin(age * 2.1f) * 7 * gesture);
                Pitch(leftUpperArm, (-45 + Mathf.Sin(age*2.4f)*24) * gesture);
                Pitch(rightUpperArm, (-45 - Mathf.Sin(age*2.4f)*24) * gesture);
            }
            if (animation == "phone")
            {
                Pitch(rightUpperArm, -48);
                Pitch(rightForearm, -102);
                Roll(rightUpperArm, -9);
                Pitch(head, 12);
            }
        }

        private float Seed() => actor == null || string.IsNullOrEmpty(actor.ActorId) ? 0 : actor.ActorId[0] * .731f;
        private void KeepFeetOnGround()
        {
            if(!leftFoot||!rightFoot||!TryGround(out var ground))return;
            float current;
            if(lie>.25f)
            {
                var skins=model.GetComponentsInChildren<SkinnedMeshRenderer>();
                if(skins.Length==0)return;current=skins[0].bounds.min.y;
                for(var i=1;i<skins.Length;i++)current=Mathf.Min(current,skins[i].bounds.min.y);
            }
            else current=Mathf.Min(leftFoot.position.y,rightFoot.position.y)-footClearance;
            var correction=Mathf.Clamp(ground-current,-.28f,.28f);
            if(Mathf.Abs(correction)>.002f)model.position+=Vector3.up*correction;
        }
        private bool TryGround(out float height)
        {
            height=0;var found=false;
            var hits=Physics.RaycastAll(transform.position+Vector3.up*1.2f,Vector3.down,3.5f,~0,QueryTriggerInteraction.Ignore);
            foreach(var hit in hits)
            {
                if(hit.collider.transform==transform||hit.collider.transform.IsChildOf(transform)||hit.normal.y<.55f)continue;
                if(!found||hit.point.y>height){height=hit.point.y;found=true;}
            }
            return found;
        }
        private void Pitch(Transform bone, float degrees) => Rotate(bone, model.right, degrees);
        private void Yaw(Transform bone, float degrees) => Rotate(bone, model.up, degrees);
        private void Roll(Transform bone, float degrees) => Rotate(bone, model.forward, degrees);
        private static void Rotate(Transform bone, Vector3 axis, float degrees)
        {
            if (bone && Mathf.Abs(degrees) > .0001f) bone.rotation = Quaternion.AngleAxis(degrees, axis) * bone.rotation;
        }
    }
}
