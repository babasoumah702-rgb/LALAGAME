using System;
using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEditor.Build;
using UnityEditor.Build.Reporting;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;

namespace BarPrototype.Editor
{
    public static class BarSceneBuilder
    {
        private const string Root = "Assets/Generated";
        public const string ScenePath = "Assets/Scenes/AmberRoom.unity";
        private static readonly Dictionary<string, Material> Mats = new();
        private static readonly Dictionary<string, Mesh> Meshes = new();
        private static Transform room;
        private static System.Random random;

        [MenuItem("Amber Room/1 - Create or rebuild the prototype scene")]
        public static void CreateScene()
        {
            Directory.CreateDirectory(Root + "/Materials");
            Directory.CreateDirectory(Root + "/Meshes");
            Directory.CreateDirectory(Root + "/Rendering");
            Directory.CreateDirectory("Assets/Scenes");
            Directory.CreateDirectory("Assets/Prefabs");
            AssetDatabase.Refresh();
            Mats.Clear(); Meshes.Clear(); random = new System.Random(821);
            ConfigureProject(); ConfigurePipeline(); Palette();
            EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);
            room = new GameObject("THE AMBER ROOM | hand-built low-poly diorama").transform;
            Architecture(); Bar(); Seating(); Decorations(); Lighting(); MakePlayer();
            var cameraObject = new GameObject("Main Camera", typeof(Camera), typeof(AudioListener), typeof(FixedRoomCamera));
            cameraObject.tag = "MainCamera";
            var camera = cameraObject.GetComponent<Camera>();
            camera.clearFlags = CameraClearFlags.SolidColor;
            camera.backgroundColor = Hex("152326");
            camera.nearClipPlane = .1f; camera.farClipPlane = 70;
            camera.allowHDR = true;
            camera.transform.rotation = Quaternion.Euler(35, 45, 0);
            camera.transform.position = new Vector3(0, 1.4f, 0) - camera.transform.forward * 24;
            camera.orthographic = true; camera.orthographicSize = 6.7f;
            camera.GetUniversalAdditionalCameraData().renderPostProcessing = true;
            new GameObject("Interface", typeof(BarHud));
            new GameObject("Runtime verification (only active with -barSmokeTest)", typeof(BarSmokeRunner));
            RenderSettings.ambientMode = AmbientMode.Trilight;
            RenderSettings.ambientSkyColor = Hex("C4BC9A") * .68f;
            RenderSettings.ambientEquatorColor = Hex("8CA5A1") * .5f;
            RenderSettings.ambientGroundColor = Hex("514735") * .45f;
            RenderSettings.reflectionIntensity = .3f;
            EditorSceneManager.SaveScene(EditorSceneManager.GetActiveScene(), ScenePath);
            EditorBuildSettings.scenes = new[] { new EditorBuildSettingsScene(ScenePath, true) };
            AssetDatabase.SaveAssets();
            Debug.Log("AMBER_SCENE_READY: " + ScenePath);
        }

        private static void ConfigureProject()
        {
            PlayerSettings.companyName = "Amber Room Studio";
            PlayerSettings.productName = "The Amber Room";
            PlayerSettings.bundleVersion = "0.1.0";
            PlayerSettings.defaultScreenWidth = 1280;
            PlayerSettings.defaultScreenHeight = 720;
            PlayerSettings.fullScreenMode = FullScreenMode.Windowed;
            PlayerSettings.resizableWindow = true;
            PlayerSettings.runInBackground = true;
            PlayerSettings.colorSpace = ColorSpace.Linear;
            PlayerSettings.SetScriptingBackend(NamedBuildTarget.Standalone, ScriptingImplementation.Mono2x);
            PlayerSettings.SetApiCompatibilityLevel(NamedBuildTarget.Standalone, ApiCompatibilityLevel.NET_Standard);
            var settings = new SerializedObject(AssetDatabase.LoadAllAssetsAtPath("ProjectSettings/ProjectSettings.asset")[0]);
            var input = settings.FindProperty("activeInputHandler");
            if (input != null) { input.intValue = 1; settings.ApplyModifiedPropertiesWithoutUndo(); }
            QualitySettings.vSyncCount = 0;
            QualitySettings.shadows = UnityEngine.ShadowQuality.All;
            QualitySettings.shadowDistance = 45;
        }

