if [[ ! "${VERSION}" =~ ^([0-9]+[.][0-9]+)[.]([0-9]+)(-(alpha|beta)[.]([0-9]+))?$ ]]; then
  echo "Version ${VERSION} must be 'X.Y.Z', 'X.Y.Z-alpha.N', or 'X.Y.Z-beta.N'"
  exit 1
fi

MAJOR=${BASH_REMATCH[0]}
MINOR=${BASH_REMATCH[1]}
PATCH=${BASH_REMATCH[2]}

cp include/* nuget/Native/include
cp -r ./unity/PitayaExample/Assets/Pitaya/Editor nuget
sed -i "s/RELEASE-VERSION/${VERSION}/g" nuget/package.json

cp _builds/android64/libpitaya-android.so nuget/Native/Android/arm64/libpitaya-android.so
cp _builds/android/libpitaya-android.so nuget/Native/Android/armv7/libpitaya-android.so
cp _builds/linux/libpitaya-linux.so nuget/Native/Linux/libpitaya-android.so
cp _builds/mac-xcode/libpitaya-mac.bundle nuget/Native/Mac/libpitaya-mac.bundle
cp _builds/ios/Release-iphoneos/libpitaya-ios.a nuget/Native/iOS/libpitaya-ios.a
cp _builds/ios/deps/libuv-v1.44.2/Release-iphoneos/libuv_a.a nuget/Native/iOS/libuv_a.a
