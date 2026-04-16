# Eaquel_Redirector

![License](https://img.shields.io/badge/license-LGPL--3.0-orange.svg)
![Android](https://img.shields.io/badge/Android-11%20(API%2030)%20--%2016%20(API%2036)-blue.svg)
![Arch](https://img.shields.io/badge/arch-armeabi--v7a%20%7C%20arm64--v8a%20%7C%20x86%20%7C%20x86--64-brightgreen.svg)
![Build](https://github.com/Eaquel/Eaquel_Redirector/actions/workflows/build.yml/badge.svg?branch=master&event=push)

Android için yüksek performanslı, saf C++23 PLT (Procedure Linkage Table) hook kütüphanesi.  
Shared library'lerin dinamik sembollerini çalışma zamanında yönlendirmek (redirect) için tasarlanmıştır.

---

## Özellikler

- Sembol adına göre PLT hook (`lsplt_register_hook`)
- Sembol ön ekine (prefix) göre toplu hook (`lsplt_register_hook_by_prefix`)
- Offset aralığı filtreli hook (`lsplt_register_hook_with_offset`)
- GNU Hash, ELF Hash ve doğrusal sembol arama
- Android packed relocation (APS2) çözümleyici
- `/proc/maps` güvenli okuma (socketpair + clone izolasyonu)
- `mmap` / `mremap` / `munmap` düz syscall (libc bypass)
- 64-bit backup bölgesi yönetimi ve otomatik geri yükleme
- Esnek sayfa boyutu desteği (`ANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES`)
- Desteklenen ABI: `arm64-v8a`, `armeabi-v7a`, `x86`, `x86_64`
- C++23, NDK 29, CMake 3.22+, AGP 8.9, Gradle 8.13

---

## Proje Yapısı

```
Eaquel_Redirector/
│
├── .github/
│   └── workflows/
│       ├── build.yml          # CI — her push'ta debug + release derleme
│       ├── pages.yml          # GitHub Pages — Doxygen dokümantasyon yayını
│       └── maven.yml          # Maven Central / GitHub Packages yayın iş akışı
│
├── gradle/
│   ├── wrapper/
│   │   ├── gradle-wrapper.jar         # Gradle wrapper çalıştırıcısı (binary)
│   │   └── gradle-wrapper.properties  # Gradle 8.13 dağıtım URL'si
│   └── libs.versions.toml             # Merkezi versiyon kataloğu (AGP, NDK, jgit...)
│
├── Redirector/                        # Ana kütüphane modülü
│   ├── Source-build.gradle.kts        # Modül build scripti (AGP library + maven-publish + signing)
│   ├── Main-AndroidManifest.xml       # Kütüphane manifest (namespace tanımı)
│   └── Bridge/                        # Native (C++23) çekirdek
│       ├── CMakeLists.txt             # CMake build tanımı (shared + static hedefler)
│       ├── Redirector.hpp             # Genel başlık: syscall wrappers, ELF structs, API bildirimleri
│       └── Redirector.cpp             # Uygulama: ELF ayrıştırma, PLT arama, hook motoru
│
├── build.gradle.kts                   # Root build script — SDK/NDK/CMake sürüm sabitleri
├── settings.gradle.kts                # Proje adı, modül dahil etme, plugin yönetimi
└── gradle.properties                  # Android Gradle özellikleri + JVM/cache optimizasyonları
```

---

## Bağımlılıklar ve Araç Sürümleri

| Araç / Bileşen          | Sürüm                          |
|-------------------------|-------------------------------|
| Gradle                  | 8.13                          |
| Android Gradle Plugin   | 8.9.0                         |
| NDK                     | 29.0.14206865                 |
| CMake                   | 3.22.1+                       |
| Build Tools             | 36.0.0                        |
| Compile SDK             | 36                            |
| Min SDK                 | 30 (Android 11)               |
| Target SDK              | 36                            |
| C++ Standardı           | C++23                         |
| JVM Uyumluluğu          | Java 17                       |
| JGit                    | 7.1.0.202411261347-r          |

---

## Derleme

### Gereksinimler

- Android Studio Meerkat (2024.3+) veya komut satırı araçları
- JDK 17+
- Android NDK 29 (SDK Manager üzerinden kurulabilir)

### Komut Satırı

```bash
# Debug derlemesi
./gradlew :Redirector:assembleDebug

# Release derlemesi
./gradlew :Redirector:assembleRelease

# Standalone (STL bağımsız) release
./gradlew :Redirector:assembleStandalone

# Maven Local yayını
./gradlew :Redirector:publishToMavenLocal
```

### İmzalama (Opsiyonel)

`gradle.properties` dosyasına veya ortam değişkeni olarak ekleyin:

```properties
signingKey=<base64-pgp-private-key>
signingPassword=<anahtar-sifresi>
```

---

## Kullanım

### Bağımlılık Ekleme

```kotlin
dependencies {
    implementation("io.github.eaquel.redirector:lsplt:<versiyon>")
}
```

### Temel API

```cpp
#include "Redirector.hpp"

// Hedef .so dosyasını bul
MapInfo* maps = lsplt_scan_maps("self");

// dev ve inode bilgisini al
struct stat st;
stat("/system/lib64/libc.so", &st);

// Hook kaydet
lsplt_register_hook(st.st_dev, st.st_ino, "open", (void*)my_open, (void**)&orig_open);

// Hook uygula
lsplt_commit_hook_manual(maps);

lsplt_free_maps(maps);
```

### API Özeti

| Fonksiyon | Açıklama |
|-----------|----------|
| `lsplt_scan_maps(pid)` | `/proc/<pid>/maps` okur, `MapInfo*` döner |
| `lsplt_free_maps(maps)` | `MapInfo*` belleğini serbest bırakır |
| `lsplt_register_hook(dev, inode, symbol, cb, backup)` | Tam sembol adıyla hook kaydeder |
| `lsplt_register_hook_by_prefix(dev, inode, prefix, cb, backup)` | Ön ek eşleşmesiyle toplu hook kaydeder |
| `lsplt_register_hook_with_offset(dev, inode, offset, size, symbol, cb, backup)` | Offset aralığı filtreli hook kaydeder |
| `lsplt_commit_hook_manual(maps)` | Kayıtlı hook'ları verilen map üzerinde uygular |
| `lsplt_commit_hook()` | `scan_maps("self")` + `commit_hook_manual` kısayolu |
| `invalidate_backups()` | Yedek bölgeleri temizler, PLT'yi orijinal değerlere döndürür |
| `lsplt_free_resources()` | Tüm global durumu serbest bırakır |

---

## CI / CD İş Akışları

### `.github/workflows/build.yml`
Her `push` ve `pull_request` olayında tetiklenir. Debug + Release + Standalone varyantlarını derler, başarısızlıkta derlemeyi durdurur.

### `.github/workflows/pages.yml`
`master` branch'e push olduğunda Doxygen ile API dokümantasyonu üretir ve GitHub Pages'e yayınlar.

### `.github/workflows/maven.yml`
Tag push (`v*`) veya manuel tetikleme ile çalışır. Artefaktları Maven Central ve GitHub Packages'e yayınlar. `SIGNING_KEY`, `SIGNING_PASSWORD`, `OSSRH_USERNAME`, `OSSRH_PASSWORD`, `GITHUB_TOKEN` secret'larına ihtiyaç duyar.

---

## Mimari Notlar

**`Redirector.hpp`** — Tüm syscall wrapper'larını (`sys_mmap`, `sys_mremap`, `sys_munmap`), ELF veri yapılarını (`ElfInfo`, `MapEntry`, `MapInfo`) ve public API bildirimlerini içerir. Derleyici optimizasyonu için `[[gnu::always_inline]]` kullanır. ARM, AArch64, i386, x86_64, RISC-V için ayrı satır içi assembly bloğu içerir.

**`Redirector.cpp`** — Şu bileşenleri uygular:
- SLEB128 çözümleyici (Android APS2 packed relocation için)
- ELF başlık ayrıştırıcı + GNU Hash / ELF Hash / doğrusal sembol arama
- Android packed relocation açıcı (`elfutil_unpack_android_relocs`)
- PLT adres bulucu (`looper`, `looper_by_prefix`)
- `/proc/maps` güvenli okuyucu (fork + socketpair ile fd geçişi)
- Hook motoru: `build_hook_infos`, `filter_hook_infos`, `merge_hook_infos`, `do_hook_addr`
- Thread-safe global durum (`pthread_mutex_t`)

---

## Lisans

Bu proje **LGPL-3.0** lisansı ile lisanslanmıştır. Ayrıntılar için [LICENSE](./LICENSE) dosyasına bakın.