        private static void ConfigurePipeline()
        {
            var pipelinePath = Root + "/Rendering/AmberURP.asset";
            var pipeline = AssetDatabase.LoadAssetAtPath<UniversalRenderPipelineAsset>(pipelinePath);
            if (!pipeline)
            {
                var renderer = ScriptableObject.CreateInstance<UniversalRendererData>();
                AssetDatabase.CreateAsset(renderer, Root + "/Rendering/AmberRenderer.asset");
                pipeline = UniversalRenderPipelineAsset.Create(renderer);
                AssetDatabase.CreateAsset(pipeline, pipelinePath);
            }
            pipeline.msaaSampleCount = 4;
            pipeline.renderScale = 1;
            pipeline.supportsHDR = true;
            pipeline.shadowDistance = 40;
            pipeline.maxAdditionalLightsCount = 8;
            // These URP 14 properties have internal setters; configure the serialized asset.
            var pipelineSettings = new SerializedObject(pipeline);
            pipelineSettings.FindProperty("m_MainLightShadowmapResolution").intValue = 2048;
            pipelineSettings.FindProperty("m_AdditionalLightsRenderingMode").intValue = (int)LightRenderingMode.PerPixel;
            pipelineSettings.FindProperty("m_AdditionalLightShadowsSupported").boolValue = false;
            pipelineSettings.ApplyModifiedPropertiesWithoutUndo();
            GraphicsSettings.defaultRenderPipeline = pipeline;
            for (int i = 0; i < QualitySettings.names.Length; i++)
            {
                QualitySettings.SetQualityLevel(i, false);
                QualitySettings.renderPipeline = pipeline;
                QualitySettings.vSyncCount = 0;
            }
            EditorUtility.SetDirty(pipeline);
        }

        private static Color Hex(string hex) { ColorUtility.TryParseHtmlString("#" + hex, out var value); return value; }
        private static void Material(string name, string color, float metallic = 0, float smoothness = .25f, float glow = 0)
        {
            var path = Root + "/Materials/" + name + ".mat";
            var mat = AssetDatabase.LoadAssetAtPath<Material>(path);
            if (!mat) { mat = new Material(Shader.Find("Universal Render Pipeline/Lit")); AssetDatabase.CreateAsset(mat, path); }
            mat.SetColor("_BaseColor", Hex(color)); mat.SetFloat("_Metallic", metallic); mat.SetFloat("_Smoothness", smoothness);
            if (glow > 0) { mat.EnableKeyword("_EMISSION"); mat.SetColor("_EmissionColor", Hex(color) * glow); }
            Mats[name] = mat; EditorUtility.SetDirty(mat);
        }
        private static void Palette()
        {
            Material("Walnut", "51382C"); Material("WalnutDark", "302A26");
            Material("Oak", "B78553"); Material("OakLight", "C69767"); Material("OakDark", "96673F");
            Material("Plaster", "607D70"); Material("Panel", "23473F"); Material("PanelLight", "345B4E");
            Material("Leather", "AC613D", 0, .38f); Material("LeatherGreen", "306357", 0, .35f);
            Material("Brass", "CEA15B", .65f, .5f); Material("Iron", "243134", .45f, .3f);
            Material("Cream", "E9D9AD"); Material("Red", "953C33"); Material("Rug", "62382F");
            Material("BottleGreen", "387B58", .1f, .5f); Material("BottleAmber", "B57932", .1f, .5f);
            Material("BottleBlue", "397E88", .1f, .5f); Material("BottleWine", "643E49", .1f, .5f);
            Material("Glass", "99BCB3", .2f, .65f); Material("Leaf", "4E7952"); Material("LeafLight", "739558");
            Material("Skin", "DEA879"); Material("Hair", "3D2929"); Material("Jacket", "E2A544");
            Material("Trousers", "334B57"); Material("Shirt", "E5DCC6"); Material("Shoe", "3B3333");
            Material("Glow", "FFD589", 0, .25f, 2.2f); Material("Window", "8CB6AE", 0, .2f, .35f);
            Material("Stage", "1A3032"); Material("Coffee", "422B21");
        }

