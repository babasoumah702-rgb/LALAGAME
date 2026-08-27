using UnityEngine;

namespace BarPrototype
{
    public sealed class CharacterPose : MonoBehaviour
    {
        public PlayerMotor motor;
        public bool useExternalMotion;
        public float externalSpeed;
        public string presentation = "idle";
        public Transform torso, leftArm, rightArm, leftLeg, rightLeg;
        private Vector3 torsoRest;
        private float phase;
        private float blend;
        private void Start() { if (torso) torsoRest = torso.localPosition; }
        private void LateUpdate()
        {
            if ((!motor && !useExternalMotion) || Time.timeScale == 0) return;
            var speed = useExternalMotion ? externalSpeed : motor.MotionSpeed;
            var walkSpeed = motor ? motor.WalkSpeed : 2.2f;
            var amount = Mathf.Clamp01(speed / walkSpeed);
            blend = Mathf.MoveTowards(blend, amount, Time.deltaTime * 9f);
            phase += Time.deltaTime * Mathf.Lerp(3, 12, amount) * Mathf.Max(1, speed / walkSpeed);
            var swing = Mathf.Sin(phase) * 28 * blend;
            if (leftArm) leftArm.localRotation = Quaternion.Euler(-swing, 0, -6);
            if (rightArm) rightArm.localRotation = Quaternion.Euler(swing, 0, 6);
            if (leftLeg) leftLeg.localRotation = Quaternion.Euler(swing, 0, 0);
            if (rightLeg) rightLeg.localRotation = Quaternion.Euler(-swing, 0, 0);
            if (torso) torso.localPosition = torsoRest + Vector3.up *
                (Mathf.Abs(Mathf.Sin(phase)) * .027f * blend + Mathf.Sin(Time.time * 2) * .006f * (1 - blend));
            if (presentation == "speak" && rightArm) rightArm.localRotation = Quaternion.Euler(-30-Mathf.Sin(Time.time*3)*8,0,8);
            if (presentation == "drink" && rightArm) rightArm.localRotation = Quaternion.Euler(-110,0,0);
            if (presentation == "sit")
            {
                if (leftLeg) leftLeg.localRotation=Quaternion.Euler(-75,0,0);
                if (rightLeg) rightLeg.localRotation=Quaternion.Euler(-75,0,0);
                if (torso) torso.localPosition=torsoRest+Vector3.down*.18f;
            }
        }
    }
}
