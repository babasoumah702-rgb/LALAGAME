using System.Collections.Generic;
using System.Linq;
using BarPrototype;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.InputSystem;

namespace LastCall
{
    [DefaultExecutionOrder(-50),RequireComponent(typeof(LocalServiceClient))]
    public sealed class LastCallGame : MonoBehaviour
    {
        public GameObject characterPrefab;
        public LocalServiceClient Client { get; private set; }
        public LastCallInterface Interface { get; private set; }
        public readonly Dictionary<string, ActorAvatar> Avatars = new Dictionary<string, ActorAvatar>();
        private float nextReport;
        private string session;
        private void Awake()
        {
            Client = GetComponent<LocalServiceClient>();
            Interface = gameObject.AddComponent<LastCallInterface>();
            Interface.Game = this;
            Client.Changed += ApplyState;
            gameObject.AddComponent<DirectorLens>().Game = this;
        }
        private void ApplyState()
        {
            var state = Client.State;
            if (state == null) return;
            bool newSession = session != state.sessionId;
            if (newSession)
            {
                foreach (var avatar in Avatars.Values) Destroy(avatar.gameObject);
                Avatars.Clear();
                session = state.sessionId;
            }
            foreach (var data in state.characters)
            {
                if (!Avatars.TryGetValue(data.id, out var avatar))
                {
                    var instance = Instantiate(characterPrefab);
                    instance.name = "Last Call | " + data.name;
                    avatar = instance.AddComponent<ActorAvatar>();
                    avatar.Setup(data.id, data.id == "USER");
                    avatar.Place(data);
                    if (ColorUtility.TryParseHtmlString("#" + data.color, out var tint)) avatar.Tint(tint);
                    Avatars.Add(data.id, avatar);
                    MakeName(avatar, data.name);
                }
                avatar.State = data;
                avatar.gameObject.SetActive(true);
                var actual = new Vector2(avatar.transform.position.x, avatar.transform.position.z);
                if (Vector2.Distance(actual, new Vector2(data.x, data.z)) > .65f) avatar.Place(data);
            }
            foreach (var pair in Avatars)
                if (!state.characters.Any(a => a.id == pair.Key)) pair.Value.gameObject.SetActive(false);
        }
        private void MakeName(ActorAvatar actor, string text)
        {
            var label = new GameObject("Name", typeof(TextMesh));
            label.transform.SetParent(actor.transform, false);
            label.transform.localPosition = new Vector3(0, 2.12f, 0);
            label.transform.rotation = Camera.main.transform.rotation;
            var mesh = label.GetComponent<TextMesh>();
            mesh.text = text;
            mesh.characterSize = .035f;
            mesh.fontSize = 64;
            mesh.anchor = TextAnchor.MiddleCenter;
            mesh.color = new Color(1, .94f, .78f);
        }
        private void Update()
        {
            var state = Client.State;
            if (state == null) return;
            bool blocked = state.paused || state.busy || state.status != "playing" || Interface.Blocking;
            foreach (var avatar in Avatars.Values)
                if (avatar.gameObject.activeSelf) avatar.Tick(blocked);
            if (Time.unscaledTime >= nextReport && state.status == "playing" && !blocked)
            {
                nextReport = Time.unscaledTime + .1f;
                foreach (var avatar in Avatars.Values)
                    if (avatar.gameObject.activeSelf) Client.Send(avatar.Report());
            }
            if (!blocked && Keyboard.current != null)
            {
                if (Keyboard.current.eKey.wasPressedThisFrame) SelectNearest();
                if (Keyboard.current.wKey.wasPressedThisFrame || Keyboard.current.aKey.wasPressedThisFrame ||
                    Keyboard.current.sKey.wasPressedThisFrame || Keyboard.current.dKey.wasPressedThisFrame ||
                    Keyboard.current.upArrowKey.wasPressedThisFrame || Keyboard.current.downArrowKey.wasPressedThisFrame ||
                    Keyboard.current.leftArrowKey.wasPressedThisFrame || Keyboard.current.rightArrowKey.wasPressedThisFrame)
                    Client.Send(new CommandDto { type = "cancel_move" });
            }
            if (!blocked && Mouse.current != null && Mouse.current.leftButton.wasPressedThisFrame &&
                !EventSystem.current.IsPointerOverGameObject())
            {
                var ray = Camera.main.ScreenPointToRay(Mouse.current.position.ReadValue());
                if (Physics.Raycast(ray, out var hit, 100))
                {
                    var avatar = hit.collider.GetComponentInParent<ActorAvatar>();
                    if (avatar && !avatar.IsPlayer) Interface.Select(avatar.ActorId);
                }
            }
        }
        private void SelectNearest()
        {
            if (!Avatars.TryGetValue("USER", out var player)) return;
            var nearest = Avatars.Values.Where(a => !a.IsPlayer && a.gameObject.activeSelf)
                .OrderBy(a => Vector3.Distance(a.transform.position, player.transform.position)).FirstOrDefault();
            if (nearest) Interface.Select(nearest.ActorId);
        }
        private void OnApplicationFocus(bool focus)
        {
            if (!focus && Client?.State?.status == "playing") Interface.Pause(true);
        }
        private void OnDestroy() { if (Client) Client.Changed -= ApplyState; }
    }

