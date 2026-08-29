using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEditor.Animations;
using UnityEditor.Callbacks;
using UnityEngine;

namespace LastCall.Editor
{
    /// <summary>Imports and reports the four replacement cast rigs before the shared controller is built.</summary>
    public static class HumanoidAnimationBuilder
    {
        private static readonly string[] Cast = { "A", "B", "C", "D" };
        private const string Output = "Assets/LastCall/SceneZero/Animation";
        private const string ControllerPath = Output + "/SharedCast.controller";

        [DidReloadScripts]
        private static void AfterReload()
        {
            EditorApplication.delayCall += () =>
            {
                if (NeedsBuild()) Build();
                else Report();
            };
        }

        private static bool NeedsBuild()
        {
            if(!AssetDatabase.LoadAssetAtPath<AnimatorController>(ControllerPath))return true;
            return Cast.Any(id=>
            {
                var prefab=AssetDatabase.LoadAssetAtPath<GameObject>("Assets/LastCall/SceneZero/Models/"+id+".prefab");
                var animator=prefab?prefab.GetComponent<Animator>():null;
                return !animator||!animator.avatar||!animator.avatar.isValid||!animator.avatar.isHuman||!animator.runtimeAnimatorController;
            });
        }

        [MenuItem("Last Call/Animation/1 - Inspect A-D rigs")]
        public static void Report()
        {
            foreach (var id in Cast)
            {
                var folder = "Assets/LastCall/Characters/" + id;
                var path = AssetDatabase.FindAssets("t:Model", new[] { folder })
                    .Select(AssetDatabase.GUIDToAssetPath)
                    .FirstOrDefault(p => p.EndsWith(".fbx", StringComparison.OrdinalIgnoreCase));
                if (string.IsNullOrEmpty(path))
                {
                    Debug.LogError("CAST_RIG missing actor=" + id);
                    continue;
                }

                var importer = AssetImporter.GetAtPath(path) as ModelImporter;
                var avatar = AssetDatabase.LoadAllAssetsAtPath(path).OfType<Avatar>().FirstOrDefault();
                var clips = AssetDatabase.LoadAllAssetsAtPath(path).OfType<AnimationClip>()
                    .Where(c => !c.name.StartsWith("__preview__", StringComparison.OrdinalIgnoreCase)).ToArray();
                var takes = importer == null ? Array.Empty<ModelImporterClipAnimation>() : importer.defaultClipAnimations;
                Debug.Log("CAST_RIG actor=" + id +
                    " path=" + path +
                    " humanoid=" + (avatar && avatar.isHuman) +
                    " avatarValid=" + (avatar && avatar.isValid) +
                    " takeInfo=" + string.Join(",", takes.Select(t => t.name + "[" + t.firstFrame + "-" + t.lastFrame + "]")) +
                    " clips=" + string.Join(",", clips.Select(c => c.name + "[" + c.length.ToString("0.00") + "s,human=" + c.humanMotion + "]")));
            }
        }

        [MenuItem("Last Call/Animation/2 - Build A-D shared controller")]
        public static void Build()
        {
            EnsureFolder(Output);
            var clips = new Dictionary<string, Dictionary<string, AnimationClip>>();
            var avatars = new Dictionary<string, Avatar>();
            foreach (var id in Cast)
            {
                var path = ModelPath(id);
                var importer = AssetImporter.GetAtPath(path) as ModelImporter;
                if (importer == null) throw new InvalidOperationException("Missing ModelImporter for " + id);
                var defaults = importer.defaultClipAnimations
                    .Where(c => c.takeName.StartsWith("preset:biped:", StringComparison.OrdinalIgnoreCase) &&
                                c.takeName.EndsWith(".001", StringComparison.OrdinalIgnoreCase) &&
                                c.takeName.IndexOf('|') < 0).ToArray();
                if (defaults == null || defaults.Length == 0)
                    throw new InvalidOperationException("The replacement FBX has no animation take: " + id);
                for (var i = 0; i < defaults.Length; i++)
                {
                    defaults[i].name = defaults[i].takeName.Substring("preset:biped:".Length)
                        .Replace(".001", string.Empty);
                    var loop = IsLoop(defaults[i].name);
                    defaults[i].loopTime = loop;
                    defaults[i].loopPose = loop;
                    defaults[i].lockRootHeightY = true;
                    defaults[i].lockRootPositionXZ = true;
                    defaults[i].lockRootRotation = true;
                    defaults[i].keepOriginalPositionY = false;
                    defaults[i].keepOriginalPositionXZ = false;
                    defaults[i].keepOriginalOrientation = false;
                }
                importer.animationType = ModelImporterAnimationType.Human;
                importer.avatarSetup = ModelImporterAvatarSetup.CreateFromThisModel;
                importer.importAnimation = true;
                var description = importer.humanDescription;
                description.human = HumanMap();
                importer.humanDescription = description;
                importer.clipAnimations = defaults;
                importer.SaveAndReimport();
                var assets = AssetDatabase.LoadAllAssetsAtPath(path);
                clips[id] = assets.OfType<AnimationClip>()
                    .Where(c => defaults.Any(d => d.name == c.name)).GroupBy(c => c.name)
                    .ToDictionary(g => g.Key, g => g.First());
                avatars[id] = assets.OfType<Avatar>().FirstOrDefault();
                if (!clips[id].ContainsKey("walk")) throw new InvalidOperationException("Walk clip was not imported for " + id);
                if (!avatars[id] || !avatars[id].isValid || !avatars[id].isHuman)
                    throw new InvalidOperationException("Invalid Humanoid avatar for " + id);
            }

            if (AssetDatabase.LoadAssetAtPath<AnimatorController>(ControllerPath))
                AssetDatabase.DeleteAsset(ControllerPath);
            var controller = AnimatorController.CreateAnimatorControllerAtPath(ControllerPath);
            controller.AddParameter("MotionSpeed", AnimatorControllerParameterType.Float);
            controller.AddParameter("Speaking", AnimatorControllerParameterType.Float);
            var machine = controller.layers[0].stateMachine;
            Add(machine,"IdleA",Pick(clips["A"],"standing_relax","wait"));
            Add(machine,"IdleB",Pick(clips["B"],"standing_relax","wait"));
            Add(machine,"IdleC",Pick(clips["C"],"idle","standing_relax","wait"));
            Add(machine,"IdleD",Pick(clips["D"],"wait","look_around"));
            Add(machine,"Walk",Pick(clips["C"],"walk"));
            Add(machine,"Sit",Pick(clips["A"],"sit"));
            Add(machine,"Phone",Pick(clips["B"],"make_a_call_01","play_mobile_game"));
            Add(machine,"Look",Pick(clips["A"],"look_around","wait"));
            Add(machine,"TalkA",Pick(clips["A"],"agree","wave_goodbye_02"));
            Add(machine,"TalkB",Pick(clips["B"],"greet_04","greet_01","agree"));
            Add(machine,"TalkC",Pick(clips["C"],"agree","greet_02"));
            Add(machine,"TalkD",Pick(clips["D"],"greet_01","laugh_02","agree"));
            Add(machine,"Dance",Pick(clips["D"],"sing_01","sing_02"));
            machine.defaultState = machine.states.First(s=>s.state.name=="IdleA").state;

            foreach (var id in Cast)
            {
                var obsolete=Output+"/"+id+" Cast.overrideController";
                if(AssetDatabase.LoadAssetAtPath<AnimatorOverrideController>(obsolete))AssetDatabase.DeleteAsset(obsolete);
                ConfigurePrefab(id, avatars[id], controller);
            }
            AssetDatabase.SaveAssets();
            AssetDatabase.Refresh();
            Report();
            Debug.Log("CAST_ANIMATION_READY controller=" + ControllerPath);
        }

