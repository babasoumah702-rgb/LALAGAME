using System.Collections.Generic;
using UnityEngine;
namespace LastCall
{
    // Additive architecture, aligned to night-navigation.ts. The original bar and its art are kept.
    public sealed class NightStage:MonoBehaviour
    {
        public LastCallGame Game;
        private GameObject stage,door;
        private readonly List<Light> emergency=new List<Light>();
        private readonly Dictionary<Light,float> originalLights=new Dictionary<Light,float>();
        private readonly List<GameObject> opened=new List<GameObject>();
        private Material stone,wall,metal,grass,linen,glow;
        private bool built;
        public static string Area(Vector3 p){if(p.y>4&&p.z>=2.3f)return "rooftop";if(p.x>=6.8f&&p.z>=-5.85f&&p.z<2.5f)return "stairs";return p.z< -5.18f?"corridor":"bar";}
        private void Build()
        {
            built=true;stage=new GameObject("Full night | additive corridor stairs rooftop");
            stone=OtomeArt.Cloth(new Color(.16f,.17f,.19f),.2f);wall=OtomeArt.Cloth(new Color(.12f,.09f,.1f),.25f);
            metal=OtomeArt.Cloth(new Color(.1f,.12f,.14f),.6f,.55f);grass=OtomeArt.Cloth(new Color(.07f,.12f,.1f),.1f);
            linen=OtomeArt.Cloth(new Color(.66f,.62f,.56f),.1f);glow=OtomeArt.Flat(new Color(.8f,.69f,.4f));
            foreach(var light in FindObjectsOfType<Light>())originalLights[light]=light.intensity;
            foreach(var t in FindObjectsOfType<Transform>())if((t.name=="Hall side"||t.name=="Hall brass line")&&t.position.x>0&&t.position.z< -5){opened.Add(t.gameObject);t.gameObject.SetActive(false);}
            Box("Corridor floor",new Vector3(2.95f,-.1f,-5.9f),new Vector3(10.5f,.2f,1.45f),stone);
            Box("Corridor back wall",new Vector3(4.1f,1.45f,-6.62f),new Vector3(8.2f,2.9f,.16f),wall);
            Box("Corridor ceiling",new Vector3(3.75f,2.95f,-5.9f),new Vector3(8.9f,.1f,1.5f),wall);
            // End before the stair lane (x >= 6.8); the corridor must open onto the first riser.
            Box("Corridor bar wall",new Vector3(3.55f,1.7f,-5.14f),new Vector3(6.2f,3.4f,.12f),wall);
            door=Box("Side door | dampened sound",new Vector3(-1,1.15f,-5.2f),new Vector3(1.25f,2.3f,.07f),wall,false);
            for(int i=0;i<4;i++){Box("Corridor sconce",new Vector3(.7f+i*1.8f,2.2f,-6.51f),new Vector3(.17f,.06f,.05f),glow,false);Lamp(new Vector3(.7f+i*1.8f,2.15f,-5.8f),new Color(1,.72f,.42f),.75f,3);}
            // 28 visible risers over a continuous walkable collision surface, no teleport.
            for(int i=0;i<28;i++){
                float z=-5.7f+(i+.5f)*8.2f/28,h=(i+1)*.15f;
                Box("Stair "+(i+1),new Vector3(7.45f,h/2,z),new Vector3(1.3f,h,8.2f/28+.008f),stone,false);
                Box("Step edge light",new Vector3(8.02f,h+.015f,z),new Vector3(.025f,.025f,.14f),glow,false);
                if(i%4==0)for(int side=-1;side<=1;side+=2)Box("Stair rail post",new Vector3(7.45f+side*.69f,h+.5f,z),new Vector3(.04f,1,.04f),metal);
            }
            var ramp=new GameObject("Continuous stair collision",typeof(MeshCollider));ramp.transform.SetParent(stage.transform,false);var surface=new Mesh();surface.vertices=new[]{new Vector3(6.8f,0,-5.7f),new Vector3(8.1f,0,-5.7f),new Vector3(6.8f,4.2f,2.5f),new Vector3(8.1f,4.2f,2.5f)};surface.triangles=new[]{0,2,1,1,2,3};surface.RecalculateNormals();surface.RecalculateBounds();ramp.GetComponent<MeshCollider>().sharedMesh=surface;
            for(int side=-1;side<=1;side+=2){var rail=Box("Sloping handrail",new Vector3(7.45f+side*.69f,3.12f,-1.6f),new Vector3(.045f,.045f,9.25f),metal);rail.transform.rotation=Quaternion.Euler(-27.12f,0,0);}
            Box("Roof slab",new Vector3(1.3f,4.08f,5.8f),new Vector3(13.7f,.24f,6.6f),stone);
            Box("Roof grass",new Vector3(.4f,4.215f,6.2f),new Vector3(9,.025f,4.6f),grass,false);
            foreach(float x in new[]{-5.6f,8.2f})Box("Roof parapet",new Vector3(x,4.72f,5.8f),new Vector3(.16f,1.05f,6.7f),wall);
            Box("North parapet",new Vector3(1.3f,4.72f,9.13f),new Vector3(13.9f,1.05f,.16f),wall);
            Box("South parapet",new Vector3(.48f,4.72f,2.42f),new Vector3(12.1f,1.05f,.16f),wall);
            Box("Roof door jamb",new Vector3(6.73f,5.35f,2.5f),new Vector3(.12f,2.3f,.18f),metal);
            Box("Roof door jamb",new Vector3(8.13f,5.35f,2.5f),new Vector3(.12f,2.3f,.18f),metal);
            Box("Roof door lintel",new Vector3(7.43f,6.5f,2.5f),new Vector3(1.5f,.12f,.18f),metal);
            var openDoor=Box("Roof door open",new Vector3(8.07f,5.35f,3.08f),new Vector3(.055f,2.2f,1.1f),metal);
            for(int i=0;i<6;i++){var cushion=Box("Linen cushion",new Vector3(-2.1f+i*1.25f,4.31f,5.3f+(i%2)*.85f),new Vector3(.82f,.2f,.66f),linen,false);cushion.transform.rotation=Quaternion.Euler(0,i*17,0);}
            var rand=new System.Random(314159);
            for(int i=0;i<65;i++){
                float x=-4.9f+(float)rand.NextDouble()*10,z=7.9f+(float)rand.NextDouble()*.85f;
                Box("Flower stalk",new Vector3(x,4.39f,z),new Vector3(.014f,.3f,.014f),grass,false);
                var bloom=GameObject.CreatePrimitive(PrimitiveType.Sphere);bloom.name="Pale flower cluster";bloom.transform.SetParent(stage.transform,false);bloom.transform.position=new Vector3(x,4.57f,z);bloom.transform.localScale=new Vector3(.10f,.07f,.10f);Release(bloom.GetComponent<Collider>());bloom.GetComponent<Renderer>().sharedMaterial=linen;
            }
            for(int i=0;i<17;i++){float x=-5+i*.75f,y=6.6f-Mathf.Sin(i/16f*Mathf.PI)*.6f;Box("Weak string light",new Vector3(x,y,7.9f),new Vector3(.035f,.07f,.035f),glow,false);if(i%5==0)Lamp(new Vector3(x,y,7.9f),new Color(1,.75f,.45f),.65f,4);}
            for(int i=0;i<24;i++){
                float x=-24+i*2.2f,h=2+(float)rand.NextDouble()*10,z=18+(float)rand.NextDouble()*10;
                Box("City silhouette",new Vector3(x,h/2-1,z),new Vector3(1.5f,h,1.8f),metal,false);
                for(float yy=1;yy<h;yy+=1.25f)if(rand.NextDouble()>.3)Box("Distant city window",new Vector3(x,yy-1,z-.92f),new Vector3(.45f,.25f,.02f),glow,false);
            }
            Lamp(new Vector3(7.45f,2.5f,-5.4f),new Color(.4f,.75f,.67f),1,4);
            Lamp(new Vector3(7.45f,5.9f,2.8f),new Color(.4f,.65f,.8f),1.3f,5);
            Lamp(new Vector3(.5f,7.2f,5.2f),new Color(.6f,.7f,.85f),2.1f,12);
            Lamp(new Vector3(4.5f,6.4f,6.7f),new Color(1,.78f,.55f),1.5f,8);
            for(int i=0;i<55;i++)Box("Faint star",new Vector3(-24+(float)rand.NextDouble()*48,17+(float)rand.NextDouble()*12,-15+(float)rand.NextDouble()*45),Vector3.one*(.025f+(float)rand.NextDouble()*.025f),glow,false);
        }
        private GameObject Box(string name,Vector3 position,Vector3 scale,Material mat,bool solid=true){var o=GameObject.CreatePrimitive(PrimitiveType.Cube);o.name=name;o.transform.SetParent(stage.transform,false);o.transform.position=position;o.transform.localScale=scale;o.GetComponent<Renderer>().sharedMaterial=mat;if(!solid)Release(o.GetComponent<Collider>());return o;}
        private static void Release(Object value){if(Application.isPlaying)Destroy(value);else DestroyImmediate(value);}
        private void Lamp(Vector3 p,Color c,float intensity,float range){var l=new GameObject("Night emergency / garden light",typeof(Light)).GetComponent<Light>();l.transform.SetParent(stage.transform,false);l.transform.position=p;l.color=c;l.intensity=intensity;l.range=range;emergency.Add(l);}
        private void Update(){var s=Game.Client?.State;if(s?.story==null){if(stage)stage.SetActive(false);return;}if(!built)Build();stage.SetActive(true);
            bool cut=s.late!=null&&s.late.powerState!="normal";
            foreach(var pair in originalLights)if(pair.Key)pair.Key.intensity=pair.Value*(cut?.04f:1);
            if(door){bool near=false;foreach(var a in Game.Avatars.Values)if(a.gameObject.activeSelf&&Vector3.Distance(a.transform.position,new Vector3(-1,0,-5.2f))<1.5f)near=true;door.transform.localScale=new Vector3(near||s.late?.doorOpen==true?.05f:1.25f,2.3f,.07f);}
        }
        private void OnDestroy(){foreach(var pair in originalLights)if(pair.Key)pair.Key.intensity=pair.Value;foreach(var go in opened)if(go)go.SetActive(true);if(stage)Release(stage);}
    }
}
