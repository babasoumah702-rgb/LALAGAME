using UnityEngine;

namespace BarPrototype
{
    [ExecuteAlways, RequireComponent(typeof(Camera))]
    public sealed class FixedRoomCamera : MonoBehaviour
    {
        public Vector3 focus = new Vector3(0, 1.4f, 0);
        [Range(20, 65)] public float pitch = 35;
        public float yaw = 45;
        public float distance = 24;
        public float halfHeight = 6.7f;
        public float minimumHalfWidth = 9.2f;
        private void LateUpdate()
        {
            var cam = GetComponent<Camera>();
            cam.orthographic = true;
            cam.orthographicSize = Mathf.Max(halfHeight, minimumHalfWidth / Mathf.Max(.4f, cam.aspect));
            transform.rotation = Quaternion.Euler(pitch, yaw, 0);
            transform.position = focus - transform.forward * distance;
        }
    }
}
