using System;
using System.Collections;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Diagnostics;
using System.IO;
using System.Net.WebSockets;
using System.Net.Http;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using UnityEngine;
using UnityEngine.Networking;

namespace LastCall
{
    public sealed partial class LocalServiceClient : MonoBehaviour
    {
        public StateDto State { get; private set; }
        public BootstrapDto Bootstrap { get; private set; }
        public string Status { get; private set; } = "正在准备本地关系世界…";
        public string PlayerId { get; private set; }
        public bool Ready { get; private set; }
        public string BaseUrl => baseUrl;
        public string Token => token;
        public int PresentationEpoch { get; private set; }
        private bool resetPresentation;
        public event Action Changed;
        public event Action<string> Error;
        public event Action<string> Acknowledged;
        public event Action<string,string> Rejected;
        private Process service;
        private string baseUrl, token;
        private long commandSequence;
        private readonly Dictionary<string,EventDto> visibleHistory=new Dictionary<string,EventDto>();
        private ClientWebSocket socket;
        private readonly CancellationTokenSource cancellation = new CancellationTokenSource();
        private readonly ConcurrentQueue<string> incoming = new ConcurrentQueue<string>();
        private readonly ConcurrentDictionary<string,string> pending = new ConcurrentDictionary<string,string>();
        private readonly ConcurrentDictionary<string,CommandDto> latestPositions=new ConcurrentDictionary<string,CommandDto>();
        private readonly ConcurrentQueue<string> controls=new ConcurrentQueue<string>();
        private readonly SemaphoreSlim sendLock = new SemaphoreSlim(1);
        private readonly HttpClient http = new HttpClient(new HttpClientHandler { UseProxy = false });

