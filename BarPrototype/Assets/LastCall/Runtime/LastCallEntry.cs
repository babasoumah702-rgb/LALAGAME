using System.Linq;
using UnityEngine;
using UnityEngine.UI;

namespace LastCall
{
    public sealed partial class LastCallInterface
    {
        private void BuildEntry()
        {
            Clear(root);
            entryVisible=true;
            entryPanel=Box("Entry",root,0,0,Width,Height);
            Fill(entryPanel);
            var plate=OtomeArt.Bar();
            if(plate)
            {
                var photo=Box("Painting",entryPanel,0,0,Width,Height);
                Fill(photo);
                var image=photo.gameObject.AddComponent<RawImage>();
                image.texture=plate;
                image.color=Color.white;
                image.raycastTarget=false;
            }
            var veil=Panel("Veil",entryPanel,0,0,Width,Height,new Color(.06f,.06f,.08f,plate?0.48f:0.94f));
            Fill(veil);
            float left=(Width-760)/2;
            var form=Panel("Arrival",entryPanel,left,55,760,610,ink);
            Panel("Brass",form,0,0,4,610,gold);
            Label(form,"LALAGAME  /  LA LA LAND",28,22,700,52,34);
            Label(form,"闭店前最后一局",30,78,650,34,23);
            Label(form,"今晚，你带着怎样的自己来到这里？可跳过。\n门里面的夜已经开始。第三杯还在桌上，椅子是空的。",30,119,700,60,17,muted);
            var config=Client.Bootstrap;
            if(config==null)
            {
                Label(form,Client.Status,30,225,680,90,20);
                ActionButton(form,"退出",30,515,180,45,Quit);
                return;
            }
            Label(form,"01  今晚的入口",30,187,700,26,15,gold);
            for(int i=0;i<config.roles.Length;i++)
            {
                int index=i;
                string title=(i==roleIndex?"● ":"")+config.roles[i].name;
                ActionButton(form,title,30+(i%3)*235,222+(i/3)*52,220,44,()=>{roleIndex=index;BuildEntry();},i==roleIndex);
            }
            roleDetail=Label(form,config.roles[roleIndex].description,30,327,700,38,16,muted);
            Label(form,"02  参与意愿与表达方式",30,375,700,25,15,gold);
            ActionButton(form,config.intents[intentIndex].name,30,411,220,42,()=>{
                intentIndex=(intentIndex+1)%config.intents.Length;BuildEntry();
            },true);
            ActionButton(form,config.styles[styleIndex].name,265,411,220,42,()=>{
                styleIndex=(styleIndex+1)%config.styles.Length;BuildEntry();
            });
            ActionButton(form,online?"在线模型 · 开":"规则模式 · 离线",500,411,220,42,()=>{online=!online;BuildEntry();});
            Label(form,"在线模式会把本局表达发送到你配置的模型网关。角色均为虚构成年人。\n离线模式不调用模型。语音未开放；存档仅保存在本机。",30,463,700,48,13,muted);
            ActionButton(form,"推门，开始今晚",30,531,330,48,()=>{
                Client.OpenSession(new SessionRequest{
                    role=config.roles[roleIndex].id,entryIntent=config.intents[intentIndex].id,
                    style=config.styles[styleIndex].id,online=online,mode="new"
                });
            },true);
            var save=config.sessions?.FirstOrDefault();
            if(save!=null)
                ActionButton(form,"继续上次的夜晚",375,531,220,48,()=>Client.OpenSession(new SessionRequest{mode="resume",sessionId=save.id}));
            ActionButton(form,"退出",610,531,110,48,Quit);
        }
        private void Quit()
        {
            Client.Save();
#if UNITY_EDITOR
            UnityEditor.EditorApplication.isPlaying=false;
#else
            Application.Quit();
#endif
        }
    }
}
