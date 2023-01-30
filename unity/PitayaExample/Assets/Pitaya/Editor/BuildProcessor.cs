using System.IO;
using UnityEditor;
using UnityEditor.Build;
using UnityEditor.Build.Reporting;
using UnityEditor.Build.Reporting;
using UnityEngine;

public class DeleteUnnecessaryResources : IPreprocessBuildWithReport, IPostprocessBuildWithReport
{
    private string IOS_LIB = "Assets/Pitaya/Native/iOS/";
    
    private const string IGNORE_MARK = "~";

    private const string SIMULATOR_IDENTIFIER = "simulator";
    
    private const string DEVICE_IDENTIFIER = "device";
 
    public int callbackOrder => default;
 
    public void OnPreprocessBuild(BuildReport report)
    {
        if (BuildTarget.iOS == EditorUserBuildSettings.activeBuildTarget)
        {
            string sourcePath = GetPathToHide();
            AssetDatabase.MoveAsset(sourcePath, sourcePath + IGNORE_MARK);
            AssetDatabase.Refresh(); 
        }
    }
 
    public void OnPostprocessBuild(BuildReport report)
    {
        if (BuildTarget.iOS == EditorUserBuildSettings.activeBuildTarget)
        {
            string sourcePath = GetPathToHide();
            AssetDatabase.MoveAsset(sourcePath + IGNORE_MARK, sourcePath);
            AssetDatabase.Refresh();
        }
    }

    private string GetPathToHide()
    {
        if(PlayerSettings.iOS.sdkVersion == iOSSdkVersion.DeviceSDK)
        {
            return IOS_LIB + SIMULATOR_IDENTIFIER;
        } else {
            return IOS_LIB + DEVICE_IDENTIFIER;
        }
    }
}
