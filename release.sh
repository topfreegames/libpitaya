#!/bin/bash

if [[ ! "${VERSION}" =~ ^([0-9]+[.][0-9]+)[.]([0-9]+)(-(alpha|beta)[.]([0-9]+))?$ ]]; then
  echo "Version ${VERSION} must be 'X.Y.Z', 'X.Y.Z-alpha.N', or 'X.Y.Z-beta.N'"
  exit 1
fi

MAJOR=${BASH_REMATCH[0]}
MINOR=${BASH_REMATCH[1]}
PATCH=${BASH_REMATCH[2]}

cp include/* package/Native/include
cp -r ./unity/PitayaExample/Assets/Pitaya/Editor package
sed -i "s/RELEASE-VERSION/${VERSION}/g" package/package.json

cp libpitaya_android64/* package/Native/Android/arm64/
cp libpitaya_android/* package/Native/Android/armv7/
cp libpitaya_linux/* package/Native/Linux/
cp libpitaya_mac/* package/Native/Mac/
cp libpitaya_ios/* package/Native/iOS/
