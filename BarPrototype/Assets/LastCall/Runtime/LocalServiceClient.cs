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
        public event Action Changed;
        public event Action<string> Error;
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
            PlayerId=PlayerPrefs.GetString("LastCall.PlayerId","");
            if(string.IsNullOrEmpty(PlayerId))
            {
                PlayerId=Guid.NewGuid().ToString("N");
                PlayerPrefs.SetString("LastCall.PlayerId",PlayerId);PlayerPrefs.Save();
            }
            var serverRoot=Application.isEditor
                ? Path.GetFullPath(Path.Combine(Application.dataPath,"../Server"))
                : Path.GetFullPath(Path.Combine(Application.dataPath,"../Server"));
            var node=Path.Combine(serverRoot,"node.exe");
            if(!File.Exists(node)&&Application.isEditor)node=@"D:\node.exe";
            var script=Path.Combine(serverRoot,"dist/server.js");
            if(!File.Exists(script)||!File.Exists(node)){Fail("缺少本地后端或 Node 运行时。请使用完整运行目录。");yield break;}
            token=Guid.NewGuid().ToString("N")+Guid.NewGuid().ToString("N");
            var startInfo=new ProcessStartInfo(node,"\""+script+"\" --managed")
            {
                WorkingDirectory=serverRoot,UseShellExecute=false,CreateNoWindow=true,
                WindowStyle=ProcessWindowStyle.Hidden,RedirectStandardInput=true,
                RedirectStandardOutput=true,RedirectStandardError=true
            };
            startInfo.EnvironmentVariables["LASTCALL_SESSION_TOKEN"]=token;
            if(Array.IndexOf(Environment.GetCommandLineArgs(),"-lastCallVerify")>=0)
            {
                var local=Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
                startInfo.EnvironmentVariables["LASTCALL_DATA_DIR"]=Path.Combine(local,"LALAGAME","Verification");
                startInfo.EnvironmentVariables["LASTCALL_CONFIG_DIR"]=Path.Combine(local,"LALAGAME","private");
            }
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
                if(message.type=="trace"){UnityEngine.Debug.Log("LASTCALL_TRACE "+message.message);continue;}
                if(message.ready&&message.port>0)
                {
                    baseUrl=new UriBuilder("http","127.0.0.1",message.port).Uri.GetLeftPart(UriPartial.Authority);
                    UnityEngine.Debug.Log("LASTCALL_SERVICE_READY");
                    continue;
                }
                if(message.type=="ack"){pending.TryRemove(message.id,out _);continue;}
                if(message.type=="error"){if(!string.IsNullOrEmpty(message.id))pending.TryRemove(message.id,out _);Fail(message.message);continue;}
                if(message.state!=null&&!string.IsNullOrEmpty(message.state.sessionId))AcceptState(message.state);
            }
        }
        public void Fail(string text){Status=text;UnityEngine.Debug.LogWarning("LASTCALL_STATUS "+text);Error?.Invoke(text);Changed?.Invoke();}
        private void AcceptState(StateDto state)
        {
            if(State==null||State.sessionId!=state.sessionId)visibleHistory.Clear();
            foreach(var item in state.events??Array.Empty<EventDto>())visibleHistory[item.id]=item;
            state.events=visibleHistory.Values.OrderBy(item=>item.seq).ToArray();
            State=state;Changed?.Invoke();
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
        private IEnumerator Post(string path,string json,bool readState)
        {
            var task=LocalRequest(HttpMethod.Post,path,json);
            while(!task.IsCompleted)yield return null;
            if(task.IsFaulted){Fail("本地请求未完成，请稍后重试。");yield break;}
            if(readState){var message=JsonUtility.FromJson<Envelope>(task.Result);if(message.state!=null&&!string.IsNullOrEmpty(message.state.sessionId))AcceptState(message.state);}
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
