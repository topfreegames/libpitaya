#!/bin/bash

if [[ ! "${VERSION}" =~ ^([0-9]+)[.]([0-9]+)[.]([0-9]+)(-(alpha|beta)[.]([0-9]+))?$ ]]; then
  echo "Version ${VERSION} must be 'X.Y.Z', 'X.Y.Z-alpha.N', or 'X.Y.Z-beta.N'"
  exit 1
fi

MAJOR=${BASH_REMATCH[1]}
MINOR=${BASH_REMATCH[2]}
PATCH=${BASH_REMATCH[3]}
SUFFIX=${BASH_REMATCH[4]}

sed -i "s/PC_VERSION_MAJOR.*/PC_VERSION_MAJOR $MAJOR/g" include/pitaya_version.h 
sed -i "s/PC_VERSION_MINOR.*/PC_VERSION_MINOR $MINOR/g" include/pitaya_version.h 
sed -i "s/PC_VERSION_REVISION.*/PC_VERSION_REVISION $PATCH/g" include/pitaya_version.h
sed -i "s/PC_VERSION_SUFFIX.*/PC_VERSION_SUFFIX \"${SUFFIX}\"/g" include/pitaya_version.h

sed -i "s/RELEASE-VERSION/${VERSION}/g" package/package.json
sed -i "s/RELEASE-VERSION/${VERSION}/g" unity/PitayaExample/LibPitaya.nuspec 

cp include/* package/Native/include
cp -r ./unity/PitayaExample/Assets/Pitaya/Editor package

cp libpitaya_android64/* package/Native/Android/arm64/
cp libpitaya_android/* package/Native/Android/armv7/
cp libpitaya_linux/* package/Native/Linux/
cp libpitaya_mac/* package/Native/Mac/
find libpitaya_ios -type f -exec cp "{}" package/Native/iOS/ \;
