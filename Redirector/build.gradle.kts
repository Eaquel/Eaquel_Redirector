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
    namespace         = "io.github.eaquel.redirector"

    sourceSets {
        getByName("main") {
            manifest.srcFile("Source/Main/AndroidManifest.xml")
        }
    }

    buildFeatures {
        buildConfig      = false
        prefab           = true
        prefabPublishing = true
    }

    androidResources {
        enable = false
    }

    packaging {
        jniLibs { excludes += "**/*.so" }
    }

    prefab {
        register("eaquel_redirector") {
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

    externalNativeBuild {
        cmake {
            path    = file("Source/Main/Bridge/CMakeLists.txt")
            version = androidCmakeVersion
        }
    }

    buildTypes {
        all {
            externalNativeBuild {
                cmake {
                    abiFilters("arm64-v8a", "armeabi-v7a", "x86", "x86_64")
                    
                    val commonFlags = arrayOf(
                        "-Wall", "-Werror", "-Qunused-arguments",
                        "-fvisibility=hidden", "-fvisibility-inlines-hidden",
                        "-fno-exceptions", "-fno-rtti",
                        "-ffunction-sections", "-fdata-sections",
                        "-fomit-frame-pointer", "-fstack-protector-strong",
                        "-Wno-builtin-macro-redefined", "-D__FILE__=__FILE_NAME__"
                    )
                    
                    cppFlags("-std=c++23", *commonFlags)
                    
                    val buildDir = layout.buildDirectory.get().asFile.absolutePath
                    val isLogDisabled = if (project.hasProperty("disableLogs")) "ON" else "OFF"
                    
                    arguments(
                        "-DDEBUG_SYMBOLS_PATH=$buildDir/symbols/$name",
                        "-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON",
                        "-DREDIRECTOR_LOGGING_DISABLED=$isLogDisabled"
                    )
                }
            }
        }
        debug {
            // Debug build devre dışı — CI sadece release/standalone üretir
            isDefault = false
            enableUnitTestCoverage = false
            enableAndroidTestCoverage = false
        }
        release {
            isMinifyEnabled = false
            externalNativeBuild {
                cmake {
                    val releaseFlags = arrayOf("-Wl,--gc-sections", "-flto", "-fno-unwind-tables", "-fno-asynchronous-unwind-tables")
                    cppFlags(*releaseFlags)
                    arguments(
                        "-DCMAKE_BUILD_TYPE=Release",
                        "-DCMAKE_CXX_FLAGS_RELEASE=-Oz -DNDEBUG -flto"
                    )
                }
            }
        }
        create("standalone") {
            initWith(getByName("release"))
            externalNativeBuild {
                cmake {
                    arguments += "-DANDROID_STL=none"
                }
            }
        }
    }

    lint {
        abortOnError       = true
        checkReleaseBuilds = false
    }
}