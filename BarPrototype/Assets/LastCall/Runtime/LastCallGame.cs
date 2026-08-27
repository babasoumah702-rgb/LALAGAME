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
}
