using System;
using System.Linq;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using UnityEngine.InputSystem;
using UnityEngine.InputSystem.UI;

namespace LastCall
{
    public sealed partial class LastCallInterface : MonoBehaviour
    {
        public LastCallGame Game;
        public bool Blocking => entryVisible || pauseVisible || notesVisible || introInputVisible || (expression && expression.isFocused);
        public string FocusId => selected;
        public bool Talking => expression && expression.isFocused;
        private RectTransform root, entryPanel, pausePanel, notesPanel, rightPanel;
        private Text clockText, modeText, feedText, targetText, cardText, toastText, roleDetail;
        private InputField expression;
        private Button sendButton;
        private Button[] cardButtons;
        private bool entryVisible=true,pauseVisible,notesVisible,builtEntry,editing,refresh=true,online=true;
        private string selected="B",cardId="approach",viewSession="",lastActors="",sceneMode="";
        private int roleIndex=2,intentIndex,styleIndex,lastEvent=-1;
        private float toastUntil;
        private Vector2 size;
        private string entryStatus;
        private LocalServiceClient Client=>Game.Client;
        private float Width=>root.rect.width;
        private float Height=>root.rect.height;
        private void Start()
        {
            SharedFont=Font.CreateDynamicFontFromOSFont(new[]{"Microsoft YaHei","Arial"},24);
            var backdrop=new GameObject("Last Call | background",typeof(Camera)).GetComponent<Camera>();
            backdrop.transform.SetParent(transform,false);
            backdrop.clearFlags=CameraClearFlags.SolidColor;
            backdrop.backgroundColor=new Color(.07f,.07f,.09f);
            backdrop.cullingMask=0;
            backdrop.depth=Camera.main.depth-10;
            var canvas=new GameObject("Last Call | Canvas",typeof(Canvas),typeof(CanvasScaler),typeof(GraphicRaycaster));
            canvas.transform.SetParent(transform,false);
            canvas.GetComponent<Canvas>().renderMode=RenderMode.ScreenSpaceOverlay;
            var scaler=canvas.GetComponent<CanvasScaler>();
            scaler.uiScaleMode=CanvasScaler.ScaleMode.ScaleWithScreenSize;
            scaler.referenceResolution=new Vector2(1280,720);scaler.matchWidthOrHeight=.5f;
            root=canvas.GetComponent<RectTransform>();canvasRoot=root;
            if(!FindObjectOfType<EventSystem>())
            {
                var events=new GameObject("Last Call | Input",typeof(EventSystem),typeof(InputSystemUIInputModule));
                events.GetComponent<InputSystemUIInputModule>().AssignDefaultActions();
            }
            Client.Changed+=()=>refresh=true;
            Client.Error+=Toast;
            Client.Acknowledged+=CardAcknowledged;
            Client.Rejected+=CardRejected;
            Canvas.ForceUpdateCanvases();
            BuildEntry();
        }
        private void Update()
        {
            if(root)root.GetComponent<Canvas>().enabled=!NightPresentation.CinematicActive;
            if(!root)return;
            if(entryVisible&&!Client.Ready&&entryStatus!=Client.Status){entryStatus=Client.Status;BuildEntry();}
            if(Client.Ready&&!builtEntry&&Client.State==null){BuildEntry();builtEntry=true;}
            if(Client.State!=null&&Client.State.sessionId!=viewSession)
            {
                viewSession=Client.State.sessionId;entryVisible=false;pauseVisible=Client.State.paused;
                queuedCard=sentCard=null;approachId=null;partyStarting=false;
                notesVisible=false;lastEvent=-1;BuildWorld();
                if(pauseVisible)ShowPause();
            }
            if(Client.State!=null&&!entryVisible)
            {
                if(Client.State.intro?.phase=="elevator"){UpdateIntroUI();return;}
                if(introUIVisible){introUIVisible=false;BuildWorld();}
                // Scene chapters hand off to each other without the actor list changing, so a refresh
                // would never rebuild the frame. Detect the chapter boundary and rebuild once.
                string mode=CurrentSceneMode();
                if(mode!=sceneMode&&!cardsExpanded){sceneMode=mode;BuildWorld();}
                if(Keyboard.current!=null&&Keyboard.current.escapeKey.wasPressedThisFrame)
                {
                    if(expression&&expression.isFocused)EventSystem.current.SetSelectedGameObject(null);
                    else if(notesVisible)CloseNotes();
                    else Pause(!pauseVisible);
                }
                bool isEditing=expression&&expression.isFocused&&!FullNightVerification.Running&&!SceneOneVerification.Running&&!SceneTwoThreeVerification.Running;
                if(editing!=isEditing)
                {
                    editing=isEditing;
                    Client.Send(new CommandDto{type="pause",paused=editing||pauseVisible||notesVisible});
                }
                if(Client.State.status=="ended"&&!notesVisible&&!NightPresentation.CinematicActive){ShowReflection();notesVisible=true;}
                TickCardFlow();
                if(refresh){RefreshWorld();refresh=false;}
            }
            if(toastText&&Time.unscaledTime>toastUntil)toastText.text="";
            var current=new Vector2(Width,Height);
            if(size!=current&&Client.State!=null&&!entryVisible&&!pauseVisible&&!notesVisible)
            {size=current;BuildWorld();refresh=true;}
        }
        private string CurrentSceneMode()
        {
            var state=Client?.State;
            if(state==null)return "";
            if(state.intro?.phase=="elevator")return "intro";
            if(state.late!=null)return "night"+state.late.chapter;
            if(state.scene3!=null)return "scene3";
            if(state.scene2!=null)return "scene2";
            if(state.scene1!=null)return "scene1";
            return "world";
        }
        public void Select(string id)
        {
            if(selected!=id)CancelQueuedCard();
            selected=id;
            RefreshWorld();
        }
        public void Pause(bool value)
        {
            if(Client?.State==null)return;
            pauseVisible=value;
            Client.Send(new CommandDto{type="pause",paused=value});
            if(value)ShowPause();else if(pausePanel)Destroy(pausePanel.gameObject);
            EventSystem.current?.SetSelectedGameObject(null);
        }
        private void Toast(string text)
        {
            if(text.Contains("closer"))text="请先点击「走近」，再开始对话。";
            toastUntil=Time.unscaledTime+6;
            if(toastText)toastText.text=text;
        }
        private void OnDestroy()
        {
            if(Client)Client.Error-=Toast;
            if(Client){Client.Acknowledged-=CardAcknowledged;Client.Rejected-=CardRejected;}
            if(SharedFont)Destroy(SharedFont);
        }
    }
}
