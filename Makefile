ANDROID_TOOLCHAIN_FILE = temp/android-ndk-r17b/build/cmake/android.toolchain.cmake

setup-mac:
	@brew install ninja

setup-android-mac:
	@mkdir -p temp
	@curl https://dl.google.com/android/repository/android-ndk-r17b-darwin-x86_64.zip -o temp/android-ndk-r17b.zip
	@unzip -qo temp/android-ndk-r17b.zip -d temp/

setup-android-linux:
	@mkdir -p temp
	@curl https://dl.google.com/android/repository/android-ndk-r17b-linux-x86_64.zip -o temp/android-ndk-r17b.zip
	@unzip -qo temp/android-ndk-r17b.zip -d temp/

build-android:
	@rm -rf _builds/android
	@cmake -GNinja -H. -B_builds/android -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release \
				   -DCMAKE_TOOLCHAIN_FILE=${ANDROID_TOOLCHAIN_FILE} \
				   -DANDROID_ABI=armeabi-v7a
	@cmake --build _builds/android

build-mac:
	@rm -rf _builds/mac
	@cmake -H. -B_builds/mac -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_MACOS_BUNDLE=ON
	@cmake --build _builds/mac

build-linux:
	@rm -rf _builds/linux
	@cmake -H. -B_builds/linux -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON
	@cmake --build _builds/linux

build-linux-debug:
	@rm -rf _builds/linux-debug
	@cmake -H. -B_builds/linux-debug -GNinja -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=ON
	@cmake --build _builds/linux-debug

build-ios:
	@rm -rf _builds/ios
	@cmake -H. -B_builds/ios -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../../cmake/ios.toolchain.cmake
	@cmake --build _builds/ios

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
	@docker run --name libpitaya libpitaya:latest
	@mkdir -p _builds/linux
	@docker cp `docker ps -aqf "name=libpitaya"`:/app/_builds/linux/libpitaya-linux.so _builds/linux
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