        private static Transform Group(string name, Transform parent = null)
        {
            var go = new GameObject(name); go.transform.SetParent(parent ? parent : room, false); return go.transform;
        }
        private static GameObject Box(string name, Vector3 position, Vector3 size, string mat, Transform parent = null, bool collider = false)
        {
            var go = GameObject.CreatePrimitive(PrimitiveType.Cube); go.name = name;
            go.transform.SetParent(parent ? parent : room, false); go.transform.localPosition = position; go.transform.localScale = size;
            go.GetComponent<Renderer>().sharedMaterial = Mats[mat];
            if (!collider) UnityEngine.Object.DestroyImmediate(go.GetComponent<Collider>());
            return go;
        }
        private static Mesh Lathe(string key, Vector2[] profile, int sides = 12)
        {
            if (Meshes.TryGetValue(key, out var cached)) return cached;
            var path = Root + "/Meshes/" + key + ".asset";
            var mesh = AssetDatabase.LoadAssetAtPath<Mesh>(path);
            if (mesh) { Meshes[key] = mesh; return mesh; }
            var vertices = new List<Vector3>(); var triangles = new List<int>();
            for (int r = 0; r < profile.Length - 1; r++)
            for (int s = 0; s < sides; s++)
            {
                var a = s * Mathf.PI * 2 / sides; var b = (s + 1) * Mathf.PI * 2 / sides;
                var index = vertices.Count;
                vertices.Add(new Vector3(Mathf.Cos(a) * profile[r].x, profile[r].y, Mathf.Sin(a) * profile[r].x));
                vertices.Add(new Vector3(Mathf.Cos(a) * profile[r+1].x, profile[r+1].y, Mathf.Sin(a) * profile[r+1].x));
                vertices.Add(new Vector3(Mathf.Cos(b) * profile[r+1].x, profile[r+1].y, Mathf.Sin(b) * profile[r+1].x));
                vertices.Add(new Vector3(Mathf.Cos(b) * profile[r].x, profile[r].y, Mathf.Sin(b) * profile[r].x));
                triangles.AddRange(new[] { index, index + 1, index + 2, index, index + 2, index + 3 });
            }
            mesh = new Mesh { name = key }; mesh.SetVertices(vertices); mesh.SetTriangles(triangles, 0); mesh.RecalculateNormals(); mesh.RecalculateBounds();
            AssetDatabase.CreateAsset(mesh, path); Meshes[key] = mesh; return mesh;
        }
        private static GameObject Shape(string name, Vector3 position, Vector3 size, string mat, string shape = "Cylinder", Transform parent = null)
        {
            Vector2[] profile = shape switch
            {
                "Bottle" => new[] { new Vector2(0,0), new Vector2(.5f,0), new Vector2(.5f,.62f), new Vector2(.19f,.78f), new Vector2(.19f,1), new Vector2(0,1) },
                "Shade" => new[] { new Vector2(0,0), new Vector2(.5f,0), new Vector2(.21f,1), new Vector2(0,1) },
                "Orb" => new[] { new Vector2(0,0), new Vector2(.36f,.15f), new Vector2(.5f,.5f), new Vector2(.36f,.85f), new Vector2(0,1) },
                _ => new[] { new Vector2(0,0), new Vector2(.5f,0), new Vector2(.5f,1), new Vector2(0,1) }
            };
            var go = new GameObject(name, typeof(MeshFilter), typeof(MeshRenderer));
            go.transform.SetParent(parent ? parent : room, false); go.transform.localPosition = position; go.transform.localScale = size;
            go.GetComponent<MeshFilter>().sharedMesh = Lathe(shape, profile);
            go.GetComponent<MeshRenderer>().sharedMaterial = Mats[mat]; return go;
        }
        private static void SolidBounds(Transform parent, Vector3 center, Vector3 size)
        {
            var col = parent.gameObject.AddComponent<BoxCollider>(); col.center = center; col.size = size;
        }

        private static void Architecture()
        {
            var shell = Group("01 | architecture & plank floor");
            Box("Floating foundation", new Vector3(0,-.24f,0), new Vector3(12.5f,.46f,10.5f), "WalnutDark", shell, true);
            Box("Brass foundation reveal", new Vector3(0,-.1f,0), new Vector3(12.54f,.035f,10.54f), "Brass", shell);
            for (int row = 0; row < 20; row++)
            for (int col = 0; col < 6; col++)
            {
                var color = new[] { "Oak", "OakLight", "OakDark" }[random.Next(3)];
                Box("Individual oak plank", new Vector3(-5 + col * 2, .014f, -4.75f + row * .5f), new Vector3(1.985f,.04f,.485f), color, shell);
            }
            Box("Back plaster wall", new Vector3(0,1.75f,5.12f), new Vector3(12.5f,3.5f,.22f), "Plaster", shell, true);
            Box("Right plaster wall", new Vector3(6.12f,1.75f,0), new Vector3(.22f,3.5f,10.5f), "Plaster", shell, true);
            Box("Back green wainscot", new Vector3(0,.6f,4.965f), new Vector3(12,.98f,.08f), "Panel", shell);
            Box("Right green wainscot", new Vector3(5.965f,.6f,0), new Vector3(.08f,.98f,10), "Panel", shell);
            for (int i = 0; i < 12; i++) Box("Back panel stile", new Vector3(-5.5f+i,.59f,4.89f), new Vector3(.035f,.93f,.055f), "PanelLight", shell);
            for (int i = 0; i < 10; i++) Box("Side panel stile", new Vector3(5.89f,.59f,-4.5f+i), new Vector3(.055f,.93f,.035f), "PanelLight", shell);
            foreach (var y in new[] { .12f, 1.1f, 3.4f })
            {
                Box("Back molding", new Vector3(0,y,4.89f), new Vector3(12.25f,.07f,.15f), "Walnut", shell);
                Box("Side molding", new Vector3(5.89f,y,0), new Vector3(.15f,.07f,10.2f), "Walnut", shell);
            }
            var borders = Group("Invisible safety boundaries", shell);
            var left = Group("Open left boundary", borders); SolidBounds(left, new Vector3(-6.18f,1,0), new Vector3(.25f,2,10.5f));
            var front = Group("Open front boundary", borders); SolidBounds(front, new Vector3(0,1,-5.18f), new Vector3(12.5f,2,.25f));
            Box("Open edge trim", new Vector3(-6.06f,.035f,0), new Vector3(.12f,.07f,10.15f), "Walnut", shell);
            Box("Open edge trim", new Vector3(0,.035f,-5.06f), new Vector3(12.15f,.07f,.12f), "Walnut", shell);
        }

