ANDROID_TOOLCHAIN_FILE ?= temp/android-ndk-r25b/build/cmake/android.toolchain.cmake

setup-mac:
	@brew install ninja cmake

setup-android-mac:
	@mkdir -p temp
	@curl https://dl.google.com/android/repository/android-ndk-r25b-darwin.zip -o temp/android-ndk-r25b.zip
	@unzip -qo temp/android-ndk-r25b.zip -d temp/

setup-android-linux:
	@mkdir -p temp
	@curl https://dl.google.com/android/repository/android-ndk-r25b-linux.zip -o temp/android-ndk-r25b.zip
	@unzip -qo temp/android-ndk-r25b.zip -d temp/

build-android:
	@rm -rf _builds/android
	@cmake -GNinja -H. -B_builds/android -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release \
				   -DCMAKE_TOOLCHAIN_FILE=${ANDROID_TOOLCHAIN_FILE} \
					 -DANDROID_PLATFORM=android-23
				   -DANDROID_ABI=armeabi-v7a
	@cmake --build _builds/android

build-android-64:
	@rm -rf _builds/android64
	@cmake -GNinja -H. -B_builds/android64 -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release \
				   -DCMAKE_TOOLCHAIN_FILE=${ANDROID_TOOLCHAIN_FILE} \
					 -DANDROID_PLATFORM=android-23
				   -DANDROID_ABI=arm64-v8a
	@cmake --build _builds/android64

build-windows:
	@echo "Attention: This should be run from Developer PowerShell for VS, else it will fail"
	@rm -rf _builds/windows
	@cmake -H. -B_builds/windows -G "Visual Studio 17 2022" -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release
	@cmake --build  _builds/windows --config Release -j

build-mac-universal:
	@rm -rf _builds/mac
	@cmake -H. -B_builds/mac -GNinja -DPLATFORM=MAC -DCMAKE_BUILD_TYPE=Release -DBUILD_MACOS_BUNDLE=ON
	@cmake --build _builds/mac -j

build-mac-xcode:
	@rm -rf _builds/mac-xcode
	@cmake -H. -B_builds/mac-xcode -GXcode -DBUILD_MACOS_BUNDLE=ON
	@cmake --build _builds/mac-xcode --config Release -j

build-linux:
	@rm -rf _builds/linux
	@cmake -H. -B_builds/linux -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON
	@cmake --build _builds/linux -j

build-linux-debug:
	@rm -rf _builds/linux-debug
	@cmake -H. -B_builds/linux-debug -GNinja -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=ON
	@cmake --build _builds/linux-debug -j

check-devteam-env:
ifndef APPLE_DEVELOPMENT_TEAM
	$(error APPLE_DEVELOPMENT_TEAM is undefined)
endif

## Needs development team for building iOS fat
build-ios-fat: check-devteam-env
	@rm -rf _builds/ios_combined
	@cmake -H. -GXcode -B_builds/ios_combined -DCMAKE_INSTALL_PREFIX=./_builds/ios_combined -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=$(APPLE_DEVELOPMENT_TEAM) -DCMAKE_BUILD_TYPE=Release -DPLATFORM=OS64COMBINED -DCMAKE_TOOLCHAIN_FILE=../../cmake/ios.toolchain.cmake
	@cmake --build _builds/ios_combined --config Release -j

build-openssl-ios-simulator64:
	@rm -rf _builds/openssl/simulator
	@rm -rf _builds/tmp _builds/ios-xcframework-openssl/
	@echo "Building simulator lib"
	@./build-openssl.py --prefix _builds/openssl/simulator --force --openssl-tar ./deps/openssl-1.1.1q.tar.gz ios-sim-universal
	@echo "Done"

build-openssl-ios-xcframework:
	@rm -rf _builds/openssl/simulator
	@rm -rf _builds/openssl/device
	@rm -rf _builds/tmp _builds/ios-xcframework-openssl/
	@echo "Building simulator lib"
	@./build-openssl.py --prefix _builds/openssl/simulator --force --openssl-tar ./deps/openssl-1.1.1q.tar.gz ios-sim-universal
	@echo "Building arm device lib"
	@./build-openssl.py --prefix _builds/openssl/device --force --openssl-tar ./deps/openssl-1.1.1q.tar.gz ios-universal
	@echo "Building xcframework at _builds/ios-xcframework-openssl"
	@mkdir -p _builds/tmp _builds/ios-xcframework-openssl/
	@xcodebuild -create-xcframework -library _builds/openssl/device/lib/libcrypto.a -headers ./deps/openssl/ios/include -library _builds/openssl/simulator/lib/libcrypto.a -headers ./deps/openssl/ios/include -output _builds/ios-xcframework-openssl/libcrypto.xcframework
	@xcodebuild -create-xcframework -library _builds/openssl/device/lib/libssl.a -headers ./deps/openssl/ios/include -library _builds/openssl/simulator/lib/libssl.a -headers ./deps/openssl/ios/include -output _builds/ios-xcframework-openssl/libssl.xcframework
	@echo "Done"

