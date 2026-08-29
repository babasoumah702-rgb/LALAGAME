using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEditor.Build.Reporting;
using UnityEngine;
using UnityEngine.UI;

namespace LastCall.Editor
{
    public static class SceneZeroBuilder
    {
        private const string Root="Assets/LastCall/SceneZero";
        private static readonly Dictionary<string,Material> materials=new Dictionary<string,Material>();
        [MenuItem("Last Call/Scene 0/1 - Add elevator (preserve bar)")]
        public static void Prepare()
        {
            Directory.CreateDirectory(Root+"/Materials");Directory.CreateDirectory(Root+"/Models");
            Directory.CreateDirectory("Library/SceneZeroSafety");
            File.Copy(LastCallSceneBuilder.ScenePath,"Library/SceneZeroSafety/LastCall-"+DateTime.Now.ToString("yyyyMMdd-HHmmss")+".unity");
            EditorSceneManager.OpenScene(LastCallSceneBuilder.ScenePath);
            var game=UnityEngine.Object.FindObjectOfType<LastCallGame>();
            if(!game)throw new Exception("Open the existing Last Call scene first");
            game.artCatalog=BuildCatalog();
            var intro=UnityEngine.Object.FindObjectOfType<SceneZeroController>();
            if(!intro)
            {
                var boundary=GameObject.Find("Open front boundary");
                if(boundary)UnityEngine.Object.DestroyImmediate(boundary);
                intro=CreateElevator();
                PrefabUtility.SaveAsPrefabAssetAndConnect(intro.gameObject,Root+"/SceneZero.prefab",InteractionMode.AutomatedAction);
            }
            intro.startEye=new Vector3(-1,1.64f,-8.65f);
            intro.floorDisplay.characterSize=.025f;
            intro.floorDisplay.transform.localPosition=new Vector3(0,2.53f,-6.765f);
            var destination=intro.transform.Find("Destination");
            if(destination){destination.localPosition=new Vector3(0,2.32f,-6.768f);destination.GetComponent<TextMesh>().characterSize=.010f;}
            if(!intro.transform.Find("Floor display housing"))
                Box(intro.transform,"Floor display housing",new Vector3(0,2.46f,-6.72f),new Vector3(.82f,.50f,.05f),Mat("Display casing",Hex("101820"),.25f,.6f),false);
            var east=intro.transform.Find("Front boundary east");
            if(east){east.localPosition=new Vector3(4.4f,1,-5.18f);east.localScale=new Vector3(5.75f,2,.25f);}
            foreach(var t in intro.GetComponentsInChildren<TextMesh>())t.transform.localRotation=Quaternion.identity;
            var textShader=Shader.Find("LastCall/WorldText");
            foreach(var t in UnityEngine.Object.FindObjectsOfType<TextMesh>()){
                if(t.GetComponent<Renderer>().sharedMaterial.shader==textShader)continue;
                var m=new Material(textShader);m.mainTexture=t.font?t.font.material.mainTexture:t.GetComponent<Renderer>().sharedMaterial.mainTexture;
                string path=Root+"/Materials/Text-"+t.GetInstanceID()+".mat";
                AssetDatabase.CreateAsset(m,path);t.GetComponent<Renderer>().sharedMaterial=m;
            }
            if(!intro.transform.Find("Panel seams")){
                var seams=new GameObject("Panel seams").transform;seams.SetParent(intro.transform,false);
                var trim=Mat("Panel seam",Hex("48555F"),.3f,.7f);
                foreach(float side in new[]{-1f,1f})foreach(float z in new[]{-9.1f,-8.45f,-7.8f,-7.15f})
                    Box(seams,"Metal panel joint",new Vector3(side*1.521f,1.55f,z),new Vector3(.006f,2.7f,.011f),trim,false);
            }
            PrefabUtility.ApplyPrefabInstance(intro.gameObject,InteractionMode.AutomatedAction);
            Physics.SyncTransforms();ExportNavigation(intro);
            EditorSceneManager.SaveScene(EditorSceneManager.GetActiveScene());
            AssetDatabase.SaveAssets();
            Debug.Log("SCENE0_PREPARED");
        }
        private static LastCallArtCatalog BuildCatalog()
        {
            var catalog=AssetDatabase.LoadAssetAtPath<LastCallArtCatalog>(Root+"/ArtCatalog.asset");
            if(!catalog){catalog=ScriptableObject.CreateInstance<LastCallArtCatalog>();AssetDatabase.CreateAsset(catalog,Root+"/ArtCatalog.asset");}
            catalog.textures=Directory.GetFiles("Assets/LastCall/Stage").Where(p=>new[]{".png",".jpg",".jpeg"}.Contains(Path.GetExtension(p).ToLowerInvariant()))
                .Select(p=>new LastCallArtCatalog.TextureItem{id=Path.GetFileNameWithoutExtension(p),texture=AssetDatabase.LoadAssetAtPath<Texture2D>(p.Replace('\\','/'))}).ToArray();
            var list=new List<LastCallArtCatalog.ModelItem>();
            foreach(var id in new[]{"A","B","C","D","OWNER","BARTENDER"})
            {
                string folder="Assets/LastCall/Characters/"+id;
                var path=Directory.GetFiles(folder,"*.fbx").FirstOrDefault();
                if(path==null)throw new Exception("Missing model "+id);
                var source=AssetDatabase.LoadAssetAtPath<GameObject>(path.Replace('\\','/'));
                if(!source)throw new Exception("Model import failed "+id);
                var instance=UnityEngine.Object.Instantiate(source);instance.name=id+" | production model";
                var mat=Mat("Cast-"+id,Color.white,.22f);
                var tex=Directory.GetFiles(folder,"*",SearchOption.AllDirectories).FirstOrDefault(p=>!p.EndsWith(".meta")&&(p.IndexOf("_basecolor.",StringComparison.OrdinalIgnoreCase)>=0||p.IndexOf("tripo_rgb",StringComparison.OrdinalIgnoreCase)>=0));
                if(tex!=null)mat.SetTexture("_BaseMap",AssetDatabase.LoadAssetAtPath<Texture2D>(tex.Replace('\\','/')));
                foreach(var renderer in instance.GetComponentsInChildren<Renderer>())renderer.sharedMaterials=renderer.sharedMaterials.Select(_=>mat).ToArray();
                var prefab=PrefabUtility.SaveAsPrefabAsset(instance,Root+"/Models/"+id+".prefab");
                UnityEngine.Object.DestroyImmediate(instance);
                list.Add(new LastCallArtCatalog.ModelItem{id=id,prefab=prefab});
            }
            catalog.models=list.ToArray();EditorUtility.SetDirty(catalog);return catalog;
        }
        private static Material Mat(string name,Color color,float smooth=.2f,float metal=0,bool emission=false)
        {
            if(materials.TryGetValue(name,out var cached)&&cached)return cached;
            string path=Root+"/Materials/"+name+".mat";
            var mat=AssetDatabase.LoadAssetAtPath<Material>(path);
            if(!mat){mat=new Material(Shader.Find("Universal Render Pipeline/Lit"));AssetDatabase.CreateAsset(mat,path);}
            mat.SetColor("_BaseColor",color);mat.SetFloat("_Smoothness",smooth);mat.SetFloat("_Metallic",metal);
            if(emission){mat.EnableKeyword("_EMISSION");mat.SetColor("_EmissionColor",color*2);}
            materials[name]=mat;return mat;
        }
        private static Color Hex(string value){ColorUtility.TryParseHtmlString("#"+value,out var c);return c;}
        private static GameObject Box(Transform parent,string name,Vector3 pos,Vector3 scale,Material mat,bool collider=true)
        {
            var o=GameObject.CreatePrimitive(PrimitiveType.Cube);o.name=name;o.transform.SetParent(parent,false);
            o.transform.localPosition=pos;o.transform.localScale=scale;o.GetComponent<Renderer>().sharedMaterial=mat;
            if(!collider)UnityEngine.Object.DestroyImmediate(o.GetComponent<Collider>());return o;
        }
        private static SceneZeroController CreateElevator()
        {
            var root=new GameObject("Scene 0 | elevator and threshold");root.transform.position=new Vector3(-1,0,0);
            var r=root.transform;var intro=root.AddComponent<SceneZeroController>();
            var steel=Mat("Brushed steel",Hex("9CA9B1"),.28f,.75f);
            var dark=Mat("Graphite",Hex("20282D"),.16f,.2f);
            var floor=Mat("Elevator stone",Hex("545F69"),.32f);
            var wood=Mat("Threshold wood",Hex("271B1A"));
            var burgundy=Mat("Wine panel",Hex("431E2A"),.25f);
            var brass=Mat("Threshold brass",Hex("C7A16A"),.48f,.6f);
            var glow=Mat("Cool strip",Hex("D8ECFF"),.1f,0,true);
            Box(r,"Elevator floor",new Vector3(0,-.07f,-8),new Vector3(3.3f,.2f,3.15f),floor);
            Box(r,"Elevator rear",new Vector3(0,1.5f,-9.5f),new Vector3(3.3f,3.2f,.15f),steel);
            foreach(float side in new[]{-1f,1f})
            {
                Box(r,"Elevator side",new Vector3(side*1.6f,1.5f,-8),new Vector3(.14f,3.2f,3.15f),steel);
                Box(r,"Hand rail",new Vector3(side*1.46f,.98f,-8),new Vector3(.06f,.065f,2.45f),brass,false);
                Box(r,"Wall light",new Vector3(side*1.49f,2.7f,-8),new Vector3(.025f,.055f,2.4f),glow,false);
                Box(r,"Door frame",new Vector3(side*1.24f,1.5f,-6.6f),new Vector3(.38f,3.2f,.24f),dark);
                Box(r,"Hall side",new Vector3(side*1.48f,1.4f,-5.85f),new Vector3(.17f,2.8f,1.5f),burgundy);
                Box(r,"Hall brass line",new Vector3(side*1.375f,1.5f,-5.85f),new Vector3(.025f,.032f,1.5f),brass,false);
            }
            Box(r,"Elevator ceiling",new Vector3(0,3.03f,-8),new Vector3(3.3f,.1f,3.1f),dark);
            Box(r,"Ceiling lamp",new Vector3(0,2.96f,-8),new Vector3(1.7f,.025f,1.5f),glow,false);
            Box(r,"Door header",new Vector3(0,2.91f,-6.6f),new Vector3(2.15f,.3f,.24f),dark);
            intro.leftDoor=Box(r,"Sliding door left",new Vector3(-.52f,1.4f,-6.6f),new Vector3(1.03f,2.8f,.09f),steel).transform;
            intro.rightDoor=Box(r,"Sliding door right",new Vector3(.52f,1.4f,-6.6f),new Vector3(1.03f,2.8f,.09f),steel).transform;
            Box(r,"Door centre seam",new Vector3(0,1.4f,-6.659f),new Vector3(.012f,2.8f,.004f),dark,false).SetActive(false);
            Box(r,"Threshold floor",new Vector3(0,-.07f,-5.78f),new Vector3(3.2f,.2f,1.7f),wood);
            Box(r,"Threshold ceiling",new Vector3(0,2.9f,-5.78f),new Vector3(3.2f,.15f,1.7f),wood);
            Box(r,"Front boundary west",new Vector3(-3.38f,1,-5.18f),new Vector3(3.88f,2,.25f),wood);
            Box(r,"Front boundary east",new Vector3(4.12f,1,-5.18f),new Vector3(5.12f,2,.25f),wood);
            intro.floorDisplay=Sign(r,"Floor display","28",new Vector3(0,2.78f,-6.76f),.075f,Hex("DDEEFF"));
            Sign(r,"Destination","LA LA LAND",new Vector3(.0f,2.56f,-6.77f),.022f,Hex("9AAABA"));
            Sign(r,"Hall sign","L A   L A   L A N D",new Vector3(0,2.52f,-5.12f),.05f,Hex("E7BD8D"));
            var light=new GameObject("Elevator cold light",typeof(Light)).GetComponent<Light>();light.transform.SetParent(r,false);
            light.transform.localPosition=new Vector3(0,2.7f,-8);light.type=LightType.Point;light.range=4;light.intensity=2.0f;light.color=Hex("E4F0FF");
            var warm=new GameObject("Threshold warm light",typeof(Light)).GetComponent<Light>();warm.transform.SetParent(r,false);
            warm.transform.localPosition=new Vector3(0,2.4f,-5.1f);warm.type=LightType.Point;warm.range=4;warm.intensity=2;warm.color=Hex("FFC383");
            BuildPhone(intro,r,dark,brass);
            return intro;
        }
        private static TextMesh Sign(Transform parent,string name,string value,Vector3 position,float size,Color color)
        {
            var o=new GameObject(name,typeof(TextMesh));o.transform.SetParent(parent,false);o.transform.localPosition=position;
            o.transform.localRotation=Quaternion.identity;
            var t=o.GetComponent<TextMesh>();t.text=value;t.characterSize=size;t.fontSize=64;t.anchor=TextAnchor.MiddleCenter;t.color=color;return t;
        }
        private static void BuildPhone(SceneZeroController intro,Transform parent,Material dark,Material brass)
        {
            var rig=new GameObject("First person | phone and sleeves").transform;rig.SetParent(parent,false);rig.localPosition=new Vector3(0,1,-8);
            intro.phoneRig=rig;
            Box(rig,"Phone edge",Vector3.zero,new Vector3(.245f,.46f,.019f),brass,false);
            Box(rig,"Phone body",new Vector3(0,0,-.008f),new Vector3(.23f,.449f,.013f),dark,false);
            var sleeve=Mat("Player sleeve",Hex("192328"));var skin=Mat("Hands",Hex("CFAF99"),.23f);
            foreach(float side in new[]{-1f,1f})
            {
                var arm=Box(rig,"Sleeve",new Vector3(side*.17f,-.255f,.035f),new Vector3(.105f,.29f,.09f),sleeve,false);
                arm.transform.localRotation=Quaternion.Euler(0,0,side*-16);
                Box(rig,"Hand",new Vector3(side*.13f,-.075f,-.004f),new Vector3(.06f,.13f,.055f),skin,false);
                for(int j=0;j<3;j++)Box(rig,"Finger",new Vector3(side*.114f,-.047f-j*.027f,-.032f),new Vector3(.042f,.017f,.026f),skin,false);
            }
            var screen=new GameObject("Phone screen",typeof(RectTransform),typeof(Canvas),typeof(GraphicRaycaster),typeof(Button));
            screen.transform.SetParent(rig,false);screen.transform.localPosition=new Vector3(0,0,-.018f);
            screen.transform.localRotation=Quaternion.identity;screen.transform.localScale=Vector3.one*.00052f;
            var rect=screen.GetComponent<RectTransform>();rect.sizeDelta=new Vector2(420,800);
            intro.phoneScreen=screen.GetComponent<Canvas>();intro.phoneScreen.renderMode=RenderMode.WorldSpace;
            var bg=new GameObject("Screen glass",typeof(RectTransform),typeof(Image));bg.transform.SetParent(screen.transform,false);
            var bgRect=bg.GetComponent<RectTransform>();bgRect.anchorMin=Vector2.zero;bgRect.anchorMax=Vector2.one;bgRect.offsetMin=bgRect.offsetMax=Vector2.zero;
            bg.GetComponent<Image>().color=new Color(.042f,.063f,.084f);
            PhoneText(screen.transform,"22:30",new Vector2(0,333),44,new Color(.72f,.80f,.88f));
            PhoneText(screen.transform,"一条新消息",new Vector2(0,205),28,new Color(.55f,.65f,.73f));
            intro.phoneMessage=PhoneText(screen.transform,"今晚见。",new Vector2(0,66),48,Color.white);
            intro.phoneHint=PhoneText(screen.transform,"给你留了位置。",new Vector2(0,-36),30,new Color(.76f,.82f,.87f));
            intro.phoneSource=PhoneText(screen.transform,"预设文案",new Vector2(0,-232),22,new Color(.4f,.52f,.61f));
            PhoneText(screen.transform,"E  收起",new Vector2(0,-330),21,new Color(.4f,.52f,.61f));
            rig.gameObject.SetActive(false);
        }
        private static Text PhoneText(Transform parent,string value,Vector2 position,int size,Color color)
        {
            var o=new GameObject("Phone typography",typeof(RectTransform),typeof(Text));o.transform.SetParent(parent,false);
            var rect=o.GetComponent<RectTransform>();rect.sizeDelta=new Vector2(380,100);rect.anchoredPosition=position;
            var t=o.GetComponent<Text>();t.text=value;t.font=Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");t.fontSize=size;t.alignment=TextAnchor.MiddleCenter;t.color=color;t.raycastTarget=false;return t;
        }
        [Serializable] private class Nav { public float cell=.25f,minX=-8.5f,minZ=-10.5f;public int width=60,height=62;public int[] blocked;public Wall[] walls; }
        [Serializable] private class Wall { public float x,z,w,h; }
        private static void ExportNavigation(SceneZeroController intro)
        {
            var doors=new[]{intro.leftDoor.GetComponent<Collider>(),intro.rightDoor.GetComponent<Collider>()};
            foreach(var d in doors)d.enabled=false;Physics.SyncTransforms();
            var nav=new Nav();var blocked=new List<int>();
            for(int z=0;z<nav.height;z++)for(int x=0;x<nav.width;x++){
                float px=nav.minX+(x+.5f)*nav.cell,pz=nav.minZ+(z+.5f)*nav.cell;
                if(!Physics.Raycast(new Vector3(px,.14f,pz),Vector3.down,.5f)||Physics.CheckCapsule(new Vector3(px,.35f,pz),new Vector3(px,1.45f,pz),.27f))blocked.Add(z*nav.width+x);
            }
            nav.blocked=blocked.ToArray();nav.walls=UnityEngine.Object.FindObjectsOfType<Collider>().Where(c=>c.enabled&&c.bounds.size.y>1.5f&&!c.name.Contains("safety"))
                .Select(c=>new Wall{x=c.bounds.center.x,z=c.bounds.center.z,w=c.bounds.size.x,h=c.bounds.size.z}).ToArray();
            File.WriteAllText("Server/scenarios/navigation.json",JsonUtility.ToJson(nav,true));
            foreach(var d in doors)d.enabled=true;
        }
        [MenuItem("Last Call/Scene 0/2 - Build Windows verification")]
        public static void Build()
        {
            EditorSceneManager.OpenScene(LastCallSceneBuilder.ScenePath);
            if(!UnityEngine.Object.FindObjectOfType<SceneZeroController>())throw new Exception("Prepare Scene 0 first");
            const string destination="Builds/Scene0-Windows";
            Directory.CreateDirectory(destination);
            PlayerSettings.insecureHttpOption=InsecureHttpOption.AlwaysAllowed;
            var result=BuildPipeline.BuildPlayer(new BuildPlayerOptions{scenes=new[]{LastCallSceneBuilder.ScenePath},locationPathName=destination+"/LastCall.exe",target=BuildTarget.StandaloneWindows64,options=BuildOptions.None});
            if(result.summary.result!=BuildResult.Succeeded)throw new Exception("Scene 0 build failed");
            foreach(var folder in new[]{"dist","node_modules","scenarios"})Copy(Path.Combine("Server",folder),Path.Combine(destination,"Server",folder));
            foreach(var name in new[]{"package.json","package-lock.json","NODE-LICENSE.txt"})File.Copy("Server/"+name,destination+"/Server/"+name,true);
            File.Copy(@"D:\node.exe",destination+"/Server/node.exe",true);
            Debug.Log("SCENE0_BUILD_READY");
        }
        public static void PrepareAndBuild(){Prepare();Build();}
        private static void Copy(string source,string destination){
            Directory.CreateDirectory(destination);foreach(var f in Directory.GetFiles(source))File.Copy(f,Path.Combine(destination,Path.GetFileName(f)),true);
            foreach(var d in Directory.GetDirectories(source))Copy(d,Path.Combine(destination,Path.GetFileName(d)));
        }
    }
}
