setup-go:
	@sudo rm -rf ~/.gimme
	@gimme 1.10.2
	@echo Go installed version $(shell go version)

setup-go-linux:
	@mkdir -p ~/bin
	@curl https://dl.google.com/go/go1.10.3.linux-amd64.tar.gz -o ~/bin/go1.10.3.linux-amd64.tar.gz
	@cd ~/bin && tar xf ~/bin/go1.10.3.linux-amd64.tar.gz
	@~/bin/go/bin/go version

setup-go-mac:
	@mkdir -p ~/bin
	@curl https://dl.google.com/go/go1.10.3.darwin-amd64.tar.gz -o ~/bin/go1.10.3.darwin-amd64.tar.gz
	@cd ~/bin && tar xf ~/bin/go1.10.3.darwin-amd64.tar.gz

setup-node-linux:
	@curl https://nodejs.org/dist/v8.11.3/node-v8.11.3-linux-x64.tar.xz -o ~/node-v8.11.3-linux-x64.tar.xz
	@cd ~ && tar xf ~/node-v8.11.3-linux-x64.tar.xz && mv ~/node-v8.11.3-linux-x64 ~/node

setup-node-mac:
	@curl https://nodejs.org/dist/v8.11.3/node-v8.11.3-darwin-x64.tar.xz -o ~/node-v8.11.3-darwin-x64.tar.xz
	@cd ~ && tar xf ~/node-v8.11.3-darwin-x64.tar.xz

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
