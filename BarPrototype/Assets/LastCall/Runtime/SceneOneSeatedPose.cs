using UnityEngine;
using BarPrototype;

namespace LastCall
{
    // The imported cast uses static meshes, so deform a private mesh instance instead of pretending it has a skeleton.
    public sealed class SceneOneSeatedPose:MonoBehaviour
    {
        private ActorAvatar actor;private MeshFilter filter;private Mesh original,seated;private Vector3 anchor;
        public bool IsSeated {get;private set;}
        private GameObject chair;
        private void Start(){
            actor=GetComponent<ActorAvatar>();anchor=transform.position;
            var visual=GetComponent<PlayerMotor>().VisualRoot;filter=visual.Find("Cast mesh")?.GetComponentInChildren<MeshFilter>();
            if(!filter||!filter.sharedMesh||!filter.sharedMesh.isReadable)return;
            original=filter.sharedMesh;seated=Instantiate(original);seated.name="Scene1 seated mesh instance";
            var vertices=seated.vertices;
            for(int i=0;i<vertices.Length;i++){
                var p=visual.InverseTransformPoint(filter.transform.TransformPoint(vertices[i]));
                float y=p.y;
                if(y>.85f)p.y-=.4f;
                else if(y>.43f){p.y=.43f+(y-.43f)*.048f;p.z+=.32f*(.85f-y)/.42f;}
                else p.z+=.32f*Mathf.Clamp01(y/.43f);
                vertices[i]=filter.transform.InverseTransformPoint(visual.TransformPoint(p));
            }
            seated.vertices=vertices;seated.RecalculateBounds();seated.RecalculateNormals();
            chair=new GameObject("A wall-side chair");chair.transform.position=anchor;chair.transform.rotation=visual.rotation;
            Part("seat",new Vector3(0,.43f,0),new Vector3(.5f,.1f,.45f));Part("back",new Vector3(0,.71f,-.2f),new Vector3(.5f,.57f,.08f));
            foreach(float x in new[]{-.19f,.19f})foreach(float z in new[]{-.16f,.16f})Part("leg",new Vector3(x,.2f,z),new Vector3(.045f,.4f,.045f));
        }
        private void Part(string name,Vector3 position,Vector3 size){var part=GameObject.CreatePrimitive(PrimitiveType.Cube);part.name=name;part.transform.SetParent(chair.transform,false);part.transform.localPosition=position;part.transform.localScale=size;Destroy(part.GetComponent<Collider>());part.GetComponent<Renderer>().material=OtomeArt.Flat(new Color(.16f,.07f,.065f));}
        private void LateUpdate(){if(!seated||actor.State==null)return;IsSeated=actor.State.route.Length==0&&Vector3.Distance(transform.position,anchor)<.4f;filter.sharedMesh=IsSeated?seated:original;}
        private void OnDestroy(){if(filter&&original)filter.sharedMesh=original;if(seated)Destroy(seated);if(chair)Destroy(chair);}
    }
}
