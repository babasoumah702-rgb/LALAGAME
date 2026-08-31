using System;
using System.IO;
using UnityEditor;
using UnityEditor.Build;
using UnityEditor.Build.Reporting;
using UnityEditor.SceneManagement;
using UnityEngine;

namespace LastCall.Editor
{
    public static class LalalandReleaseBuilder
    {
        private const string MacDestination = "Builds/Lalaland-macOS-Player/Lalaland.app";

        [MenuItem("Lalaland/Build macOS Player")]
        public static void BuildMacOS()
        {
            if (!File.Exists(LastCallSceneBuilder.ScenePath))
                throw new FileNotFoundException("Lalaland scene is missing.", LastCallSceneBuilder.ScenePath);

            EditorSceneManager.OpenScene(LastCallSceneBuilder.ScenePath);
            ConfigurePlayer();
            Directory.CreateDirectory(Path.GetDirectoryName(MacDestination));

            var report = BuildPipeline.BuildPlayer(new BuildPlayerOptions
            {
                scenes = new[] { LastCallSceneBuilder.ScenePath },
                locationPathName = MacDestination,
                target = BuildTarget.StandaloneOSX,
                options = BuildOptions.None
            });

            if (report.summary.result != BuildResult.Succeeded)
                throw new Exception("Lalaland macOS build failed: " + report.summary.result);

            Debug.Log("LALALAND_MACOS_BUILD_READY " + Path.GetFullPath(MacDestination));
        }

        private static void ConfigurePlayer()
        {
            PlayerSettings.productName = "Lalaland";
            PlayerSettings.bundleVersion = "0.3.0";
            PlayerSettings.defaultScreenWidth = 1280;
            PlayerSettings.defaultScreenHeight = 720;
            PlayerSettings.resizableWindow = true;
            PlayerSettings.insecureHttpOption = InsecureHttpOption.AlwaysAllowed;
            PlayerSettings.SetApplicationIdentifier(NamedBuildTarget.Standalone, "com.amberroom.lalaland");
            PlayerSettings.SetScriptingBackend(NamedBuildTarget.Standalone, ScriptingImplementation.Mono2x);
            PlayerSettings.SetApiCompatibilityLevel(NamedBuildTarget.Standalone, ApiCompatibilityLevel.NET_Standard);
            PlayerSettings.SetArchitecture(NamedBuildTarget.Standalone, 2);
        }
    }
}
