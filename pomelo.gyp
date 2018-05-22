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
      'no_tls_support%': "false",
      'no_uv_support%': "false",
      'build_pypomelo%': "false",
      'python_header%': "/usr/include/python2.7",
      'build_jpomelo%': "false",
      'build_cspomelo%': "false",
      'build_type%': "Release",
      'use_xcode%': "true",
  },

    'target_defaults': {
      'conditions': [
        ['OS == "win"', {
          'msvs_settings': {
            'VCCLCompilerTool': {
              'AdditionalOptions': [ '/TP' ],
            }
          },
          'defines': {
              '_WIN32',
              'WIN32',
              '_CRT_NONSTDC_NO_DEPRECATE',
              '_WINDOWS',
              '_WINDLL',
              'UNICODE',
              '_UNICODE',
          },
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
        ['use_xcode == "true"', {
          'xcode_settings': {'OTHER_LDFLAGS': ['-lz']},
        }, {
          'product_dir': 'output',
        }],
        ['build_type=="Debug"', {
          'cflags': ['-g', '-O0', '-Wall', '-Wextra', '-pedantic']
        }],
        ['build_type=="Release"', {
          'cflags': ['-g', '-O3', '-Wall', '-Wextra', '-pedantic']
        }],
        [ 'no_uv_support == "false"', {
          'conditions' : [
            ['use_sys_uv == "false"', {
              'dependencies': [
                './deps/uv/uv.gyp:libuv',
              ],
              'include_dirs': [
                './deps/uv/include',
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
              'link_settings': {
                'libraries': [
                  '-lssl',
                  '-lcrypto',
                ]
              }
            }], # use_sys_openssl
          ],
        }],  # no tls support
        ]
      }], # no uv support
      ],
    },

    'targets': [
      {
        'target_name': 'libpitaya',
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
          ['build_for_mac == "true" or build_for_ios == "true" or build_for_linux == "true"' , {
            'type': 'static_library',
            'defines': ['BUILDING_PC_SHARED=1'],
          }, {
            'type': 'shared_library',
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
            'link_settings': {
               'libraries': [
                 '-lz',
               ],
            },
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
      #{
        #'target_name': 'tests',
        #'type': 'executable',
        #'dependencies': [
          #'libpitaya',
        #],
        #'include_dirs': [
          #'./include/',
          #'/usr/local/include',
          #'./deps/munit'
        #],
        #'sources': [
          #'./test/main.c',
          #'./test/test-tr_tcp.c',
          #'./test/test-tr_tls.c',
          #'./test/test_pc_client.c',
          #'./test/test_reconnection.c',
          #'./test/test_compression.c',
          #'./test/test_kick.c',
          #'./test/test_session.c',
          #'./test/test_request.c',
          #'./test/test_notify.c',
          #'./deps/munit/munit.c',
        #],
      #},
    ],
    'conditions': [
      ['build_pypomelo == "true"', {
        'targets':[ {
          'target_name': 'pypomelo',
          'type': 'shared_library',
          'dependencies': [
            'libpitaya',
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
            'libpitaya',
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
                'libpitaya',
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
                'libpitaya',
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
