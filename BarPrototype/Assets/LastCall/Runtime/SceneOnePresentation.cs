using UnityEngine;

namespace LastCall
{
    public sealed class SceneOneObject:MonoBehaviour {public string objectId;}
    [DefaultExecutionOrder(200)]
    public sealed class SceneOnePresentation:MonoBehaviour
    {
        public LastCallGame Game;
        private Transform cup,chair,phone,arrivalPhone;private string session;
        private readonly Vector3 tableCup=new Vector3(1.73f,.84f,-1.9f);
        private void Start(){
            cup=GameObject.Find("third drink")?.transform;chair=GameObject.Find("empty chair")?.transform;
            if(cup){var c=cup.gameObject.AddComponent<BoxCollider>();c.isTrigger=true;cup.gameObject.AddComponent<SceneOneObject>().objectId="third_drink";}
            if(chair){var c=chair.gameObject.AddComponent<BoxCollider>();c.isTrigger=true;chair.gameObject.AddComponent<SceneOneObject>().objectId="reserved_seat";}
            var go=GameObject.CreatePrimitive(PrimitiveType.Cube);go.name="A phone | screen hidden";phone=go.transform;phone.localScale=new Vector3(.07f,.012f,.13f);
            Destroy(go.GetComponent<Collider>());go.GetComponent<Renderer>().material=OtomeArt.Flat(new Color(.04f,.045f,.055f));go.SetActive(false);
            var arrivalGo=Instantiate(go);arrivalGo.name="D phone | screen hidden";arrivalPhone=arrivalGo.transform;
        }
        private void Update(){
            var state=Game.Client?.State;var s=state?.scene1;
            if(s==null)return;
            if(session!=state.sessionId)session=state.sessionId;
            bool intro=state.intro?.phase=="elevator";
            if(cup){
                cup.gameObject.SetActive(!intro&&(s.phase=="drink_delivery"||s.drinkPlaced));
                if(s.phase=="drink_delivery"&&Game.Avatars.TryGetValue("BARTENDER",out var bartender))cup.position=bartender.HandAnchor?bartender.HandAnchor.position:bartender.transform.position+Vector3.up*1.12f+bartender.GetComponent<BarPrototype.PlayerMotor>().VisualRoot.forward*.3f;
                else cup.position=tableCup;
            }
            if(Game.Avatars.TryGetValue("A",out var a)&&phone){
                if(!a.GetComponent<CastActionAdapter>()&&!a.GetComponent<SceneOneSeatedPose>())a.gameObject.AddComponent<SceneOneSeatedPose>();
                bool reacting=s.phoneAt>=0&&state.elapsed-s.phoneAt<3&&!intro;phone.gameObject.SetActive(reacting);
                phone.position=a.transform.position+Vector3.up*(a.GetComponent<SceneOneSeatedPose>()?.IsSeated==true?.8f:1.08f)+a.GetComponent<BarPrototype.PlayerMotor>().VisualRoot.forward*.23f;
                if(a.HandAnchor)phone.position=a.HandAnchor.position;
                phone.rotation=Quaternion.Euler(0,a.State.yaw,0);
            }
            if(arrivalPhone){
                bool showing=!intro&&s.arrivalAt>=0&&state.elapsed-s.arrivalAt<8;arrivalPhone.gameObject.SetActive(showing);
                if(showing&&Game.Avatars.TryGetValue("D",out var d)){var v=d.GetComponent<BarPrototype.PlayerMotor>().VisualRoot;arrivalPhone.position=d.transform.position+Vector3.up*1.02f+v.forward*.24f+v.right*.15f;arrivalPhone.rotation=v.rotation;if(d.HandAnchor)arrivalPhone.position=d.HandAnchor.position;}
            }
            if(state.scene2!=null)return;
        }
        private void OnDestroy(){if(phone)Destroy(phone.gameObject);if(arrivalPhone)Destroy(arrivalPhone.gameObject);}
    }
}
