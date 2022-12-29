#if UNITY_IPHONE
using UnityEditor;
using UnityEditor.Callbacks;
using UnityEditor.iOS.Xcode;
using System.IO;

public class PitayaBuildPostprocessor
{
	[PostProcessBuild]
	public static void OnPostprocessBuild(BuildTarget buildTarget, string path)
	{
		if (buildTarget == BuildTarget.iOS)
		{
			string projPath = path + "/Unity-iPhone.xcodeproj/project.pbxproj";

			PBXProject proj = new PBXProject();
			proj.ReadFromString(File.ReadAllText(projPath));

			string targetGuid = proj.GetUnityFrameworkTargetGuid();

			// Pitaya should be linked with zlib when on iOS.
			proj.AddBuildProperty(targetGuid, "OTHER_LDFLAGS", "-lz");

			File.WriteAllText(projPath, proj.WriteToString());
		}
	}
}
#endif
