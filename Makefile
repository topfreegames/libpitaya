setup-go:
	@curl -sL -o ~/bin/gimme https://raw.githubusercontent.com/travis-ci/gimme/master/gimme
	@chmod +x ~/bin/gimme
	@~/bin/gimme stable

setup-node:
	@curl -o- https://raw.githubusercontent.com/creationix/nvm/v0.33.11/install.sh | bash
	@nvm install 10.0.0
	@nvm use 10.0.0

setup-gyp:
	@git clone https://chromium.googlesource.com/external/gyp.git ${TRAVIS_BUILD_DIR}/gyp
	@cd ${TRAVIS_BUILD_DIR}/gyp && sudo python setup.py install

setup-ci: setup-gyp setup-node setup-go
	@gyp --depth=. pomelo.gyp -f make --generator-output=build -Duse_sys_openssl=false -Dbuild_type=Release -Duse_xcode=false

.PHONY: build

build:
	cd build && make

test: build
	./run-tests.sh