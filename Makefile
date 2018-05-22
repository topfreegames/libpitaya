setup-node:
	@curl -o- https://raw.githubusercontent.com/creationix/nvm/v0.33.11/install.sh | bash
	@. ~/.nvm/nvm.sh && nvm install 10.0.0

install-gyp:
	@git clone https://chromium.googlesource.com/external/gyp.git ~/gyp
	@cd ~/gyp && sudo python setup.py install

gyp-linux:
	@gyp --depth=. pomelo.gyp -f ninja --generator-output=build -Duse_sys_openssl=false -Dbuild_type=Release -Dbuild_cspomelo=true -Dbuild_for_linux=true -Duv_library=static_library -Dtarget_arch=x64

gyp-ios-mac:
	@gyp --depth=. pomelo.gyp -f ninja --generator-output=build -Duse_sys_openssl=false -Dbuild_type=Release -Dbuild_cspomelo=true -Dbuild_for_mac=true -Dbuild_for_ios=true -Duv_library=static_library -Dtarget_arch=x64

gyp-android:
	@gyp --depth=. -Dtarget_arch=arm -Dpomelo_library=static_library pomelo.gyp -f ninja-linux --generator-output=build -Duse_sys_openssl=false -Dbuild_type=Release -Dbuild_for_linux=true -Duv_library=static_library -DOS=android -Duse_sys_zlib=true

.PHONY: build

test-deps: setup-node
	@docker run -p 4222:4222 -d --name nats-main nats
	@docker run -d -p 2379:2379 -p 2380:2380 appcelerator/etcd
	@-(cd test/server && go get)

build:
	cd build/out/Release_x64 && ninja

test: build test-deps
	./run-tests.sh
