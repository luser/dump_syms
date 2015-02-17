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
                },
            }, { # OS != "win"
                'cflags': [
                    '-g',
                    '-Wall',
                    '-Werror',
                    '--std=c++0x',
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
      ]
    },
    ]
}
