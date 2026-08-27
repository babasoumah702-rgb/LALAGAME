using System;
using System.IO;
using System.Linq;
using System.Net.WebSockets;
using System.Text;
using System.Threading.Tasks;
using UnityEngine;

namespace LastCall
{
    public sealed partial class LocalServiceClient
    {
        private async Task SocketLoop()
        {
            while (!cancellation.IsCancellationRequested)
            {
                try
                {
                    socket = new ClientWebSocket();
                    socket.Options.Proxy = null;
                    socket.Options.SetRequestHeader("Authorization", "Bearer " + token);
                    string endpoint = baseUrl.Replace("http://", "ws://") + "/api/events";
                    await socket.ConnectAsync(new Uri(endpoint), cancellation.Token);
                    while(controls.TryDequeue(out _)){}
                    foreach (var pair in pending.OrderBy(p=>p.Key)) controls.Enqueue(pair.Value);
                    _=SendPump(socket);
                    await ReceiveMessages();
                }
                catch (Exception)
                {
                    if (!cancellation.IsCancellationRequested) await Task.Delay(1000);
                }
                finally
                {
                    socket?.Dispose();
                    socket = null;
                }
            }
        }
        private async Task ReceiveMessages()
        {
            byte[] buffer = new byte[16384];
            while (socket.State == WebSocketState.Open)
            {
                using (var stream = new MemoryStream())
                {
                    WebSocketReceiveResult response;
                    do
                    {
                        response = await socket.ReceiveAsync(new ArraySegment<byte>(buffer), cancellation.Token);
                        if (response.MessageType == WebSocketMessageType.Close) return;
                        stream.Write(buffer, 0, response.Count);
                        if (stream.Length > 4194304) throw new IOException("Message too large");
                    } while (!response.EndOfMessage);
                    incoming.Enqueue(Encoding.UTF8.GetString(stream.ToArray()));
                }
            }
        }
        private void OnDestroy()
        {
            cancellation.Cancel();
            http.Dispose();
            socket?.Abort();
            if (service == null) return;
            try
            {
                if (!service.HasExited)
                {
                    service.StandardInput.Close();
                    if (!service.WaitForExit(1000)) service.Kill();
                }
            }
            catch (Exception) { }
            finally { service.Dispose(); }
        }
        public void Send(CommandDto command)
        {
            if (!Ready || State == null) return;
            command.id=(++commandSequence).ToString("D10")+"-"+command.id;
            command.sessionId=State.sessionId;
            command.cursor=State.cursor;
            string json = JsonUtility.ToJson(command);
            if(command.type=="position")latestPositions[command.actor]=command;
            else{pending[command.id]=json;controls.Enqueue(json);}
        }
        private async Task SendPump(ClientWebSocket connection)
        {
            try
            {
                while(!cancellation.IsCancellationRequested&&socket==connection&&connection.State==WebSocketState.Open)
                {
                    if(controls.TryDequeue(out var control)){await SendText(control);continue;}
                    if(!latestPositions.IsEmpty&&State!=null)
                    {
                        var batch=new System.Collections.Generic.List<CommandDto>();
                        foreach(var key in latestPositions.Keys)
                            if(latestPositions.TryRemove(key,out var item))batch.Add(item);
                        await SendText(JsonUtility.ToJson(new PositionBatchDto{sessionId=State.sessionId,cursor=State.cursor,items=batch.ToArray()}));
                    }
                    else await Task.Delay(10,cancellation.Token);
                }
            }
            catch(Exception){}
        }
        private async Task SendText(string text)
        {
            await sendLock.WaitAsync();
            try
            {
                ClientWebSocket connection = socket;
                if (connection != null && connection.State == WebSocketState.Open)
                {
                    byte[] bytes = Encoding.UTF8.GetBytes(text);
                    await connection.SendAsync(new ArraySegment<byte>(bytes), WebSocketMessageType.Text, true, cancellation.Token);
                }
            }
            catch (Exception) { }
            finally { sendLock.Release(); }
        }
    }
}
