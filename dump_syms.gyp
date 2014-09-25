# -*- Mode: python; indent-tabs-mode: nil; -*-
{
    'variables': {
        'have_tbb': '<!(! pkg-config --atleast-version=2.2 tbb; echo $?)',
    },
    'targets': [
    {
        'target_name': 'dump_syms',
        'type': 'executable',
        'sources': [
            'dump_syms.cpp',
            'PDBParser.cpp',
            'utils.cpp',
        ],
        'conditions': [
            ['OS=="win"', {
                'msvs_settings': {
                    'VCCLCompilerTool': {
                        'ExceptionHandling': '1',
                        'WarnAsError': 'true',
                        'Optimization': '0',
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
        ]
    }
    ]
}
