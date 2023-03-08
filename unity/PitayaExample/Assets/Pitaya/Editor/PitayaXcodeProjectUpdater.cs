#if UNITY_IOS || UNITY_EDITOR_OSX
using System.Collections.Generic;
using UnityEditor;
using UnityEditor.Callbacks;
using UnityEditor.iOS.Xcode;
using System;
using System.IO;
using System.Linq;

public class PitayaBuildPostprocessor
{
    private const string SIMULATOR_IDENTIFIER = "simulator";
    private const string DEVICE_IDENTIFIER = "device";
    private const string PITAYA_IOS_PATH = "PitayaNativeLibraries/iOS";

    [PostProcessBuild]
    public static void OnPostprocessBuild(BuildTarget buildTarget, string path)
    {
        if (buildTarget == BuildTarget.iOS)
        {
            var projPath = path + "/Unity-iPhone.xcodeproj/project.pbxproj";
            var currPath = Directory.GetCurrentDirectory();
            var lookupPaths = new string[]
            {
                Path.Combine(currPath, "Assets"), // relative to project path
                Path.Combine(currPath, "Library", "PackageCache"), // relative to project path
                Path.Combine(path, "Libraries") // relative to build path
            };

            PBXProject proj = new PBXProject();
            proj.ReadFromString(File.ReadAllText(projPath));

            var targetGuid = proj.GetUnityFrameworkTargetGuid();

            // Pitaya should be linked with zlib when on iOS.
            proj.AddBuildProperty(targetGuid, "OTHER_LDFLAGS", "-lz");

            var filesToRemove = new List<string>();
            foreach (var lookupPath in lookupPaths)
            {
                if (Directory.Exists(lookupPath)) {
                    filesToRemove
                    .AddRange(Directory.GetFiles(lookupPath, "*", SearchOption.AllDirectories)
                    .Where(f => f.Contains(GetPathToHide())));
                }
            }

            // To avoid referencing wrong libraries, the folder containing those
            // libraries should be hidden depending on which target the build
            // is running (device or simulator).
            foreach (var file in filesToRemove)
            {
                var relative = GetRelativePath(path + Path.DirectorySeparatorChar, file);
                var fileGuid = proj.FindFileGuidByRealPath(relative);
                if (fileGuid == null)
                {
                    fileGuid = proj.FindFileGuidByProjectPath(relative);
                }
                // Files will be kept in the build folder, but won't be referenced in the built project
                proj.RemoveFile(fileGuid);
            }

            File.WriteAllText(projPath, proj.WriteToString());
        }
    }

    private static string GetPathToHide()
    {
        if(PlayerSettings.iOS.sdkVersion == iOSSdkVersion.DeviceSDK)
        {
            return Path.Combine(PITAYA_IOS_PATH, SIMULATOR_IDENTIFIER);
        }

        return Path.Combine(PITAYA_IOS_PATH, DEVICE_IDENTIFIER);
    }

    private static string GetRelativePath(string relativeTo, string path)
    {
        var uri = new Uri(relativeTo);
        var rel = Uri.UnescapeDataString(uri.MakeRelativeUri(new Uri(path))
                        .ToString())
                        .Replace(Path.AltDirectorySeparatorChar, Path.DirectorySeparatorChar);
        if (rel.Contains(Path.DirectorySeparatorChar.ToString()) == false)
        {
            rel = $".{ Path.DirectorySeparatorChar }{ rel }";
        }
        return rel;
    }
}
#endif