        private static void Bar()
        {
            var bar = Group("02 | bar counter & backbar");
            Box("Counter body", new Vector3(-1.8f,.53f,2.26f), new Vector3(6.5f,1.06f,1.05f), "Panel", bar, true);
            Box("Counter walnut top", new Vector3(-1.8f,1.13f,2.26f), new Vector3(6.85f,.16f,1.34f), "Walnut", bar);
            Box("Counter brass edge", new Vector3(-1.8f,1.085f,1.573f), new Vector3(6.85f,.035f,.025f), "Brass", bar);
            for (int i = 0; i < 6; i++)
            {
                float x = -4.5f + i * 1.08f;
                Box("Recessed counter panel", new Vector3(x,.55f,1.718f), new Vector3(.92f,.7f,.035f), "PanelLight", bar);
                Box("Panel inset", new Vector3(x,.55f,1.688f), new Vector3(.79f,.57f,.018f), "Panel", bar);
            }
            Box("Foot rail", new Vector3(-1.8f,.24f,1.47f), new Vector3(6.4f,.045f,.045f), "Brass", bar);
            Box("Back cabinet", new Vector3(-1.8f,.45f,4.58f), new Vector3(6.8f,.9f,.68f), "Walnut", bar, true);
            Box("Back cabinet top", new Vector3(-1.8f,.94f,4.56f), new Vector3(7,.08f,.82f), "Oak", bar);
            for (int i = 0; i < 6; i++)
            {
                Box("Cabinet door", new Vector3(-4.65f + i*1.14f,.46f,4.222f), new Vector3(1.03f,.71f,.05f), "WalnutDark", bar);
                Shape("Cabinet handle", new Vector3(-4.3f+i*1.14f,.48f,4.16f), new Vector3(.06f,.05f,.06f), "Brass", "Orb", bar);
            }
            Box("Shelf backing", new Vector3(-1.8f,1.95f,4.86f), new Vector3(6.8f,1.8f,.08f), "WalnutDark", bar);
            foreach (float y in new[] {1.1f,1.75f,2.4f,2.86f}) Box("Display shelf", new Vector3(-1.8f,y,4.63f), new Vector3(6.92f,.07f,.55f), "Walnut", bar);
            for (int i = 0; i < 5; i++) Box("Shelf vertical", new Vector3(-5.23f+i*1.72f,1.98f,4.64f), new Vector3(.07f,1.82f,.51f), "Walnut", bar);
            for (int shelf = 0; shelf < 2; shelf++)
            for (int i = 0; i < 23; i++) Bottle(new Vector3(-5.0f+i*.28f,1.14f+shelf*.65f,4.48f), .72f+(float)random.NextDouble()*.28f, bar);
            Sign("AMBER ROOM", new Vector3(-1.8f,3.12f,4.85f), .065f, "Cream", bar);
            for (int i = 0; i < 5; i++) Stool(new Vector3(-4.35f+i*1.28f,0,.87f), bar);
            Bottle(new Vector3(-3.6f,1.22f,2.3f), 1, bar);
            Bottle(new Vector3(.55f,1.22f,2.42f), .9f, bar);
            foreach (float x in new[] {-3.2f,-1.5f,.1f}) Cup(new Vector3(x,1.22f,2.03f), bar);
            Box("Bar towel", new Vector3(-2.2f,1.22f,2.34f), new Vector3(.55f,.012f,.4f), "Cream", bar);
            var tap = Group("Two brass beer taps", bar);
            foreach (float x in new[] {-.8f,-.47f})
            {
                Shape("Tap column", new Vector3(x,1.22f,2.6f), new Vector3(.08f,.4f,.08f), "Brass", parent:tap);
                Box("Tap neck", new Vector3(x,1.6f,2.52f), new Vector3(.08f,.08f,.22f), "Brass", tap);
                Box("Tap handle", new Vector3(x,1.68f,2.44f), new Vector3(.065f,.19f,.065f), "Walnut", tap);
            }
        }

