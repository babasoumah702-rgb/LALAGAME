using UnityEngine;
using UnityEngine.InputSystem;

namespace BarPrototype
{
    [RequireComponent(typeof(CharacterController))]
    public sealed class PlayerMotor : MonoBehaviour
    {
        [SerializeField] private float walkSpeed = 2.8f;
        [SerializeField] private float runSpeed = 4.6f;
        [SerializeField] private float turnSpeed = 720f;
        [SerializeField] private float gravity = -22f;
        [SerializeField] private Transform visualRoot;
        private CharacterController controller;
        private InputAction moveAction;
        private InputAction runAction;
        private float verticalSpeed;
        private Camera viewCamera;
        public float MotionSpeed { get; private set; }
        public float WalkSpeed => walkSpeed;
        public float RunSpeed => runSpeed;
        public Transform VisualRoot { get => visualRoot; set => visualRoot = value; }

        private void Awake()
        {
            controller = GetComponent<CharacterController>();
            viewCamera = Camera.main;
            moveAction = new InputAction("Move", InputActionType.Value);
            moveAction.AddCompositeBinding("2DVector")
                .With("Up", "<Keyboard>/w").With("Down", "<Keyboard>/s")
                .With("Left", "<Keyboard>/a").With("Right", "<Keyboard>/d");
            moveAction.AddCompositeBinding("2DVector")
                .With("Up", "<Keyboard>/upArrow").With("Down", "<Keyboard>/downArrow")
                .With("Left", "<Keyboard>/leftArrow").With("Right", "<Keyboard>/rightArrow");
            runAction = new InputAction("Run", InputActionType.Button);
            runAction.AddBinding("<Keyboard>/leftShift");
            runAction.AddBinding("<Keyboard>/rightShift");
        }

        private void OnEnable() { moveAction?.Enable(); runAction?.Enable(); }
        private void OnDisable()
        {
            moveAction?.Disable(); runAction?.Disable(); MotionSpeed = 0;
        }
        private void OnDestroy() { moveAction?.Dispose(); runAction?.Dispose(); }
        private void Update()
        {
            if (Time.timeScale == 0 || !Application.isFocused) { MotionSpeed = 0; return; }
            Step(moveAction.ReadValue<Vector2>(), runAction.IsPressed(), Time.deltaTime);
        }

        public static Vector3 ScreenDirection(Vector2 input, Quaternion cameraRotation)
        {
            input = Vector2.ClampMagnitude(input, 1f);
            var right = cameraRotation * Vector3.right;
            var forward = cameraRotation * Vector3.forward;
            right.y = 0; forward.y = 0;
            return right.normalized * input.x + forward.normalized * input.y;
        }

        public void Step(Vector2 input, bool running, float dt)
        {
            if (dt <= 0 || Time.timeScale == 0) { MotionSpeed = 0; return; }
            if (!controller) controller = GetComponent<CharacterController>();
            if (!viewCamera) viewCamera = Camera.main;
            var rotation = viewCamera ? viewCamera.transform.rotation : Quaternion.Euler(35, 45, 0);
            var direction = ScreenDirection(input, rotation);
            var speed = running ? runSpeed : walkSpeed;
            if (controller.isGrounded && verticalSpeed < 0) verticalSpeed = -2f;
            verticalSpeed += gravity * dt;
            var previous = transform.position;
            controller.Move((direction * speed + Vector3.up * verticalSpeed) * dt);
            var displacement = transform.position - previous;
            displacement.y = 0;
            MotionSpeed = displacement.magnitude / dt;
            if (direction.sqrMagnitude > .001f && visualRoot)
                visualRoot.rotation = Quaternion.RotateTowards(visualRoot.rotation,
                    Quaternion.LookRotation(direction), turnSpeed * dt);
        }

        public void Teleport(Vector3 position)
        {
            if (!controller) controller = GetComponent<CharacterController>();
            controller.enabled = false;
            transform.position = position;
            controller.enabled = true;
            verticalSpeed = 0;
            MotionSpeed = 0;
            Physics.SyncTransforms();
        }
    }
}
