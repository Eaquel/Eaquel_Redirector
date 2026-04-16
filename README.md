<div align="center">

# Eaquel Redirector

[![Android Sürümü](https://img.shields.io/badge/Android-11%20(API%2030)%20--%2016%20(API%2036)-1a73e8?style=for-the-badge&logo=android&logoColor=white)](https://developer.android.com)
[![Mimari](https://img.shields.io/badge/Mimari-arm64--v8a%20%7C%20armeabi--v7a%20%7C%20x86__64-00c853?style=for-the-badge&logo=cpu&logoColor=white)](#mimari-desteği)
[![C++ Standardı](https://img.shields.io/badge/C++23-Saf%20Native-9c27b0?style=for-the-badge&logo=cplusplus&logoColor=white)](#derleme-gereksinimleri)
[![Derleme](https://img.shields.io/badge/Derleme-Başarılı-43a047?style=for-the-badge&logo=githubactions&logoColor=white)](#derleme)
[![Sürüm](https://img.shields.io/badge/Sürüm-1.0.0-b71c1c?style=for-the-badge&logo=semver&logoColor=white)](#sürümler)
[![Lisans](https://img.shields.io/badge/Lisans-Araştırma%20Amaçlı-ff6f00?style=for-the-badge&logo=opensourceinitiative&logoColor=white)](#lisans-ve-yasal-uyarı)

**Eaquel_Redirector** — Modern Android ekosistemi için tasarlanmış,  
yüksek performanslı, Stealth odaklı ve saf C++23 tabanlı  
Procedure Linkage Table ve Global Offset Table Kancalama Çerçevesidir.

</div>

---

## İçindekiler

- [Eaquel\_Redirector Nedir?](#eaquel_redirector-nedir)
- [Neden Eaquel\_Redirector?](#neden-eaquel_redirector)
- [Temel Kavramlar Sözlüğü](#temel-kavramlar-sözlüğü)
- [Mimari ve Çalışma Prensibi](#mimari-ve-çalışma-prensibi)
- [Temel Özellikler](#temel-özellikler)
- [Proje Yapısı](#proje-yapısı)
- [Sürümler ve Bağımlılıklar](#sürümler-ve-bağımlılıklar)
- [Derleme Gereksinimleri](#derleme-gereksinimleri)
- [Derleme](#derleme)
- [Kurulum ve Entegrasyon](#kurulum-ve-entegrasyon)
- [Hızlı Başlangıç ve API Kullanımı](#hızlı-başlangıç-ve-api-kullanımı)
- [API Referans Tablosu](#api-referans-tablosu)
- [Stealth Modları](#stealth-modları)
- [Rust Submodule Entegrasyonu](#rust-submodule-entegrasyonu)
- [Lisans ve Yasal Uyarı](#lisans-ve-yasal-uyarı)

---

## Eaquel\_Redirector Nedir?

**Eaquel_Redirector**, Android işletim sistemi üzerinde çalışan uygulamaların dinamik bağlayıcı mekanizmalarına müdahale etmeye yarayan, sıfırdan tasarlanmış bir C++23 kancalama çerçevesidir. Kancalama (hooking), bir programın çalışması sırasında belirli bir fonksiyonun orijinal adresi yerine sizin yazdığınız özel bir fonksiyona yönlendirilmesi işlemidir.

Bu çerçeve özellikle şu senaryolar için geliştirilmiştir:

- Güvenlik araştırmacılarının sistem kütüphanelerinin davranışını çalışma zamanında incelemesi
- ReZygisk ve CSOLoader gibi Zygote tabanlı modüllerin, sistem süreçlerine temiz ve iz bırakmayan biçimde entegre olması
- Android 14, 15 ve 16 sürümlerinde giderek sertleşen bellek güvenliği politikaları altında güvenilir çalışma

---

## Neden Eaquel\_Redirector?

### Mevcut Çözümlerle Karşılaştırma

| Özellik | LSPlt / PLStl | Eaquel\_Redirector |
|---|---|---|
| Android 16 QPR2 Uyumu | Çöküyor | Tam Uyumlu |
| 16 KB Sayfa Hizalaması | Desteksiz | Doğrudan Destekleniyor |
| DT\_RELR Paketli Yerleşim | Çözülemiyor | Yerleşik Çözümleyici |
| Sembol Önbellekleme | Her seferinde yeniden arama | O(1) Sabit Zaman |
| Stealth Modu | İz Bırakıyor | 3 Kademe Stealth |
| GOT Doğrudan Yazımı | Yok | Destekleniyor |
| Thread Güvenliği | Kısmi | RAII Mutex Yönetimi |
| Derleme Boyutu | Büyük | LTO + Section GC ile Küçük |

### Öne Çıkan Avantajlar

**Android 16 QPR2 ve 16 KB Sayfa Hizalaması Tam Uyumu**

Android 16 ile birlikte, özellikle yüksek performanslı ARM64 cihazlarda bellek sayfası boyutu 4 KB'dan 16 KB'a yükseltilmiştir. Bu değişiklik belleğe yazma işlemi yapan eski kancalama çözümlerini çökertiyor ya da güvenlik duvarlarına takılmalarına neden oluyordu. Eaquel_Redirector bu yeni hizalama gereksinimlerini başından itibaren göz önünde bulundurarak tasarlanmıştır. Aynı zamanda Android'in RELRO (Relocation Read-Only) zorunlu uygulama politikasıyla da tam uyumludur.

**Modern ELF Formatı Çözümlemesi: DT_RELR ve APS2**

Geleneksel kancalama araçları yalnızca klasik REL ve RELA bölümlerini anlayabilir. Ancak modern Android sistem kütüphaneleri, ikili dosya boyutunu küçültmek için DT_RELR adı verilen sıkıştırılmış yerleşim formatını kullanmaya başlamıştır. Eaquel_Redirector bu formatı yerleşik olarak çözümleyebildiği için Android 14 ve sonrasındaki tüm sistem kütüphanelerine sorunsuz kanca atabilir.

**Üç Kademe Stealth Mimarisi**

Kancalama işlemleri normalde bellek haritasında açık izler bırakır; bu izler Anti-Cheat sistemleri veya uygulama bütünlük denetçileri tarafından tespit edilebilir. Eaquel_Redirector üç farklı gizlilik modu sunarak bu izlerin yok edilmesini sağlar.

**O(1) Sabit Zamanlı Sembol Önbellekleme**

Zygote gibi binlerce sembol barındıran süreçlerde her kanca atma girişiminde sıfırdan sembol araması yapmak ciddi performans kaybına yol açar. Eaquel_Redirector, `std::unordered_map` destekli `g_sym_cache` adlı bir sembol önbelleği kullanır; aynı sembole yapılan tüm sonraki erişimler sabit sürede tamamlanır.

**Sıfır İz Derleme (Zero Footprint Build)**

LTO (Link Time Optimization), `-fvisibility=hidden` ve Section Garbage Collection bir arada kullanılarak derlenmiştir. Bu sayede RTTI tabloları veya C++ istisna yönetimi artıkları gibi gereksiz veriler hedef sürecin belleğine bırakılmaz.

---

## Temel Kavramlar Sözlüğü

| Terim | Açıklama |
|---|---|
| **ELF** | Android ve Linux'ta kullanılan standart ikili dosya biçimi. Her `.so` kütüphane ve uygulama ELF formatındadır. |
| **Shared Library** | Birden fazla süreç tarafından belleğe yüklenerek kullanılan `.so` uzantılı kod kitaplığı. |
| **Dinamik Bağlayıcı** | Uygulama başlatıldığında kütüphaneleri belleğe yükleyen ve fonksiyon adreslerini tablolara yazan sistem bileşeni (`linker64`). |
| **Sembol** | ELF dosyalarında her fonksiyon veya değişkene verilen isim ve adres çifti (`open`, `malloc` vb.). |
| **PLT** | Fonksiyon çağrısını gerçek adrese yönlendiren küçük atlama tablosu (Procedure Linkage Table). |
| **GOT** | Dinamik bağlayıcının paylaşılan kütüphane fonksiyonlarının gerçek adreslerini yazdığı tablo (Global Offset Table). |
| **Kancalama** | Bir fonksiyona yapılan çağrıyı orijinal hedef yerine başka bir fonksiyona yönlendirme işlemi. |
| **DT\_RELR** | Android 14+ sistem kütüphanelerinde kullanılan sıkıştırılmış yerleşim formatı. |
| **APS2** | Android Paketlenmiş Semboller Sürüm 2; Google'ın araç zincirinin ürettiği modern sembol formatı. |
| **Zygote** | Tüm Android uygulama süreçlerinin çatallandığı (fork) ana süreç. |
| **ReZygisk** | Magisk tabanlı Zygisk implementasyonu; Zygote'a modül yükleme altyapısı sağlar. |
| **CSOLoader** | ReZygisk üzerinde özel paylaşılan kütüphanelerin yüklenmesini kolaylaştıran bileşen. |
| **RELRO** | Kütüphane yüklendikten sonra yerleşim tablolarını salt okunur hale getiren Android güvenlik politikası. |
| **mremap** | Bellek bölgesini farklı bir adrese taşıyan Linux sistem çağrısı. |
| **LTO** | Tüm derleme birimlerini birlikte optimize eden teknik (Link Time Optimization). |
| **RAII** | C++ kaynak yönetim felsefesi; nesne yok edildiğinde kaynaklar otomatik serbest bırakılır. |
| **Cache Temizliği** | Bellek kodu değiştirildiğinde ARM64 talimat ve veri önbelleklerinin temizlenmesi işlemi. |

---

## Mimari ve Çalışma Prensibi

Eaquel_Redirector'ın bir kancayı yerleştirmesi beş aşamada gerçekleşir:

```
Aşama 1 — Bellek Haritasını Tarama
  /proc/<pid>/maps
       |
       +-- socketpair + clone izolasyonu (güvenli, ayrıcalıksız okuma)
       +-- /apex/ bölgeleri atlanır
       +-- linker_alloc alanı korunur
       |
       v
Aşama 2 — ELF Ayrıştırma
  ELF Başlık Doğrulama
  Sembol Tablosu:  [1] GNU Hash  ->  [2] ELF Hash  ->  [3] Doğrusal Tarama
  Yerleşim Bölümü: [1] REL/RELA  ->  [2] DT_RELR   ->  [3] APS2
       |
       v
Aşama 3 — Önbellekleme
  g_sym_cache: unordered_map<isim, adres>
  Ilk erişim: O(log n)  —>  Sonraki erişimler: O(1)
       |
       v
Aşama 4 — Kanca Yazma
  mprotect ile yazma izni aç
  Orijinal değeri yedekle
  PLT / GOT girişine yeni adres yaz
  mprotect ile korumayı geri al
  ARM64: dcache + icache temizle
       |
       v
Aşama 5 — Stealth İz Silme
  Seçilen gizlilik moduna göre bellek izleri temizlenir
  Anti-Cheat ve bütünlük denetçileri için görünmez hale getirilir
```

### Veri Akışı

```
Hedef Süreç Belleği
        |
        |  [PLT Kancası]
        v
  libc.so::open  ------>  Eaquel_Redirector
  (Orijinal)                    |
                           [Stealth Katmanı]
                           [Sembol Önbelleği]
                                |
  .got.plt  <------------------ +  [GOT Yazımı]
  (Adres Tablosu)               |
                                |
  Kullanıcı Kodu  <------------ +  [Yönlendirme]
  (Özel Fonksiyon)
```

---

## Temel Özellikler

### Kancalama Yetenekleri

| Özellik | Açıklama |
|---|---|
| PLT Kancalama | Dinamik kütüphane çağrılarının geçtiği PLT atlama tablosuna müdahale eder |
| GOT Kancalama | Adres tablosuna doğrudan yazar; PLT'den daha düşük seviyeli müdahale |
| Önek Tabanlı Toplu Kancalama | Belirtilen ön ek ile başlayan tüm sembolleri tek çağrıyla kancalar |
| Ofset Aralığı Filtreli Kancalama | Büyük kütüphanelerin yalnızca belirli bölümlerini hedef alır |

### Güvenlik ve Gizlilik

| Özellik | Açıklama |
|---|---|
| 3 Kademe Stealth Mimarisi | DIRECT\_PATCH, TRAMPOLINE ve BACKUP\_FULL modları |
| Güvenli Bellek Haritası Okuma | `socketpair + clone` ile izole alt süreçte `/proc/maps` okunur |
| Thread Güvenli Mimari | RAII prensibiyle yönetilen `std::mutex` ve `std::lock_guard` |
| RELRO Uyumlu Yazma Döngüsü | Kancalama için anlık yazma izni açılıp kapatılır |

### Performans

| Özellik | Açıklama |
|---|---|
| Sembol Önbelleği | `std::unordered_map` ile O(1) erişim; tekrarlı aramalar hızlandırılır |
| ARM64 Cache Temizliği | Satır içi Assembly ile dcache + icache temizlenir; pipeline tutarsızlığı önlenir |
| Akıllı Harita Filtreleme | `/apex/` ve `linker_alloc` alanları otomatik atlanır |
| LTO + Section GC | Kullanılmayan kod bölümleri ve RTTI artıkları hedef belleğe bırakılmaz |

### ELF Format Desteği

| Format | Açıklama |
|---|---|
| REL / RELA | Klasik yerleşim tabloları |
| DT\_RELR | Android 14+ sıkıştırılmış paketlenmiş yerleşim formatı |
| APS2 | Android Paketlenmiş Semboller Sürüm 2 |
| GNU Hash | Bloom filtreli hızlı sembol araması |
| ELF Hash | GNU Hash bulunamadığında fallback |

---

## Proje Yapısı

```
Eaquel_Redirector/
|
+-- .github/
|   +-- workflows/
|       +-- build.yml            — GitHub Actions derleme ve yayın hattı
|
+-- Gradle/
|   +-- gradle-wrapper.jar       — Gradle sarmalayıcı JAR dosyası
|   +-- gradle-wrapper.properties— Gradle sürüm ve dağıtım yapılandırması
|   +-- libs.versions.toml       — Merkezi sürüm kataloğu (Version Catalog)
|
+-- Redirector/
|   +-- build.gradle.kts         — Android Kütüphane modülü derleme betiği
|   +-- Source/
|       +-- Main/
|           +-- AndroidManifest.xml
|           +-- Bridge/
|               +-- CMakeLists.txt   — C++ derleme yapılandırması
|               +-- Redirector.hpp   — Public API başlık dosyası
|               +-- Redirector.cpp   — Tüm kancalama mantığının implementasyonu
|
+-- build.gradle.kts             — Kök proje derleme betiği
+-- settings.gradle.kts          — Proje modül ayarları
+-- gradle.properties            — JVM ve Gradle performans özellikleri
+-- gradlew                      — Unix/macOS Gradle sarmalayıcı betiği
+-- gradlew.bat                  — Windows Gradle sarmalayıcı betiği
```

---

## Sürümler ve Bağımlılıklar

### Android ve Derleme Araç Zinciri

| Bileşen | Sürüm | Açıklama |
|---|---|---|
| Gradle | 9.4.1 | Proje derleme otomasyon sistemi |
| Android Gradle Plugin | 9.1.1 | Android projelerini Gradle ile derleyen eklenti |
| Android NDK | r29.0.14206865 | C ve C++ Android kütüphaneleri geliştirme kiti |
| CMake | 4.1.0+ | Platforma bağımsız derleme yapılandırması |
| Derleme Hedefi API | 36 (Android 16) | Derlenen en yüksek Android API sürümü |
| Minimum API | 30 (Android 11) | Desteklenen en düşük Android sürümü |
| C++ Standardı | C++23 | Kullanılan C++ sürümü |
| JVM Uyumluluğu | Java 21 | Kotlin ve Gradle için gerekli JVM sürümü |

### Desteklenen Mimariler

| Mimari | Açıklama | Hedef Cihaz |
|---|---|---|
| arm64-v8a | 64-bit ARM | 2016 sonrası modern akıllı telefonlar |
| armeabi-v7a | 32-bit ARM | Eski ve düşük güçlü ARM cihazlar |
| x86_64 | 64-bit Intel/AMD | Android emülatörleri, Chromebook x86 |

### GitHub Actions Sürümleri

| Eylem | Sürüm | Görev |
|---|---|---|
| actions/checkout | v6.0.2 | Depo kaynak kodunu çeker |
| actions/setup-java | v5.2.0 | JDK kurar |
| actions/cache | v5.0.5 | Bağımlılık önbellekleme |
| actions/upload-artifact | v7.0.1 | Derleme çıktılarını artifact olarak yükler |
| actions/download-artifact | v8.0.1 | Yüklenmiş artifactları indirir |
| actions/setup-node | v6.3.0 | Node.js kurar ve önbellekler |
| actions/setup-python | v6.2.0 | Python kurar |
| actions/setup-go | v6.4.0 | Go derleyicisi kurar |
| actions/upload-pages-artifact | v3 | GitHub Pages için dosyaları hazırlar |
| actions/deploy-pages | v5.0.0 | GitHub Pages sitesini yayına alır |
| actions/configure-pages | v5 | GitHub Pages ayarlarını yapılandırır |
| actions/github-script | v9.0.0 | JavaScript ile GitHub API erişimi |
| docker/build-push-action | v7.1.0 | Docker imajı derler ve depoya yükler |
| docker/setup-buildx-action | v4 | Gelişmiş Docker Buildx kurar |
| docker/login-action | v3 | Docker kayıt defterine oturum açar |
| peter-evans/create-pull-request | v8.1.1 | Otomatik çekme isteği oluşturur |
| actions/stale | v9 | Eski sorun ve çekme isteklerini işaretler |
| github/super-linter | v7 | Çoklu dil kod kalite denetimi |
| peaceiris/actions-gh-pages | v4 | Statik siteyi Pages'e gönderir |
| actions/upload-release-asset | v1.0.2 | GitHub Release'e dosya yükler |

> **Not:** `actions/setup-node` v6.3.0, `devEngines` alanı desteği ve Node.js 24 uyumluluğu getirir. GitHub Actions çalışıcısının en az **v2.327** sürümünde olması gerekir.

---

## Derleme Gereksinimleri

| Araç | Minimum Sürüm | Nereden İndirilir |
|---|---|---|
| Android Studio | Meerkat (2024.3+) | developer.android.com/studio |
| Java Development Kit | 21 (LTS) | adoptium.net |
| Android NDK | r29.0.14206865 | Android Studio SDK Manager |
| CMake | 4.1.0+ | Android Studio SDK Manager |

---

## Derleme

### Komut Satırından Derleme

```bash
# Hata ayıklama modunda derleme — imzasız, hata bilgisi zengin
./gradlew :Redirector:assembleDebug

# Yayın modunda derleme — optimize edilmiş, küçük boyutlu
./gradlew :Redirector:assembleRelease

# Yerel Maven deposuna yayımlama
./gradlew :Redirector:publishToMavenLocal

# Tüm mimariler için tek seferde derleme
./gradlew :Redirector:assembleRelease \
    -PabiFilters="arm64-v8a,armeabi-v7a,x86_64"

# Önbellek temizleyerek temiz derleme
./gradlew clean :Redirector:assembleRelease
```

### GitHub Actions Otomatik Derleme Hattı

`master` dalına her kod gönderiminde ve her çekme isteğinde tetiklenir:

```
git push / pull request
        |
        v
  Java 21 Kurulumu
        |
        v
  Gradle Bagimlılık Onbellekleme (~/.gradle/caches)
        |
        v
  CCache (C++ Derleyici Onbellegi)
        |
        v
  ./gradlew :Redirector:assemble
  arm64-v8a + armeabi-v7a + x86_64
        |
        v
  AAR dosyasını GitHub Actions Artifact olarak yukle
```

---

## Kurulum ve Entegrasyon

### Gradle Bağımlılığı (AAR ve Prefab)

```kotlin
android {
    buildFeatures {
        prefab = true
    }
}

dependencies {
    implementation("io.github.eaquel.redirector:eaquel_redirector:1.0.0")
}
```

### CMakeLists.txt Bağlama

```cmake
find_package(eaquel_redirector REQUIRED CONFIG)

target_link_libraries(
    SizinKutuphaneniz
    PRIVATE
    eaquel_redirector::eaquel_redirector
)
```

---

## Hızlı Başlangıç ve API Kullanımı

### Temel Kanca Atma

```cpp
#include "Redirector.hpp"
#include <sys/stat.h>
#include <fcntl.h>

static int (*orijinal_open)(const char* yol, int bayraklar, ...) = nullptr;

int benim_open_kancam(const char* yol, int bayraklar, ...) {
    if (strcmp(yol, "/data/blocked_file") == 0) {
        errno = EACCES;
        return -1;
    }
    return orijinal_open(yol, bayraklar);
}

void kankalari_kur() {
    // 1. Bellek haritasını tara
    MapInfo* harita = redirector_scan_maps("self");

    // 2. Hedef kütüphanenin cihaz ve inode bilgisini al
    struct stat dosya_bilgisi;
    stat("/system/lib64/libc.so", &dosya_bilgisi);

    // 3. Kancayı kaydet
    redirector_register_hook(
        dosya_bilgisi.st_dev,
        dosya_bilgisi.st_ino,
        "open",
        reinterpret_cast<void*>(benim_open_kancam),
        reinterpret_cast<void**>(&orijinal_open)
    );

    // 4. Kancaları belleğe uygula
    redirector_commit_hook_manual(harita);

    // 5. Bellek haritası nesnesini serbest bırak
    redirector_free_maps(harita);
}
```

### Global Offset Table Kancalama

```cpp
redirector_register_got_hook(
    dosya_bilgisi.st_dev,
    dosya_bilgisi.st_ino,
    "malloc",
    reinterpret_cast<void*>(benim_malloc_kancam),
    reinterpret_cast<void**>(&orijinal_malloc)
);
```

### Önek ile Toplu Kancalama

```cpp
struct stat ssl_bilgisi;
stat("/system/lib64/libssl.so", &ssl_bilgisi);

redirector_register_hook_by_prefix(
    ssl_bilgisi.st_dev,
    ssl_bilgisi.st_ino,
    "ssl_",
    reinterpret_cast<void*>(genel_ssl_kancasi),
    reinterpret_cast<void**>(&orijinal_ssl_fonk)
);
```

### Kancayı Geri Alma

```cpp
redirector_unhook(
    dosya_bilgisi.st_dev,
    dosya_bilgisi.st_ino,
    "open"
);
```

### ReZygisk ve CSOLoader için Optimum Kurulum

```cpp
er_init_for_zygisk();
```

---

## API Referans Tablosu

| Fonksiyon | Açıklama |
|---|---|
| `redirector_scan_maps(pid)` | Belirtilen sürecin bellek haritasını `socketpair + clone` izolasyonu ile okur. `"self"` mevcut süreci ifade eder. |
| `redirector_free_maps(maps)` | `scan_maps` ile elde edilen nesneyi serbest bırakır; bellek sızıntısını önler. |
| `redirector_register_hook(dev, inode, sembol, kanca, yedek)` | Cihaz/inode çiftiyle tanımlanan kütüphanede PLT kancası kaydeder. |
| `redirector_register_got_hook(dev, inode, sembol, kanca, yedek)` | Aynı parametrelerle GOT tablosuna doğrudan yazar. |
| `redirector_register_hook_by_prefix(dev, inode, önek, kanca, yedek)` | Belirtilen ön ek ile başlayan tüm sembolleri tek çağrıyla kancalar. |
| `redirector_register_hook_with_offset(dev, inode, offset, size, sembol, kanca, yedek)` | Bellek ofset aralığı filtresiyle yalnızca belirtilen aralıktaki sembolleri kancalar. |
| `redirector_commit_hook_manual(maps)` | Kayıtlı tüm kancaları verilen bellek haritasına uygular. |
| `redirector_commit_hook()` | Mevcut sürecin haritasını otomatik tarayarak kayıtlı kancaları uygular. |
| `redirector_unhook(dev, inode, sembol)` | Belirtilen sembolün kancasını kaldırır; orijinal değeri yedekten geri yükler. |
| `invalidate_backups()` | Tüm sembol yedeklerini geçersiz kılar. Fork sonrası çağrılması gerekebilir. |
| `redirector_free_resources()` | Tüm dahili kaynakları, önbellekleri ve kayıtlı kancaları serbest bırakır. |
| `er_set_stealth_level(seviye)` | Gizlilik seviyesini değiştirir. `DIRECT_PATCH` genel kullanım için tavsiye edilir. |
| `er_set_cleanup_callback(cb)` | Kaynaklar serbest bırakılmadan önce çağrılacak callback fonksiyonunu kaydeder. |
| `er_init_for_zygisk()` | ReZygisk ve CSOLoader ortamları için optimal gizlilik ve önbellek parametrelerini otomatik yapılandırır. |

---

## Stealth Modları

Eaquel_Redirector, farklı tehdit modellerine karşı üç ayrı gizlilik modu sunar:

### Mod Karşılaştırması

| Mod | Kullanım Amacı | İz Düzeyi | Performans |
|---|---|---|---|
| `DIRECT_PATCH` | Genel kullanım — Tavsiye edilen | Çok Düşük | Maksimum |
| `TRAMPOLINE` | Derin analiz ortamları | Düşük | Yüksek |
| `BACKUP_FULL` | Hassas ortamlar, maksimum gizlilik | Sıfır | Orta |

### Mod Açıklamaları

**DIRECT\_PATCH** — GOT girişini doğrudan yeni adresle yazar. `mremap` veya anonim bellek izleri bırakmaz. Çoğu Anti-Cheat sistemi için görünmezdir.

**TRAMPOLINE** — Orijinal PLT/GOT girişini koruyup araya küçük bir sıçrama köprüsü kodu yerleştirir. Orijinal değer korunduğundan bütünlük denetçileri değişikliği fark edemez.

**BACKUP\_FULL** — Kancalanan bellek bölgesinin tam yedeğini alır ve her denetim çağrısında orijinal değerleri gösterir. En kapsamlı koruma sağlar; hafif performans maliyeti vardır.

### Mod Seçimi

```cpp
er_set_stealth_level(ErStealthLevel::DIRECT_PATCH);   // Varsayılan, tavsiye edilen
er_set_stealth_level(ErStealthLevel::TRAMPOLINE);     // Gelişmiş gizlilik
er_set_stealth_level(ErStealthLevel::BACKUP_FULL);    // Maksimum gizlilik
```

---

## Rust Submodule Entegrasyonu

`Redirector.hpp` dosyasının sonuna eklenen `extern "C"` bloğu sayesinde Rust tarafı, `eaquel_redirector_static` STATIC kütüphanesiyle doğrudan bağlanabilir. CMakeLists.txt değiştirilmemiştir; mevcut `eaquel_redirector_static` hedefi Rust tarafından doğrudan kullanılır.

### Neden `extern "C"` Gerekli?

C++ derleyicisi sembol isimlerini name mangling ile dönüştürür. Örneğin `redirector_scan_maps` fonksiyonu C++ tarafında `_Z20redirector_scan_mapsPKc` gibi bir isim alır. Rust'ın `bindgen` veya `cxx` crate'i bu dönüştürülmüş isimleri çözemez. `extern "C"` bloğu name mangling'i devre dışı bırakarak sembollerin sade C isimleriyle dışa aktarılmasını sağlar.

### CMake ve Cargo Entegrasyonu

```
Rust Projesi (Rezygiske)
        |
        |  cargo build
        v
  build.rs
  cargo:rustc-link-lib=static=eaquel_redirector_static
        |
        v
  eaquel_redirector_static  (CMakeLists.txt tarafından uretilir)
        |
        v
  Redirector.hpp  extern "C"  blogu  -->  Rust bindgen bağlamaları
```

`build.rs` örneği:

```rust
fn main() {
    println!("cargo:rustc-link-lib=static=eaquel_redirector_static");
    println!("cargo:rustc-link-search=native=<path_to_static_lib>");
    println!("cargo:rerun-if-changed=Redirector.hpp");
}
```

### bindgen ile Rust Bağlamaları Üretme

```bash
bindgen Redirector.hpp \
    --allowlist-function "redirector_.*" \
    --allowlist-function "er_.*" \
    --allowlist-function "invalidate_backups" \
    --allowlist-type "MapInfo" \
    --allowlist-type "MapEntry" \
    --allowlist-type "ErStealthLevel" \
    -o src/bindings.rs \
    -- -x c++ -std=c++23
```

### Rust'tan Temel Kullanım

```rust
use std::ffi::CString;

unsafe {
    let pid = CString::new("self").unwrap();
    let maps = redirector_scan_maps(pid.as_ptr());

    // ... hook kaydı ve commit ...

    redirector_free_maps(maps);
}
```

---

## Lisans ve Yasal Uyarı

**Eaquel_Redirector**, aşağıdaki amaçlar için geliştirilmiştir:

- Tersine mühendislik araştırması
- Güvenlik açığı analizi ve siber güvenlik araştırmaları
- Android sistem davranışının incelenmesi
- Eğitim amaçlı kancalama mimarisi öğrenimi

Bu projenin kötüye kullanımı, yetkisiz yazılım değişikliği, ticari hile araçları veya yasadışı amaçlarla kullanımından doğacak tüm yasal ve hukuki sorumluluklar tamamen son kullanıcıya aittir. Geliştirici ve katkıda bulunanlar herhangi bir sorumluluğu kabul etmez.

---

<div align="center">

Developed by **Eaquel** — 2026

[![Android](https://img.shields.io/badge/Android-11--16-1a73e8?style=flat-square&logo=android)](https://developer.android.com)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-9c27b0?style=flat-square&logo=cplusplus)](https://en.cppreference.com)
[![NDK](https://img.shields.io/badge/NDK-r29-00897b?style=flat-square)](https://developer.android.com/ndk)
[![Version](https://img.shields.io/badge/1.0.0-Release-b71c1c?style=flat-square)](https://github.com/Eaquel/Eaquel_Redirector/releases)

</div>
