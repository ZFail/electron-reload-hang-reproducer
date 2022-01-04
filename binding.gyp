{
  'targets': [{
    'target_name': 'addon',
    'defines': ['V8_DEPRECATION_WARNINGS=1'],
    'sources': ['addon.cpp'],
    'include_dirs': ["<!@(node -p \"require('node-addon-api').include\")"],
    'dependencies': ["<!(node -p \"require('node-addon-api').gyp\")"],
    'xcode_settings': {
      'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
      'CLANG_CXX_LIBRARY': 'libc++',
      'MACOSX_DEPLOYMENT_TARGET': '10.7',
    },
    'msvs_settings': {
      'VCCLCompilerTool': {
        'ExceptionHandling': 1
      },
    },
  }]
}