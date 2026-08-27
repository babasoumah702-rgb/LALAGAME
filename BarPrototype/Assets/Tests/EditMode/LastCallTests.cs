using System.IO;
using NUnit.Framework;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using LastCall;

namespace BarPrototype.Tests
{
    public sealed class LastCallTests
    {
        [SetUp] public void OpenScene()
        {
            EditorSceneManager.OpenScene(LastCall.Editor.LastCallSceneBuilder.ScenePath);
        }
        [Test] public void LastCallHasSeparateSceneAndCharacterPrefab()
        {
            var game=Object.FindObjectOfType<LastCallGame>();
            Assert.That(game,Is.Not.Null);
            Assert.That(game.characterPrefab,Is.Not.Null);
            Assert.That(game.characterPrefab.GetComponent<CharacterController>(),Is.Not.Null);
            Assert.That(File.Exists("Assets/Scenes/AmberRoom.unity"),Is.True);
            Assert.That(File.Exists("Server/scenarios/navigation.json"),Is.True);
        }
        [Test] public void LastCallHasNoMissingScriptsOrMaterials()
        {
            foreach(var item in Object.FindObjectsOfType<GameObject>())
                Assert.That(GameObjectUtility.GetMonoBehavioursWithMissingScriptCount(item),Is.Zero,item.name);
            foreach(var renderer in Object.FindObjectsOfType<Renderer>())
                foreach(var material in renderer.sharedMaterials)
                {
                    Assert.That(material,Is.Not.Null,renderer.name);
                    Assert.That(material.shader,Is.Not.Null,renderer.name);
                    Assert.That(material.shader.name,Is.Not.EqualTo("Hidden/InternalErrorShader"),renderer.name);
                }
        }
        [Test] public void OutsideTerraceHasFloorAndBoundary()
        {
            Assert.That(GameObject.Find("Terrace floor").GetComponent<Collider>(),Is.Not.Null);
            Assert.That(GameObject.Find("Terrace left safety").GetComponent<Collider>(),Is.Not.Null);
            Physics.SyncTransforms();
            Assert.That(Physics.Raycast(new Vector3(-7.1f,.14f,-3.2f),Vector3.down,.5f),Is.True);
        }
        [Test] public void NullStateMessageIsNotAnActiveWorld()
        {
            var envelope=JsonUtility.FromJson<Envelope>("{\"type\":\"state\",\"version\":1,\"state\":null}");
            Assert.That(envelope.state==null||string.IsNullOrEmpty(envelope.state.sessionId),Is.True);
        }
    }
}
