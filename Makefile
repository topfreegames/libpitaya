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

gyp-linux:
	@gyp --depth=. pomelo.gyp -f ninja --generator-output=build -Duse_sys_openssl=false -Dbuild_type=Release -Duse_xcode=false -Dbuild_cspomelo=true -Dbuild_for_linux=true

gyp-ios-mac:
	@gyp --depth=. pomelo.gyp -f ninja --generator-output=build -Duse_sys_openssl=false -Dbuild_type=Release -Duse_xcode=false -Dbuild_cspomelo=true -Dbuild_for_mac=true -Dbuild_for_ios=true

.PHONY: build

test-deps: setup-node setup-go
	@-(cd test/server && go get)

build:
	cd build/out/Release_x64 && ninja

test: build test-deps
	./run-tests.sh
