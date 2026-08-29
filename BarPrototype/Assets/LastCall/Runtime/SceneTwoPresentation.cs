using UnityEngine;

namespace LastCall
{
    // Scene 2 and Scene 3 presentation. Scene 2 shows time passing through the room itself: the glass
    // empties and coasters stack up as guests leave. Scene 3 then goes quieter still.
    [DefaultExecutionOrder(210)]
    public sealed class SceneTwoPresentation:MonoBehaviour
    {
        public LastCallGame Game;
        private Transform glass,deck;
        private readonly Transform[] coasters=new Transform[4];
        private string session;
        private float glassBase=1;
        private readonly Vector3 table=new Vector3(1.65f,0,-1.8f);
        private void Start()
        {
            glass=GameObject.Find("second glass")?.transform;
            if(glass)glassBase=glass.localScale.y;
            // The deck is a physical object on the table, not a UI card: the player can walk up to it.
            var go=GameObject.CreatePrimitive(PrimitiveType.Cube);
            go.name="La La Land Social Tarot | deck";
            deck=go.transform;deck.SetParent(transform,false);
            deck.localScale=new Vector3(.09f,.022f,.14f);
            deck.position=table+new Vector3(0,.79f,.02f);
            Destroy(go.GetComponent<Collider>());
            go.GetComponent<Renderer>().material=OtomeArt.Flat(new Color(.10f,.09f,.13f));
            var marker=go.AddComponent<BoxCollider>();marker.isTrigger=true;
            go.AddComponent<SceneOneObject>().objectId="tarot_deck";
            go.SetActive(false);
            for(int i=0;i<coasters.Length;i++)
            {
                var pad=GameObject.CreatePrimitive(PrimitiveType.Cylinder);
                pad.name="coaster "+(i+1);
                Destroy(pad.GetComponent<Collider>());
                pad.transform.SetParent(transform,false);
                pad.transform.localScale=new Vector3(.11f,.004f,.11f);
                pad.transform.position=table+new Vector3(-.34f+i*.055f,.765f+i*.008f,.26f);
                pad.GetComponent<Renderer>().material=OtomeArt.Flat(new Color(.22f,.19f,.17f));
                pad.SetActive(false);
                coasters[i]=pad.transform;
            }
        }
        private void Update()
        {
            var state=Game.Client?.State;
            var two=state?.scene2;var three=state?.scene3;
            if(state?.late!=null||two==null&&three==null){Quiet();return;}
            if(session!=state.sessionId)session=state.sessionId;
            // The glass drains and the coasters pile up: the montage is read off the table.
            if(glass&&two!=null)
            {
                float level=Mathf.Clamp(two.drinkLevel,.12f,1f);
                glass.localScale=new Vector3(glass.localScale.x,glassBase*level,glass.localScale.z);
            }
            int pads=three!=null?coasters.Length:(two?.coasters??0);
            for(int i=0;i<coasters.Length;i++)if(coasters[i])coasters[i].gameObject.SetActive(i<pads);
            bool deckOut=three!=null||two?.deckPlaced==true;
            if(deck)deck.gameObject.SetActive(deckOut);
        }
        private void Quiet()
        {
            if(deck)deck.gameObject.SetActive(false);
            foreach(var pad in coasters)if(pad)pad.gameObject.SetActive(false);
        }
        private void OnDestroy()
        {
            if(deck)Destroy(deck.gameObject);
            foreach(var pad in coasters)if(pad)Destroy(pad.gameObject);
        }
    }
}
