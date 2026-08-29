using System.Reflection;
using System.Linq;
using LastCall;
using NUnit.Framework;
using UnityEditor.SceneManagement;
using UnityEngine;

namespace BarPrototype.Tests
{
    public sealed class NightStageTests
    {
        [Test] public void StairLaneHasContinuousFloorAndNoCorridorWallAcrossIt()
        {
            EditorSceneManager.OpenScene(LastCall.Editor.LastCallSceneBuilder.ScenePath);
            var host=new GameObject("Stage geometry verification");
            var stage=host.AddComponent<NightStage>();
            try
            {
                typeof(NightStage).GetMethod("Build",BindingFlags.Instance|BindingFlags.NonPublic).Invoke(stage,null);
                Physics.SyncTransforms();
                for(float z=-5.6f;z<=2.4f;z+=.3f)
                {
                    float y=(z+5.7f)/8.2f*4.2f;
                    var blockers=Physics.OverlapCapsule(new Vector3(7.45f,y+.35f,z),new Vector3(7.45f,y+1.45f,z),.23f)
                        .Where(c=>c.name!="Continuous stair collision").ToArray();
                    Assert.That(blockers,Is.Empty,"Stair obstruction at z="+z+": "+string.Join(",",blockers.Select(c=>c.name)));
                    Assert.That(Physics.Raycast(new Vector3(7.45f,y+.2f,z),Vector3.down,out var hit,.5f),Is.True);
                    Assert.That(hit.point.y,Is.EqualTo(y).Within(.02f));
                }
            }
            finally {Object.DestroyImmediate(host);}
        }
        [TestCase(7.45f,1f,-3f,"stairs")]
        [TestCase(3f,4.3f,5f,"rooftop")]
        [TestCase(3f,.09f,-5.9f,"corridor")]
        [TestCase(3f,.09f,4f,"bar")]
        public void FloorAreaUsesHeight(float x,float y,float z,string expected)
        {Assert.That(NightStage.Area(new Vector3(x,y,z)),Is.EqualTo(expected));}
    }
}
