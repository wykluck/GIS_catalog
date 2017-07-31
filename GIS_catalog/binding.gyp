﻿{
  "targets": [
    {
      "target_name": "addon",
      "sources": [
        "main.cpp",
        "commonutils.cpp",
        "gdal_translate.cpp",
        "gdal_translate.h",
        "CatalogDB.cpp",
        "CatalogDB.h",
        "DatasetStruct.h",
        "commonutils.h",
        "grfmt_base.cpp",
        "grfmt_base.hpp",
        "grfmt_gdal.cpp",
        "grfmt_gdal.hpp"
      ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")",
        "cpp/concurrentqueue"
      ],
      "cflags!": [
        "-fno-exceptions"
      ],
      "cflags_cc!": [
        "-fno-exceptions"
      ],
      "conditions": [
        [
          "OS=='win'",
          {
            "msvs_configuration_attributes": {
              "OutputDirectory": "$(ProjectDir)$(Configuration)"
            },
            "configurations": {
              "Debug": {
                "msvs_settings": {
                  "VCCLCompilerTool": {
                    "ExceptionHandling": "0",
                    "AdditionalOptions": [
                      "/MP /EHsc"
                    ]
                  },
                  "VCLibrarianTool": {
                    "AdditionalOptions": [
                      "/LTCG"
                    ]
                  },
                  "VCLinkerTool": {
                    "LinkIncremental": 1,
                    "LinkTimeCodeGeneration": 1,
                    "AdditionalDependencies": [ "gdald.lib", "mongocxx.lib", "bsoncxx.lib" ]
                  }
                }
              },
              "Release": {
                "msvs_settings": {
                  "VCCLCompilerTool": {
                    "RuntimeLibrary": 0,
                    "Optimization": 3,
                    "FavorSizeOrSpeed": 1,
                    "InlineFunctionExpansion": 2,
                    "WholeProgramOptimization": "true",
                    "OmitFramePointers": "true",
                    "EnableFunctionLevelLinking": "true",
                    "EnableIntrinsicFunctions": "true",
                    "RuntimeTypeInfo": "false",
                    "ExceptionHandling": "0",
                    "AdditionalOptions": [
                      "/MP /EHsc"
                    ]
                  },
                  "VCLibrarianTool": {
                    "AdditionalOptions": [
                      "/LTCG"
                    ]
                  },
                  "VCLinkerTool": {
                    "LinkTimeCodeGeneration": 1,
                    "OptimizeReferences": 2,
                    "EnableCOMDATFolding": 2,
                    "LinkIncremental": 1,
                    "AdditionalDependencies": [ "gdal.lib", "mongocxx.lib", "bsoncxx.lib" ]
                  }
                }
              }
            }
          }
        ]
      ]
    }
  ]
}