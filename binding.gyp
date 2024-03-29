{
    "targets": [
        {
            "target_name": "simple-relighter",
            "sources": [
                "src/relighter.h",
                "src/relighter.cpp",
                "src/third-party/lodepng/lodepng.cpp"
            ],
            "include_dirs": [
                "<!(node -e \"require('nan')\")",
                "src/third-party"
            ],
            'cflags!': ['-fno-exceptions', '-fno-rtti'],
            'cflags_cc!': ['-fno-exceptions', '-fno-rtti'],
            'conditions': [
                ['OS=="mac"', {
                    'xcode_settings': {
                        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
                    }
                }]
            ],
            "defines": ["NOMINMAX"]
        }
    ]
}
