using NUnit.Framework;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.Rendering;

namespace BarPrototype.Tests
{
    public sealed class SceneTests
    {
        [SetUp]
        public void OpenGeneratedScene()
        {
            EditorSceneManager.OpenScene(BarPrototype.Editor.BarSceneBuilder.ScenePath);
        }

        [Test]
        public void SceneHasOnePlayerAndAnActiveRenderPipeline()
        {
            Assert.That(Object.FindObjectsByType<PlayerMotor>(FindObjectsSortMode.None).Length, Is.EqualTo(1));
            Assert.That(GraphicsSettings.defaultRenderPipeline, Is.Not.Null);
            Assert.That(Camera.main, Is.Not.Null);
            Assert.That(Camera.main.orthographic, Is.True);
            var player = Object.FindFirstObjectByType<PlayerMotor>();
            Assert.That(Vector3.Distance(player.transform.position,new Vector3(.1f,.05f,-2.35f)),Is.LessThan(.01f));
        }

        [Test]
        public void PlayerPrefabHasControllerAndPoseReferences()
        {
            var prefab = AssetDatabase.LoadAssetAtPath<GameObject>("Assets/Prefabs/Player.prefab");
            Assert.That(prefab, Is.Not.Null);
            Assert.That(prefab.GetComponent<CharacterController>(), Is.Not.Null);
            var pose = prefab.GetComponent<CharacterPose>();
            Assert.That(pose.motor, Is.Not.Null);
            Assert.That(pose.leftLeg && pose.rightLeg && pose.leftArm && pose.rightArm && pose.torso, Is.True);
        }

        [Test]
        public void SceneHasNoMissingScriptsOrMaterials()
        {
            var objects = Object.FindObjectsByType<GameObject>(FindObjectsSortMode.None);
            foreach (var item in objects)
                Assert.That(GameObjectUtility.GetMonoBehavioursWithMissingScriptCount(item), Is.Zero, item.name);
            foreach (var renderer in Object.FindObjectsByType<Renderer>(FindObjectsSortMode.None))
                foreach (var material in renderer.sharedMaterials)
                {
                    Assert.That(material, Is.Not.Null, renderer.name);
                    Assert.That(material.shader, Is.Not.Null, renderer.name);
                    Assert.That(material.shader.name, Is.Not.EqualTo("Hidden/InternalErrorShader"), renderer.name);
                }
            Assert.That(Object.FindObjectsByType<Collider>(FindObjectsSortMode.None).Length, Is.GreaterThan(20));
        }

        [TestCase(16f / 9)]
        [TestCase(16f / 10)]
        public void RoomFitsInCamera(float aspect)
        {
            var camera = Camera.main;
            var framing = camera.GetComponent<FixedRoomCamera>();
            camera.aspect = aspect;
            camera.orthographicSize = Mathf.Max(framing.halfHeight, framing.minimumHalfWidth / aspect);
            foreach (float x in new[] { -6.25f, 6.25f })
            foreach (float z in new[] { -5.25f, 5.25f })
            {
                var floor = camera.WorldToViewportPoint(new Vector3(x, -.46f, z));
                Assert.That(floor.x, Is.InRange(.01f, .99f));
                Assert.That(floor.y, Is.InRange(.01f, .99f));
            }
            var top = camera.WorldToViewportPoint(new Vector3(6.25f, 3.5f, 5.25f));
            Assert.That(top.y, Is.LessThan(.99f));
        }
    }
}
