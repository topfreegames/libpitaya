Pod::Spec.new do |s|
  s.name             = 'Pitaya'
  s.version          = '0.0.1'
  s.summary          = 'The pod for the pitaya client'

  s.description      = <<-DESC
    The missing pod for libpitaya with ssl support!
  DESC

  s.homepage         = 'https://github.com/topfreegames/libpitaya'
  s.license          = { :type => 'MIT', :file => 'LICENSE' }
  s.author           = { 'Guthyerrz Maciel' => 'guthyerrz.ufcg@gmail.com' }
  s.source           = { :git => 'https://github.com/topfreegames/libpitaya.git', :tag => s.version.to_s }

  s.ios.deployment_target = '8.0'

  s.source_files = ['src/**/*', 'include/**/*', 'cs/contrib/*']

  s.dependency 'OpenSSL-TFG', '~> 1.1.1a.3' 
  s.dependency 'libuv', '~> 1.4.0'

end