    [DefaultExecutionOrder(100)]
    public sealed class DirectorLens : MonoBehaviour
    {
        public LastCallGame Game;
        private Camera cam;
        private FixedRoomCamera room;
        private Vector3 focus = new Vector3(-.65f, 1.2f, 0);
        private Vector3 focusSpeed;
        private float size = 6.2f, sizeSpeed;
        private float pitch = 35, pitchSpeed;
        private float yaw = 45, yawSpeed;
        private string lastFocus, lastPlace, lastBeat;
        private void Awake()
        {
            cam = Camera.main;
            if (!cam) return;
            room = cam.GetComponent<FixedRoomCamera>();
            if (room) room.enabled = false;
            Apply(true);
        }
        private void OnDestroy()
        {
            if (room) room.enabled = true;
        }
        private void LateUpdate()
        {
            if (!cam) cam = Camera.main;
            if (!cam) return;
            var shot = Compose();
            bool cut = ShouldCut(shot);
            if (cut)
            {
                focus = shot.focus;
                size = shot.size;
                pitch = shot.pitch;
                yaw = shot.yaw;
                focusSpeed = Vector3.zero;
                sizeSpeed = pitchSpeed = yawSpeed = 0;
            }
            else
            {
                float time = shot.kind == "wide" ? .85f : shot.kind == "close" ? .42f : .55f;
                focus = Vector3.SmoothDamp(focus, shot.focus, ref focusSpeed, time);
                size = Mathf.SmoothDamp(size, shot.size, ref sizeSpeed, time);
                pitch = Mathf.SmoothDamp(pitch, shot.pitch, ref pitchSpeed, time);
                yaw = Mathf.SmoothDamp(yaw, shot.yaw, ref yawSpeed, time);
            }
            Apply(false);
            FaceNames();
        }
        private Shot Compose()
        {
            var wide = new Shot { kind = "wide", focus = new Vector3(-.65f, 1.2f, 0), size = 6.2f, pitch = 35, yaw = 45 };
            if (!Game || Game.Client?.State == null) return wide;
            var state = Game.Client.State;
            Game.Avatars.TryGetValue("USER", out var player);
            ActorAvatar target = null;
            var focusId = Game.Interface ? Game.Interface.FocusId : "";
            if (!string.IsNullOrEmpty(focusId)) Game.Avatars.TryGetValue(focusId, out target);
            if (target && !target.gameObject.activeSelf) target = null;
            if (state.lastCall || state.status == "ended")
                return new Shot { kind = "wide", focus = player ? Mix(wide.focus, Lift(player), .28f) : wide.focus, size = 6.5f, pitch = 32, yaw = 45 };
            if (!player) return wide;
            var playerLift = Lift(player);
            if (!target)
                return new Shot { kind = "wide", focus = Mix(wide.focus, playerLift, .4f), size = 5.6f, pitch = 34, yaw = 45 };
            float gap = Vector3.Distance(player.transform.position, target.transform.position);
            var mid = Mix(playerLift, Lift(target), .55f);
            float turn = Mathf.Clamp(Mathf.DeltaAngle(45, YawToward(player, target)) * .18f, -10f, 10f);
            bool talk = state.busy || Game.Interface.Talking;
            if (talk)
                return new Shot { kind = "close", focus = Mix(Lift(target), playerLift, .22f), size = 1.72f, pitch = 16, yaw = 45 + turn };
            if (gap < 2.15f)
                return new Shot { kind = "close", focus = Mix(Lift(target), playerLift, .35f), size = 2.05f, pitch = 20, yaw = 45 + turn };
            if (gap < 4.2f)
                return new Shot { kind = "medium", focus = mid, size = 3.25f, pitch = 27, yaw = 45 + turn * .6f };
            if (gap < 6.4f)
                return new Shot { kind = "medium", focus = Lift(target), size = 3.55f, pitch = 26, yaw = 45 + turn };
            return new Shot { kind = "wide", focus = Mix(wide.focus, playerLift, .4f), size = 5.9f, pitch = 34, yaw = 45 };
        }
        private bool ShouldCut(Shot shot)
        {
            if (!Game || Game.Client?.State == null) return false;
            var state = Game.Client.State;
            var focusId = Game.Interface ? Game.Interface.FocusId : "";
            Game.Avatars.TryGetValue("USER", out var player);
            string place = player && player.State != null ? player.State.location : "";
            string beat = state.lastCall ? "last" : state.busy ? "busy" : shot.kind;
            bool cut = false;
            if (lastFocus != focusId && !string.IsNullOrEmpty(focusId))
            {
                Game.Avatars.TryGetValue(focusId, out var next);
                float gap = player && next ? Vector3.Distance(player.transform.position, next.transform.position) : 9;
                cut = gap > 3.4f;
            }
            if (lastPlace != place && !string.IsNullOrEmpty(place) && !string.IsNullOrEmpty(lastPlace)) cut = true;
            if (lastBeat != "last" && state.lastCall) cut = true;
            lastFocus = focusId;
            lastPlace = place;
            lastBeat = beat;
            return cut;
        }
        private void Apply(bool snap)
        {
            cam.orthographic = true;
            cam.orthographicSize = Mathf.Max(1.35f, size);
            cam.transform.rotation = Quaternion.Euler(pitch, yaw, 0);
            cam.transform.position = focus - cam.transform.forward * 24;
            if (snap)
            {
                focusSpeed = Vector3.zero;
                sizeSpeed = pitchSpeed = yawSpeed = 0;
            }
        }
        private void FaceNames()
        {
            if (Game == null) return;
            var rotation = cam.transform.rotation;
            foreach (var avatar in Game.Avatars.Values)
            {
                var label = avatar.transform.Find("Name");
                if (label) label.rotation = rotation;
            }
        }
        private static Vector3 Lift(ActorAvatar actor) => actor.transform.position + Vector3.up * 1.42f;
        private static Vector3 Mix(Vector3 a, Vector3 b, float t) => Vector3.Lerp(a, b, t);
        private static float YawToward(ActorAvatar from, ActorAvatar to)
        {
            var delta = to.transform.position - from.transform.position;
            delta.y = 0;
            if (delta.sqrMagnitude < .01f) return 45;
            return Quaternion.LookRotation(delta).eulerAngles.y;
        }
        private struct Shot
        {
            public string kind;
            public Vector3 focus;
            public float size, pitch, yaw;
        }
    }
}
