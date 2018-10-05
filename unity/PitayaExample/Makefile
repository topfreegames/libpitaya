clean:
	@rm -rf Temp/bin/Release
	@rm -rf obj

build-mac: clean
	@msbuild Pitaya.csproj /t:Rebuild /p:DefineConstants="UNITY_STANDALONE_OSX" /p:Configuration=Release 
	@cp Temp/bin/Release/Pitaya.dll NugetOutput/lib/mac/Pitaya.dll

build-ios: clean
	@msbuild Pitaya.csproj /t:Rebuild /p:DefineConstants="UNITY_IPHONE" /p:Configuration=Release
	@cp Temp/bin/Release/Pitaya.dll NugetOutput/lib/ios/Pitaya.dll

build-linux: clean
	@msbuild Pitaya.csproj /t:Rebuild /p:DefineConstants="UNITY_STANDALONE_LINUX" /p:Configuration=Release
	@cp Temp/bin/Release/Pitaya.dll NugetOutput/lib/linux/Pitaya.dll

build-android: clean
	@msbuild Pitaya.csproj /t:Rebuild /p:DefineConstants="UNITY_ANDROID" /p:Configuration=Release
	@cp Temp/bin/Release/Pitaya.dll NugetOutput/lib/android/Pitaya.dll

build-all: build-mac build-ios build-linux build-android
	@mkdir -p NugetOutput/binaries
	@rm -rf NugetOutput/binaries/*
	@cp -r Assets/Pitaya/Native NugetOutput/binaries

pack: build-all
	@nuget pack NugetOutput/LibPitaya.nuspec -OutputDirectory NugetOutput