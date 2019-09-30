{
    "targets": [
        {
            "target_name": "simple-relighter",
            "sources": [
                "src/relighter.h",
                "src/relighter.cpp",
                "src/third-party/lodepng/lodepng.cpp"
            ],
            "conditions": [
                ['OS!="win"', {
                    'ldflags': [
                        '-lc++experimental'
                    ]
                }]
            ],
            "include_dirs": [
                "<!(node -e \"require('nan')\")"
            ],
            "defines": ["NOMINMAX"]
        }
    ]
}