        private static void Bottle(Vector3 p, float scale, Transform parent)
        {
            var mat = new[] { "BottleGreen", "BottleAmber", "BottleBlue", "BottleWine" }[random.Next(4)];
            Shape("Faceted bottle", p, new Vector3(.14f,.43f,.14f)*scale, mat, "Bottle", parent);
            Shape("Cream bottle label", p+Vector3.up*.12f*scale, new Vector3(.143f,.11f,.143f)*scale, "Cream", parent:parent);
            Shape("Bottle foil", p+Vector3.up*.39f*scale, new Vector3(.062f,.045f,.062f)*scale, "Brass", parent:parent);
        }
        private static void Cup(Vector3 p, Transform parent)
        {
            Shape("Coaster", p, new Vector3(.23f,.014f,.23f), "Cream", parent:parent);
            Shape("Glass tumbler", p+Vector3.up*.018f, new Vector3(.13f,.17f,.13f), "Glass", parent:parent);
            Shape("Amber drink", p+Vector3.up*.155f, new Vector3(.11f,.008f,.11f), "BottleAmber", parent:parent);
        }
        private static void Stool(Vector3 p, Transform parent)
        {
            var stool = Group("Leather bar stool", parent); stool.localPosition = p;
            Shape("Leather cushion", new Vector3(0,.73f,0), new Vector3(.57f,.13f,.57f), "Leather", parent:stool);
            Shape("Seat brass rim", new Vector3(0,.70f,0), new Vector3(.58f,.04f,.58f), "Brass", parent:stool);
            for (int i = 0; i < 4; i++)
            {
                float x = i % 2 == 0 ? -.18f : .18f; float z = i < 2 ? -.18f : .18f;
                Box("Stool leg", new Vector3(x,.35f,z), new Vector3(.055f,.7f,.055f), "Walnut", stool);
            }
            Box("Stool crossbar", new Vector3(0,.23f,-.18f), new Vector3(.42f,.04f,.04f), "Brass", stool);
            SolidBounds(stool, new Vector3(0,.43f,0), new Vector3(.59f,.86f,.59f));
        }
        private static void Seating()
        {
            var seating = Group("03 | lounge seating");
            Rug(new Vector3(3.8f,.045f,.55f), new Vector3(3.4f,.015f,5.2f), seating);
            for (int i = 0; i < 3; i++)
            {
                var booth = Group("Green leather banquette", seating); booth.localPosition = new Vector3(5.21f,0,2.6f-i*1.38f);
                Box("Booth plinth", new Vector3(0,.16f,0), new Vector3(1.14f,.3f,1.34f), "Walnut", booth);
                Box("Seat cushion", new Vector3(-.05f,.43f,0), new Vector3(1.08f,.23f,1.28f), "LeatherGreen", booth);
                Box("Upholstered back", new Vector3(.48f,.78f,0), new Vector3(.2f,.8f,1.32f), "LeatherGreen", booth);
                for (int k = 0; k < 5; k++) Box("Back piping", new Vector3(.365f,.82f,-.5f+k*.25f), new Vector3(.02f,.61f,.018f), "PanelLight", booth);
                SolidBounds(booth, new Vector3(0,.58f,0), new Vector3(1.16f,1.16f,1.34f));
            }
            Table(new Vector3(3.72f,0,2), .95f, seating);
            Table(new Vector3(3.72f,0,-.5f), .95f, seating);
            Chair(new Vector3(2.7f,0,2), 90, seating);
            Chair(new Vector3(2.7f,0,-.5f), 90, seating);
            Rug(new Vector3(-3.1f,.045f,-2.33f), new Vector3(3.25f,.015f,3.4f), seating);
            Table(new Vector3(-3.2f,0,-2.25f), 1.22f, seating);
            Chair(new Vector3(-4.29f,0,-2.4f), 90, seating);
            Chair(new Vector3(-2.24f,0,-1.67f), -120, seating);
            Chair(new Vector3(-3.1f,0,-3.43f), 0, seating);
            Table(new Vector3(3.85f,0,-3.53f), .92f, seating);
            Chair(new Vector3(4.86f,0,-3.6f), -90, seating);
            Chair(new Vector3(3.1f,0,-4.3f), 35, seating);
        }
        private static void Rug(Vector3 p, Vector3 size, Transform parent)
        {
            Box("Woven burgundy rug", p,size,"Rug",parent);
            Box("Rug inner field", p+Vector3.up*.009f, new Vector3(size.x-.19f,.01f,size.z-.19f),"Red",parent);
            Box("Rug center", p+Vector3.up*.016f, new Vector3(size.x-.28f,.01f,size.z-.28f),"Rug",parent);
            for (int i = 0; i < 8; i++)
                Box("Woven diamond detail", p+new Vector3(-size.x*.38f+i*size.x*.108f,.025f,0), new Vector3(.09f,.008f,.09f), "Oak", parent).transform.localRotation = Quaternion.Euler(0,45,0);
        }
        private static void Table(Vector3 p, float diameter, Transform parent)
        {
            var table = Group("Round bistro table", parent); table.localPosition = p;
            Shape("Cast iron base", Vector3.zero, new Vector3(.53f,.09f,.53f), "Iron", parent:table);
            Shape("Table pedestal", new Vector3(0,.08f,0), new Vector3(.11f,.68f,.11f), "Iron", parent:table);
            Shape("Oak tabletop", new Vector3(0,.77f,0), new Vector3(diameter,.09f,diameter), "Oak", parent:table);
            Shape("Top brass edge", new Vector3(0,.76f,0), new Vector3(diameter+.018f,.025f,diameter+.018f), "Brass", parent:table);
            var col = table.gameObject.AddComponent<CapsuleCollider>(); col.center = new Vector3(0,.43f,0); col.radius = diameter*.5f; col.height = .88f;
            Cup(new Vector3(.21f,.86f,.1f), table);
            Shape("Candle holder", new Vector3(-.14f,.86f,-.09f), new Vector3(.12f,.12f,.12f), "Brass", parent:table);
            Shape("Candle", new Vector3(-.14f,.98f,-.09f), new Vector3(.065f,.095f,.065f), "Glow", parent:table);
        }
        private static void Chair(Vector3 p, float yaw, Transform parent)
        {
            var chair = Group("Upholstered dining chair", parent); chair.localPosition = p; chair.localRotation = Quaternion.Euler(0,yaw,0);
            Box("Seat", new Vector3(0,.46f,0), new Vector3(.59f,.13f,.58f), "Leather", chair);
            Box("Chair back", new Vector3(0,.78f,-.25f), new Vector3(.59f,.51f,.105f), "Walnut", chair);
            Box("Back cushion", new Vector3(0,.79f,-.18f), new Vector3(.47f,.35f,.04f), "Leather", chair);
            for (int i = 0; i < 4; i++) Box("Chair leg", new Vector3(i%2==0?-.22f:.22f,.22f,i<2?-.22f:.22f), new Vector3(.055f,.44f,.055f), "Walnut", chair);
            SolidBounds(chair,new Vector3(0,.52f,0),new Vector3(.62f,1.04f,.62f));
        }

