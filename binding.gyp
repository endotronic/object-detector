{
  'targets': [
    {
      'target_name': 'object-detector',
      'sources': [ 'src/addon.cpp', 'src/detector.cpp', 'src/predictor.cpp', 'dlib/all/source.cpp' ],
      'libraries': [ '-lpng', '-ljpeg'],
      'include_dirs': [ './' ],
      'defines': [ 'DLIB_JPEG_SUPPORT', 'DLIB_PNG_SUPPORT', 'DLIB_NO_GUI_SUPPORT' ],
      'cflags!': [ '-fno-exceptions', '-fno-rtti' ],
      'cflags_cc!': [ '-fno-exceptions', '-fno-rtti' ],
      'conditions': [
        ['OS=="mac"', {
          'xcode_settings': {
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
            'GCC_ENABLE_CPP_RTTI': 'YES'
          },
          'include_dirs': [
            'include', '/usr/local/include'
          ],
          'libraries': [ '-L/usr/local/lib'],
        }]
      ]
    }
  ]
}

