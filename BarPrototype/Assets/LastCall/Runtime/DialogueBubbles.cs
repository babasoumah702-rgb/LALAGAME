using System.Collections;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;
using UnityEngine.Networking;
using UnityEngine.UI;

namespace LastCall
{
    [DefaultExecutionOrder(250)]
    public sealed class DialogueBubbles:MonoBehaviour
    {
        public LastCallGame Game;
        private sealed class Bubble {public EventDto item;public RectTransform rect;public Text text;public float age;public int page;public string[] pages;}
        private sealed class VoiceItem {public string path;public int interactions;public int seq;}
        private readonly List<Bubble> bubbles=new List<Bubble>();
        private RectTransform root;private Canvas canvas;private string session;private int cursor,epoch=-1,voiceCursor;
        private AudioSource voice;private readonly HashSet<string> voiced=new HashSet<string>();
        private readonly List<VoiceItem> voiceQueue=new List<VoiceItem>();
        private bool voicePlaying;
        public int VisibleCount=>bubbles.Count(b=>b.rect.gameObject.activeSelf);
        private void Start(){
            var go=new GameObject("Dialogue bubbles",typeof(Canvas),typeof(CanvasScaler));go.transform.SetParent(transform,false);
            canvas=go.GetComponent<Canvas>();canvas.renderMode=RenderMode.ScreenSpaceOverlay;canvas.sortingOrder=20;
            var scaler=go.GetComponent<CanvasScaler>();scaler.uiScaleMode=CanvasScaler.ScaleMode.ScaleWithScreenSize;scaler.referenceResolution=new Vector2(1280,720);scaler.matchWidthOrHeight=.5f;
            root=go.GetComponent<RectTransform>();
            var ao=new GameObject("Cast voice",typeof(AudioSource));ao.transform.SetParent(transform,false);
            voice=ao.GetComponent<AudioSource>();voice.spatialBlend=0f;voice.playOnAwake=false;voice.volume=.9f;
        }
        private void LateUpdate(){
            var state=Game.Client?.State;if(root==null||state==null)return;
            if(session!=state.sessionId||epoch!=Game.Client.PresentationEpoch){epoch=Game.Client.PresentationEpoch;session=state.sessionId;cursor=state.cursor;voiceCursor=state.cursor;voiced.Clear();voiceQueue.Clear();foreach(var b in bubbles)Destroy(b.rect.gameObject);bubbles.Clear();}
            foreach(var e in state.events.Where(e=>e.seq>cursor).OrderBy(e=>e.seq)){
                if(e.type!="speech"||e.privacy=="private"||e.level=="gesture")continue;
                Add(e);
            }
            cursor=System.Math.Max(cursor,state.cursor);
            bool hidden=NightPresentation.CinematicActive||Game.Interface.OverlayOpen||state.intro?.phase=="elevator";
            var placed=new List<Rect>();
            if(state.scene3!=null)placed.Add(new Rect(root.rect.width-484,116,460,126));
            foreach(var b in bubbles.ToArray()){
                if(!state.paused&&!hidden)b.age+=Time.unscaledDeltaTime;
                if(b.age>Mathf.Clamp(b.pages[b.page].Length*.12f,4,9)){
                    b.age=0;b.page++;if(b.page>=b.pages.Length){Destroy(b.rect.gameObject);bubbles.Remove(b);continue;}SetText(b);
                }
                b.rect.gameObject.SetActive(false);if(hidden)continue;
                Vector2 p;float w=b.rect.rect.width,h=b.rect.rect.height;
                if(b.item.actor=="USER")p=new Vector2(24,root.rect.height-318);
                else{
                    if(!Game.Avatars.TryGetValue(b.item.actor,out var a)||!a.gameObject.activeSelf)continue;
                    Vector3 head=a.HeadAnchor+Vector3.up*.12f;
                    var cam=Camera.main;var screen=cam.WorldToScreenPoint(head);
                    if(screen.z<=0||screen.x<0||screen.x>Screen.width||screen.y<0||screen.y>Screen.height)continue;
                    bool occluded=Physics.RaycastAll(cam.transform.position,(head-cam.transform.position).normalized,Vector3.Distance(head,cam.transform.position)-.15f)
                        .Any(hit=>!hit.collider.isTrigger&&hit.collider.GetComponentInParent<ActorAvatar>()==null);
                    if(occluded)continue;
                    p=new Vector2(screen.x/Screen.width*root.rect.width-w/2,(1-screen.y/Screen.height)*root.rect.height-h-14);
                }
                p.x=Mathf.Clamp(p.x,16,root.rect.width-w-16);if(b.item.actor!="USER")p.y=Mathf.Clamp(p.y,118,root.rect.height-290-h);
                var rect=new Rect(p,new Vector2(w,h));
                if(b.item.actor!="USER"&&placed.Any(r=>r.Overlaps(rect))){
                    bool found=false;
                    for(int level=0;level<3&&!found;level++)foreach(float x in new[]{p.x,p.x-w-12,p.x+w+12,16,root.rect.width-w-16}){
                        var candidate=new Rect(new Vector2(Mathf.Clamp(x,16,root.rect.width-w-16),Mathf.Max(118,p.y-level*(h+8))),new Vector2(w,h));
                        if(placed.Any(r=>r.Overlaps(candidate)))continue;rect=candidate;found=true;break;
                    }
                    if(!found)continue;p=rect.position;
                }
                if(placed.Any(r=>r.Overlaps(rect)))continue;
                placed.Add(rect);b.rect.anchoredPosition=new Vector2(p.x,-p.y);b.rect.gameObject.SetActive(true);
            }
            PlayVoices(state);
        }
        private void Add(EventDto item){
            foreach(var previous in bubbles.Where(b=>b.item.actor==item.actor).ToArray()){Destroy(previous.rect.gameObject);bubbles.Remove(previous);}
            if(bubbles.Count>=4){Destroy(bubbles[0].rect.gameObject);bubbles.RemoveAt(0);}
            var go=new GameObject("Bubble "+item.actor,typeof(RectTransform),typeof(Image));var r=go.GetComponent<RectTransform>();r.SetParent(root,false);r.anchorMin=r.anchorMax=r.pivot=new Vector2(0,1);
            go.GetComponent<Image>().color=new Color(.055f,.065f,.08f,.94f);go.GetComponent<Image>().raycastTarget=false;
            var label=new GameObject("Dialogue",typeof(RectTransform),typeof(Text));var t=label.GetComponent<Text>();t.transform.SetParent(r,false);
            t.rectTransform.anchorMin=Vector2.zero;t.rectTransform.anchorMax=Vector2.one;t.rectTransform.offsetMin=new Vector2(14,12);t.rectTransform.offsetMax=new Vector2(-14,-10);
            t.font=Game.Interface.SharedFont;t.fontSize=17;t.color=new Color(.96f,.94f,.89f);t.supportRichText=false;t.raycastTarget=false;t.alignment=TextAnchor.UpperLeft;t.horizontalOverflow=HorizontalWrapMode.Wrap;
            var parts=new List<string>();for(int i=0;i<item.text.Length;i+=54)parts.Add(item.text.Substring(i,System.Math.Min(54,item.text.Length-i)));
            if(parts.Count==0)parts.Add("");var bubble=new Bubble{item=item,rect=r,text=t,pages=parts.ToArray()};SetText(bubble);bubbles.Add(bubble);
        }
        private void SetText(Bubble b){
            b.text.text=b.item.name+" · "+LastCallInterface.GenerationLabel(b.item.generationSource)+"\n"+b.pages[b.page]+(b.pages.Length>1?"  "+(b.page+1)+"/"+b.pages.Length:"");
            float width=b.item.actor=="USER"?root.rect.width-48:330;
            b.rect.sizeDelta=new Vector2(width,160);Canvas.ForceUpdateCanvases();b.rect.sizeDelta=new Vector2(width,Mathf.Clamp(b.text.preferredHeight+25,64,166));
        }
        private void PlayVoices(StateDto state){
            var counts=new Dictionary<string,int>();
            foreach(var c in state.characters)if(!counts.ContainsKey(c.id))counts[c.id]=c.interactions;
            foreach(var e in state.events){
                if(e.seq<=voiceCursor||e.type!="speech"||e.actor=="USER"||string.IsNullOrEmpty(e.audio))continue;
                if(!voiced.Add(e.id))continue;
                int interactions=0;counts.TryGetValue(e.actor,out interactions);
                voiceQueue.Add(new VoiceItem{path=e.audio,interactions=interactions,seq=e.seq});
            }
            if(!voicePlaying&&voiceQueue.Count>0)StartCoroutine(VoicePump());
        }
        private IEnumerator VoicePump(){
            voicePlaying=true;
            while(voiceQueue.Count>0){
                var item=voiceQueue.OrderByDescending(v=>v.interactions).ThenBy(v=>v.seq).First();
                voiceQueue.Remove(item);
                yield return PlayClip(item.path);
            }
            voicePlaying=false;
        }
        private IEnumerator PlayClip(string path){
            var client=Game.Client;if(client==null)yield break;
            using(var req=UnityWebRequestMultimedia.GetAudioClip(client.BaseUrl+path,AudioType.WAV)){
                req.SetRequestHeader("Authorization","Bearer "+client.Token);
                yield return req.SendWebRequest();
                if(req.result==UnityWebRequest.Result.Success){
                    var clip=DownloadHandlerAudioClip.GetContent(req);
                    if(clip!=null){voice.Stop();voice.clip=clip;voice.Play();while(voice.isPlaying)yield return null;}
                }
            }
        }
    }
}
