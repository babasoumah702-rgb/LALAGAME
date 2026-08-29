using System.Linq;
using LastCall;
using LastCall.Editor;
using NUnit.Framework;
using UnityEditor;
using UnityEditor.Animations;
using UnityEngine;

namespace BarPrototype.Tests
{
    public class HumanoidCastAnimationTests
    {
        [OneTimeSetUp]
        public void BuildAnimationAssets()
        {
            HumanoidAnimationBuilder.Build();
        }

        [Test]
        public void ReplacementPrefabsHaveValidHumanoidAvatarsAndRealAnchors()
        {
            foreach(var id in new[]{"A","B","C","D"})
            {
                var prefab=AssetDatabase.LoadAssetAtPath<GameObject>("Assets/LastCall/SceneZero/Models/"+id+".prefab");
                Assert.That(prefab,Is.Not.Null,id);
                Assert.That(prefab.GetComponentInChildren<SkinnedMeshRenderer>(),Is.Not.Null,id);
                var animator=prefab.GetComponent<Animator>();
                Assert.That(animator,Is.Not.Null,id);
                Assert.That(animator.avatar,Is.Not.Null,id);
                Assert.That(animator.avatar.isValid&&animator.avatar.isHuman,Is.True,id);
                Assert.That(animator.runtimeAnimatorController,Is.Not.Null,id);
                Assert.That(animator.GetBoneTransform(HumanBodyBones.Head),Is.Not.Null,id+" head");
                Assert.That(animator.GetBoneTransform(HumanBodyBones.LeftHand),Is.Not.Null,id+" left hand");
                Assert.That(animator.GetBoneTransform(HumanBodyBones.RightHand),Is.Not.Null,id+" right hand");
                Assert.That(HumanoidCastAnimator.Supports(id),Is.True,id);
            }
            Assert.That(HumanoidCastAnimator.Supports("BARTENDER"),Is.False);
        }

        [Test]
        public void SharedControllerContainsLocomotionConversationAndStoryStates()
        {
            var controller=AssetDatabase.LoadAssetAtPath<AnimatorController>("Assets/LastCall/SceneZero/Animation/SharedCast.controller");
            Assert.That(controller,Is.Not.Null);
            var states=controller.layers[0].stateMachine.states.Select(s=>s.state.name).ToArray();
            foreach(var required in new[]{"IdleA","IdleB","IdleC","IdleD","Walk","Sit","Phone","Look","TalkA","TalkB","TalkC","TalkD","Dance"})
                Assert.That(states,Contains.Item(required),required);
        }

        [Test]
        public void OriginalReplacementTakesWereImportedIndividually()
        {
            var minimum=new[]{12,16,25,21};var ids=new[]{"A","B","C","D"};
            for(var i=0;i<ids.Length;i++)
            {
                var folder="Assets/LastCall/Characters/"+ids[i];
                var path=AssetDatabase.FindAssets("t:Model",new[]{folder}).Select(AssetDatabase.GUIDToAssetPath).First(p=>p.EndsWith(".fbx"));
                var names=AssetDatabase.LoadAllAssetsAtPath(path).OfType<AnimationClip>().Select(c=>c.name).Distinct().ToArray();
                Assert.That(names.Length,Is.GreaterThanOrEqualTo(minimum[i]),ids[i]);
                Assert.That(names,Contains.Item("walk"),ids[i]);
                Assert.That(names,Contains.Item("sit"),ids[i]);
            }
        }
    }
}
