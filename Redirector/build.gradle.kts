plugins {
    id("com.android.library")
    id("maven-publish")
    id("signing")
}

val androidTargetSdkVersion  : Int    by rootProject.extra
val androidMinSdkVersion     : Int    by rootProject.extra
val androidBuildToolsVersion : String by rootProject.extra
val androidCompileSdkVersion : Int    by rootProject.extra
val androidNdkVersion        : String by rootProject.extra
val androidCmakeVersion      : String by rootProject.extra

android {
    compileSdk       = androidCompileSdkVersion
    ndkVersion       = androidNdkVersion
    buildToolsVersion = androidBuildToolsVersion
    namespace        = "com.eaquel.redirector"

    sourceSets {
        getByName("main") {
            manifest.srcFile("Source/Main/AndroidManifest.xml")
        }
    }

    buildFeatures {
        buildConfig        = false
        prefabPublishing   = true
        androidResources   = false
        prefab             = true
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
        minSdk    = androidMinSdkVersion
        targetSdk = androidTargetSdkVersion
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
        abortOnError         = true
        checkReleaseBuilds   = false
    }

    externalNativeBuild {
        cmake {
            path    = file("Source/Main/Bridge/CMakeLists.txt")
            version = androidCmakeVersion
        }
    }

    publishing {
        singleVariant("release")    { withSourcesJar(); withJavadocJar() }
        singleVariant("standalone") { withSourcesJar(); withJavadocJar() }
    }
}

val buildDir = layout.buildDirectory.get().asFile.absolutePath

val symbolsReleaseTask = tasks.register<Jar>("generateReleaseSymbolsJar") {
    from("$buildDir/symbols/release")
    exclude("**/dex_builder")
    archiveClassifier.set("symbols")
    archiveBaseName.set("release")
}

val symbolsStandaloneTask = tasks.register<Jar>("generateStandaloneSymbolsJar") {
    from("$buildDir/symbols/standalone")
    exclude("**/dex_builder")
    archiveClassifier.set("symbols")
    archiveBaseName.set("standalone")
}

val ver: String = runCatching {
    val proc = ProcessBuilder("git", "describe", "--tags", "--abbrev=0")
        .directory(rootProject.projectDir)
        .redirectErrorStream(true)
        .start()
    proc.inputStream.bufferedReader().readText().trim().removePrefix("v")
        .takeIf { it.isNotBlank() } ?: "0.0"
}.getOrDefault("0.0")

publishing {
    publications {
        fun MavenPublication.setup() {
            group   = "io.github.eaquel.redirector"
            version = ver
            pom {
                name.set("Eaquel_Redirector")
                description.set("PLT hook framework — Eaquel/Eaquel_Redirector")
                url.set("https://github.com/Eaquel/Eaquel_Redirector")
                licenses {
                    license {
                        name.set("Apache-2.0")
                        url.set("https://www.apache.org/licenses/LICENSE-2.0")
                    }
                }
                developers {
                    developer { name.set("Eaquel"); email.set("shakeofangel@gmail.com") }
                }
                scm {
                    connection.set("scm:git:git://github.com/Eaquel/Eaquel_Redirector.git")
                    url.set("https://github.com/Eaquel/Eaquel_Redirector")
                }
            }
        }
        register<MavenPublication>("Redirector") {
            artifactId = "Redirector"
            afterEvaluate {
                from(components.getByName("release"))
                artifact(symbolsReleaseTask)
            }
            setup()
        }
        register<MavenPublication>("RedirectorStandalone") {
            artifactId = "Redirector-standalone"
            afterEvaluate {
                from(components.getByName("standalone"))
                artifact(symbolsStandaloneTask)
            }
            setup()
        }
    }
    repositories {
        maven {
            name = "GitHubPackages"
            url  = uri("https://maven.pkg.github.com/Eaquel/Eaquel_Redirector")
            credentials {
                username = System.getenv("GITHUB_ACTOR")
                password = System.getenv("GITHUB_TOKEN")
            }
        }
        mavenLocal()
    }
}

signing {
    val signingKey      = findProperty("signingKey")      as String?
    val signingPassword = findProperty("signingPassword") as String?
    if (signingKey != null && signingPassword != null) {
        useInMemoryPgpKeys(signingKey, signingPassword)
    }
    sign(publishing.publications)
}
