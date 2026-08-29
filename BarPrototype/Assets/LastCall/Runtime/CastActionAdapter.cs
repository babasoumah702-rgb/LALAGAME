using System.Collections.Generic;
using BarPrototype;
using UnityEngine;
namespace LastCall
{
    // Private pose meshes for the existing static cast. Originals and materials remain untouched.
    // Hip/knee and shoulder pivots bend the silhouette; hand/head anchors share the same mapping.
    [DefaultExecutionOrder(90)]
    public sealed class CastActionAdapter:MonoBehaviour
    {
        private sealed class Part {public MeshFilter filter;public Mesh original,copy;public Vector3[] rest,work;}
        private readonly List<Part> parts=new List<Part>();
        private ActorAvatar actor;private HumanoidCastAnimator humanoid;private Transform visual;private float height,sit,offer,lie,lean;
        private GameObject chair;private Vector3 first;
        public Transform RightHand {get;private set;}
        public Transform Head {get;private set;}
        public bool IsSeated=>humanoid?humanoid.IsSeated:sit>.5f;
        private void Start(){actor=GetComponent<ActorAvatar>();visual=GetComponent<PlayerMotor>().VisualRoot;height=CastModel.HeightOf(actor.ActorId);first=transform.position;
            humanoid=GetComponent<HumanoidCastAnimator>();var imported=visual.Find("Cast mesh");
            if(humanoid&&humanoid.Head&&humanoid.RightHand){Head=humanoid.Head;RightHand=humanoid.RightHand;}
            else{
                foreach(var f in visual.GetComponentsInChildren<MeshFilter>())if(imported&&f.sharedMesh&&f.sharedMesh.isReadable&&f.transform.IsChildOf(imported)){
                    var original=f.sharedMesh;var copy=Instantiate(original);copy.name=original.name+" | private action adaptation";copy.MarkDynamic();var v=original.vertices;
                    for(int i=0;i<v.Length;i++)v[i]=visual.InverseTransformPoint(f.transform.TransformPoint(v[i]));parts.Add(new Part{filter=f,original=original,copy=copy,rest=v,work=new Vector3[v.Length]});f.sharedMesh=copy;
                }
                RightHand=new GameObject("Right hand | prop anchor").transform;RightHand.SetParent(visual,false);Head=new GameObject("Head | speech anchor").transform;Head.SetParent(visual,false);
            }
            chair=new GameObject("Pose chair");chair.transform.SetParent(transform,false);ChairPart(new Vector3(0,.42f,0),new Vector3(.48f,.08f,.45f));ChairPart(new Vector3(0,.72f,-.22f),new Vector3(.48f,.6f,.055f));foreach(float x in new[]{-.18f,.18f})foreach(float z in new[]{-.17f,.17f})ChairPart(new Vector3(x,.2f,z),new Vector3(.035f,.4f,.035f));
        }
        private void ChairPart(Vector3 pos,Vector3 scale){var o=GameObject.CreatePrimitive(PrimitiveType.Cube);o.transform.SetParent(chair.transform,false);o.transform.localPosition=pos;o.transform.localScale=scale;Destroy(o.GetComponent<Collider>());o.GetComponent<Renderer>().sharedMaterial=OtomeArt.Cloth(new Color(.16f,.1f,.11f),.2f);}
        private Vector3 Deform(Vector3 p){
            float hip=height*.52f,knee=height*.265f,drop=hip-.46f;
            Vector3 seated=p;
            if(p.y>=hip)seated.y-=drop;
            else if(p.y>=knee){seated.y=.46f;seated.z+=hip-p.y;}
            else {seated.y=p.y+( .46f-knee);seated.z+=hip-knee;}
            var q=Vector3.Lerp(p,seated,sit);
            float arm=Mathf.SmoothStep(0,1,Mathf.InverseLerp(height*.10f,height*.19f,Mathf.Abs(p.x)))*Mathf.Clamp01((height*.84f-p.y)/(height*.08f));
            if(p.x>0&&p.y>height*.36f){var pivot=new Vector3(height*.18f,height*.76f-drop*sit,0);q=Vector3.Lerp(q,pivot+Quaternion.Euler(-68,0,-10)*(q-pivot),arm*offer);}
            q=Quaternion.Euler(-lean*9,0,0)*(q-new Vector3(0,.08f,0))+new Vector3(0,.08f,0);
            if(lie>0){var lying=Quaternion.Euler(90,0,0)*p+new Vector3(0,.25f,-height*.45f);q=Vector3.Lerp(q,lying,lie);}
            return q;
        }
        private void LateUpdate(){if(actor?.State==null||!visual||!RightHand||!Head||!chair)return;var s=actor.State;var world=FindObjectOfType<LastCallGame>().Client.State;
            bool aSeat=actor.ActorId=="A"&&world?.story?.chapter==1&&(s.route==null||s.route.Length==0)&&Vector3.Distance(first,transform.position)<.4f;
            float ts=aSeat||s.posture=="sit"?1:0,tl=s.posture=="lie"?1:0;
            if(humanoid){sit=Mathf.MoveTowards(sit,ts,Time.deltaTime*1.7f);chair.SetActive(humanoid.IsSeated&&s.area!="rooftop"&&!actor.IsPlayer);chair.transform.rotation=visual.rotation;return;}
            float ago=(world?.elapsed??0)-s.gestureAt;float to=(s.gesture=="offer"||s.gesture=="flip"||s.gesture=="drink")&&ago>=0&&ago<3?Mathf.Sin(Mathf.Clamp01(ago/3)*Mathf.PI):0;
            if(actor.ActorId=="BARTENDER"&&world?.scene1?.phase=="drink_delivery")to=.8f;
            if(actor.ActorId=="A"&&world?.scene1!=null&&world.elapsed-world.scene1.phoneAt<3&&world.scene1.phoneAt>=0)to=.8f;
            float tn=s.posture=="lean"?1:0;float old=sit+lie+offer+lean;
            sit=Mathf.MoveTowards(sit,ts,Time.deltaTime*1.7f);lie=Mathf.MoveTowards(lie,tl,Time.deltaTime*1.4f);offer=Mathf.MoveTowards(offer,to,Time.deltaTime*3);lean=Mathf.MoveTowards(lean,tn,Time.deltaTime*2);
            bool changed=Mathf.Abs(old-sit-lie-offer-lean)>.00001f;
            if(changed)foreach(var part in parts){for(int i=0;i<part.rest.Length;i++)part.work[i]=part.filter.transform.InverseTransformPoint(visual.TransformPoint(Deform(part.rest[i])));part.copy.vertices=part.work;part.copy.RecalculateBounds();part.copy.RecalculateNormals();}
            if(s.gesture=="dance"&&ago<6&&ago>=0)visual.localRotation=Quaternion.Euler(0,s.yaw+Mathf.Sin(ago*1.8f)*7,Mathf.Sin(ago*1.8f)*2);
            RightHand.localPosition=Deform(new Vector3(height*.19f,height*.43f,.045f));Head.localPosition=Deform(new Vector3(0,height,0));
            RightHand.localRotation=Quaternion.Euler(-offer*65,0,0);
            chair.SetActive(sit>.2f&&s.area!="rooftop"&&!actor.IsPlayer);chair.transform.rotation=visual.rotation;
        }
        private void OnDestroy(){foreach(var p in parts){if(p.filter)p.filter.sharedMesh=p.original;if(p.copy)Destroy(p.copy);}}
    }
}
