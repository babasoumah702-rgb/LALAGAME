using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using UnityEngine;
using UnityEngine.InputSystem;
using UnityEngine.InputSystem.LowLevel;

namespace BarPrototype
{
    // Opt-in verification only. Normal game launches never run or write test artifacts.
    public sealed class BarSmokeRunner : MonoBehaviour
    {
        [Serializable] private sealed class Check { public string name; public bool passed; public string detail; }
        [Serializable] private sealed class Report
        {
            public string unityVersion, graphicsDevice, timestamp;
            public bool passed;
            public float averageFps;
            public List<Check> checks = new();
            public List<string> errors = new();
        }
        private readonly Report report = new();
        private string output;
        private PlayerMotor player;
        private BarHud hud;
        private Vector3 spawn;
        private bool active;
        private bool finished;
        private float startedAt;

        private void Awake()
        {
            var args = Environment.GetCommandLineArgs();
            active = Array.IndexOf(args, "-barSmokeTest") >= 0;
            enabled = active;
            if (!active) return;
            int index = Array.IndexOf(args, "-barArtifacts");
            output = index >= 0 && index + 1 < args.Length ? args[index + 1] : Path.Combine(Application.persistentDataPath, "Verification");
            Directory.CreateDirectory(output);
            report.unityVersion = Application.unityVersion;
            report.graphicsDevice = SystemInfo.graphicsDeviceName;
            report.timestamp = DateTime.Now.ToString("O");
            Application.logMessageReceived += Log;
            startedAt = Time.realtimeSinceStartup;
        }
        private void Log(string condition, string trace, LogType type)
        {
            if (type == LogType.Error || type == LogType.Exception || type == LogType.Assert) report.errors.Add(condition + "\n" + trace);
        }
        private void CheckResult(string name, bool pass, string detail)
        {
            report.checks.Add(new Check { name = name, passed = pass, detail = detail });
            Debug.Log("AMBER_TEST " + (pass ? "PASS " : "FAIL ") + name + ": " + detail);
        }
        private void Update()
        {
            if (active && !finished && Time.realtimeSinceStartup - startedAt > 100)
            {
                CheckResult("watchdog", false, "Smoke test timed out"); Finish();
            }
        }
        private IEnumerator Start()
        {
            if (!active) yield break;
            yield return null;
            // Allow the native window to become visible/focused before exercising real bindings.
            float focusDeadline = Time.realtimeSinceStartup + 6;
            while (!Application.isFocused && Time.realtimeSinceStartup < focusDeadline) yield return null;
            player = FindFirstObjectByType<PlayerMotor>(); hud = FindFirstObjectByType<BarHud>();
            CheckResult("scene_components", player && hud && Camera.main, "Player, UI, main camera");
            if (!player || !hud || !Camera.main) { Finish(); yield break; }
            spawn = player.transform.position;
            CheckResult("spawn_position",Vector3.Distance(spawn,new Vector3(.1f,.05f,-2.35f))<.15f,spawn.ToString("F3"));
            hud.SetPaused(false); player.enabled = false;
            for (int i = 0; i < 8; i++)
            {
                var angle = i * Mathf.PI / 4;
                var input = new Vector2(Mathf.Cos(angle), Mathf.Sin(angle));
                var world = PlayerMotor.ScreenDirection(input, Camera.main.transform.rotation);
                var view = Camera.main.transform.InverseTransformDirection(world);
                CheckResult("direction_" + i, Mathf.Abs(world.magnitude - 1) < .001f && Vector2.Dot(new Vector2(view.x, view.y).normalized, input) > .9f, world.ToString("F3"));
            }
            var distances = new List<float>();
            foreach (int fps in new[] { 30, 60, 144 })
            {
                player.Teleport(new Vector3(0,.06f,-2.5f));
                var start = player.transform.position;
                for (int i = 0; i < fps / 2; i++) player.Step(Vector2.up, false, 1f / fps);
                var delta = player.transform.position - start; delta.y = 0; distances.Add(delta.magnitude);
                CheckResult("walk_" + fps + "fps", Mathf.Abs(delta.magnitude - player.WalkSpeed * .5f) < .05f, delta.magnitude.ToString("F3"));
            }
            CheckResult("framerate_invariance", Mathf.Abs(distances[0] - distances[2]) < .025f, "30 vs 144 FPS displacement");
            player.Teleport(new Vector3(0,.06f,-2.5f));
            var runStart = player.transform.position;
            for (int i = 0; i < 30; i++) player.Step(Vector2.up, true, 1f / 60);
            var runDelta = player.transform.position - runStart; runDelta.y = 0;
            CheckResult("run_speed", Mathf.Abs(runDelta.magnitude-player.RunSpeed*.5f)<.05f,runDelta.magnitude.ToString("F3"));
            var stopStart = player.transform.position;
            for (int i = 0; i < 30; i++) player.Step(Vector2.zero, false, 1f / 60);
            var stopDelta = player.transform.position - stopStart; stopDelta.y = 0;
            CheckResult("release_stops", stopDelta.magnitude < .001f, stopDelta.ToString());
            // Push against all four room boundaries, keeping the route away from furniture.
            var positions = new[] { new Vector3(-5.5f,.06f,-.2f),new Vector3(5.5f,.06f,-4.7f),new Vector3(0,.06f,-4.7f),new Vector3(2,.06f,4.5f) };
            var worldDirections = new[] { Vector3.left, Vector3.right, Vector3.back, Vector3.forward };
            for(int i=0;i<positions.Length;i++)
            {
                player.Teleport(positions[i]);
                var direction=ToInput(worldDirections[i]);
                for(int frame=0;frame<120;frame++)player.Step(direction,true,1f/60);
                var p=player.transform.position;
                CheckResult("boundary_"+i,Mathf.Abs(p.x)<6 && Mathf.Abs(p.z)<5 && p.y>-.1f,p.ToString("F3"));
            }
            player.Teleport(new Vector3(-1.8f,.06f,-.05f));
            for(int i=0;i<120;i++)player.Step(ToInput(Vector3.forward),false,1f/60);
            CheckResult("counter_or_stool_collision",player.transform.position.z<1.5f && player.transform.position.z>-.05f,player.transform.position.ToString("F3"));
            player.Teleport(new Vector3(-5.7f,.06f,-1.3f));
            for(int i=0;i<60;i++)player.Step(ToInput(new Vector3(-1,0,1).normalized),false,1f/60);
            CheckResult("wall_sliding",player.transform.position.x> -6 && player.transform.position.z>.1f,player.transform.position.ToString("F3"));
            player.Teleport(new Vector3(0,.06f,-2.5f));
            hud.SetPaused(true);var pauseStart=player.transform.position;
            player.Step(Vector2.up,true,1);
            CheckResult("pause_blocks_movement",player.transform.position==pauseStart && Time.timeScale==0,"Paused movement remains zero");
            hud.SetPaused(false);
            CheckResult("resume",Time.timeScale==1,"Time scale restored");

            player.Teleport(spawn);player.enabled=true;
            // Queue device events through the actual Input System bindings.
            if(Keyboard.current!=null && Application.isFocused)
            {
                foreach(var key in new[]{Key.W,Key.A,Key.S,Key.D,Key.UpArrow,Key.LeftArrow,Key.DownArrow,Key.RightArrow})
                {
                    player.Teleport(spawn);
                    var inputStart=player.transform.position;
                    yield return PressKeys(new[]{key},.18f);
                    var displacement=player.transform.position-inputStart;displacement.y=0;
                    CheckResult("keyboard_"+key+"_binding",displacement.magnitude>.2f,displacement.magnitude.ToString("F3"));
                }
                player.Teleport(spawn);
                yield return PressKeys(new[]{Key.W,Key.LeftShift},.25f);
                var sprintDelta=player.transform.position-spawn;sprintDelta.y=0;
                CheckResult("keyboard_shift_binding",sprintDelta.magnitude>player.WalkSpeed*.25f*1.3f,sprintDelta.magnitude.ToString("F3"));
                InputSystem.QueueStateEvent(Keyboard.current,new KeyboardState(Key.Escape));
                yield return null;yield return null;
                CheckResult("escape_binding",hud.Paused,"Escape opens pause menu");
                InputSystem.QueueStateEvent(Keyboard.current,new KeyboardState());yield return null;
                hud.SetPaused(false);
                var releasedPosition=player.transform.position;
                yield return new WaitForSecondsRealtime(.35f);
                var releasedDelta=player.transform.position-releasedPosition;releasedDelta.y=0;
                CheckResult("keyboard_release_stops",releasedDelta.magnitude<.06f,releasedDelta.ToString("F3"));
            }
            else CheckResult("keyboard_focus",false,"Window must be focused to verify keyboard input");
            player.Teleport(spawn);
            // Capture a deterministic idle pose after all physical input checks.
            player.enabled=false;
            foreach(var renderer in FindObjectsByType<Renderer>(FindObjectsSortMode.None))
            foreach(var material in renderer.sharedMaterials)
                if(!material || !material.shader || material.shader.name=="Hidden/InternalErrorShader")
                    report.errors.Add("Missing/error material: "+renderer.name);
            CheckResult("materials",report.errors.Count==0,"No missing or error materials");
            Screen.SetResolution(1920,1080,FullScreenMode.Windowed);
            yield return new WaitForSecondsRealtime(1);
            float startTime=Time.realtimeSinceStartup;int frames=0;
            while(Time.realtimeSinceStartup-startTime<5){frames++;yield return null;}
            report.averageFps=frames/(Time.realtimeSinceStartup-startTime);
            CheckResult("1080p_performance",report.averageFps>=55,report.averageFps.ToString("F1")+" FPS");
            yield return Capture("bar-1920x1080.png");
            Screen.SetResolution(1280,800,FullScreenMode.Windowed);yield return new WaitForSecondsRealtime(.7f);
            yield return Capture("bar-1280x800.png");
            hud.SetPaused(true);yield return null;yield return Capture("pause-menu.png");hud.SetPaused(false);
            Finish();
        }
        private Vector2 ToInput(Vector3 world)
        {
            var right=Camera.main.transform.right;right.y=0;right.Normalize();
            var forward=Camera.main.transform.forward;forward.y=0;forward.Normalize();
            return new Vector2(Vector3.Dot(world,right),Vector3.Dot(world,forward));
        }
        private IEnumerator PressKeys(Key[] keys,float seconds)
        {
            InputSystem.QueueStateEvent(Keyboard.current,new KeyboardState(keys));
            yield return new WaitForSecondsRealtime(seconds);
            InputSystem.QueueStateEvent(Keyboard.current,new KeyboardState());
            yield return null;
        }
        private IEnumerator Capture(string name)
        {
            yield return new WaitForEndOfFrame();
            var texture=ScreenCapture.CaptureScreenshotAsTexture();
            File.WriteAllBytes(Path.Combine(output,name),texture.EncodeToPNG());
            Destroy(texture);
        }
        private void Finish()
        {
            if(finished)return;finished=true;
            if(hud)hud.SetPaused(false);
            report.passed=report.errors.Count==0 && report.checks.TrueForAll(check=>check.passed);
            File.WriteAllText(Path.Combine(output,"smoke-report.json"),JsonUtility.ToJson(report,true));
            Application.logMessageReceived-=Log;
            Debug.Log("AMBER_SMOKE_COMPLETE: "+report.passed);
            Application.Quit(report.passed?0:1);
        }
    }
}
