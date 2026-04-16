# Eaquel_Redirector

![Android](https://img.shields.io/badge/Android-11%20(API%2030)%20--%2016%20(API%2036)-blue.svg)
![Arch](https://img.shields.io/badge/arch-armeabi--v7a%20%7C%20arm64--v8a%20%7C%20x86%20%7C%20x86--64-brightgreen.svg)
![Build](https://github.com/Eaquel/Eaquel_Redirector/actions/workflows/build.yml/badge.svg?branch=master&event=push)

Android için yüksek performanslı, saf **C++23** PLT (Procedure Linkage Table) hook kütüphanesi.  
Shared library'lerin dinamik sembollerini çalışma zamanında yönlendirmek için tasarlanmıştır.

---

## Özellikler

- Sembol adına göre PLT hook
- Sembol ön ekine göre toplu hook
- Offset aralığı ile filtreli hook
- GNU Hash, ELF Hash ve doğrusal sembol arama desteği
- Android packed relocation (APS2) çözümleyici
- `/proc/maps` güvenli okuma (socketpair + clone izolasyonu)
- Düz syscall desteği (`mmap` / `mremap` / `munmap`)
- 64-bit backup bölgesi yönetimi
- Desteklenen mimariler: `arm64-v8a`, `armeabi-v7a`, `x86`, `x86_64`
- C++23 standardı ile yazılmıştır

---

## Proje Yapısı

```
Eaquel_Redirector/
├── .github/
│   └── workflows/
│       └── build.yml
├── Gradle/
│   ├── gradle-wrapper.jar
│   ├── gradle-wrapper.properties
│   └── libs.versions.toml
├── Redirector/
│   ├── build.gradle.kts
│   └── Source/
│       └── Main/
│           ├── AndroidManifest.xml
│           └── Bridge/
│               ├── CMakeLists.txt
│               ├── Redirector.hpp
│               └── Redirector.cpp
├── build.gradle.kts
├── settings.gradle.kts
├── gradle.properties
├── gradlew
└── gradlew.bat
```

---

## Sürümler

### Android / Gradle

| Bileşen               | Sürüm            |
|-----------------------|------------------|
| Gradle                | 9.4.1            |
| Android Gradle Plugin | 9.1.1            |
| NDK                   | 29.0.14206865    |
| CMake                 | 4.1.0+           |
| Compile SDK           | 36               |
| Min SDK               | 30               |
| Target SDK            | 36               |
| C++ Standardı         | C++23            |
| JVM Uyumluluğu        | Java 21          |

### GitHub Actions

| Action                              | Sürüm  | Tarih          | Notlar                                                                |
|-------------------------------------|--------|----------------|-----------------------------------------------------------------------|
| `actions/checkout`                  | v6.0.2 | 9 Ocak 2026    | Repo kodunu çeker, her workflow'un temeli                             |
| `actions/setup-node`                | v6.3.0 | 3 Mart 2026    | Node.js kurar ve cache'ler                                            |
| `actions/setup-python`              | v6.2.0 | Ocak 2026      | Python kurar ve pip cache'ler                                         |
| `actions/setup-java`                | v5.2.0 | 21 Ocak 2026   | Java kurar (Temurin, Zulu vb.)                                        |
| `actions/setup-go`                  | v6.4.0 | 30 Mart 2026   | Go kurar ve cache'ler                                                 |
| `actions/cache`                     | v5.0.5 | 13 Nisan 2026  | Dependency cache'leme                                                 |
| `actions/upload-artifact`           | v7.0.1 | 10 Nisan 2026  | Dosyaları artifact olarak yükler (non-zipped destekli)                |
| `actions/download-artifact`         | v8.0.1 | Mart 2026      | Artifact'leri indirir                                                 |
| `actions/upload-pages-artifact`     | v3     | Güncel (v3 major) | GitHub Pages için dosyaları hazırlar                               |
| `actions/deploy-pages`              | v5.0.0 | 25 Mart 2026   | Pages sitesini deploy eder                                            |
| `actions/configure-pages`           | v5     | Güncel         | Pages ayarlarını yapılandırır                                         |
| `actions/github-script`             | v9.0.0 | 9 Nisan 2026   | JavaScript ile GitHub API kullan                                      |
| `docker/build-push-action`          | v7.1.0 | 10 Nisan 2026  | Docker image build ve push                                            |
| `docker/setup-buildx-action`        | v4     | Güncel         | Buildx kurar                                                          |
| `docker/login-action`               | v3     | Güncel         | Docker registry login                                                 |
| `peter-evans/create-pull-request`   | v8.1.1 | 10 Nisan 2026  | Otomatik PR oluşturur                                                 |
| `actions/stale`                     | v9     | Güncel         | Eski issue ve PR'leri stale yapar                                     |
| `github/super-linter`               | v7     | Güncel         | Birçok dil için linting                                               |
| `peaceiris/actions-gh-pages`        | v4     | Güncel         | Statik siteyi Pages'e deploy (alternatif)                             |
| `actions/upload-release-asset`      | v1.0.2 | Eski (2021+)   | GitHub Release'a asset yükler                                         |

> **Not:** `actions/setup-node` v6.3.0 (3 Mart 2026) — `devEngines` desteği, Node 24 uyumlu.  
> Runner'ın en az **v2.327+** olması gerekir.

---

## Derleme

### Gereksinimler

- Android Studio Meerkat (2024.3+) veya daha yeni
- JDK 21
- Android NDK r29

### Komut Satırı

```bash
# Debug derlemesi
./gradlew :Redirector:assembleDebug

# Release derlemesi
./gradlew :Redirector:assembleRelease

# Maven Local'e yayın
./gradlew :Redirector:publishToMavenLocal
```

---

## Kullanım

### Gradle Bağımlılığı

```kotlin
dependencies {
    implementation("io.github.eaquel.redirector:Redirector:<versiyon>")
}
```

### Temel API Kullanımı

```cpp
#include "Redirector.hpp"

MapInfo* maps = lsplt_scan_maps("self");

struct stat st;
stat("/system/lib64/libc.so", &st);

// Hook kaydet
lsplt_register_hook(st.st_dev, st.st_ino, "open", (void*)my_open, (void**)&orig_open);

// Hook'ları uygula
lsplt_commit_hook_manual(maps);

lsplt_free_maps(maps);
```

### API Özeti

| Fonksiyon | Açıklama |
|-----------|----------|
| `lsplt_scan_maps(pid)` | `/proc/<pid>/maps` okur |
| `lsplt_free_maps(maps)` | Map bilgilerini serbest bırakır |
| `lsplt_register_hook(dev, inode, symbol, cb, backup)` | Tek sembol hook kaydı |
| `lsplt_register_hook_by_prefix(dev, inode, prefix, cb, backup)` | Önek ile toplu hook |
| `lsplt_register_hook_with_offset(...)` | Offset ile filtreli hook |
| `lsplt_commit_hook_manual(maps)` | Hook'ları uygula |
| `lsplt_commit_hook()` | Kısa yol |
| `invalidate_backups()` | Yedekleri temizle |
| `lsplt_free_resources()` | Tüm kaynakları serbest bırak |
