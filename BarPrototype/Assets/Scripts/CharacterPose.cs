using UnityEngine;

namespace BarPrototype
{
    public sealed class CharacterPose : MonoBehaviour
    {
        public PlayerMotor motor;
        public Transform torso, leftArm, rightArm, leftLeg, rightLeg;
        private Vector3 torsoRest;
        private float phase;
        private float blend;
        private void Start() { if (torso) torsoRest = torso.localPosition; }
        private void LateUpdate()
        {
            if (!motor || Time.timeScale == 0) return;
            var amount = Mathf.Clamp01(motor.MotionSpeed / motor.WalkSpeed);
            blend = Mathf.MoveTowards(blend, amount, Time.deltaTime * 9f);
            phase += Time.deltaTime * Mathf.Lerp(3, 12, amount) * Mathf.Max(1, motor.MotionSpeed / motor.WalkSpeed);
            var swing = Mathf.Sin(phase) * 28 * blend;
            if (leftArm) leftArm.localRotation = Quaternion.Euler(-swing, 0, -6);
            if (rightArm) rightArm.localRotation = Quaternion.Euler(swing, 0, 6);
            if (leftLeg) leftLeg.localRotation = Quaternion.Euler(swing, 0, 0);
            if (rightLeg) rightLeg.localRotation = Quaternion.Euler(-swing, 0, 0);
            if (torso) torso.localPosition = torsoRest + Vector3.up *
                (Mathf.Abs(Mathf.Sin(phase)) * .027f * blend + Mathf.Sin(Time.time * 2) * .006f * (1 - blend));
        }
    }
}
