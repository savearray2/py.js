{
    "targets": [{
        "target_name": "pyjs",
        "cflags!": [ "-fno-exceptions" ],
        "cflags_cc!": [ "-fno-exceptions" ],
        "sources": [
            "src/pyjs.cpp",
            "src/pyjs_pyobj.cpp",
			"src/pyjs_async.cpp",
            "src/pyjs_utils.cpp",
			"src/pyjs_common.cpp",
			"src/pyjs_contrib.cpp"
        ],
        "conditions": [
            ['OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris"', {
                "cflags": [
                    "<!(python3-config --cflags)",
                    "-Xlinker -export-dynamic"
                ],
                "cflags!": [ "-fno-exceptions" ],
                "cflags_cc!": [ "-fno-exceptions" ],
                "cflags_cc": [ "-std=c++17" ],
                "libraries": [
                    "<!(python3-config --libs)"
                ]
                }],
            ['OS=="mac"', {
                "xcode_settings": {
                    "cflags": [
                        "<!(python3-config --cflags)",
                        "-Xlinker -export-dynamic",
                    ],
                    "OTHER_CFLAGS": [
                        "<!(python3-config --cflags)"
                    ],
                    "OTHER_LDFLAGS": [
                        "<!(python3-config --ldflags)"
                    ],
					"GCC_ENABLE_CPP_EXCEPTIONS": "YES",
    				'CLANG_CXX_LIBRARY': 'libc++',
   					'MACOSX_DEPLOYMENT_TARGET': '10.7'
                }
            }],
			['OS=="win"', {
				"include_dirs": [
					'<@(python_include)\\include'
				],
				"library_dirs": [
					"<@(python_include)\\libs"
				],
				"defines": [
					'_HAS_EXCEPTIONS=1'
				],
			}]
        ],
		'msvs_settings': {
    		'VCCLCompilerTool': { 'ExceptionHandling': 1 },
		},
        'include_dirs': [
            "<!@(node -p \"require('node-addon-api').include\")"
        ],
        'dependencies': [
            "<!(node -p \"require('node-addon-api').gyp\")"
        ]
    }]
}