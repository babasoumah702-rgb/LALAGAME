using System.Collections.Generic;
using System.IO;
using System.Linq;
using BarPrototype;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.InputSystem;
using UnityEngine.Rendering;

namespace LastCall
{
    [DefaultExecutionOrder(-50),RequireComponent(typeof(LocalServiceClient))]
    public sealed class LastCallGame : MonoBehaviour
    {
        public GameObject characterPrefab;
        public LastCallArtCatalog artCatalog;
        public SceneZeroController Intro { get; private set; }
        public LocalServiceClient Client { get; private set; }
        public LastCallInterface Interface { get; private set; }
        public readonly Dictionary<string, ActorAvatar> Avatars = new Dictionary<string, ActorAvatar>();
        private float nextReport;
        private string session;
        private void Awake()
        {
            Client = GetComponent<LocalServiceClient>();
            LastCallArtCatalog.Current=artCatalog;
            Intro=FindObjectOfType<SceneZeroController>();
            if(Intro)Intro.Bind(this);
            Interface = gameObject.AddComponent<LastCallInterface>();
            Interface.Game = this;
            Client.Changed += ApplyState;
            OtomeStage.Open();
            foreach(var text in FindObjectsOfType<TextMesh>())WorldTextDepth.Apply(text);
            gameObject.AddComponent<DirectorLens>().Game = this;
            gameObject.AddComponent<DialogueBubbles>().Game = this;
            gameObject.AddComponent<SceneOnePresentation>().Game = this;
            gameObject.AddComponent<SceneTwoPresentation>().Game = this;
            gameObject.AddComponent<NightStage>().Game=this;gameObject.AddComponent<NightPresentation>().Game=this;
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
                    OtomeCast.Dress(instance, data.id);
                    if(state.story!=null)instance.AddComponent<CastActionAdapter>();
                    // Server routes avoid live character footprints. Let two routed actors clear a
                    // narrow doorway without CharacterController deadlock; room and prop collision
                    // remains enabled, so nobody can walk through walls or furniture.
                    var body=instance.GetComponent<CharacterController>();
                    foreach(var other in Avatars.Values)
                    {
                        var otherBody=other.GetComponent<CharacterController>();
                        if(body&&otherBody)Physics.IgnoreCollision(body,otherBody,true);
                    }
                    Avatars.Add(data.id, avatar);
                    MakeName(avatar, data.name, data.id);
                }
                avatar.State = data;
                var nameLabel=avatar.transform.Find("Name")?.GetComponent<TextMesh>();
                if(nameLabel){nameLabel.text=data.name;nameLabel.gameObject.SetActive(state.intro==null||state.intro.version==0);}
                avatar.gameObject.SetActive(true);
                var actual = new Vector2(avatar.transform.position.x, avatar.transform.position.z);
                if (Vector2.Distance(actual, new Vector2(data.x, data.z)) > .65f) avatar.Place(data);
            }
            foreach (var pair in Avatars)
                if (!state.characters.Any(a => a.id == pair.Key)) pair.Value.gameObject.SetActive(false);
        }
        private void MakeName(ActorAvatar actor, string text, string actorId)
        {
            var label = new GameObject("Name", typeof(TextMesh));
            label.transform.SetParent(actor.transform, false);
            label.transform.localPosition = new Vector3(0, OtomeCast.NameY(actorId), 0);
            label.transform.rotation = Camera.main.transform.rotation;
            var mesh = label.GetComponent<TextMesh>();
            mesh.text = text;
            mesh.characterSize = .035f;
            mesh.fontSize = 64;
            mesh.anchor = TextAnchor.MiddleCenter;
            mesh.color = new Color(.93f, .93f, .95f);
            WorldTextDepth.Apply(mesh);
        }
        private void Update()
        {
            var state = Client.State;
            if (state == null) return;
            bool blocked = state.paused || state.status != "playing" || (Interface.Blocking&&!FullNightVerification.Running) || NightPresentation.CinematicActive || (Intro&&Intro.Active);
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
                    var item=hit.collider.GetComponent<SceneOneObject>();
                    if(item&&state.scene1!=null)Client.Send(new CommandDto{type="observe_object",objectTarget=item.objectId});
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
            if (!focus && !FullNightVerification.Running && Client?.State?.status == "playing") Interface.Pause(true);
        }
        private void OnDestroy() { if (Client) Client.Changed -= ApplyState; }
    }

    [DefaultExecutionOrder(100)]
    public sealed class DirectorLens : MonoBehaviour
    {
        public LastCallGame Game;
        private Camera cam;
        private FixedRoomCamera room;
        private float lookYaw = 205, lookPitch = 8;
        private bool armed, walking;
        private bool userLook;
        public void TakePose(float yaw,float pitch){lookYaw=yaw;lookPitch=Mathf.DeltaAngle(0,pitch);armed=true;walking=false;}
        private void Awake()
        {
            cam = Camera.main;
            if (!cam) return;
            room = cam.GetComponent<FixedRoomCamera>();
            if (room) room.enabled = false;
            cam.orthographic = false;
            cam.fieldOfView = 70;
            cam.nearClipPlane = .08f;
            cam.farClipPlane = 80;
            cam.transform.position = new Vector3(.1f, 1.58f, -2.35f);
            cam.transform.rotation = Quaternion.Euler(lookPitch, lookYaw, 0);
        }
        private void OnDestroy()
        {
            if (room) room.enabled = true;
            Cursor.lockState = CursorLockMode.None;
            Cursor.visible = true;
        }
        private void LateUpdate()
        {
            if(NightPresentation.CinematicActive)return;
            if(Game&&Game.Intro&&Game.Intro.Active)return;
            if (!cam) cam = Camera.main;
            if (!cam) return;
            cam.orthographic = false;
            Cursor.lockState = CursorLockMode.None;
            Cursor.visible = true;
            bool playing = Game && Game.Client?.State != null && Game.Client.State.status == "playing";
            bool blocked = !playing || (Game.Interface && Game.Interface.Blocking);
            ActorAvatar player = null;
            if (Game) Game.Avatars.TryGetValue("USER", out player);
            ActorAvatar them = null;
            var focusId = Game && Game.Interface ? Game.Interface.FocusId : "";
            if (Game && !string.IsNullOrEmpty(focusId)) Game.Avatars.TryGetValue(focusId, out them);
            bool conversation=player&&player.State!=null&&!string.IsNullOrEmpty(player.State.conversationTarget);
            if(conversation&&Game.Avatars.TryGetValue(player.State.conversationTarget,out var partner))them=partner;
            bool approaching = player && player.State != null && player.State.route != null && player.State.route.Length > 0 && them;
            if (!approaching && playing && !blocked && Keyboard.current != null)
            {
                if(conversation&&!userLook&&(Keyboard.current.qKey.isPressed||Keyboard.current.cKey.isPressed||Keyboard.current.rKey.isPressed||Keyboard.current.fKey.isPressed)){Game.Client.Send(new CommandDto{type="release_facing"});userLook=true;}
                float turn = 110f * Time.unscaledDeltaTime;
                if (Keyboard.current.qKey.isPressed) lookYaw -= turn;
                if (Keyboard.current.cKey.isPressed) lookYaw += turn;
                if (Keyboard.current.rKey.isPressed) lookPitch = Mathf.Clamp(lookPitch - turn, -72f, 78f);
                if (Keyboard.current.fKey.isPressed) lookPitch = Mathf.Clamp(lookPitch + turn, -72f, 78f);
            }
            if (!approaching && playing && !blocked && Mouse.current != null && Mouse.current.rightButton.isPressed)
            {
                if(conversation&&!userLook){Game.Client.Send(new CommandDto{type="release_facing"});userLook=true;}
                var delta = Mouse.current.delta.ReadValue();
                lookYaw += delta.x * .12f;
                lookPitch = Mathf.Clamp(lookPitch - delta.y * .12f, -72f, 78f);
            }
            if (player && player.gameObject.activeSelf)
            {
                if (!armed)
                {
                    var visual = player.GetComponent<PlayerMotor>()?.VisualRoot;
                    if (visual) lookYaw = visual.eulerAngles.y;
                    armed = true;
                }
                HideSelf(player);
                cam.transform.position = player.transform.position + Vector3.up * (player.State?.posture=="lie"?.3f:player.State?.posture=="sit"||Game.Client?.State?.scene1?.seated==true?1.08f:1.55f);
                if(!conversation)userLook=false;
                if (approaching||conversation&&!userLook) FacePerson(them);
                else if (walking && them) FacePerson(them, true);
                cam.transform.rotation = Quaternion.Euler(lookPitch, lookYaw, 0);
                var visualRoot = player.GetComponent<PlayerMotor>()?.VisualRoot;
                if (visualRoot)
                    visualRoot.rotation = Quaternion.Euler(0, lookYaw, 0);
            }
            walking = approaching;
            FaceNames();
        }
        private void FacePerson(ActorAvatar them, bool snap = false)
        {
            if (!them || !cam) return;
            var dir = them.HeadAnchor-Vector3.up*.18f - cam.transform.position;
            dir.y = Mathf.Clamp(dir.y, -2f, 2f);
            if (dir.sqrMagnitude < .01f) return;
            var euler = Quaternion.LookRotation(dir).eulerAngles;
            float yaw = euler.y;
            float pitch = euler.x > 180 ? euler.x - 360 : euler.x;
            pitch = Mathf.Clamp(pitch, -72f, 78f);
            if (snap)
            {
                lookYaw = yaw;
                lookPitch = pitch;
                return;
            }
            lookYaw = Mathf.MoveTowardsAngle(lookYaw, yaw, 220f * Time.deltaTime);
            lookPitch = Mathf.MoveTowards(lookPitch, pitch, 110f * Time.deltaTime);
        }
        private static void HideSelf(ActorAvatar player)
        {
            foreach (var body in player.GetComponentsInChildren<Renderer>(true))
                if (body && !body.GetComponent<TextMesh>()) body.enabled = false;
            var label = player.transform.Find("Name");
            if (label) label.gameObject.SetActive(false);
        }
        private void FaceNames()
        {
            if (Game == null || !cam) return;
            var rotation = cam.transform.rotation;
            foreach (var avatar in Game.Avatars.Values)
            {
                if (avatar.IsPlayer) continue;
                var label = avatar.transform.Find("Name");
                if (label) label.rotation = rotation;
            }
        }
    }

    public static class OtomeArt
    {
        public const string BarPath = "Assets/LastCall/Stage/otome-bar.png";
        private static Texture2D bar;
        private static readonly Dictionary<string, Texture2D> sheets = new Dictionary<string, Texture2D>();

        public static Texture2D Bar()
        {
            if (bar) return bar;
            bar = Read("skin-bar", false) ?? Read("otome-bar", false);
            return bar;
        }

        public static Texture2D Tile(string id)
        {
            var key = "tile:" + id;
            if (sheets.TryGetValue(key, out var cached) && cached) return cached;
            var texture = Read(id, false);
            if (!texture) return null;
            texture.wrapMode = TextureWrapMode.Repeat;
            sheets[key] = texture;
            return texture;
        }

        public static Texture2D Picture(string id)
        {
            if (sheets.TryGetValue(id, out var cached) && cached) return cached;
            var texture = Read(id, true);
            if (texture) sheets[id] = texture;
            return texture;
        }

        public static Material Flat(Color color, Texture texture = null)
        {
            var shader = Shader.Find("Universal Render Pipeline/Unlit")
                ?? Shader.Find("Unlit/Color")
                ?? Shader.Find("Sprites/Default");
            var material = new Material(shader);
            if (material.HasProperty("_BaseMap") && texture) material.SetTexture("_BaseMap", texture);
            if (material.HasProperty("_BaseColor")) material.SetColor("_BaseColor", color);
            if (material.HasProperty("_MainTex") && texture) material.SetTexture("_MainTex", texture);
            if (material.HasProperty("_Color")) material.SetColor("_Color", color);
            if (material.HasProperty("_Cull")) material.SetFloat("_Cull", 0);
            return material;
        }

        public static Material Sheet(Texture texture)
        {
            var shader = Shader.Find("Universal Render Pipeline/Unlit")
                ?? Shader.Find("Sprites/Default")
                ?? Shader.Find("Unlit/Transparent");
            var material = new Material(shader);
            material.mainTexture = texture;
            if (material.HasProperty("_BaseMap")) material.SetTexture("_BaseMap", texture);
            if (material.HasProperty("_MainTex")) material.SetTexture("_MainTex", texture);
            if (material.HasProperty("_BaseColor")) material.SetColor("_BaseColor", Color.white);
            if (material.HasProperty("_Color")) material.SetColor("_Color", Color.white);
            if (material.HasProperty("_Cull")) material.SetFloat("_Cull", 0);
            if (material.HasProperty("_Surface"))
            {
                material.SetFloat("_Surface", 1);
                material.SetFloat("_Blend", 0);
                material.SetFloat("_AlphaClip", 1);
                if (material.HasProperty("_Cutoff")) material.SetFloat("_Cutoff", .12f);
                material.EnableKeyword("_SURFACE_TYPE_TRANSPARENT");
                material.EnableKeyword("_ALPHATEST_ON");
                material.SetInt("_SrcBlend", (int)BlendMode.SrcAlpha);
                material.SetInt("_DstBlend", (int)BlendMode.OneMinusSrcAlpha);
                material.SetInt("_ZWrite", 0);
                material.SetOverrideTag("RenderType", "Transparent");
                material.renderQueue = 3000;
            }
            return material;
        }

        public static Material Cloth(Color color, float smoothness = .18f, float metallic = 0f)
        {
            var shader = Shader.Find("Universal Render Pipeline/Lit");
            if (!shader) return Flat(color);
            var material = new Material(shader);
            if (material.HasProperty("_BaseColor")) material.SetColor("_BaseColor", color);
            if (material.HasProperty("_Color")) material.SetColor("_Color", color);
            if (material.HasProperty("_Smoothness")) material.SetFloat("_Smoothness", smoothness);
            if (material.HasProperty("_Metallic")) material.SetFloat("_Metallic", metallic);
            return material;
        }

        public static Material Skin(Color tint, Texture texture, float smoothness, float metallic = 0f, float tile = 4f)
        {
            var material = Cloth(tint, smoothness, metallic);
            if (texture)
            {
                material.mainTexture = texture;
                if (material.HasProperty("_BaseMap")) material.SetTexture("_BaseMap", texture);
                if (material.HasProperty("_MainTex")) material.SetTexture("_MainTex", texture);
                material.SetTextureScale("_BaseMap", new Vector2(tile, tile));
                material.SetTextureScale("_MainTex", new Vector2(tile, tile));
            }
            return material;
        }

        public static Color Hex(string hex)
        {
            ColorUtility.TryParseHtmlString("#" + hex, out var color);
            return color;
        }

        private static Texture2D Read(string id, bool punch)
        {
            if(LastCallArtCatalog.Current)
            {
                var asset=LastCallArtCatalog.Current.Texture(id);
                if(asset)return asset;
            }
            foreach (var ext in new[] { ".png", ".jpg", ".jpeg" })
            {
                var path = Path.Combine(Application.dataPath, "LastCall/Stage", id + ext);
                if (!File.Exists(path)) continue;
                var texture = new Texture2D(2, 2, TextureFormat.RGBA32, false);
                if (!texture.LoadImage(File.ReadAllBytes(path))) continue;
                texture.wrapMode = TextureWrapMode.Clamp;
                texture.filterMode = FilterMode.Bilinear;
                if (punch) PunchStudio(texture);
                return texture;
            }
            return null;
        }

        private static void PunchStudio(Texture2D texture)
        {
            var pixels = texture.GetPixels32();
            for (int i = 0; i < pixels.Length; i++)
            {
                var p = pixels[i];
                int max = Mathf.Max(p.r, Mathf.Max(p.g, p.b));
                int min = Mathf.Min(p.r, Mathf.Min(p.g, p.b));
                if (min > 168 && max - min < 22) p.a = 0;
                else if (min > 148 && max - min < 16) p.a = (byte)Mathf.Clamp((180 - min) * 8, 0, 255);
                pixels[i] = p;
            }
            texture.SetPixels32(pixels);
            texture.Apply(false, false);
        }
    }

    public static class OtomeStage
    {
        public static void Open()
        {
            if (GameObject.Find("Last Call | otome")) return;
            var root = new GameObject("Last Call | otome").transform;
            RecolorRoom();
            WarmLights();
            RenameSign();
            Mural(root);
            Photos(root);
            Motifs(root);
            RenderSettings.ambientSkyColor = OtomeArt.Hex("A8A0E8") * .92f;
            RenderSettings.ambientEquatorColor = OtomeArt.Hex("6A5FB8") * .85f;
            RenderSettings.ambientGroundColor = OtomeArt.Hex("221F3A") * .85f;
            RenderSettings.ambientIntensity = 1.6f;
            if (Camera.main) Camera.main.backgroundColor = OtomeArt.Hex("262040");
        }

        private static void RecolorRoom()
        {
            var plaster = OtomeArt.Tile("skin-plaster");
            var wood = OtomeArt.Tile("skin-wood");
            var leather = OtomeArt.Tile("skin-leather");
            var floor = OtomeArt.Tile("skin-floor");
            foreach (var mesh in Object.FindObjectsOfType<Renderer>())
            {
                if(mesh.GetComponentInParent<SceneZeroController>())continue;
                if (!mesh || mesh.GetComponentInParent<ActorAvatar>() || mesh.GetComponentInParent<Canvas>()) continue;
                if (mesh.name.StartsWith("Last Call |")) continue;
                var n = mesh.name.ToLowerInvariant();
                if (n.Contains("jacket")) continue;
                Material skin = null;
                if (n.Contains("plaster")) skin = OtomeArt.Skin(Color.white, plaster, .14f, 0, 4.5f);
                else if (n.Contains("oak") || n.Contains("plank") || n.Contains("rug")) skin = OtomeArt.Skin(Color.white, floor, .32f, 0, 5f);
                else if (n.Contains("walnut") || n.Contains("wainscot") || n.Contains("panel") || n.Contains("coffee")) skin = OtomeArt.Skin(Color.white, wood, .22f, 0, 3.2f);
                else if (n.Contains("leather")) skin = OtomeArt.Skin(Color.white, leather, .38f, 0, 2.4f);
                else if (n.Contains("stage")) skin = OtomeArt.Cloth(OtomeArt.Hex("141210"), .08f);
                else if (n.Contains("brass")) skin = OtomeArt.Cloth(OtomeArt.Hex("C4925A"), .55f, .65f);
                else if (n.Contains("iron")) skin = OtomeArt.Cloth(OtomeArt.Hex("2A2A30"), .28f, .45f);
                else if (n.Contains("cream")) skin = OtomeArt.Cloth(OtomeArt.Hex("E8D4B8"), .18f);
                if (skin == null) continue;
                mesh.sharedMaterial = skin;
                mesh.shadowCastingMode = ShadowCastingMode.On;
            }
        }

        private static void WarmLights()
        {
            foreach (var light in Object.FindObjectsOfType<Light>())
            {
                if(light.GetComponentInParent<SceneZeroController>())continue;
                if (light.type == LightType.Directional)
                {
                    light.color = OtomeArt.Hex("C8CCFF");
                    light.intensity = 1.65f;
                }
                else
                {
                    light.color = OtomeArt.Hex("B0A8F0");
                    light.intensity = Mathf.Max(1.5f, light.intensity * 1.4f);
                }
            }
        }

        private static void RenameSign()
        {
            foreach (var label in Object.FindObjectsOfType<TextMesh>())
                if (label.text == "AMBER ROOM")
                {
                    label.text = "LA LA LAND";
                    label.color = OtomeArt.Hex("F0C8A0");
                }
        }

        private static void Mural(Transform root)
        {
            var texture = OtomeArt.Bar();
            if (!texture) return;
            var plate = GameObject.CreatePrimitive(PrimitiveType.Quad);
            plate.name = "Last Call | otome mural";
            plate.transform.SetParent(root, false);
            plate.transform.position = new Vector3(-1.8f, 2.15f, 4.72f);
            plate.transform.rotation = Quaternion.Euler(0, 180, 0);
            var aspect = texture.width / (float)Mathf.Max(1, texture.height);
            plate.transform.localScale = new Vector3(6.4f, 6.4f / aspect, 1);
            Object.Destroy(plate.GetComponent<Collider>());
            var mesh = plate.GetComponent<Renderer>();
            mesh.sharedMaterial = OtomeArt.Cloth(Color.white, .12f);
            if (texture)
            {
                mesh.sharedMaterial.mainTexture = texture;
                if (mesh.sharedMaterial.HasProperty("_BaseMap")) mesh.sharedMaterial.SetTexture("_BaseMap", texture);
            }
            mesh.shadowCastingMode = ShadowCastingMode.On;
        }

        private static void Photos(Transform root)
        {
            var texture = OtomeArt.Tile("skin-photos");
            if (!texture) return;
            var plate = GameObject.CreatePrimitive(PrimitiveType.Quad);
            plate.name = "Last Call | photo wall";
            plate.transform.SetParent(root, false);
            plate.transform.position = new Vector3(5.88f, 1.72f, 2.15f);
            plate.transform.rotation = Quaternion.Euler(0, -90, 0);
            var aspect = texture.width / (float)Mathf.Max(1, texture.height);
            plate.transform.localScale = new Vector3(2.4f, 2.4f / aspect, 1);
            Object.Destroy(plate.GetComponent<Collider>());
            var mesh = plate.GetComponent<Renderer>();
            mesh.sharedMaterial = OtomeArt.Skin(Color.white, texture, .1f, 0, 1f);
            mesh.shadowCastingMode = ShadowCastingMode.Off;
        }

        private static void Motifs(Transform root)
        {
            var table = new Vector3(1.65f, 0, -1.8f);
            Cube("empty chair", table + new Vector3(-.7f, .42f, .1f), new Vector3(.38f, .08f, .38f), OtomeArt.Hex("5A1820"), root);
            Cube("empty chair back", table + new Vector3(-.7f, .72f, -.12f), new Vector3(.38f, .55f, .06f), OtomeArt.Hex("5A1820"), root);
            Cube("third drink", table + new Vector3(.08f, .8f, -.1f), new Vector3(.08f, .16f, .07f), OtomeArt.Hex("F0C8A0"), root);
            Cube("second glass", table + new Vector3(-.16f, .78f, .08f), new Vector3(.07f, .13f, .07f), OtomeArt.Hex("E8D4B8"), root);
        }

        private static void Cube(string name, Vector3 position, Vector3 scale, Color color, Transform parent)
        {
            var go = GameObject.CreatePrimitive(PrimitiveType.Cube);
            go.name = name;
            go.transform.SetParent(parent, false);
            go.transform.position = position;
            go.transform.localScale = scale;
            Object.Destroy(go.GetComponent<Collider>());
            var mesh = go.GetComponent<Renderer>();
            mesh.sharedMaterial = OtomeArt.Cloth(color, .28f);
            mesh.shadowCastingMode = ShadowCastingMode.Off;
        }
    }

    public static class CastModel
    {
        public static System.Func<string, GameObject> Loader;

        public static bool Wear(GameObject instance, string actorId)
        {
            var source = LastCallArtCatalog.Current?LastCallArtCatalog.Current.Model(actorId):null;
            if(!source&&Loader!=null)source=Loader(actorId);
            if (!source) return false;
            var visual = instance.GetComponent<PlayerMotor>()?.VisualRoot;
            if (!visual) return false;
            visual.localScale = Vector3.one;
            foreach (var body in visual.GetComponentsInChildren<Renderer>(true))
                body.enabled = false;
            var model = UnityEngine.Object.Instantiate(source, visual, false);
            model.name = "Cast mesh";
            foreach (var col in model.GetComponentsInChildren<Collider>(true))
                UnityEngine.Object.Destroy(col);
            var pose = instance.GetComponent<CharacterPose>();
            if (pose) pose.enabled = false;
            Fit(model, instance.transform, HeightOf(actorId));
            if(HumanoidCastAnimator.Supports(actorId)&&model.GetComponentInChildren<SkinnedMeshRenderer>())
            {
                var motion=instance.GetComponent<HumanoidCastAnimator>();
                if(!motion)motion=instance.AddComponent<HumanoidCastAnimator>();
                motion.Configure(model);
            }
            return true;
        }

        public static float HeightOf(string actorId) => actorId == "C" ? 1.82f : actorId == "D" ? 1.58f : 1.68f;

        private static void Fit(GameObject model, Transform root, float height)
        {
            var skins = model.GetComponentsInChildren<Renderer>();
            if (skins.Length == 0) return;
            var box = skins[0].bounds;
            for (int i = 1; i < skins.Length; i++) box.Encapsulate(skins[i].bounds);
            float size = Mathf.Max(box.size.y, .01f);
            model.transform.localScale *= height / size;
            box = skins[0].bounds;
            for (int i = 1; i < skins.Length; i++) box.Encapsulate(skins[i].bounds);
            model.transform.position += new Vector3(root.position.x - box.center.x, root.position.y - box.min.y, root.position.z - box.center.z);
        }
    }

    public static class OtomeCast
    {
        public static void Dress(GameObject instance, string actorId)
        {
            Hide(instance, "Ground marker");
            if (CastModel.Wear(instance, actorId)) return;
            var visual = instance.GetComponent<PlayerMotor>()?.VisualRoot;
            if (!visual) return;
            float tall = actorId == "C" ? 1.08f : actorId == "D" ? .94f : 1f;
            visual.localScale = new Vector3((actorId == "C" ? .9f : .8f) * tall, tall, .86f);
            var skin = OtomeArt.Hex("E8C8B4");
            var silver = OtomeArt.Hex("C8D0D8");
            Color coat, shirt, hair, legs, boots;
            Outfit(actorId, out coat, out shirt, out hair, out legs, out boots);
            Paint(instance, "Jacket body", coat, .2f);
            Paint(instance, "Sleeve", actorId == "USER" ? shirt : coat, .2f);
            Paint(instance, "Jacket seam", silver, .7f, .8f);
            Paint(instance, "Shirt front", shirt, .16f);
            Paint(instance, "Hair silhouette", hair, .22f);
            Paint(instance, "Swept fringe", hair, .22f);
            Paint(instance, "Trouser leg", legs, .18f);
            Paint(instance, "Boot", boots, .55f, .12f);
            Paint(instance, "Boot sole", OtomeArt.Hex("141210"), .2f);
            Paint(instance, "Faceted head", skin, .32f);
            Paint(instance, "Neck", skin, .32f);
            Paint(instance, "Hand", skin, .32f);
            Paint(instance, "Nose", skin, .32f);
            Paint(instance, "Eye", OtomeArt.Hex("2A1818"), .15f);
            Hide(instance, "Scarf");
            Hide(instance, "Scarf tail");
            Hide(instance, "Ground marker");
            var torso = Child(instance, "Breathing body");
            var head = Child(instance, "Faceted head");
            Each(instance, "Trouser leg", leg =>
                Add("crease", new Vector3(0, 0, .11f), new Vector3(.02f, .92f, .02f), Color.Lerp(legs, Color.white, .12f), .25f, 0, leg));
            if (actorId == "A" || actorId == "USER") Glasses(head, silver);
            if (actorId == "A")
            {
                Add("button", new Vector3(0, .04f, .175f), new Vector3(.03f, .03f, .02f), silver, .7f, .8f, torso);
                Add("ring L", new Vector3(-.02f, 0, .04f), new Vector3(.04f, .02f, .04f), silver, .7f, .85f, Child(instance, "Hand"));
            }
            if (actorId == "USER")
            {
                Hide(instance, "Jacket seam");
                Each(instance, "Jacket body", body => body.localScale = new Vector3(.48f, .42f, .3f));
                Each(instance, "Sleeve", arm =>
                {
                    arm.localScale = new Vector3(.16f, .22f, .18f);
                    arm.localPosition = new Vector3(arm.localPosition.x, -.22f, 0);
                });
                Add("belt", new Vector3(0, -.24f, .02f), new Vector3(.42f, .045f, .3f), OtomeArt.Hex("3A241C"), .4f, .1f, torso);
                Add("buckle", new Vector3(0, -.24f, .16f), new Vector3(.05f, .04f, .02f), silver, .7f, .8f, torso);
            }
            if (actorId == "B")
                Add("sport watch", new Vector3(0, -.42f, .08f), new Vector3(.12f, .045f, .13f), silver, .65f, .7f, Child(instance, "Left arm pivot"));
            if (actorId == "C")
                Add("carabiner", new Vector3(-.22f, -.18f, .16f), new Vector3(.05f, .12f, .03f), silver, .7f, .8f, torso);
            if (actorId == "D")
                Add("violet streak", new Vector3(.08f, .22f, .02f), new Vector3(.07f, .2f, .1f), OtomeArt.Hex("6A5A98"), .2f, 0, head);
            if (actorId == "BARTENDER")
            {
                Hide(instance, "Jacket seam");
                Hide(instance, "Swept fringe");
                Each(instance, "Sleeve", arm =>
                {
                    arm.localScale = new Vector3(.17f, .26f, .18f);
                    arm.localPosition = new Vector3(0, -.24f, 0);
                });
                Add("apron", new Vector3(0, -.18f, .17f), new Vector3(.46f, .36f, .05f), OtomeArt.Hex("3A2418"), .35f, .08f, torso);
                Add("apron bow", new Vector3(0, -.02f, .2f), new Vector3(.12f, .05f, .04f), OtomeArt.Hex("3A2418"), .35f, .08f, torso);
                Add("towel", new Vector3(.2f, -.16f, .2f), new Vector3(.08f, .18f, .03f), OtomeArt.Hex("E8E4DC"), .12f, 0, torso);
                Add("bun", new Vector3(0, .18f, -.14f), new Vector3(.16f, .13f, .14f), hair, .22f, 0, head);
                Add("watch", new Vector3(0, -.28f, .08f), new Vector3(.1f, .04f, .12f), OtomeArt.Hex("1A1614"), .4f, .2f, Child(instance, "Left arm pivot"));
                Add("pendant", new Vector3(0, .38f, .18f), new Vector3(.04f, .04f, .02f), silver, .7f, .8f, torso);
            }
            if (actorId == "OWNER")
            {
                Hide(instance, "Jacket seam");
                Each(instance, "Jacket body", body => body.localScale = new Vector3(.5f, .7f, .32f));
                Paint(instance, "Trouser leg", coat, .22f);
                Add("necklace", new Vector3(0, .4f, .16f), new Vector3(.12f, .02f, .12f), silver, .7f, .8f, torso);
            }
        }

        public static float NameY(string actorId) => CastModel.HeightOf(actorId) + .12f;

        private static void Outfit(string actorId, out Color coat, out Color shirt, out Color hair, out Color legs, out Color boots)
        {
            coat = OtomeArt.Hex("1A1618");
            shirt = OtomeArt.Hex("2A1C1C");
            hair = OtomeArt.Hex("1A1616");
            legs = OtomeArt.Hex("3A3A40");
            boots = OtomeArt.Hex("141214");
            if (actorId == "A") { shirt = OtomeArt.Hex("1A1414"); legs = OtomeArt.Hex("3A3A42"); }
            if (actorId == "B") { coat = OtomeArt.Hex("2A2A30"); shirt = OtomeArt.Hex("D8D4D0"); legs = OtomeArt.Hex("2A2A32"); }
            if (actorId == "C") { coat = OtomeArt.Hex("242428"); shirt = OtomeArt.Hex("1A181C"); legs = OtomeArt.Hex("2A2A2E"); }
            if (actorId == "D") { coat = OtomeArt.Hex("2A2832"); shirt = OtomeArt.Hex("1A1820"); hair = OtomeArt.Hex("2A2438"); }
            if (actorId == "BARTENDER") { coat = OtomeArt.Hex("161414"); shirt = OtomeArt.Hex("141212"); legs = OtomeArt.Hex("1A1616"); boots = OtomeArt.Hex("121010"); }
            if (actorId == "OWNER") { coat = OtomeArt.Hex("161416"); shirt = OtomeArt.Hex("1A1416"); legs = OtomeArt.Hex("161416"); }
            if (actorId == "USER") { coat = OtomeArt.Hex("4E1824"); shirt = OtomeArt.Hex("4E1824"); legs = OtomeArt.Hex("3A3A42"); boots = OtomeArt.Hex("3A2418"); }
        }

        private static void Glasses(Transform head, Color silver)
        {
            if (!head) return;
            Rim("lens L", new Vector3(-.078f, .2f, .178f), head, silver);
            Rim("lens R", new Vector3(.078f, .2f, .178f), head, silver);
            Add("bridge", new Vector3(0, .2f, .182f), new Vector3(.04f, .012f, .012f), silver, .75f, .85f, head);
            Add("arm L", new Vector3(-.12f, .198f, .08f), new Vector3(.01f, .01f, .16f), silver, .75f, .85f, head);
            Add("arm R", new Vector3(.12f, .198f, .08f), new Vector3(.01f, .01f, .16f), silver, .75f, .85f, head);
        }

        private static void Rim(string name, Vector3 local, Transform parent, Color silver)
        {
            var ring = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
            ring.name = name;
            ring.transform.SetParent(parent, false);
            ring.transform.localPosition = local;
            ring.transform.localRotation = Quaternion.Euler(90, 0, 0);
            ring.transform.localScale = new Vector3(.07f, .005f, .07f);
            Object.Destroy(ring.GetComponent<Collider>());
            Finish(ring.GetComponent<Renderer>(), silver, .75f, .85f);
            Add(name + " glass", local + new Vector3(0, 0, .004f), new Vector3(.058f, .004f, .058f), OtomeArt.Hex("2A3038") * new Color(1, 1, 1, .55f), .8f, .1f, parent);
        }

        private static void Paint(GameObject root, string name, Color color, float smoothness, float metallic = 0)
        {
            Each(root, name, item =>
            {
                var body = item.GetComponent<Renderer>();
                if (!body) return;
                body.enabled = true;
                Finish(body, color, smoothness, metallic);
            });
        }

        private static void Hide(GameObject root, string name)
        {
            Each(root, name, item =>
            {
                var body = item.GetComponent<Renderer>();
                if (body) body.enabled = false;
            });
        }

        private static void Each(GameObject root, string name, System.Action<Transform> use)
        {
            foreach (var item in root.GetComponentsInChildren<Transform>(true))
                if (item.name == name) use(item);
        }

        private static Transform Child(GameObject root, string name)
        {
            foreach (var item in root.GetComponentsInChildren<Transform>(true))
                if (item.name == name) return item;
            return null;
        }

        private static void Add(string name, Vector3 local, Vector3 scale, Color color, float smoothness, float metallic, Transform parent)
        {
            if (!parent) return;
            var go = GameObject.CreatePrimitive(PrimitiveType.Cube);
            go.name = name;
            go.transform.SetParent(parent, false);
            go.transform.localPosition = local;
            go.transform.localScale = scale;
            Object.Destroy(go.GetComponent<Collider>());
            Finish(go.GetComponent<Renderer>(), color, smoothness, metallic);
        }

        private static void Finish(Renderer body, Color color, float smoothness, float metallic)
        {
            if (!body) return;
            body.sharedMaterial = OtomeArt.Cloth(color, smoothness, metallic);
            body.shadowCastingMode = ShadowCastingMode.On;
        }
    }
}