build-ios-xcframework: build-ios build-ios-simulator-64 build-ios-simulator-applesilicon
	@echo "Lipo simulator64 and arm64 together"
	@rm -rf _builds/ios-xcframework-libpitaya
	@mkdir -p _builds/tmp _builds/ios-xcframework-libpitaya/
	@lipo -create _builds/ios-simulator/libpitaya-ios.a _builds/ios-simulator-applesilicon/libpitaya-ios.a -output _builds/tmp/libpitaya-ios.a
	@lipo -create _builds/ios-simulator/deps/libuv-1.44.2/libuv_a.a _builds/ios-simulator-applesilicon/deps/libuv-1.44.2/libuv_a.a -output _builds/tmp/libuv_a.a
	@rm -rf _builds/ios-xcframework-libpitaya
	@echo "Building xcframework at _builds/ios-xcframework-libpitaya"
	@xcodebuild -create-xcframework -library _builds/tmp/libpitaya-ios.a -headers ./include -library _builds/ios/libpitaya-ios.a -headers ./include -output _builds/ios-xcframework-libpitaya/libpitaya.xcframework
	@xcodebuild -create-xcframework -library _builds/tmp/libuv_a.a -library _builds/ios/deps/libuv-1.44.2/libuv_a.a -output _builds/ios-xcframework-libpitaya/libuv.xcframework
	@rm -rf _builds/tmp
	@echo "Done"

build-ios:
	@rm -rf _builds/ios
	@cmake -H. -B_builds/ios -DCMAKE_BUILD_TYPE=Release -DPLATFORM=OS -DDEPLOYMENT_TARGET="10.0" -DCMAKE_TOOLCHAIN_FILE=../../cmake/ios.toolchain.cmake
	@cmake --build _builds/ios --config Release -j

build-ios-simulator-64:
	@rm -rf _builds/ios-simulator
	@cmake -H. -B_builds/ios-simulator -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../../cmake/ios.toolchain.cmake -DPLATFORM=SIMULATOR64 -DSIMULATOR=true
	@cmake --build _builds/ios-simulator --config Release -j

build-ios-simulator-applesilicon:
	@rm -rf _builds/ios-simulator-applesilicon
	@cmake -H. -B_builds/ios-simulator-applesilicon -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../../cmake/ios.toolchain.cmake -DPLATFORM=SIMULATORARM64 -DSIMULATOR=true
	@cmake --build _builds/ios-simulator-applesilicon --config Release -j

build-mac-tests:
	@rm -rf _builds/mac-tests
	@cmake -GNinja -H. -B_builds/mac-tests -DCMAKE_BUILD_TYPE=Release
	@cmake --build _builds/mac-tests

clean-docker-container:
	@if [[ `docker ps -aqf "name=libpitaya"` != "" ]]; then \
		docker rm `docker ps -aqf "name=libpitaya"` ; \
	fi

build-linux-docker: clean-docker-container
	@docker build -t libpitaya .
	@mkdir -p _builds
	@docker run -v $(shell pwd):/app/ --name libpitaya libpitaya:latest
	@$(MAKE) clean-docker-container

.PHONY: build

create-out-folder:
	@rm -rf out
	@mkdir -p out

build-servers-linux: create-out-folder
	@cd pitaya-servers/json-server && GOOS=linux GOARCH=amd64 CGO_ENABLED=0 go build -o ../../out/json_server_linux
	@cd pitaya-servers/protobuf-server && GOOS=linux GOARCH=amd64 CGO_ENABLED=0 go build -o ../../out/protobuf_server_linux

docker-build: build-servers-linux
	@docker build -t libpitaya-test-servers .

generate-xcode:
	@cmake -H. -B_builds/xcode -GXcode -DBUILD_TESTING=ON

update-test-protos:
	@python ./deps/nanopb-0.4.6/generator/nanopb_generator.py -I ./pitaya-servers/protobuf-server/protos/ -D ./test ./pitaya-servers/protobuf-server/protos/big-message.proto ./pitaya-servers/protobuf-server/protos/response.proto ./pitaya-servers/protobuf-server/protos/session-data.proto
	@python ./deps/nanopb-0.4.6/generator/nanopb_generator.py -I ./unity/PitayaExample/Assets/Pitaya/Protos/ -D ./test ./unity/PitayaExample/Assets/Pitaya/Protos/error.proto
