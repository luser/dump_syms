# -*- Mode: python; indent-tabs-mode: nil; -*-
{
    'variables': {
        'have_tbb': '<!(python wrap-pkg-config.py --atleast-version=2.2 tbb)',
    },
    'target_defaults': {
    'conditions': [
            ['OS=="win"', {
                'msvs_settings': {
                    'VCCLCompilerTool': {
                        'ExceptionHandling': '1',
                        'WarnAsError': 'true',
                        'Optimization': '0',
                        #'AdditionalOptions': ['/MP'],
                    },
                    'VCLinkerTool': {
                        'SubSystem': '1',
                    },
                },
            }, { # OS != "win"
                'cflags': [
                    '-g',
                    '-Wall',
                    '-Werror',
                    '-std=gnu++0x',
                ],
            }],
            ['<(have_tbb)==1', {
                'cflags': [
                    '<!@(pkg-config --cflags tbb)',
                ],
                'libraries': [
                    '<!@(pkg-config --libs tbb)',
                ],
                'defines': [
                    'HAVE_TBB',
                ],
            }],
    ]  # conditions
    }, # target_defaults
    'targets': [
    {
        'target_name': 'dump_syms',
        'type': 'executable',
        'sources': [
            'dump_syms.cpp',
        ],
        'dependencies': [
            'pdb_parser',
        ],
    },
    {
      'target_name': 'pdb_parser',
      'type': 'static_library',
      'sources': [
            'PDBParser.cpp',
            'utils.cpp',
      ],
      'direct_dependent_settings': {
          'include_dirs': [
              '<(DEPTH)',
          ],
      },
    },
    {
        'target_name': 'dump_syms_unittest',
        'type': 'executable',
        'sources': [
            'testing/dump_syms_unittest.cpp',
        ],
        'conditions': [
            ['OS=="win"', {
                'sources': [
                    'testing/memstream.cpp',
                ],
            }],
        ],
        'dependencies': [
            '<(DEPTH)/testing/testing.gyp:gmock',
            '<(DEPTH)/testing/testing.gyp:gtest',
            'pdb_parser',
        ],
    },
    ]
}