        /// <summary>Command-line entry point used by release builds.</summary>
        public static void BuildForBatch()
        {
            Build();
        }

        private static void ConfigurePrefab(string id, Avatar avatar, RuntimeAnimatorController controller)
        {
            var path = "Assets/LastCall/SceneZero/Models/" + id + ".prefab";
            var root = PrefabUtility.LoadPrefabContents(path);
            try
            {
                var animator = root.GetComponent<Animator>();
                if(!animator) animator=root.AddComponent<Animator>();
                animator.avatar = avatar;
                animator.runtimeAnimatorController = controller;
                animator.applyRootMotion = false;
                animator.updateMode = AnimatorUpdateMode.Normal;
                animator.cullingMode = AnimatorCullingMode.AlwaysAnimate;
                PrefabUtility.SaveAsPrefabAsset(root, path);
            }
            finally { PrefabUtility.UnloadPrefabContents(root); }
        }

        private static void Add(AnimatorStateMachine machine,string name,AnimationClip clip)
        {
            if(!clip)throw new InvalidOperationException("Missing controller motion " + name);
            var state=machine.AddState(name);state.motion=clip;state.speed=1;
        }

        private static AnimationClip Pick(Dictionary<string,AnimationClip> clips,params string[] names)
        {
            foreach(var name in names)if(clips.TryGetValue(name,out var clip)&&clip)return clip;
            return null;
        }

        private static bool IsLoop(string name)
        {
            return name=="walk"||name=="run"||name=="wait"||name=="idle"||
                   name=="standing_relax"||name=="look_around"||name=="play_mobile_game"||
                   name.StartsWith("make_a_call",StringComparison.OrdinalIgnoreCase);
        }

        private static HumanBone[] HumanMap()
        {
            var map = new Dictionary<string,string>
            {
                {"Hips","Hip"},{"Spine","Waist"},{"Chest","Spine01"},{"UpperChest","Spine02"},
                {"Neck","NeckTwist01"},{"Head","Head"},
                {"LeftShoulder","L_Clavicle"},{"LeftUpperArm","L_Upperarm"},{"LeftLowerArm","L_Forearm"},{"LeftHand","L_Hand"},
                {"RightShoulder","R_Clavicle"},{"RightUpperArm","R_Upperarm"},{"RightLowerArm","R_Forearm"},{"RightHand","R_Hand"},
                {"LeftUpperLeg","L_Thigh"},{"LeftLowerLeg","L_Calf"},{"LeftFoot","L_Foot"},{"LeftToes","L_ToeBase"},
                {"RightUpperLeg","R_Thigh"},{"RightLowerLeg","R_Calf"},{"RightFoot","R_Foot"},{"RightToes","R_ToeBase"}
            };
            return map.Select(pair=>new HumanBone
            {
                humanName=pair.Key,boneName=pair.Value,limit=new HumanLimit{useDefaultValues=true}
            }).ToArray();
        }

        private static string ModelPath(string id)
        {
            var path = AssetDatabase.FindAssets("t:Model", new[] { "Assets/LastCall/Characters/" + id })
                .Select(AssetDatabase.GUIDToAssetPath)
                .FirstOrDefault(p => p.EndsWith(".fbx", StringComparison.OrdinalIgnoreCase));
            if (string.IsNullOrEmpty(path)) throw new InvalidOperationException("Missing replacement FBX: " + id);
            return path;
        }

        private static void EnsureFolder(string path)
        {
            var current = "Assets";
            foreach (var part in path.Split('/').Skip(1))
            {
                var next = current + "/" + part;
                if (!AssetDatabase.IsValidFolder(next)) AssetDatabase.CreateFolder(current, part);
                current = next;
            }
        }
    }
}
