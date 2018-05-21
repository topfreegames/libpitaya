{
  'target_defaults': {
    'default_configuration': 'Release_x64',
    'configurations': {
      'Release_x64': {
        'msvs_configuration_platform': 'x64',
      },
    },
  },
  'targets': [
    {
      'target_name': 'zlib',
      'type': 'static_library',
      'sources': [
        'adler32.c',
        'compress.c',
        'crc32.c',
        'deflate.c',
        'gzclose.c',
        'gzlib.c',
        'gzread.c',
        'gzwrite.c',
        'infback.c',
        'inffast.c',
        'inflate.c',
        'inftrees.c',
        'trees.c',
        'uncompr.c',
        'zutil.c',
      ],
      'include_dirs': [
        '.',
      ],
      'direct_dependent_settings': {
        'include_dirs': [ '.' ],
      },
      'conditions': [
        ['OS != "win"', {
          'cflags': [
            '-fPIC',
          ],
        }],
      ],
    },
  ]
}
