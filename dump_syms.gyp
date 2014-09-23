{
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
                    '--std=c++11',
                ],
            }],
        ]
    }
    ]
}
