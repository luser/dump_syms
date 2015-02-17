# -*- Mode: python; indent-tabs-mode: nil; -*-
{
  'targets': [
      {
      'target_name': 'gtest',
          'type': 'static_library',
          'include_dirs': [
              '<(DEPTH)/testing/googletest',
              '<(DEPTH)/testing/googletest/include',
          ],
      'sources': [
          'googletest/src/gtest-all.cc',
      ],
      'direct_dependent_settings': {
          'include_dirs': [
              '<(DEPTH)/testing/googletest/include',
          ],
        # Visual C++ implements variadic templates strangely, and
          # VC++2012 broke Google Test by lowering this value. See
          # http://stackoverflow.com/questions/12558327/google-test-in-visual-studio-2012
          'defines': ['_VARIADIC_MAX=10'],
          'conditions': [
              ['OS=="linux"', {
                  'libraries': [
                      '-lpthread',
                  ]
              }],
          ],
      },
      'defines': ['_VARIADIC_MAX=10'],
      },
    {
      'target_name': 'gmock',
        'type': 'static_library',
        'include_dirs': [
            '<(DEPTH)/testing/googlemock',
            '<(DEPTH)/testing/googlemock/include',
            '<(DEPTH)/testing/googletest',
            '<(DEPTH)/testing/googletest/include',
        ],
      'sources': [
          '<(DEPTH)/testing/googlemock/src/gmock-all.cc',
          '<(DEPTH)/testing/googlemock/src/gmock_main.cc',
      ],
      'direct_dependent_settings': {
          'include_dirs': [
              '<(DEPTH)/testing/googlemock/include',
              '<(DEPTH)/testing/googletest/include',
          ],
        'defines': ['_VARIADIC_MAX=10'],
      },
      'defines': ['_VARIADIC_MAX=10'],
    },

  ],
}