        private IEnumerator Start()
        {
            Application.targetFrameRate = 60;
            Application.runInBackground = true;
            bool nightTest=Array.IndexOf(Environment.GetCommandLineArgs(),"-fullNightVerify")>=0;
            bool scene0Test=Array.IndexOf(Environment.GetCommandLineArgs(),"-scene0Verify")>=0;
            bool cardTest=Array.IndexOf(Environment.GetCommandLineArgs(),"-cardPlayVerify")>=0;
            bool sceneOneTest=Array.IndexOf(Environment.GetCommandLineArgs(),"-sceneOneVerify")>=0;
            bool sceneTwoThreeTest=Array.IndexOf(Environment.GetCommandLineArgs(),"-sceneTwoThreeVerify")>=0;
            PlayerId=nightTest?"full-night-verification":sceneOneTest?"scene-one-verification":cardTest?"card-play-verification":scene0Test?"scene0-verification":sceneTwoThreeTest?"scene-two-three-verification":PlayerPrefs.GetString("LastCall.PlayerId","");
            if(string.IsNullOrEmpty(PlayerId))
            {
                PlayerId=Guid.NewGuid().ToString("N");
                PlayerPrefs.SetString("LastCall.PlayerId",PlayerId);PlayerPrefs.Save();
            }
            var serverRoot=Application.isEditor
                ? Path.GetFullPath(Path.Combine(Application.dataPath,"../Server"))
                : Path.GetFullPath(Path.Combine(Application.dataPath,"../Server"));
            var node=FindNode(serverRoot);
            var script=Path.Combine(serverRoot,"dist/server.js");
            if(!File.Exists(script)){Fail("缺少本地后端。请在 BarPrototype/Server 执行 npm ci 和 npm run build。");yield break;}
            if(string.IsNullOrEmpty(node)||!File.Exists(node)){Fail("找不到 Node 运行时。请安装 Node 24，或把 node 放到 Server 目录。");yield break;}
            token=Guid.NewGuid().ToString("N")+Guid.NewGuid().ToString("N");
            var startInfo=new ProcessStartInfo(node,"\""+script+"\" --managed")
            {
                WorkingDirectory=serverRoot,UseShellExecute=false,CreateNoWindow=true,
                WindowStyle=ProcessWindowStyle.Hidden,RedirectStandardInput=true,
                RedirectStandardOutput=true,RedirectStandardError=true
            };
            startInfo.EnvironmentVariables["PATH"]="/opt/homebrew/bin:/usr/local/bin:"+(startInfo.EnvironmentVariables["PATH"]??"");
            startInfo.EnvironmentVariables["LASTCALL_SESSION_TOKEN"]=token;
            if(Array.IndexOf(Environment.GetCommandLineArgs(),"-lastCallVerify")>=0||scene0Test||cardTest||sceneOneTest||sceneTwoThreeTest||nightTest)
            {
                var local=Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
                var verificationRoot=Path.Combine(local,"LALAGAME",nightTest?"FullNightVerification":sceneOneTest?"SceneOneVerification":cardTest?"CardPlayVerification":scene0Test?"Scene0Verification":sceneTwoThreeTest?"SceneTwoThreeVerification":"Verification");
                startInfo.EnvironmentVariables["LASTCALL_DATA_DIR"]=verificationRoot;
                startInfo.EnvironmentVariables["LASTCALL_CONFIG_DIR"]=Path.Combine(verificationRoot,"private");
            }
            if(nightTest||sceneTwoThreeTest){var args=Environment.GetCommandLineArgs();int scale=Array.IndexOf(args,"-fullNightClock");if(scale>=0)startInfo.EnvironmentVariables["LASTCALL_TEST_CLOCK"]=args[scale+1];}
            service=new Process{StartInfo=startInfo};
            service.OutputDataReceived+=(sender,args)=>{if(!string.IsNullOrEmpty(args.Data))incoming.Enqueue(args.Data);};
            service.ErrorDataReceived+=(sender,args)=>{ /* Never print provider details or environment values. */ };
            try{service.Start();service.BeginOutputReadLine();service.BeginErrorReadLine();}
            catch(Exception){Fail("本地服务启动失败，请查看运行目录是否完整。");yield break;}
            var deadline=Time.realtimeSinceStartup+20;
            while(string.IsNullOrEmpty(baseUrl)&&Time.realtimeSinceStartup<deadline)yield return null;
            if(string.IsNullOrEmpty(baseUrl)){Fail("本地服务没有及时就绪。");yield break;}
            yield return FetchBootstrap();
            if(Bootstrap!=null)
            {
                Ready=true;Status="准备好了。选择今晚如何入场。";Changed?.Invoke();
                _=SocketLoop();
            }
        }
        private void Update()
        {
            while(incoming.TryDequeue(out var json))
            {
                Envelope message;
                try{message=JsonUtility.FromJson<Envelope>(json);}
                catch{continue;}
                if(message==null)continue;
                if(message.type=="reconnected"){resetPresentation=true;continue;}
                if(message.type=="trace"){UnityEngine.Debug.Log("LASTCALL_TRACE "+message.message);continue;}
                if(message.ready&&message.port>0)
                {
                    baseUrl=new UriBuilder("http","127.0.0.1",message.port).Uri.GetLeftPart(UriPartial.Authority);
                    UnityEngine.Debug.Log("LASTCALL_SERVICE_READY");
                    continue;
                }
                if(message.type=="ack"){pending.TryRemove(message.id,out _);Acknowledged?.Invoke(message.id);continue;}
                if(message.type=="error"){if(!string.IsNullOrEmpty(message.id))pending.TryRemove(message.id,out _);Rejected?.Invoke(message.id,message.message);Fail(message.message);continue;}
                if(message.state!=null&&!string.IsNullOrEmpty(message.state.sessionId))AcceptState(message.state);
            }
        }
        public void Fail(string text){Status=text;UnityEngine.Debug.LogWarning("LASTCALL_STATUS "+text);Error?.Invoke(text);Changed?.Invoke();}
        private static string FindNode(string serverRoot)
        {
            foreach(var candidate in NodeCandidates(serverRoot))
                if(!string.IsNullOrEmpty(candidate)&&File.Exists(candidate))return candidate;
            return null;
        }
        private static IEnumerable<string> NodeCandidates(string serverRoot)
        {
            yield return Path.Combine(serverRoot,"node.exe");
            yield return Path.Combine(serverRoot,"node");
            if(!Application.isEditor)yield break;
            yield return @"D:\node.exe";
            var file=Application.platform==RuntimePlatform.WindowsEditor?"node.exe":"node";
            foreach(var dir in (Environment.GetEnvironmentVariable("PATH")??"").Split(Path.PathSeparator))
                if(!string.IsNullOrEmpty(dir))yield return Path.Combine(dir,file);
            yield return "/opt/homebrew/bin/node";
            yield return "/usr/local/bin/node";
            var nvm=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile),".nvm","versions","node");
            if(!Directory.Exists(nvm))yield break;
            foreach(var version in Directory.GetDirectories(nvm))
                yield return Path.Combine(version,"bin","node");
        }
        private void AcceptState(StateDto state)
        {
            if(State==null||State.sessionId!=state.sessionId)visibleHistory.Clear();
            foreach(var item in state.events??Array.Empty<EventDto>())visibleHistory[item.id]=item;
            state.events=visibleHistory.Values.OrderBy(item=>item.seq).ToArray();
            if(string.IsNullOrEmpty(state.scene1?.phase))state.scene1=null;
            if(string.IsNullOrEmpty(state.scene2?.phase))state.scene2=null;
            if(string.IsNullOrEmpty(state.scene3?.phase))state.scene3=null;
            if(state.story?.chapter<1)state.story=null;
            if(state.late?.chapter<4)state.late=null;
            if(state.late!=null&&string.IsNullOrEmpty(state.late.cue?.id))state.late.cue=null;
            State=state;if(resetPresentation){PresentationEpoch++;resetPresentation=false;}Changed?.Invoke();
        }
        private IEnumerator FetchBootstrap()
        {
            var task=LocalRequest(HttpMethod.Get,"/api/bootstrap?playerId="+PlayerId,null);
            while(!task.IsCompleted)yield return null;
            if(task.IsFaulted){Fail("无法读取本地入口配置。");yield break;}
            Bootstrap=JsonUtility.FromJson<BootstrapDto>(task.Result);
        }
        public void OpenSession(SessionRequest options){options.playerId=PlayerId;StartCoroutine(Open(options));}
        private IEnumerator Open(SessionRequest options)
        {
            pending.Clear();
            latestPositions.Clear();
            while(controls.TryDequeue(out _)){}
            yield return Post("/api/session",JsonUtility.ToJson(options),true);
        }
        public void Save(){StartCoroutine(Post("/api/save","{}",false));}
        public void RefreshEntry(){StartCoroutine(FetchBootstrap());}
        public void ConfigureModel(ModelConfigRequestDto options,Action<bool,string> complete){StartCoroutine(ConfigureModelRequest(options,complete));}
        private IEnumerator ConfigureModelRequest(ModelConfigRequestDto options,Action<bool,string> complete)
        {
            var task=LocalRequest(HttpMethod.Post,"/api/model-config",JsonUtility.ToJson(options));
            while(!task.IsCompleted)yield return null;
            if(task.IsFaulted){complete?.Invoke(false,"保存失败。请检查接口地址、模型名和网络安全要求。");yield break;}
            yield return FetchBootstrap();
            bool configured=Bootstrap?.modelConfigured==true;
            Status=configured?"模型配置已保存。":"模型密钥已清除；可以使用离线规则模式。";
            Changed?.Invoke();complete?.Invoke(true,Status);
        }
        private IEnumerator Post(string path,string json,bool readState)
        {
            var task=LocalRequest(HttpMethod.Post,path,json);
            while(!task.IsCompleted)yield return null;
            if(task.IsFaulted){Fail("本地请求未完成，请稍后重试。");yield break;}
            if(readState){resetPresentation=true;var message=JsonUtility.FromJson<Envelope>(task.Result);if(message.state!=null&&!string.IsNullOrEmpty(message.state.sessionId))AcceptState(message.state);}
        }
        private async Task<string> LocalRequest(HttpMethod method,string path,string body)
        {
            using(var request=new HttpRequestMessage(method,baseUrl+path))
            {
                request.Headers.TryAddWithoutValidation("Authorization","Bearer "+token);
                if(body!=null)request.Content=new StringContent(body,Encoding.UTF8,"application/json");
                using(var response=await http.SendAsync(request,cancellation.Token).ConfigureAwait(false))
                {
                    response.EnsureSuccessStatusCode();
                    return await response.Content.ReadAsStringAsync().ConfigureAwait(false);
                }
            }
        }
    }
}
