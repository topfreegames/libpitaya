{
  'variables': {
      'uv_library%': "static_library",
      'build_for_linux%': "false",
      'build_for_mac%': "false",
      'build_for_ios%': "false",
      'build_for_windows%': "false",

      'pomelo_library%': "static_library",
      'build_for_ios%': "false",
      'use_sys_openssl%': "true",
      'library%': "static_library",
      'use_sys_uv%': "false",
      'use_sys_zlib%': "false",
      'no_tls_support%': "false",
      'no_uv_support%': "false",
      'build_pypomelo%': "false",
      'python_header%': "/usr/include/python2.7",
      'build_jpomelo%': "false",
      'build_cspomelo%': "false",
      'build_type%': "Release",
      'use_xcode%': "false",
  },

    'target_defaults': {
      'default_configuration': 'Release_x64',
      'configurations': {
        'Release_x64': {
          'msvs_configuration_platform': 'x64',
        },
      },
      'conditions': [
        ['OS == "win"', {
          'msvs_settings': {
            'VCCLCompilerTool': {
              #'AdditionalOptions': [ '/TP' ],
            },
          },
          'defines': [
            '_WIN32',
            'WIN32',
            '_CRT_NONSTDC_NO_DEPRECATE',
            '_WINDOWS',
            '_WINDLL',
            'UNICODE',
            '_UNICODE',
          ],
          'link_settings': {
            'libraries': [
              '-ladvapi32.lib',
              '-liphlpapi.lib',
              '-lpsapi.lib',
              '-lshell32.lib',
              '-lws2_32.lib'
            ],
          },
        }, {  # else
          'defines':[
            '_LARGEFILE_SOURCE',
            '_FILE_OFFSET_BITS=64',
            '_GNU_SOURCE'
          ]
        }],   # OS == "win"
        ['use_xcode == "false"', {
          'product_dir': 'output',
        }],
        ['build_type=="Debug"', {
          'cflags': ['-g', '-O0', '-Wall', '-Wextra', '-pedantic']
        }],
        ['build_type=="Release"', {
          'cflags': ['-g', '-O3', '-Wall', '-Wextra', '-pedantic']
        }],
	['use_sys_zlib == "true"', {
	  'link_settings': {
	    'libraries': ['-lz'],
	  },
	}, {
          'dependencies': [
	    './deps/zlib/zlib.gyp:zlib',
          ],
	}],
        [ 'no_uv_support == "false"', {
          'conditions' : [
            ['use_sys_uv == "false"', {
              'dependencies': [
                './deps/libuv/uv.gyp:libuv',
              ],
              'include_dirs': [
                './deps/libuv/include',
              ]
            }, {
              'link_settings': {
                'libraries': ['-luv']
              }
            }], # use_sys_uv
        ['no_tls_support == "false"', {
          'conditions': [
            ['use_sys_openssl == "false"', {
              'dependencies': [
                './deps/openssl/openssl.gyp:openssl',
              ],
              'include_dirs': [
                './deps/openssl/openssl/include',
              ]
            }, {
              'conditions': [
                [ 'OS=="win"', {
                  'libraries': [
                    'C:/OpenSSL-Win64/lib/libeay32.lib',
                    'C:/OpenSSL-Win64/lib/ssleay32.lib',
                  ],
                  'include_dirs': [
                    'C:/OpenSSL-Win64/include',
                  ],
                }, {
                  'include_dirs': [
                    './deps/openssl/openssl/include',
                  ],
                  'link_settings': {
                    'libraries': [
                      '-lssl',
                      '-lcrypto',
                    ],
                  },
                }],
              ],
            }], # use_sys_openssl
          ],
        }],  # no tls support
        ]
      }], # no uv support
      ],
    },

    'targets': [
      {
        'target_name': 'pitaya',
        'include_dirs': [
          './include',
          './src',
        ],
        'sources': [
          './src/pc_pomelo.c',
          './src/pc_lib.c',
          './src/pc_trans.c',
          './src/pc_trans_repo.c',
          './src/pc_JSON.c',
          './src/tr/dummy/tr_dummy.c'
        ],
        'conditions': [
          ['OS != "win"', {
            'defines': ['_GNU_SOURCE'],
            'cflags': ['-fPIC'],
          }, {
            'defines': [
              '_CRT_SECURE_NO_WARNINGS',
              '_CRT_NONSTDC_NO_DEPRECATE',
            ]
          }],
          ['OS == "android"', {
            'cflags': ['-fPIE'],
          }, {
	    'link_settings': {
	      'libraries': ['-pie'],
	    },
          }],
          ['build_for_mac == "true" or build_for_ios == "true"', {
            'type': 'static_library',
          }, {
            'type': 'shared_library',
            'defines': ['BUILDING_PC_SHARED=1'],
          }],
          ['no_uv_support == "false"', {
            'sources': [
              './src/tr/uv/pr_gzip.c',
              './src/tr/uv/pr_msg.c',
              './src/tr/uv/pr_msg_json.c',
              './src/tr/uv/pr_pkg.c',
              './src/tr/uv/tr_uv_tcp.c',
              './src/tr/uv/tr_uv_tcp_i.c',
              './src/tr/uv/tr_uv_tcp_aux.c',
            ],
            'conditions': [
              ['no_tls_support == "false"', {
                'sources': [
                  './src/tr/uv/tr_uv_tls.c',
                  './src/tr/uv/tr_uv_tls_i.c',
                  './src/tr/uv/tr_uv_tls_aux.c',
                ]}, {
                  'defines': ['PC_NO_UV_TLS_TRANS']
                }
              ], # no tls support
            ]}, {
              'defines': ['PC_NO_UV_TCP_TRANS']
            }
          ], # no uv support
        ],
      },
      {
        'target_name': 'tests',
        'type': 'executable',
        'dependencies': [
          'pitaya',
        ],
        'include_dirs': [
          './include/',
          '/usr/local/include',
          './deps/munit'
        ],
        'sources': [
          './test/main.c',
          './test/test-tr_tcp.c',
          './test/test-tr_tls.c',
          './test/test_pc_client.c',
          './test/test_reconnection.c',
          './test/test_compression.c',
          './test/test_kick.c',
          './test/test_session.c',
          './test/test_request.c',
          './test/test_notify.c',
          './test/test_stress.c',
          './deps/munit/munit.c',
        ],
      },
    ],
    'conditions': [
      ['build_pypomelo == "true"', {
        'targets':[ {
          'target_name': 'pypomelo',
          'type': 'shared_library',
          'dependencies': [
            'pitaya',
          ],
          'include_dirs': [
            './include/',
            '<(python_header)',
          ],
          'sources': [
            './py/pypomelo.c',
          ],
        }],
      }],
      ['build_jpomelo == "true"', {
        'targets':[ {
          'target_name': 'jpomelo',
          'type': 'shared_library',
          'dependencies': [
            'pitaya',
          ],
          'include_dirs': [
            './include/',
          ],
          'sources': [
            './java/com_netease_pomelo_Client.c',
          ],
        }],
      }],
      ['build_cspomelo == "true"', {
        'conditions': [
          ['build_for_linux == "true" or build_for_mac == "true" or build_for_windows == "true"', {
            'targets':[ {
              'target_name': 'pitaya_unity',
              'type': 'shared_library',
              'dependencies': [
                'pitaya',
              ],
              'conditions': [
                ['OS!="win"', {
                  'cflags': ['-fPIC'],
                }],
                ['build_for_mac == "true"', {
                  'product_extension': 'bundle',
                }],
              ],
              'include_dirs': [
                './include/',
              ],
              'sources': [
                './cs/contrib/cspomelo.c',
              ],
            }],
          }],
          ['build_for_ios == "true"', {
            'targets':[ {
              'target_name': 'pitaya_unity_ios',
              'type': 'static_library',
              'dependencies': [
                'pitaya',
              ],
              'conditions': [
                ['OS!="win"', {
                  'cflags': ['-fPIC'],
                }],
              ],
              'include_dirs': [
                './include/',
              ],
              'sources': [
                './cs/contrib/cspomelo.c',
              ],
            }],
          }],
        ]
    }],
  ],
}
