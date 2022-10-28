# How to update OpenSSL dependency

Using build-openssl.py build versions for linux, android (aarch64 and armv7a) and update files in
deps/openssl/[linux,android]

Using the script at repo https://github.com/tfgco/OpenSSL or https://github.com/krzyzanowskim/OpenSSL generate 
builds for iOS and mac and update files in deps/openssl/[mac,ios]

For updating the example project and fixing libpitaya podspec, generate a new pod version of the cocoapod
OpenSSL-TFG using pod spec at the tfgco repo above.
