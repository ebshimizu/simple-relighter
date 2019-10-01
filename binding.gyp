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
            "defines": ["NOMINMAX"]
        }
    ]
}