        private static void Decorations()
        {
            var decor = Group("04 | little details");
            var window = Group("Evening window", decor); window.localPosition = new Vector3(5.85f,2.17f,-1.9f); window.localRotation = Quaternion.Euler(0,90,0);
            Box("Window frame",Vector3.zero,new Vector3(2.0f,1.9f,.16f),"Walnut",window);
            Box("Window glass",new Vector3(0,0,-.09f),new Vector3(1.8f,1.7f,.03f),"Window",window);
            Box("Window mullion",new Vector3(0,0,-.13f),new Vector3(.06f,1.74f,.06f),"Walnut",window);
            Box("Window cross rail",new Vector3(0,0,-.13f),new Vector3(1.83f,.06f,.06f),"Walnut",window);
            Box("Deep sill",new Vector3(0,-.92f,-.18f),new Vector3(2.14f,.1f,.44f),"Oak",window);
            var art = Group("Framed geometric print", decor); art.localPosition = new Vector3(5.85f,2.23f,1.9f); art.localRotation = Quaternion.Euler(0,90,0);
            Box("Art brass frame",Vector3.zero,new Vector3(1.53f,1.22f,.10f),"Brass",art);
            Box("Art paper",new Vector3(0,0,-.06f),new Vector3(1.4f,1.09f,.02f),"Cream",art);
            Box("Abstract green hill",new Vector3(-.2f,-.19f,-.082f),new Vector3(.8f,.49f,.015f),"Panel",art);
            Box("Abstract red hill",new Vector3(.32f,-.15f,-.098f),new Vector3(.4f,.58f,.015f),"Leather",art);
            var sun = Shape("Abstract golden sun",new Vector3(.17f,.25f,-.11f),new Vector3(.32f,.015f,.32f),"Brass",parent:art); sun.transform.localRotation=Quaternion.Euler(90,0,0);
            Plant(new Vector3(5.25f,0,4.3f),1.3f,decor); Plant(new Vector3(-5.35f,0,-3.8f),1,decor);
            Plant(new Vector3(1.1f,.99f,4.52f),.43f,decor);
            var menu = Group("Chalk menu",decor); menu.localPosition=new Vector3(-5.72f,1.95f,4.88f);
            Box("Menu frame",Vector3.zero,new Vector3(.72f,1.42f,.08f),"Oak",menu);
            Box("Menu board",new Vector3(0,0,-.05f),new Vector3(.61f,1.3f,.02f),"Iron",menu);
            Sign("TONIGHT",new Vector3(0,.39f,-.071f),.018f,"Cream",menu);
            Sign("ALE\n\nWINE\n\nJAZZ",new Vector3(0,-.1f,-.071f),.022f,"Cream",menu);
            Box("Entrance mat",new Vector3(-.2f,.05f,-4.37f),new Vector3(1.65f,.025f,.72f),"Panel",decor);
        }
        private static void Plant(Vector3 p,float scale,Transform parent)
        {
            var plant=Group("Potted plant",parent);plant.localPosition=p;plant.localScale=Vector3.one*scale;
            Shape("Terracotta pot",Vector3.zero,new Vector3(.48f,.46f,.48f),"Leather",parent:plant);
            Shape("Soil",new Vector3(0,.46f,0),new Vector3(.4f,.012f,.4f),"WalnutDark",parent:plant);
            for(int i=0;i<9;i++)
            {
                var angle=i*137.5f*Mathf.Deg2Rad;
                var leaf=Shape("Faceted leaf",new Vector3(Mathf.Cos(angle)*.13f,.45f+ i%3*.15f,Mathf.Sin(angle)*.13f),new Vector3(.29f,.74f,.21f),i%2==0?"Leaf":"LeafLight","Orb",plant);
                leaf.transform.localRotation=Quaternion.Euler(Mathf.Sin(angle)*35,angle*Mathf.Rad2Deg,Mathf.Cos(angle)*35);
            }
            if(scale>.6f) SolidBounds(plant,new Vector3(0,.25f,0),new Vector3(.55f,.5f,.55f));
        }
        private static void Sign(string text,Vector3 p,float size,string color,Transform parent)
        {
            var go=new GameObject(text,typeof(TextMesh));go.transform.SetParent(parent,false);go.transform.localPosition=p;
            var label=go.GetComponent<TextMesh>();label.text=text;label.font=Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            label.fontSize=80;label.characterSize=size;label.anchor=TextAnchor.MiddleCenter;label.alignment=TextAlignment.Center;
            label.color=Mats[color].GetColor("_BaseColor");go.GetComponent<MeshRenderer>().sharedMaterial=label.font.material;
        }
        private static void Lighting()
        {
            var lighting=Group("05 | warm lighting");
            var sun=new GameObject("Soft late afternoon key",typeof(Light));sun.transform.SetParent(lighting);sun.transform.rotation=Quaternion.Euler(48,-35,0);
            var key=sun.GetComponent<Light>();key.type=LightType.Directional;key.color=Hex("FFE2B2");key.intensity=1.35f;key.shadows=LightShadows.Soft;key.shadowStrength=.72f;key.shadowBias=.035f;
            for(int i=0;i<3;i++)
            {
                float x=-4.25f+i*2.45f;
                Box("Pendant cord",new Vector3(x,2.94f,2.23f),new Vector3(.019f,.76f,.019f),"Iron",lighting);
                Shape("Brass pendant shade",new Vector3(x,2.47f,2.23f),new Vector3(.76f,.34f,.76f),"Brass","Shade",lighting);
                Shape("Warm diffuser",new Vector3(x,2.46f,2.23f),new Vector3(.69f,.03f,.69f),"Glow",parent:lighting);
                var bulb=new GameObject("Pendant warm pool",typeof(Light));bulb.transform.SetParent(lighting);bulb.transform.localPosition=new Vector3(x,2.36f,2.23f);
                var light=bulb.GetComponent<Light>();light.type=LightType.Point;light.color=Hex("FFC17B");light.intensity=2.4f;light.range=4;light.shadows=LightShadows.None;
            }
            var fill=new GameObject("Cool window fill",typeof(Light));fill.transform.SetParent(lighting);fill.transform.position=new Vector3(4.8f,2.1f,-2);
            fill.GetComponent<Light>().type=LightType.Point;fill.GetComponent<Light>().color=Hex("A4D8D4");fill.GetComponent<Light>().intensity=1.6f;fill.GetComponent<Light>().range=5;
            var volume=new GameObject("Gentle color grade",typeof(Volume));volume.transform.SetParent(lighting);
            var profilePath=Root+"/Rendering/AmberVolume.asset";
            var profile=AssetDatabase.LoadAssetAtPath<VolumeProfile>(profilePath);
            if(!profile){profile=ScriptableObject.CreateInstance<VolumeProfile>();AssetDatabase.CreateAsset(profile,profilePath);}
            if(!profile.TryGet<Bloom>(out var bloom)){bloom=profile.Add<Bloom>();AssetDatabase.AddObjectToAsset(bloom,profile);}
            bloom.intensity.Override(.18f);bloom.threshold.Override(1.1f);
            if(!profile.TryGet<Tonemapping>(out var tone)){tone=profile.Add<Tonemapping>();AssetDatabase.AddObjectToAsset(tone,profile);}
            tone.mode.Override(TonemappingMode.ACES);
            volume.GetComponent<Volume>().isGlobal=true;volume.GetComponent<Volume>().sharedProfile=profile;
            EditorUtility.SetDirty(profile);
        }

