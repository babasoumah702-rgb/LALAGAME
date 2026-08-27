using UnityEngine;
using UnityEngine.InputSystem;

namespace BarPrototype
{
    public sealed class BarHud : MonoBehaviour
    {
        public bool Paused { get; private set; }
        private Font font;
        private GUIStyle heading, subtitle, small, button, menuTitle;
        private readonly Color cream = new Color(.94f, .87f, .70f);
        private readonly Color muted = new Color(.58f, .65f, .61f);

        private void Awake()
        {
            Application.targetFrameRate = 60;
            Time.timeScale = 1;
            font = Font.CreateDynamicFontFromOSFont(new[] { "Microsoft YaHei", "Noto Sans CJK SC", "Arial" }, 24);
            if (!font) font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
        }
        private void Update()
        {
            if (Keyboard.current != null && Keyboard.current.escapeKey.wasPressedThisFrame) SetPaused(!Paused);
        }
        private void OnApplicationFocus(bool focus) { if (!focus) SetPaused(true); }
        private void OnDestroy() { Time.timeScale = 1; if (font) Destroy(font); }
        public void SetPaused(bool paused) { Paused = paused; Time.timeScale = paused ? 0 : 1; }
        private GUIStyle Style(int size, Color color, FontStyle weight = FontStyle.Normal)
        {
            return new GUIStyle(GUI.skin.label) { font = font, fontSize = size, fontStyle = weight,
                normal = { textColor = color }, alignment = TextAnchor.MiddleLeft };
        }
        private void Styles()
        {
            if (heading != null) return;
            heading = Style(30, cream, FontStyle.Bold);
            subtitle = Style(12, muted);
            small = Style(13, cream);
            menuTitle = Style(27, cream, FontStyle.Bold);
            menuTitle.alignment = TextAnchor.MiddleCenter;
            button = new GUIStyle(GUI.skin.button) { font = font, fontSize = 17, fixedHeight = 46 };
            foreach (var state in new[] { button.normal, button.hover, button.active, button.focused })
            { state.background = null; state.textColor = cream; }
            button.border = new RectOffset();
        }
        private static void Panel(Rect rect, Color color)
        {
            var old = GUI.color; GUI.color = color; GUI.DrawTexture(rect, Texture2D.whiteTexture); GUI.color = old;
        }
        private bool MenuButton(Rect rect, string text, bool primary)
        {
            bool hover = rect.Contains(Event.current.mousePosition);
            Panel(rect, hover ? new Color(.23f,.34f,.28f) : primary ? new Color(.16f,.27f,.22f) : new Color(.095f,.15f,.14f));
            Panel(new Rect(rect.x,rect.y,3,rect.height),primary || hover ? new Color(.82f,.53f,.22f) : new Color(.23f,.32f,.27f));
            return GUI.Button(rect,text,button);
        }
        private void OnGUI()
        {
            Styles();
            var scale = Mathf.Min(Screen.width / 1280f, Screen.height / 720f);
            GUI.matrix = Matrix4x4.TRS(Vector3.zero, Quaternion.identity, Vector3.one * scale);
            var w = Screen.width / scale; var h = Screen.height / scale;
            Panel(new Rect(32, 29, 3, 59), new Color(.82f, .53f, .22f));
            GUI.Label(new Rect(48, 21, 350, 44), "THE AMBER ROOM", heading);
            GUI.Label(new Rect(49, 65, 420, 24), "琥珀酒馆  /  A LITTLE PLACE TO SLOW DOWN", subtitle);
            GUI.Label(new Rect(w - 165, 31, 145, 23), "●  自由漫游", small);
            Panel(new Rect(32, h - 61, 466, 34), new Color(.025f, .05f, .052f, .83f));
            GUI.Label(new Rect(46, h - 59, 460, 29), "WASD / 方向键  移动     SHIFT  快走     ESC  暂停", small);
            GUI.Label(new Rect(w - 191, h - 57, 170, 28), "2.5D  ·  BAR STUDY 01", subtitle);
            if (!Paused) return;
            Panel(new Rect(0, 0, w, h), new Color(.01f, .025f, .026f, .76f));
            var x = w / 2 - 180; var y = h / 2 - 150;
            Panel(new Rect(x, y, 360, 300), new Color(.065f, .105f, .10f, .98f));
            Panel(new Rect(x, y, 360, 3), new Color(.82f, .53f, .22f));
            GUI.Label(new Rect(x + 30, y + 27, 300, 52), "休息片刻", menuTitle);
            GUI.Label(new Rect(x + 80, y + 76, 230, 28), "琥珀酒馆 · 游戏已暂停", subtitle);
            if (MenuButton(new Rect(x + 42, y + 126, 276, 46), "继续漫游", true)) SetPaused(false);
            if (MenuButton(new Rect(x + 42, y + 189, 276, 46), "退出游戏", false))
            {
#if UNITY_EDITOR
                UnityEditor.EditorApplication.isPlaying = false;
#else
                Application.Quit();
#endif
            }
        }
    }
}
