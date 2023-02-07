#if UNITY_IOS || UNITY_EDITOR
using UnityEditor;
using UnityEditor.Callbacks;
using UnityEditor.iOS.Xcode;
using System.IO;

public class PitayaBuildPostprocessor
{
	private const string SIMULATOR_IDENTIFIER = "simulator";
	private const string DEVICE_IDENTIFIER = "device";
	private const string FRAMEWORK_ORIGIN_PATH = "Assets/Pitaya/Native/iOS";
	private const string FRAMEWORK_UPM_ORIGIN_PATH = "Libraries/com.wildlifestudios.nuget.libpitaya/Native/iOS";

	[PostProcessBuild]
	public static void OnPostprocessBuild(BuildTarget buildTarget, string path)
	{
		if (buildTarget == BuildTarget.iOS)
		{
			var projPath = path + "/Unity-iPhone.xcodeproj/project.pbxproj";

			PBXProject proj = new PBXProject();
			proj.ReadFromString(File.ReadAllText(projPath));

			var targetGuid = proj.GetUnityFrameworkTargetGuid();

			// Pitaya should be linked with zlib when on iOS.
			proj.AddBuildProperty(targetGuid, "OTHER_LDFLAGS", "-lz");
			
			// To avoid referencing wrong libraries, the folder containing those
			// libraries should be hidden depending on which target the build
			// is running (device or simulator).
			foreach (var file in Directory.GetFiles(GetPathToHide(path))) {
				var fileGuid = proj.FindFileGuidByProjectPath(file.Substring(path.Length + 1));
				// Files will be kept in the build folder, but won't be referenced in the built project
				proj.RemoveFile(fileGuid);
			}

			File.WriteAllText(projPath, proj.WriteToString());
		}
	}

	private static string GetPathToHide(string projectPath)
	{
		var originPath = Path.Combine(projectPath, FRAMEWORK_ORIGIN_PATH);
		if (!Directory.Exists(originPath))
		{
			originPath = Path.Combine(projectPath, FRAMEWORK_UPM_ORIGIN_PATH);
		}

		if(PlayerSettings.iOS.sdkVersion == iOSSdkVersion.DeviceSDK)
		{
			return Path.Combine(originPath, SIMULATOR_IDENTIFIER);
		}

		return Path.Combine(originPath, DEVICE_IDENTIFIER);
	}
}
#endif
