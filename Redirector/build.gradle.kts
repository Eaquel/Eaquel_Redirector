import java.nio.file.Paths
import org.eclipse.jgit.api.Git
import org.eclipse.jgit.storage.file.FileRepositoryBuilder

plugins {
    id("com.android.library")
    id("maven-publish")
    id("signing")
}

buildscript {
    repositories { mavenCentral() }
    dependencies { classpath("org.eclipse.jgit:org.eclipse.jgit:7.1.0.202411261347-r") }
}

val androidTargetSdkVersion: Int by rootProject.extra
val androidMinSdkVersion: Int by rootProject.extra
val androidBuildToolsVersion: String by rootProject.extra
val androidCompileSdkVersion: Int by rootProject.extra
val androidNdkVersion: String by rootProject.extra
val androidCmakeVersion: String by rootProject.extra

fun findInPath(executable: String): String? {
    val pathEnv = System.getenv("PATH") ?: return null
    return pathEnv.split(File.pathSeparator)
        .map {
            Paths.get("$it${File.separator}$executable${if (org.gradle.internal.os.OperatingSystem.current().isWindows) ".exe" else ""}").toFile()
        }
        .firstOrNull { it.exists() }?.absolutePath
}

android {
    compileSdk = androidCompileSdkVersion
    ndkVersion = androidNdkVersion
    buildToolsVersion = androidBuildToolsVersion

    buildFeatures {
        buildConfig = false
        prefabPublishing = true
        androidResources = false
        prefab = true
    }

    packaging {
        jniLibs { excludes += "**.so" }
    }

    prefab {
        register("lsplt") {
            headers = "src/main/jni/include"
        }
    }

    defaultConfig {
        minSdk = androidMinSdkVersion
        targetSdk = androidTargetSdkVersion
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
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
                    arguments(
                        "-DCMAKE_CXX_FLAGS_RELEASE=$configFlags",
                        "-DDEBUG_SYMBOLS_PATH=${project.buildDir.absolutePath}/symbols/$name",
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
        abortOnError = true
        checkReleaseBuilds = false
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/jni/CMakeLists.txt")
            version = androidCmakeVersion
        }
    }

    namespace = "io.github.eaquel.redirector.lsplt"

    publishing {
        singleVariant("release")    { withSourcesJar(); withJavadocJar() }
        singleVariant("standalone") { withSourcesJar(); withJavadocJar() }
    }
}

val symbolsReleaseTask = tasks.register<Jar>("generateReleaseSymbolsJar") {
    from("${project.buildDir.absolutePath}/symbols/release")
    exclude("**/dex_builder")
    archiveClassifier.set("symbols")
    archiveBaseName.set("release")
}

val symbolsStandaloneTask = tasks.register<Jar>("generateStandaloneSymbolsJar") {
    from("${project.buildDir.absolutePath}/symbols/standalone")
    exclude("**/dex_builder")
    archiveClassifier.set("symbols")
    archiveBaseName.set("standalone")
}

val ver = FileRepositoryBuilder().findGitDir(rootProject.file(".git")).runCatching {
    build().use { Git(it).describe().setTags(true).setAbbrev(0).call().removePrefix("v") }
}.getOrNull() ?: "0.0"

publishing {
    publications {
        fun MavenPublication.setup() {
            group = "io.github.eaquel.redirector"
            version = ver
            pom {
                name.set("lsplt")
                description.set("PLT hook framework — Eaquel/Eaquel_Redirector")
                url.set("https://github.com/Eaquel/Eaquel_Redirector")
                developers {
                    developer { name.set("Eaquel"); email.set("shakeofangel@gmail.com") }
                }
            }
        }
        register<MavenPublication>("lsplt") {
            artifactId = "lsplt"
            afterEvaluate { from(components.getByName("release")); artifact(symbolsReleaseTask) }
            setup()
        }
        register<MavenPublication>("lspltStandalone") {
            artifactId = "lsplt-standalone"
            afterEvaluate { from(components.getByName("standalone")); artifact(symbolsStandaloneTask) }
            setup()
        }
    }
    repositories {
        maven {
            name = "GitHubPackages"
            url = uri("https://maven.pkg.github.com/Eaquel/Eaquel_Redirector")
            credentials {
                username = System.getenv("GITHUB_ACTOR")
                password = System.getenv("GITHUB_TOKEN")
            }
        }
    }
}

signing {
    val signingKey = findProperty("signingKey") as String?
    val signingPassword = findProperty("signingPassword") as String?
    if (signingKey != null && signingPassword != null) {
        useInMemoryPgpKeys(signingKey, signingPassword)
    }
    sign(publishing.publications)
}
