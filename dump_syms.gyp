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
            ['OS!="win"', {
                'cflags': [
                    '--std=c++11',
                ],
            }],
        ]
    }
    ]
}
