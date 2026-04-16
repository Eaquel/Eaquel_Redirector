plugins {
    id("com.android.library")
}

val androidMinSdkVersion     : Int    by rootProject.extra
val androidBuildToolsVersion : String by rootProject.extra
val androidCompileSdkVersion : Int    by rootProject.extra
val androidNdkVersion        : String by rootProject.extra
val androidCmakeVersion      : String by rootProject.extra

android {
    compileSdk        = androidCompileSdkVersion
    ndkVersion        = androidNdkVersion
    buildToolsVersion = androidBuildToolsVersion
    namespace         = "com.eaquel.redirector"

    sourceSets {
        getByName("main") {
            manifest.srcFile("Source/Main/AndroidManifest.xml")
        }
    }

    buildFeatures {
        buildConfig      = false
        prefabPublishing = true
        prefab           = true
    }

    androidResources {
        enable = false
    }

    packaging {
        jniLibs { excludes += "**.so" }
    }

    prefab {
        register("Redirector") {
            headers = "Source/Main/Bridge"
        }
    }

    defaultConfig {
        minSdk = androidMinSdkVersion
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_21
        targetCompatibility = JavaVersion.VERSION_21
    }

    buildTypes {
        all {
            externalNativeBuild {
                cmake {
                    abiFilters("arm64-v8a", "armeabi-v7a", "x86", "x86_64")
                    val flags = arrayOf(
                        "-Wall", "-Werror", "-Qunused-arguments",
                        "-Wno-gnu-string-literal-operator-template",
                        "-fno-rtti", "-fvisibility=hidden", "-fvisibility-inlines-hidden",
                        "-fno-exceptions", "-fno-stack-protector", "-fomit-frame-pointer",
                        "-Wno-builtin-macro-redefined", "-ffunction-sections", "-fdata-sections",
                        "-Wno-unused-value", "-D__FILE__=__FILE_NAME__", "-Wl,--exclude-libs,ALL",
                    )
                    cppFlags("-std=c++23", *flags)
                    val configFlags = arrayOf("-Oz", "-DNDEBUG").joinToString(" ")
                    val buildDir = layout.buildDirectory.get().asFile.absolutePath
                    arguments(
                        "-DCMAKE_CXX_FLAGS_RELEASE=$configFlags",
                        "-DDEBUG_SYMBOLS_PATH=$buildDir/symbols/$name",
                        "-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON"
                    )
                }
            }
        }
        release {
            externalNativeBuild {
                cmake {
                    val flags = arrayOf(
                        "-Wl,--gc-sections",
                        "-flto",
                        "-fno-unwind-tables",
                        "-fno-asynchronous-unwind-tables"
                    )
                    cppFlags += flags
                    arguments += "-DCMAKE_BUILD_TYPE=Release"
                }
            }
        }
        create("standalone") {
            initWith(getByName("release"))
            externalNativeBuild {
                cmake {
                    val flags = arrayOf(
                        "-Wl,--gc-sections",
                        "-flto",
                        "-fno-unwind-tables",
                        "-fno-asynchronous-unwind-tables"
                    )
                    cppFlags += flags
                    arguments += "-DANDROID_STL=none"
                    arguments += "-DCMAKE_BUILD_TYPE=Release"
                }
            }
        }
    }

    lint {
        abortOnError       = true
        checkReleaseBuilds = false
    }

    externalNativeBuild {
        cmake {
            path    = file("Source/Main/Bridge/CMakeLists.txt")
            version = androidCmakeVersion
        }
    }
}
