using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEditor.Build.Reporting;
using UnityEngine;
using BarPrototype;
using LastCall;

namespace LastCall.Editor
{
    public static class LastCallSceneBuilder
    {
        public const string ScenePath="Assets/Scenes/LastCall.unity";
        [MenuItem("Last Call/1 - Create MVP scene")]
        public static void Prepare()
        {
            EditorSceneManager.OpenScene("Assets/Scenes/AmberRoom.unity");
            EditorSceneManager.SaveScene(EditorSceneManager.GetActiveScene(),ScenePath);
            foreach(var hud in UnityEngine.Object.FindObjectsOfType<BarHud>())UnityEngine.Object.DestroyImmediate(hud.gameObject);
            foreach(var smoke in UnityEngine.Object.FindObjectsOfType<BarSmokeRunner>())UnityEngine.Object.DestroyImmediate(smoke.gameObject);
            foreach(var player in UnityEngine.Object.FindObjectsOfType<PlayerMotor>())UnityEngine.Object.DestroyImmediate(player.gameObject);
            var oldBoundary=UnityEngine.Object.FindObjectsOfType<Transform>().FirstOrDefault(t=>t.name=="Open left boundary");
            if(oldBoundary)UnityEngine.Object.DestroyImmediate(oldBoundary.gameObject);
            var root=new GameObject("Last Call | locations & terrace").transform;
            Cube("Terrace floor",new Vector3(-7.15f,-.07f,-3.2f),new Vector3(2.4f,.2f,2.8f),"Oak",root,true);
            Cube("Terrace left safety",new Vector3(-8.4f,1,-3.2f),new Vector3(.15f,2,3),"Walnut",root,true,false);
            Cube("Terrace north safety",new Vector3(-7.2f,1,-1.72f),new Vector3(2.45f,2,.15f),"Walnut",root,true,false);
            Cube("Terrace south safety",new Vector3(-7.2f,1,-4.66f),new Vector3(2.45f,2,.15f),"Walnut",root,true,false);
            Cube("Left edge north",new Vector3(-6.18f,1,1.7f),new Vector3(.25f,2,6.85f),"Walnut",root,true,false);
            Cube("Left edge south",new Vector3(-6.18f,1,-4.98f),new Vector3(.25f,2,.6f),"Walnut",root,true,false);
            Cube("Terrace railing",new Vector3(-8.3f,.65f,-3.2f),new Vector3(.08f,.08f,2.8f),"Brass",root);
            foreach(float z in new[]{-4.5f,-3.2f,-1.9f})
                Cube("Railing post",new Vector3(-8.3f,.34f,z),new Vector3(.07f,.7f,.07f),"Iron",root);
            Cube("Quiet divider",new Vector3(1.85f,.83f,3.2f),new Vector3(.08f,1.65f,1.2f),"Panel",root,true);
            var sceneData=JsonUtility.FromJson<SceneConfig>(File.ReadAllText("Server/scenarios/last_call.json"));
            foreach(var place in sceneData.locations)
            {
                var obj=new GameObject("Location | "+place.id);
                obj.transform.SetParent(root);
                obj.transform.position=new Vector3(place.x,.045f,place.z);
                var marker=Cube("Location marker",new Vector3(place.x,.047f,place.z),new Vector3(.46f,.015f,.46f),"Brass",root);
                if(place.id=="seat13")MakeSign("13",new Vector3(place.x,.09f,place.z),root);
            }
            var gameObject=new GameObject("Last Call | relational runtime");
            var game=gameObject.AddComponent<LastCallGame>();
            game.characterPrefab=AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Prefabs/Player.prefab");
            var fixedCamera=Camera.main.GetComponent<FixedRoomCamera>();
            fixedCamera.focus=new Vector3(-.65f,1.15f,0);
            fixedCamera.minimumHalfWidth=9.2f;
            fixedCamera.halfHeight=6.4f;
            Physics.SyncTransforms();
            ExportNavigation();
            PlayerSettings.productName="LALAGAME - Last Call";
            PlayerSettings.bundleVersion="0.2.0";
            PlayerSettings.defaultScreenWidth=1280;
            PlayerSettings.defaultScreenHeight=720;
            PlayerSettings.resizableWindow=true;
            EditorSceneManager.SaveScene(EditorSceneManager.GetActiveScene(),ScenePath);
            AssetDatabase.SaveAssets();
            Debug.Log("LASTCALL_SCENE_READY");
        }
        private static GameObject Cube(string name,Vector3 position,Vector3 scale,string material,Transform parent,bool collider=false,bool visible=true)
        {
            var go=GameObject.CreatePrimitive(PrimitiveType.Cube);
            go.name=name;go.transform.SetParent(parent);go.transform.position=position;go.transform.localScale=scale;
            go.GetComponent<Renderer>().sharedMaterial=AssetDatabase.LoadAssetAtPath<Material>("Assets/Generated/Materials/"+material+".mat");
            go.GetComponent<Renderer>().enabled=visible;
            if(!collider)UnityEngine.Object.DestroyImmediate(go.GetComponent<Collider>());
            return go;
        }
        private static void MakeSign(string value,Vector3 position,Transform parent)
        {
            var go=new GameObject("Seat 13",typeof(TextMesh));
            go.transform.SetParent(parent);go.transform.position=position;go.transform.rotation=Quaternion.Euler(90,0,0);
            var text=go.GetComponent<TextMesh>();text.text=value;text.fontSize=64;text.characterSize=.08f;text.anchor=TextAnchor.MiddleCenter;
        }
        [Serializable] private class SceneConfig { public LocationDto[] locations; }
        [Serializable] private class Wall { public float x,z,w,h; }
        [Serializable] private class Navigation { public float cell=.25f,minX=-8.5f,minZ=-5;public int width=60,height=40;public int[] blocked;public Wall[] walls; }
        private static void ExportNavigation()
        {
            var data=new Navigation();var blocked=new List<int>();
            for(int z=0;z<data.height;z++)for(int x=0;x<data.width;x++)
            {
                float px=data.minX+(x+.5f)*data.cell,pz=data.minZ+(z+.5f)*data.cell;
                bool floor=Physics.Raycast(new Vector3(px,.14f,pz),Vector3.down,.5f);
                bool obstacle=Physics.CheckCapsule(new Vector3(px,.35f,pz),new Vector3(px,1.45f,pz),.27f);
                if(!floor||obstacle)blocked.Add(z*data.width+x);
            }
            data.blocked=blocked.ToArray();
            data.walls=UnityEngine.Object.FindObjectsOfType<Collider>()
                .Where(c=>c.bounds.size.y>1.5f&&!c.name.Contains("safety"))
                .Select(c=>new Wall{x=c.bounds.center.x,z=c.bounds.center.z,w=c.bounds.size.x,h=c.bounds.size.z}).ToArray();
            Directory.CreateDirectory("Server/scenarios");
            File.WriteAllText("Server/scenarios/navigation.json",JsonUtility.ToJson(data,true));
            Debug.Log("LASTCALL_NAVIGATION "+(data.width*data.height-data.blocked.Length)+" walkable cells");
        }
        [MenuItem("Last Call/2 - Build Windows MVP")]
        public static void Build()
        {
            PlayerSettings.insecureHttpOption=InsecureHttpOption.AlwaysAllowed;
            if(!File.Exists(ScenePath))Prepare();
            const string destination="Builds/LastCall-Windows";
            Directory.CreateDirectory(destination);
            var result=BuildPipeline.BuildPlayer(new BuildPlayerOptions{
                scenes=new[]{ScenePath},locationPathName=destination+"/LastCall.exe",
                target=BuildTarget.StandaloneWindows64,options=BuildOptions.None
            });
            if(result.summary.result!=BuildResult.Succeeded)
                throw new Exception("Last Call build failed");
            foreach(string folder in new[]{"dist","node_modules","scenarios"})
                CopyTree(Path.Combine("Server",folder),Path.Combine(destination,"Server",folder));
            foreach(string file in new[]{"package.json","package-lock.json","NODE-LICENSE.txt"})
                File.Copy(Path.Combine("Server",file),Path.Combine(destination,"Server",file),true);
            File.Copy(@"D:\node.exe",Path.Combine(destination,"Server","node.exe"),true);
            Debug.Log("LASTCALL_BUILD_READY");
        }
        private static void CopyTree(string source,string destination)
        {
            Directory.CreateDirectory(destination);
            foreach(string file in Directory.GetFiles(source))
                File.Copy(file,Path.Combine(destination,Path.GetFileName(file)),true);
            foreach(string directory in Directory.GetDirectories(source))
                CopyTree(directory,Path.Combine(destination,Path.GetFileName(directory)));
        }
    }

    [InitializeOnLoad]
    static class CastModelHook
    {
        static CastModelHook()
        {
            CastModel.Loader = id =>
            {
                var folder = "Assets/LastCall/Characters/" + id;
                if (!AssetDatabase.IsValidFolder(folder)) return null;
                foreach (var guid in AssetDatabase.FindAssets("t:Model", new[] { folder }))
                {
                    var path = AssetDatabase.GUIDToAssetPath(guid);
                    if (path.EndsWith(".fbx", StringComparison.OrdinalIgnoreCase))
                        return AssetDatabase.LoadAssetAtPath<GameObject>(path);
                }
                return null;
            };
        }
    }

    class CastTextureImport : AssetPostprocessor
    {
        void OnPreprocessTexture()
        {
            if (assetPath.IndexOf("/LastCall/Characters/", StringComparison.OrdinalIgnoreCase) < 0) return;
            if (assetPath.IndexOf("_normal.", StringComparison.OrdinalIgnoreCase) < 0) return;
            var importer = (TextureImporter)assetImporter;
            importer.textureType = TextureImporterType.NormalMap;
            importer.sRGBTexture = false;
        }
    }
}