        private static void MakePlayer()
        {
            var go=new GameObject("Player | the evening wanderer");
            var controller=go.AddComponent<CharacterController>();controller.height=1.7f;controller.radius=.24f;controller.center=new Vector3(0,.86f,0);controller.skinWidth=.025f;controller.stepOffset=.18f;controller.slopeLimit=45;controller.minMoveDistance=0;
            var motor=go.AddComponent<PlayerMotor>();
            var visual=Group("Visual",go.transform);motor.VisualRoot=visual;
            var pose=go.AddComponent<CharacterPose>();pose.motor=motor;
            var torso=Group("Breathing body",visual);torso.localPosition=new Vector3(0,.91f,0);pose.torso=torso;
            Shape("Jacket body",new Vector3(0,0,0),new Vector3(.53f,.52f,.34f),"Jacket","Orb",torso);
            Box("Shirt front",new Vector3(0,.27f,.159f),new Vector3(.19f,.30f,.025f),"Shirt",torso);
            Box("Jacket seam",new Vector3(0,.08f,.158f),new Vector3(.024f,.17f,.028f),"Brass",torso);
            Box("Scarf",new Vector3(0,.48f,.09f),new Vector3(.35f,.12f,.23f),"Red",torso);
            Box("Scarf tail",new Vector3(.13f,.34f,.19f),new Vector3(.09f,.24f,.045f),"Red",torso);
            Shape("Neck",new Vector3(0,.5f,0),new Vector3(.17f,.12f,.17f),"Skin",parent:torso);
            Shape("Faceted head",new Vector3(0,.58f,0),new Vector3(.37f,.4f,.35f),"Skin","Orb",torso);
            Shape("Hair silhouette",new Vector3(0,.78f,-.035f),new Vector3(.40f,.25f,.37f),"Hair","Orb",torso);
            Box("Swept fringe",new Vector3(-.07f,.86f,.134f),new Vector3(.24f,.11f,.12f),"Hair",torso).transform.localRotation=Quaternion.Euler(0,0,-14);
            Box("Nose",new Vector3(0,.75f,.175f),new Vector3(.065f,.075f,.08f),"Skin",torso);
            foreach(float x in new[]{-.083f,.083f}) Box("Eye",new Vector3(x,.795f,.161f),new Vector3(.035f,.032f,.021f),"Hair",torso);
            for(int side=0;side<2;side++)
            {
                float sign=side==0?-1:1;
                var arm=Group(side==0?"Left arm pivot":"Right arm pivot",torso);arm.localPosition=new Vector3(sign*.3f,.41f,0);
                Shape("Sleeve",new Vector3(0,-.41f,0),new Vector3(.18f,.43f,.2f),"Jacket","Orb",arm);
                Shape("Hand",new Vector3(0,-.51f,0),new Vector3(.145f,.17f,.145f),"Skin","Orb",arm);
                var leg=Group(side==0?"Left leg pivot":"Right leg pivot",visual);leg.localPosition=new Vector3(sign*.135f,.96f,0);
                Shape("Trouser leg",new Vector3(0,-.72f,0),new Vector3(.21f,.75f,.23f),"Trousers",parent:leg);
                Box("Boot",new Vector3(0,-.82f,.055f),new Vector3(.23f,.19f,.36f),"Shoe",leg);
                Box("Boot sole",new Vector3(0,-.91f,.055f),new Vector3(.235f,.035f,.37f),"WalnutDark",leg);
                if(side==0){pose.leftArm=arm;pose.leftLeg=leg;}else{pose.rightArm=arm;pose.rightLeg=leg;}
            }
            Shape("Ground marker",new Vector3(0,.055f,0),new Vector3(.7f,.01f,.7f),"Brass",parent:go.transform);
            visual.localRotation=Quaternion.Euler(0,205,0);
            var prefab=PrefabUtility.SaveAsPrefabAsset(go,"Assets/Prefabs/Player.prefab");
            UnityEngine.Object.DestroyImmediate(go);
            var instance=(GameObject)PrefabUtility.InstantiatePrefab(prefab);instance.transform.position=new Vector3(.1f,.05f,-2.35f);
            PrefabUtility.RecordPrefabInstancePropertyModifications(instance.transform);
        }

        [MenuItem("Amber Room/2 - Build Windows game")]
        public static void BuildWindows()
        {
            if(!File.Exists(ScenePath))CreateScene();
            Directory.CreateDirectory("Builds/Windows");
            var report=BuildPipeline.BuildPlayer(new BuildPlayerOptions{scenes=new[]{ScenePath},locationPathName="Builds/Windows/AmberRoom.exe",target=BuildTarget.StandaloneWindows64,options=BuildOptions.None});
            if(report.summary.result!=BuildResult.Succeeded)throw new Exception("Windows build failed: "+report.summary.result);
            Debug.Log("AMBER_BUILD_READY: "+Path.GetFullPath("Builds/Windows/AmberRoom.exe"));
        }
    }
}
