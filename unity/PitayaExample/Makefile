# UNITY_PATH should contain the path to the Unity.app folder

ifeq ($(OS),Windows_NT)
	UNITY_PATH?=C:/Program Files/Unity/Hub/Editor/2021.3.13f1/Editor
	NET_STANDARD_VER?=2.1.0
	CONTENTS_DIR?=Data
	MSBUILD?=MSBuild.exe
else
	UNITY_PATH?=/Applications/Unity/Hub/Editor/2019.4.40f1/Unity.app
	NET_STANDARD_VER?=2.0.0
	CONTENTS_DIR?=Contents
	MSBUILD?=msbuild
endif



clean:
	@rm -rf Temp/bin/Release
	@rm -rf obj
	@rm -rf NugetOutput/*.nupkg
	@rm -rf NugetOutput/binaries

build-mac: clean
	@${MSBUILD} Pitaya-Build.csproj /t:Rebuild /p:DefineConstants="UNITY_STANDALONE_OSX" /p:Configuration=Release
	@mkdir -p NugetOutput/lib/netstandard2.0/mac
	@cp Temp/bin/Release/Pitaya.dll NugetOutput/lib/netstandard2.0/mac/Pitaya.dll

build-win: clean
	@${MSBUILD} Pitaya-Build.csproj /t:Rebuild /p:DefineConstants="UNITY_STANDALONE_WIN" /p:Configuration=Release
	@mkdir -p NugetOutput/lib/netstandard2.0/win
	@cp Temp/bin/Release/Pitaya.dll NugetOutput/lib/netstandard2.0/win/Pitaya.dll

build-ios: clean
	@${MSBUILD} Pitaya-Build.csproj /t:Rebuild /p:DefineConstants="UNITY_IPHONE" /p:Configuration=Release
	@mkdir -p NugetOutput/lib/netstandard2.0/ios
	@cp Temp/bin/Release/Pitaya.dll NugetOutput/lib/netstandard2.0/ios/Pitaya.dll

build-linux: clean
	@${MSBUILD} Pitaya-Build.csproj /t:Rebuild /p:DefineConstants="UNITY_STANDALONE_LINUX" /p:Configuration=Release
	@mkdir -p NugetOutput/lib/netstandard2.0/linux
	@cp Temp/bin/Release/Pitaya.dll NugetOutput/lib/netstandard2.0/linux/Pitaya.dll

build-android: clean
	@${MSBUILD} Pitaya-Build.csproj /t:Rebuild /p:DefineConstants="UNITY_ANDROID" /p:Configuration=Release
	@mkdir -p NugetOutput/lib/netstandard2.0/android
	@cp Temp/bin/Release/Pitaya.dll NugetOutput/lib/netstandard2.0/android/Pitaya.dll

build-all: build-template build-mac build-ios build-linux build-android build-win
	@cp -r Assets/Pitaya/Native NugetOutput/binaries

pack: build-all
	@nuget pack LibPitaya.nuspec -OutputDirectory NugetOutput

test:
	${UNITY_PATH}/Contents/MacOS/Unity -runTests -projectPath $(shell pwd) -testResults $(shell pwd)/test-results-playmode.xml -testPlatform playmode -batchmode -nographics -logFile

build-template:
	@echo "Configuring build template to use ${UNITY_VERSION}"
	@sed 's~UNITY_PATH~${UNITY_PATH}~g; s~NET_STANDARD_VER~${NET_STANDARD_VER}~g; s~CONTENTS_DIR~${CONTENTS_DIR}~g' Pitaya-Build-Template.csproj > Pitaya-Build.csproj


