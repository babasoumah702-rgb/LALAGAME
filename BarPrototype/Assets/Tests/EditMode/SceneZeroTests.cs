using System.Linq;
using NUnit.Framework;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using LastCall;

namespace BarPrototype.Tests
{
    public class SceneZeroTests
    {
        [SetUp] public void Open()=>EditorSceneManager.OpenScene(LastCall.Editor.LastCallSceneBuilder.ScenePath);
        [Test] public void ElevatorIsEditableAndHasIndependentDoors()
        {
            var intro=Object.FindObjectOfType<SceneZeroController>();Assert.That(intro,Is.Not.Null);
            Assert.That(intro.leftDoor,Is.Not.Null);Assert.That(intro.rightDoor,Is.Not.Null);
            Assert.That(intro.leftDoor.GetComponent<Collider>(),Is.Not.Null);
            Assert.That(AssetDatabase.LoadAssetAtPath<GameObject>("Assets/LastCall/SceneZero/SceneZero.prefab"),Is.Not.Null);
            Assert.That(intro.phoneRig.childCount,Is.GreaterThan(5));
        }
        [Test] public void AllSixProductionModelsAndStageTexturesAreSerialized()
        {
            var catalog=Object.FindObjectOfType<LastCallGame>().artCatalog;
            Assert.That(catalog,Is.Not.Null);Assert.That(catalog.models.Length,Is.EqualTo(6));
            foreach(var id in new[]{"A","B","C","D","OWNER","BARTENDER"}){
                var model=catalog.Model(id);Assert.That(model,Is.Not.Null,id);
                foreach(var r in model.GetComponentsInChildren<Renderer>())foreach(var m in r.sharedMaterials){
                    Assert.That(m.shader.name,Is.EqualTo("Universal Render Pipeline/Lit"));Assert.That(m.GetTexture("_BaseMap"),Is.Not.Null,id);
                }
            }
            Assert.That(catalogcatalogTexture(catalog),Is.True);
        }
        private bool catalogcatalogTexture(LastCallArtCatalog c)=>new[]{"skin-floor","skin-bar","skin-wood","skin-leather","skin-photos","skin-plaster"}.All(id=>c.Texture(id));
        [Test] public void ClosedDoorsAndFrontBoundaryBlockWalking()
        {
            var i=Object.FindObjectOfType<SceneZeroController>();i.SetDoors(0);Physics.SyncTransforms();
            Assert.That(Physics.Linecast(new Vector3(-1.2f,1,-7.4f),new Vector3(-1.2f,1,-6f)),Is.True);
            i.SetDoors(1);Physics.SyncTransforms();
            Assert.That(Physics.Linecast(new Vector3(-1.2f,1,-7.4f),new Vector3(-1.2f,1,-6f)),Is.False);
            foreach(float x in new[]{-5f,-3f,1f,3f,6f})
                Assert.That(Physics.Linecast(new Vector3(x,1,-4.8f),new Vector3(x,1,-5.6f)),Is.True,x.ToString());
        }
        [Test] public void EditableSceneContainsNoMissingScriptsOrShaders()
        {
            foreach(var root in EditorSceneManager.GetActiveScene().GetRootGameObjects())foreach(var t in root.GetComponentsInChildren<Transform>(true))
                Assert.That(GameObjectUtility.GetMonoBehavioursWithMissingScriptCount(t.gameObject),Is.Zero,t.name);
            foreach(var r in Object.FindObjectsOfType<Renderer>())foreach(var m in r.sharedMaterials)
                if(m)Assert.That(m.shader.name,Is.Not.EqualTo("Hidden/InternalErrorShader"),r.name);
        }
        [Test] public void ElevatorAndThresholdHaveContinuousGround()
        {
            Physics.SyncTransforms();
            foreach(float z in new[]{-8f,-7.2f,-6.4f,-5.6f,-4.8f})
                Assert.That(Physics.Raycast(new Vector3(-1,.14f,z),Vector3.down,.5f),Is.True,z.ToString());
        }
    }
}
