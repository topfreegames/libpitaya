clean:
	@rm -rf Temp/bin/Debug

build-mac: clean
	@msbuild Pitaya.csproj  /p:DefineConstants="UNITY_STANDALONE_OSX"
	@cp Temp/bin/Debug/Pitaya.dll NugetOutput/lib/Pitaya-Mac.dll

build-ios: clean
	@msbuild Pitaya.csproj  /p:DefineConstants="UNITY_IPHONE"
	@cp Temp/bin/Debug/Pitaya.dll NugetOutput/lib/Pitaya-IOS.dll

build-linux: clean
	@msbuild Pitaya.csproj  /p:DefineConstants="UNITY_STANDALONE_LINUX"
	@cp Temp/bin/Debug/Pitaya.dll NugetOutput/lib/Pitaya-Linux.dll

build-android: clean
	@msbuild Pitaya.csproj  /p:DefineConstants="UNITY_ANDROID"
	@cp Temp/bin/Debug/Pitaya.dll NugetOutput/lib/Pitaya-Android.dll

build-all: build-mac build-ios build-linux build-android
	@mkdir -p NugetOutput/binaries
	@rm -rf NugetOutput/binaries/*
	@cp -r Assets/Pitaya/Native NugetOutput/binaries

pack: build-all
	@nuget pack NugetOutput/LibPitaya.nuspec -OutputDirectory NugetOutput