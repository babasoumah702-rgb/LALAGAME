using System;
using UnityEngine;
using UnityEngine.UI;

namespace LastCall
{
    public sealed partial class LastCallInterface
    {
        public Font SharedFont { get; private set; }
        private readonly Color ink = new Color(.09f,.09f,.11f,.96f);
        private readonly Color cream = new Color(.94f,.94f,.96f);
        private readonly Color muted = new Color(.62f,.64f,.68f);
        private readonly Color green = new Color(.22f,.24f,.28f);
        private readonly Color gold = new Color(.78f,.80f,.84f);
        private Transform canvasRoot;
        private RectTransform Box(string name, Transform parent, float x,float y,float w,float h)
        {
            var go=new GameObject(name,typeof(RectTransform));
            var rect=go.GetComponent<RectTransform>();
            rect.SetParent(parent,false);
            rect.anchorMin=rect.anchorMax=new Vector2(0,1);
            rect.pivot=new Vector2(0,1);
            rect.anchoredPosition=new Vector2(x,-y);
            rect.sizeDelta=new Vector2(w,h);
            return rect;
        }
        private RectTransform Panel(string name,Transform parent,float x,float y,float w,float h,Color color)
        {
            var rect=Box(name,parent,x,y,w,h);
            rect.gameObject.AddComponent<Image>().color=color;
            return rect;
        }
        private Text Label(Transform parent,string value,float x,float y,float w,float h,int size=16,Color? color=null)
        {
            var rect=Box("Text",parent,x,y,w,h);
            var text=rect.gameObject.AddComponent<Text>();
            text.font=SharedFont;text.text=value;text.fontSize=size;
            text.color=color??cream;text.raycastTarget=false;text.supportRichText=false;
            text.horizontalOverflow=HorizontalWrapMode.Wrap;
            text.verticalOverflow=VerticalWrapMode.Truncate;
            text.alignment=TextAnchor.MiddleLeft;
            return text;
        }
        private Button ActionButton(Transform parent,string title,float x,float y,float w,float h,Action action,bool primary=false)
        {
            var rect=Panel(title,parent,x,y,w,h,primary?green:new Color(.16f,.16f,.18f));
            var button=rect.gameObject.AddComponent<Button>();
            var colors=button.colors;
            colors.highlightedColor=new Color(.55f,.58f,.64f);
            colors.pressedColor=new Color(.32f,.34f,.38f);
            colors.disabledColor=new Color(.22f,.22f,.24f);
            button.colors=colors;
            var label=Label(rect,title,10,0,w-20,h,15);
            label.alignment=TextAnchor.MiddleCenter;
            button.onClick.AddListener(()=>action());
            return button;
        }
        private InputField InputBox(Transform parent,float x,float y,float w,float h)
        {
            var rect=Panel("Your words",parent,x,y,w,h,new Color(.08f,.08f,.1f));
            var field=rect.gameObject.AddComponent<InputField>();
            var text=Label(rect,"",12,8,w-24,h-16,17);
            text.alignment=TextAnchor.UpperLeft;
            var placeholder=Label(rect,"也可以用自己的话说…",12,8,w-24,h-16,16,muted);
            placeholder.alignment=TextAnchor.UpperLeft;
            field.textComponent=text;field.placeholder=placeholder;
            field.lineType=InputField.LineType.MultiLineNewline;field.characterLimit=200;
            field.selectionColor=new Color(.7f,.72f,.76f,.45f);
            return field;
        }
        private void Clear(Transform parent)
        {
            for(int i=parent.childCount-1;i>=0;i--){var child=parent.GetChild(i).gameObject;child.SetActive(false);Destroy(child);}
        }
        private void Fill(RectTransform rect)
        {
            rect.anchorMin=Vector2.zero;rect.anchorMax=Vector2.one;
            rect.offsetMin=rect.offsetMax=Vector2.zero;
        }
    }
}
