setup-go:
	@sudo rm -rf ~/.gimme
	@gimme 1.10.2
	@echo Go installed version $(shell go version)

setup-node:
	@curl -o- https://raw.githubusercontent.com/creationix/nvm/v0.33.11/install.sh | bash
	@. ~/.nvm/nvm.sh && nvm install 10.0.0

install-gyp:
	@git clone https://chromium.googlesource.com/external/gyp.git ~/gyp
	@cd ~/gyp && sudo python setup.py install

gyp-linux-release:
	@gyp --depth=. pitaya.gyp -f ninja --generator-output=build -Duse_sys_openssl=false -Dbuild_type=Release -Dbuild_for_linux=true -Dtarget_arch=x64 -Dpitaya_target=pitaya-linux

gyp-linux-debug:
	@gyp --depth=. pitaya.gyp -f ninja --generator-output=build -Duse_sys_openssl=false -Dbuild_type=Debug -Dbuild_for_linux=true -Dtarget_arch=x64 -Dpitaya_target=pitaya-linux

gyp-mac-debug:
	@gyp --depth=. pitaya.gyp -f ninja --generator-output=build -Duse_sys_openssl=false -Dbuild_type=Debug -Dbuild_for_mac=true -Dtarget_arch=x64 -Dpitaya_target=pitaya-mac

gyp-mac-release:
	@gyp --depth=. pitaya.gyp -f ninja --generator-output=build -Duse_sys_openssl=false -Dbuild_type=Release -Dbuild_for_mac=true -Dtarget_arch=x64 -Dpitaya_target=pitaya-mac

gyp-ios:
	@gyp --depth=. pitaya.gyp -f ninja --generator-output=build -Duse_sys_openssl=false -Dbuild_type=Release -Dbuild_for_ios=true -Dtarget_arch=x64 -Dpitaya_target=pitaya-ios

gyp-android:
	@gyp --depth=. pitaya.gyp -f ninja-linux --generator-output=build -Dbuild_for_android=true -DOS=android -Dbuild_type=Release -Duse_sys_openssl=false -Duse_sys_zlib=false -Dtarget_arch=arm  -Dpitaya_target=pitaya-android

.PHONY: build

test-deps: setup-node setup-go
	@-(cd test/server && go get)

build:
	cd build/out/Release_x64 && ninja

test: build test-deps
	./run-tests.sh
