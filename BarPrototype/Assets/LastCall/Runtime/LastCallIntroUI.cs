using UnityEngine;
using UnityEngine.UI;
using UnityEngine.InputSystem;
using UnityEngine.EventSystems;

namespace LastCall
{
    public sealed partial class LastCallInterface
    {
        public bool IntroEditing=>introInputVisible;
        public bool IsPaused=>pauseVisible;
        private bool introUIVisible,introInputVisible;
        private RectTransform introInputPanel;
        private InputField introField;
        private Text introMode;
        private void BuildIntroUI()
        {
            Clear(root);expression=null;rightPanel=null;pausePanel=null;notesPanel=null;
            introUIVisible=true;introInputVisible=false;
            size=new Vector2(Width,Height);
            Camera.main.rect=new Rect(0,0,1,1);
            Label(root,"LA LA LAND  /  今晚见",28,22,500,35,18,muted);
            introMode=Label(root,"",28,60,680,28,12,muted);
            Label(root,"按住右键观察    E 查看 / 收起手机    T 留下一句话    Esc 暂停",28,Height-47,790,30,13,muted);
            ActionButton(root,"查看手机",Width-268,Height-54,112,36,()=>Game.Intro.TogglePhone());
            ActionButton(root,"写一句话",Width-146,Height-54,118,36,OpenIntroInput);
        }
        private void UpdateIntroUI()
        {
            var state=Client.State;
            if(introMode)introMode.text=(state.mode=="online"?"在线模式":"离线规则模式")+"  ·  "+(state.intro.generationStatus=="pending"?"消息准备中":state.intro.generationStatus);
            if(Keyboard.current?.escapeKey.wasPressedThisFrame==true)
            {
                if(introInputVisible)CloseIntroInput(false);
                else Pause(!pauseVisible);
            }
            else if(Keyboard.current?.tKey.wasPressedThisFrame==true&&!pauseVisible&&!introInputVisible)OpenIntroInput();
            if(size!=new Vector2(Width,Height)&&!introInputVisible&&!pauseVisible)BuildIntroUI();
        }
        public void OpenIntroInput()
        {
            if(introInputVisible||pauseVisible||Client.State?.intro?.phase!="elevator")return;
            introInputVisible=true;Client.Send(new CommandDto{type="pause",paused=true});
            var body=Modal("此刻，你想说什么？",650,350);introInputPanel=body.parent.GetComponent<RectTransform>();
            Label(body,"这只是你此刻的想法，不会自动发送给酒吧里的人。",28,79,592,45,16,muted);
            introField=InputBox(body,28,140,592,105);
            introField.text=Client.State.intro.playerText??"";
            ActionButton(body,"记下，继续上升",28,273,285,45,()=>CloseIntroInput(true),true);
            ActionButton(body,"暂时留白",328,273,292,45,()=>CloseIntroInput(false));
            introField.ActivateInputField();
        }
        public void CloseIntroInput(bool submit)
        {
            if(!introInputVisible)return;
            if(submit)Client.Send(new CommandDto{type="intro_text",text=introField.text});
            introInputVisible=false;
            EventSystem.current?.SetSelectedGameObject(null);
            if(introInputPanel)Destroy(introInputPanel.gameObject);
            Client.Send(new CommandDto{type="pause",paused=pauseVisible});
        }
        // Future speech transcription enters through the same reviewed text path; no microphone is opened.
        public void AcceptIntroTranscript(string text)
        {
            OpenIntroInput();
            if(introField)introField.text=(text??"").Substring(0,Mathf.Min(200,(text??"").Length));
        }
    }
}
