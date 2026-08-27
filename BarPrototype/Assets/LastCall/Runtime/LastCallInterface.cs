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
        public bool Blocking => entryVisible || pauseVisible || notesVisible || (expression && expression.isFocused);
        private RectTransform root, entryPanel, pausePanel, notesPanel, rightPanel;
        private Text clockText, modeText, feedText, targetText, cardText, toastText, roleDetail;
        private InputField expression;
        private Button sendButton;
        private Button[] cardButtons;
        private bool entryVisible=true,pauseVisible,notesVisible,builtEntry,editing,refresh=true,online=true;
        private string selected="B",cardId="approach",viewSession="",lastActors="";
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
            backdrop.backgroundColor=new Color(.04f,.065f,.07f);
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
            Canvas.ForceUpdateCanvases();
            BuildEntry();
        }
        private void Update()
        {
            if(!root)return;
            if(entryVisible&&!Client.Ready&&entryStatus!=Client.Status){entryStatus=Client.Status;BuildEntry();}
            if(Client.Ready&&!builtEntry&&Client.State==null){BuildEntry();builtEntry=true;}
            if(Client.State!=null&&Client.State.sessionId!=viewSession)
            {
                viewSession=Client.State.sessionId;entryVisible=false;pauseVisible=Client.State.paused;
                notesVisible=false;lastEvent=-1;BuildWorld();
                if(pauseVisible)ShowPause();
            }
            if(Client.State!=null&&!entryVisible)
            {
                if(Keyboard.current!=null&&Keyboard.current.escapeKey.wasPressedThisFrame)
                {
                    if(expression&&expression.isFocused)EventSystem.current.SetSelectedGameObject(null);
                    else if(notesVisible)CloseNotes();
                    else Pause(!pauseVisible);
                }
                bool isEditing=expression&&expression.isFocused;
                if(editing!=isEditing)
                {
                    editing=isEditing;
                    Client.Send(new CommandDto{type="pause",paused=editing||pauseVisible||notesVisible});
                }
                if(Client.State.status=="ended"&&!notesVisible){ShowReflection();notesVisible=true;}
                if(refresh){RefreshWorld();refresh=false;}
            }
            if(toastText&&Time.unscaledTime>toastUntil)toastText.text="";
            var current=new Vector2(Width,Height);
            if(size!=current&&Client.State!=null&&!entryVisible&&!pauseVisible&&!notesVisible)
            {size=current;BuildWorld();refresh=true;}
        }
        public void Select(string id)
        {
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
            if(SharedFont)Destroy(SharedFont);
        }
    }
}
