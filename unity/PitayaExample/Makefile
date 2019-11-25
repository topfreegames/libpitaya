clean:
	@rm -rf Temp/bin/Release
	@rm -rf obj
	@rm -rf NugetOutput/*.nupkg
	@rm -rf NugetOutput/binaries

build-mac: clean
	@msbuild Pitaya-Build.csproj /t:Rebuild /p:DefineConstants="UNITY_STANDALONE_OSX" /p:Configuration=Release
	@cp Temp/bin/Release/Pitaya.dll NugetOutput/lib/mac/Pitaya.dll

build-win: clean
	@msbuild Pitaya-Build.csproj /t:Rebuild /p:DefineConstants="UNITY_STANDALONE_WIN" /p:Configuration=Release
	@cp Temp/bin/Release/Pitaya.dll NugetOutput/lib/win/Pitaya.dll

build-ios: clean
	@msbuild Pitaya-Build.csproj /t:Rebuild /p:DefineConstants="UNITY_IPHONE" /p:Configuration=Release
	@cp Temp/bin/Release/Pitaya.dll NugetOutput/lib/ios/Pitaya.dll

build-linux: clean
	@msbuild Pitaya-Build.csproj /t:Rebuild /p:DefineConstants="UNITY_STANDALONE_LINUX" /p:Configuration=Release
	@cp Temp/bin/Release/Pitaya.dll NugetOutput/lib/linux/Pitaya.dll

build-android: clean
	@msbuild Pitaya-Build.csproj /t:Rebuild /p:DefineConstants="UNITY_ANDROID" /p:Configuration=Release
	@cp Temp/bin/Release/Pitaya.dll NugetOutput/lib/android/Pitaya.dll

build-all: build-template build-mac build-ios build-linux build-android build-win
	@cp -r Assets/Pitaya/Native NugetOutput/binaries

pack: build-all
	@nuget pack LibPitaya.nuspec -OutputDirectory NugetOutput

UNITY_VERSION=Unity201836f1

test:
	/Applications/${UNITY_VERSION}/Unity.app/Contents/MacOS/Unity -runTests -projectPath $(shell pwd) -testResults $(shell pwd)/test-results-playmode.xml -testPlatform playmode -batchmode -nographics -logFile

UNITY_VERSION=

build-template:
	@sed 's/UNITY_VERSION/${UNITY_VERSION}/g' Pitaya-Build-Template.csproj > Pitaya-Build.csproj

