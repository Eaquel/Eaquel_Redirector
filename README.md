<div align="center">

```
███████╗ █████╗  ██████╗ ██╗   ██╗███████╗██╗
██╔════╝██╔══██╗██╔═══██╗██║   ██║██╔════╝██║
█████╗  ███████║██║   ██║██║   ██║█████╗  ██║
██╔══╝  ██╔══██║██║▄▄ ██║██║   ██║██╔══╝  ██║
███████╗██║  ██║╚██████╔╝╚██████╔╝███████╗███████╗
╚══════╝╚═╝  ╚═╝ ╚══▀▀═╝  ╚═════╝ ╚══════╝╚══════╝

██████╗ ███████╗██████╗ ██╗██████╗ ███████╗ ██████╗████████╗ ██████╗ ██████╗
██╔══██╗██╔════╝██╔══██╗██║██╔══██╗██╔════╝██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗
██████╔╝█████╗  ██║  ██║██║██████╔╝█████╗  ██║        ██║   ██║   ██║██████╔╝
██╔══██╗██╔══╝  ██║  ██║██║██╔══██╗██╔══╝  ██║        ██║   ██║   ██║██╔══██╗
██║  ██║███████╗██████╔╝██║██║  ██║███████╗╚██████╗   ██║   ╚██████╔╝██║  ██║
╚═╝  ╚═╝╚══════╝╚═════╝ ╚═╝╚═╝  ╚═╝╚══════╝ ╚═════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝
```

<br>

[![Android Sürümü](https://img.shields.io/badge/Android-11%20(API%2030)%20--%2016%20(API%2036)-1a73e8?style=for-the-badge&logo=android&logoColor=white)](https://developer.android.com)
[![Mimari](https://img.shields.io/badge/Mimari-arm64--v8a%20%7C%20armeabi--v7a%20%7C%20x86__64-00c853?style=for-the-badge&logo=cpu&logoColor=white)](#mimari-desteği)
[![C++ Standardı](https://img.shields.io/badge/C++23-Saf%20Native-9c27b0?style=for-the-badge&logo=cplusplus&logoColor=white)](#derleme-gereksinimleri)
[![Derleme](https://img.shields.io/badge/Derleme-Başarılı-43a047?style=for-the-badge&logo=githubactions&logoColor=white)](#derleme)
[![Sürüm](https://img.shields.io/badge/Sürüm-1.0.0--2026-b71c1c?style=for-the-badge&logo=semver&logoColor=white)](#sürümler)
[![Lisans](https://img.shields.io/badge/Lisans-Araştırma%20Amaçlı-ff6f00?style=for-the-badge&logo=opensourceinitiative&logoColor=white)](#lisans-ve-yasal-uyarı)

<br>

> **Eaquel_Redirector** — Modern Android ekosistemi için tasarlanmış,  
> yüksek performanslı, Stealth (Gizlilik) odaklı ve saf C++23 tabanlı  
> Procedure Linkage Table ve Global Offset Table Kancalama Çerçevesidir.

<br>

```
╔══════════════════════════════════════════════════════════════════════════════╗
║  Hedef Süreç Belleği                                                        ║
║  ┌─────────────────┐    PLT Kancası    ┌──────────────────────────────┐    ║
║  │  libc.so::open  │ ─────────────────▶│  Eaquel_Redirector           │    ║
║  │  (Orijinal)     │                   │  ┌────────────────────────┐  │    ║
║  └─────────────────┘                   │  │  Stealth Katmanı       │  │    ║
║                                        │  │  • DIRECT_PATCH modu   │  │    ║
║  ┌─────────────────┐    GOT Yazımı     │  │  • TRAMPOLINE modu     │  │    ║
║  │  .got.plt       │ ◀─────────────────│  │  • BACKUP_FULL modu    │  │    ║
║  │  (Adres Tablosu)│                   │  └────────────────────────┘  │    ║
║  └─────────────────┘                   │  ┌────────────────────────┐  │    ║
║                                        │  │  Sembol Önbelleği      │  │    ║
║  ┌─────────────────┐    Yönlendirme    │  │  O(1) Karmaşıklık      │  │    ║
║  │  Kullanıcı Kodu │ ◀─────────────────│  │  std::unordered_map    │  │    ║
║  │  (Özel Fonksiyon│                   │  └────────────────────────┘  │    ║
║  └─────────────────┘                   └──────────────────────────────┘    ║
╚══════════════════════════════════════════════════════════════════════════════╝
```

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
- [Lisans ve Yasal Uyarı](#lisans-ve-yasal-uyarı)

---

## Eaquel\_Redirector Nedir?

**Eaquel_Redirector**, Android işletim sistemi üzerinde çalışan uygulamaların dinamik bağlayıcı mekanizmalarına müdahale etmeye yarayan, sıfırdan tasarlanmış bir C++23 kancalama çerçevesidir. Kancalama (hooking), bir programın çalışması sırasında belirli bir fonksiyonun orijinal adresi yerine sizin yazdığınız özel bir fonksiyona yönlendirilmesi işlemidir.

Bu çerçeve özellikle şu senaryolar için geliştirilmiştir:

- Güvenlik araştırmacılarının sistem kütüphanelerinin davranışını çalışma zamanında incelemesi
- ReZygisk ve CSOLoader gibi Zygote tabanlı modüllerin, sistem süreçlerine temiz ve iz bırakmayan biçimde entegre olması
- Android 14, 15 ve 16 sürümlerinde giderek sertleşen bellek güvenliği politikaları altında güvenilir çalışma

Adı geçen her teknik terimin ne anlama geldiği aşağıdaki sözlükte adım adım açıklanmaktadır.

---

## Neden Eaquel\_Redirector?

### Mevcut Çözümlerin Yetersizliği

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    MEVCUT ÇÖZÜMLER vs EAQUEL_REDIRECTOR                     │
├──────────────────────────┬────────────────┬────────────────────────────────┤
│ Özellik                  │ LSPlt / PLStl  │ Eaquel_Redirector               │
├──────────────────────────┼────────────────┼────────────────────────────────┤
│ Android 16 QPR2 Uyumu    │ ✗  Çöküyor     │ ✓  Tam Uyumlu                  │
│ 16KB Sayfa Hizalaması    │ ✗  Desteksiz   │ ✓  Doğrudan Destekleniyor       │
│ DT_RELR Paketli Yerleşim │ ✗  Çözülemiyor │ ✓  Yerleşik Çözümleyici         │
│ Sembol Önbellekleme      │ ✗  Her seferinde│ ✓  O(1) Karmaşıklık            │
│ Stealth (Gizlilik) Modu  │ ✗  İz Bırakıyor│ ✓  3 Kademe Stealth             │
│ GOT Doğrudan Yazımı      │ ✗  Yok         │ ✓  Destekleniyor                │
│ Thread Güvenliği         │ △  Kısmi       │ ✓  RAII Mutex Yönetimi          │
│ Derleme Boyutu           │ △  Büyük       │ ✓  LTO + Section GC ile Küçük   │
└──────────────────────────┴────────────────┴────────────────────────────────┘
```

### Eaquel\_Redirector'ın Öne Çıkan Avantajları

**Android 16 QPR2 ve 16 Kilobayt Sayfa Hizalaması Tam Uyumu**

Android 16 ile birlikte, özellikle yüksek performanslı ARM64 cihazlarda bellek sayfası boyutu 4 kilobayttan 16 kilobayta yükseltilmiştir. Bu değişiklik, belleğe yazma işlemi yapan eski kancalama çözümlerini çökertiyor veya güvenlik duvarlarına takılmalarına neden oluyordu. Eaquel_Redirector, bu yeni hizalama gereksinimlerini başından itibaren göz önünde bulundurarak tasarlanmıştır.

Aynı zamanda Android'in RELRO (Relocation Read-Only — Yerleşim Alanları Salt Okunur) zorunlu uygulama politikasıyla da tam uyumludur. RELRO, kütüphane yüklenip bağlandıktan sonra yerleşim tablolarının salt okunur hale getirilmesini zorunlu kılar; Eaquel_Redirector bu kısıtlamaları aşmak için özel bellek koruma kaldırma ve yeniden uygulama döngüleri kullanır.

**Modern ELF İkili Formatı Çözümlemesi: DT_RELR ve APS2**

Geleneksel kancalama araçları yalnızca klasik REL ve RELA (Relocation — Yerleşim) bölümlerini anlayabilir. Ancak modern Android sistem kütüphaneleri, ikili dosya boyutunu küçültmek için DT_RELR adı verilen sıkıştırılmış yerleşim formatını kullanmaya başlamıştır. Eaquel_Redirector, bu formatı yerleşik olarak çözümleyebilir; dolayısıyla Android 14 ve sonrasında gelen tüm sistem kütüphanelerine sorunsuz kanca atabilir.

**Üç Kademe Stealth (Gizlilik) Mimarisi**

Kancalama işlemleri normalde bellek haritasında açık izler bırakır; bu izler Anti-Cheat (Hile Önleme) sistemleri veya uygulama bütünlük denetçileri tarafından kolaylıkla tespit edilebilir. Eaquel_Redirector, üç farklı gizlilik modu sunarak bu izlerin yok edilmesini sağlar. Modların ayrıntıları ilerleyen bölümlerde açıklanmaktadır.

**O(1) Sabit Zamanlı Sembol Önbellekleme**

Zygote gibi binlerce sembol barındıran süreçlerde her kanca atma girişiminde sıfırdan sembol araması yapmak ciddi performans kaybına yol açar. Eaquel_Redirector, `std::unordered_map` veri yapısı ile desteklenen `g_sym_cache` adlı bir sembol önbelleği kullanır. Bu önbellek sayesinde bir sembolü ilk kez bulduktan sonra, aynı sembole yapılan tüm sonraki erişimler sabit sürede tamamlanır.

**Sıfır İz Derleme (Zero Footprint Build)**

LTO (Link Time Optimization — Bağlama Zamanı Optimizasyonu), `-fvisibility=hidden` derleyici bayrağı ve Section Garbage Collection (Bölüm Çöp Toplama) mekanizmaları birlikte kullanılarak derlenmiştir. Bu sayede RTTI (Runtime Type Information — Çalışma Zamanı Tür Bilgisi) tabloları veya C++ istisna yönetimi artıkları gibi gereksiz veriler hedef sürecin belleğine bırakılmaz.

---

## Temel Kavramlar Sözlüğü

Bu bölüm, Eaquel_Redirector'ı anlamak için bilmeniz gereken tüm teknik terimleri Türkçe olarak, sade bir dille açıklar.

---

### ELF (Executable and Linkable Format — Yürütülebilir ve Bağlanabilir Format)

Android ve Linux sistemlerinde kullanılan standart ikili dosya biçimidir. Her `.so` (shared object — paylaşılan nesne) uzantılı kütüphane ve her uygulama ikili dosyası ELF formatındadır. Bu format içinde kod bölümleri, veri bölümleri, sembol tabloları ve yerleşim tabloları bulunur.

---

### Shared Library (Paylaşılan Dinamik Kütüphane)

`.so` uzantılı dosyalar, birden fazla uygulama veya süreç tarafından aynı anda belleğe yüklenerek kullanılabilen kod kitaplıklarıdır. Android'de `libc.so`, `libandroid.so`, `libOpenSLES.so` bunlara örnektir. Bu kütüphaneler programa derleme sırasında değil, çalışma zamanında (runtime) bağlanır.

---

### Dinamik Bağlayıcı (Dynamic Linker)

Bir uygulama başlatıldığında, ihtiyaç duyduğu paylaşılan kütüphaneleri bulup belleğe yükleyen ve fonksiyon adreslerini doğru tablolara yazan Android sistem bileşenidir. Android'deki dinamik bağlayıcı `linker64` veya `linker` adıyla çalışır.

---

### Sembol (Symbol)

ELF dosyalarında her fonksiyon, değişken veya veri bloğu bir sembol olarak tanımlanır. Semboller, isimleri ve bellekteki adresleri ile birlikte sembol tablosunda saklanır. Örneğin `open`, `malloc`, `pthread_create` birer semboldür.

---

### Procedure Linkage Table — Yordam Bağlantı Tablosu (PLT)

Bir uygulama paylaşılan bir kütüphanedeki fonksiyonu çağırdığında, doğrudan o fonksiyonun adresine atlayamaz; çünkü bu adres derleme sırasında bilinmez. Bunun yerine PLT adı verilen küçük bir atlama tablosu kullanılır. Her PLT girişi küçük bir kod parçacığından oluşur ve ilgili Global Offset Table (Küresel Ofset Tablosu) girişine bakarak gerçek adresi bulur. Kancalama amacıyla PLT girişlerinin değiştirilmesi işlemine **PLT Kancalama (PLT Hooking)** denir.

---

### Global Offset Table — Küresel Ofset Tablosu (GOT)

Dinamik bağlayıcının, paylaşılan kütüphane fonksiyonlarının gerçek bellek adreslerini yazdığı tablodur. PLT girişleri bu tabloya bakarak fonksiyonun nerede olduğunu öğrenir. Eaquel_Redirector, bu tabloya doğrudan yazarak da kancalama gerçekleştirebilir; bu işleme **GOT Kancalama (GOT Hooking)** denir.

---

### Runtime (Çalışma Zamanı)

Programın derlenip diske yazıldığı değil, kullanıcı tarafından başlatıldığı ve belleğin üzerinde aktif olarak çalıştığı andır. Eaquel_Redirector, tüm işlemlerini çalışma zamanında gerçekleştirir; yani kaynak koduna veya ikili dosyaya dokunmaz.

---

### Kancalama (Hooking)

Bir programın çalışması sırasında, belirli bir fonksiyona yapılan çağrıyı orijinal hedef yerine sizin belirlediğiniz farklı bir fonksiyona yönlendirme işlemidir. Bu teknik güvenlik araştırmacıları tarafından sistem çağrılarını incelemek, uygulama davranışını analiz etmek ve modüler eklenti sistemleri geliştirmek amacıyla kullanılır.

---

### Yerleşim (Relocation)

Dinamik bağlayıcının, bir kütüphane bellekteki konumunu belirledikten sonra tüm sembol adreslerini güncellemesine denir. Kütüphaneler her seferinde farklı bir bellek adresine yüklenebildiğinden, içlerindeki adres referanslarının düzeltilmesi gerekir.

---

### DT_RELR — Paketlenmiş Yerleşim Formatı

Modern Android sistem kütüphanelerinde kullanılan sıkıştırılmış bir yerleşim tablosu formatıdır. Klasik RELA tablolarına kıyasla ikili dosya boyutunu önemli ölçüde küçültür. Eaquel_Redirector bu formatı yerleşik olarak çözümleyebildiği için Android 14 ve sonrasındaki kütüphanelerle sorunsuz çalışır.

---

### APS2 — Android Paketlenmiş Semboller Sürüm 2

Google'ın Android bağlantı araç zincirinin ürettiği, sembollerin sıkıştırılmış biçimde saklandığı bir başka modern format. Eaquel_Redirector bu formatı da anlayabilir ve çözümleyebilir.

---

### Zygote

Android'de tüm uygulama süreçlerinin doğduğu ana süreçtir. Her uygulama başlatıldığında, Android bu Zygote sürecini çatallayarak (fork ederek) yeni bir süreç oluşturur. ReZygisk gibi araçlar bu sürece erken müdahale ederek modülleri tüm uygulamalara enjekte edebilir.

---

### ReZygisk ve CSOLoader

**ReZygisk**, Magisk tabanlı bir Zygisk implementasyonudur; sistemin kök erişimi yöneticisine eklenti olarak çalışır ve Zygote sürecine modül yükleme altyapısı sağlar. **CSOLoader** ise bu altyapı üzerinde özel paylaşılan kütüphanelerin yüklenmesini kolaylaştıran bir yükleyici bileşenidir. Eaquel_Redirector her ikisiyle de doğrudan entegrasyon için özel bir başlatma fonksiyonu içerir.

---

### RELRO (Relocation Read-Only — Yerleşim Alanları Salt Okunur)

Dinamik bağlayıcı kütüphane yükleme ve adres bağlama işlemini tamamladıktan sonra, yerleşim tablolarını salt okunur olarak işaretleyen güvenlik politikasıdır. Android bu politikayı her sürümde biraz daha sertleştirmektedir. Eaquel_Redirector, kancalama için gereken anlık yazma iznini güvenli biçimde açıp kapayarak bu politika ile uyumlu çalışır.

---

### mremap — Bellek Yeniden Eşleme Sistem Çağrısı

Linux çekirdeğinin bir bellek bölgesini farklı bir adrese taşıyan sistem çağrısıdır. Bazı eski kancalama araçları bu çağrıyı kullanır ve bu durum izleme sistemleri tarafından tespit edilebilir. Eaquel_Redirector, Stealth modlarında `mremap` kullanmaktan kaçınarak tespit riskini azaltır.

---

### LTO (Link Time Optimization — Bağlama Zamanı Optimizasyonu)

Derleyicinin tüm derleme birimlerini ayrı ayrı değil, birlikte optimize etmesini sağlayan bir teknik. Kullanılmayan kod ve veri bölümlerini ortadan kaldırır, ikili dosya boyutunu küçültür ve çalışma zamanı performansını artırır.

---

### RAII (Resource Acquisition Is Initialization — Kaynak Edinimi Başlangıçtır)

C++ programlamada bir nesne oluşturulduğunda kaynağın alındığı, nesne yok edildiğinde kaynağın otomatik olarak serbest bırakıldığı bir tasarım felsefesidir. `std::lock_guard`, kilit nesnesinin ömrü boyunca mutex'i tutup, kapsam dışına çıkıldığında otomatik olarak serbest bırakır; bu sayede kilitler unutulmaz ve kilitlenme (deadlock) riski en aza indirilir.

---

### Cache Temizliği — i-cache ve d-cache

ARM64 mimarisinde talimat önbelleği (instruction cache) ve veri önbelleği (data cache) ayrı birimlerdir. Bir bellek bölgesindeki makine kodu değiştirildiğinde, işlemcinin eski kodu önbellekten sunmaması için hem veri hem talimat önbelleğinin temizlenmesi gerekir. Eaquel_Redirector, kancalama sonrasında satır içi (inline) ARM64 Assembly komutlarıyla bu temizliği gerçekleştirir.

---

## Mimari ve Çalışma Prensibi

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                    EAQUEL_REDIRECTOR ÇALIŞMA AKIŞI                           │
│                                                                              │
│  AŞAMA 1: Bellek Haritasını Tarama                                           │
│  ─────────────────────────────────                                           │
│  /proc/<pid>/maps                                                            │
│       │                                                                      │
│       ▼                                                                      │
│  ┌──────────────────────────────────┐                                        │
│  │  socketpair + clone izolasyonu   │  ← Güvenli, ayrıcalıksız okuma        │
│  │  /apex/ bölgeleri → atla         │  ← Gereksiz alanlar filtrelenir        │
│  │  linker_alloc → atla             │  ← Dinamik bağlayıcı alanı korunur    │
│  └──────────────────────────────────┘                                        │
│       │                                                                      │
│       ▼                                                                      │
│  AŞAMA 2: ELF Ayrıştırma                                                     │
│  ────────────────────────                                                    │
│  ┌──────────────────────────────────┐                                        │
│  │  ELF Başlık Doğrulama            │                                        │
│  │  Bölüm Tablosu Okuma             │                                        │
│  │  Sembol Tablosu: GNU Hash        │  ← Önce hızlı hash araması             │
│  │                  ELF Hash        │  ← Fallback (Geri dönüş) seçeneği      │
│  │                  Doğrusal Tarama │  ← Son çare arama                      │
│  │  Yerleşim Bölümü: REL/RELA       │                                        │
│  │                   DT_RELR        │  ← Paketlenmiş modern format           │
│  │                   APS2           │  ← Android paketlenmiş semboller       │
│  └──────────────────────────────────┘                                        │
│       │                                                                      │
│       ▼                                                                      │
│  AŞAMA 3: Önbellekleme                                                       │
│  ─────────────────────                                                       │
│  g_sym_cache: std::unordered_map<isim, adres>                                │
│  İlk erişim: O(log n)  →  Sonraki erişimler: O(1)                            │
│       │                                                                      │
│       ▼                                                                      │
│  AŞAMA 4: Kanca Yazma                                                        │
│  ────────────────────                                                        │
│  ┌──────────────────────────────────┐                                        │
│  │  mprotect ile yazma izni aç      │                                        │
│  │  Orijinal değeri yedekle         │                                        │
│  │  PLT/GOT girişine yeni adres yaz │                                        │
│  │  mprotect ile korumayı geri al   │                                        │
│  │  ARM64: dcache + icache temizle  │                                        │
│  └──────────────────────────────────┘                                        │
│       │                                                                      │
│       ▼                                                                      │
│  AŞAMA 5: Stealth İz Silme                                                   │
│  ─────────────────────────                                                   │
│  Seçilen gizlilik moduna göre bellek izleri temizlenir                       │
│  Anti-Cheat sistemleri ve bütünlük denetçileri için görünmez hale getirilir │
└──────────────────────────────────────────────────────────────────────────────┘
```

---

## Temel Özellikler

### Kancalama Yetenekleri

**Procedure Linkage Table Kancalama**  
Dinamik kütüphane çağrılarının geçtiği PLT atlama tablosuna müdahale ederek fonksiyon çağrısını yeniden yönlendirir. Her çağrı bu tabloya uğradığından, fonksiyonun orijinal implementasyonuna dokunmadan tüm çağrı trafiğini yakalamanın en temiz yoludur.

**Global Offset Table Kancalama**  
PLT'nin danıştığı adres tablosuna doğrudan yazmayı sağlar. Bu yaklaşım PLT kancalamasından daha düşük seviyede çalışır ve bazı durumlarda daha kapsamlı kapsama alanı sunar.

**Önek Tabanlı Toplu Kancalama**  
Belirtilen bir isme sahip tüm sembolleri tek seferde kancalamanızı sağlar. Örneğin `"ssl_"` ön ekiyle başlayan tüm OpenSSL sembollerini tek bir çağrıyla kancalayabilirsiniz.

**Ofset Aralığı ile Filtreli Kancalama**  
Belirli bir bellek ofset aralığında bulunan sembolleri seçici biçimde kancalar. Bu özellik, büyük kütüphanelerin yalnızca belirli bölümlerini hedef almanızı sağlar.

### Güvenlik ve Gizlilik

**Üç Kademe Stealth (Gizlilik) Mimarisi**  
Anti-Cheat sistemlerine ve bütünlük denetçilerine karşı farklı seviyelerde koruma sağlayan üç farklı mod içerir. Ayrıntılar için [Stealth Modları](#stealth-modları) bölümüne bakınız.

**Güvenli Bellek Haritası Okuma**  
`socketpair` ve `clone` sistem çağrılarıyla izole edilmiş bir alt süreçte `/proc/maps` okur; bu sayede ana sürecin yetkilerini riske atmaz.

**Thread Güvenli Mimari**  
Tüm kritik bölümler `std::mutex` ile korunmaktadır ve RAII prensibiyle yönetilen `std::lock_guard` kullanılarak kilitler hiçbir koşulda unutulmaz.

### Performans

**Sembol Önbelleği**  
`std::unordered_map` destekli önbellek sayesinde aynı sembol için tekrarlanan aramalar sabit sürede tamamlanır. Zygote gibi sembol yoğun ortamlarda bu optimizasyon kritik bir fark yaratır.

**Satır İçi ARM64 Assembly Önbellek Temizliği**  
Kanca yazma sonrasında işlemci talimat hattının (pipeline) tutarsız duruma düşmesini önlemek için ARM64 mimarisine özgü veri ve talimat önbelleği temizleme komutları çalıştırılır.

**Akıllı Bellek Haritası Filtreleme**  
`/proc/maps` okunurken `/apex/` sistem çerçevesi alanları ve `linker_alloc` dinamik bağlayıcı özel alanları atlanır; böylece gereksiz işlem yükü önlenir.

---

## Proje Yapısı

```
Eaquel_Redirector/
│
├── .github/
│   └── workflows/
│       └── build.yml                ← GitHub Actions otomatik derleme ve yayın hattı
│
├── Gradle/
│   ├── gradle-wrapper.jar           ← Gradle sarmalayıcı JAR dosyası
│   ├── gradle-wrapper.properties    ← Gradle sürüm ve dağıtım yapılandırması
│   └── libs.versions.toml           ← Merkezi sürüm kataloğu (Version Catalog)
│
├── Redirector/
│   ├── build.gradle.kts             ← Android Kütüphane modülü derleme betiği
│   └── Source/
│       └── Main/
│           ├── AndroidManifest.xml  ← Android kütüphane manifest dosyası
│           └── Bridge/
│               ├── CMakeLists.txt   ← C++ derleme yapılandırması (CMake)
│               ├── Redirector.hpp   ← Genel C++ başlık dosyası (API tanımları)
│               └── Redirector.cpp   ← Tüm kancalama mantığının implementasyonu
│
├── build.gradle.kts                 ← Kök proje derleme betiği
├── settings.gradle.kts              ← Proje modül ayarları
├── gradle.properties                ← JVM ve Gradle performans özellikleri
├── gradlew                          ← Unix/macOS Gradle sarmalayıcı betiği
└── gradlew.bat                      ← Windows Gradle sarmalayıcı betiği
```

---

## Sürümler ve Bağımlılıklar

### Android ve Derleme Araç Zinciri

| Bileşen | Sürüm | Açıklama |
|---|---|---|
| Gradle Derleme Aracı | 9.4.1 | Proje derleme otomasyon sistemi |
| Android Gradle Eklentisi | 9.1.1 | Android projelerini Gradle ile derleyen eklenti |
| Android Native Development Kit | r29.0.14206865 | C ve C++ Android kütüphaneleri geliştirme kiti |
| CMake (C++ Derleme Sistemi) | 4.1.0+ | Platforma bağımsız derleme yapılandırması |
| Derleme Hedefi Android API Düzeyi | 36 (Android 16) | Derlenen en yüksek Android API sürümü |
| Minimum Android API Düzeyi | 30 (Android 11) | Desteklenen en düşük Android sürümü |
| Hedef Android API Düzeyi | 36 (Android 16) | Davranış uyumluluğu için hedef API |
| C++ Dil Standardı | C++23 | Kullanılan C++ sürümü |
| Java Sanal Makinesi Uyumluluğu | Java 21 | Kotlin ve Gradle için gerekli JVM sürümü |

### Desteklenen Mimariler

| Mimari Adı | Açıklama | Hedef Cihaz Sınıfı |
|---|---|---|
| arm64-v8a | 64-bit ARM mimarisi | 2016 sonrası modern akıllı telefonlar |
| armeabi-v7a | 32-bit ARM mimarisi | Eski ve düşük güçlü ARM cihazlar |
| x86_64 | 64-bit Intel/AMD mimarisi | Android emülatörleri, Chromebook x86 |

### GitHub Actions Sürümleri

| Eylem Adı | Sürüm | Tarih | Görev |
|---|---|---|---|
| actions/checkout | v6.0.2 | 9 Ocak 2026 | Depo kaynak kodunu çeker |
| actions/setup-node | v6.3.0 | 3 Mart 2026 | Node.js kurar ve önbellekler |
| actions/setup-python | v6.2.0 | Ocak 2026 | Python kurar ve pip önbellekler |
| actions/setup-java | v5.2.0 | 21 Ocak 2026 | Java Development Kit kurar |
| actions/setup-go | v6.4.0 | 30 Mart 2026 | Go derleyicisi kurar ve önbellekler |
| actions/cache | v5.0.5 | 13 Nisan 2026 | Bağımlılık önbellekleme |
| actions/upload-artifact | v7.0.1 | 10 Nisan 2026 | Derleme çıktılarını yapay eser olarak yükler |
| actions/download-artifact | v8.0.1 | Mart 2026 | Yüklenmiş yapay eserleri indirir |
| actions/upload-pages-artifact | v3 | Güncel | GitHub Pages için dosyaları hazırlar |
| actions/deploy-pages | v5.0.0 | 25 Mart 2026 | GitHub Pages sitesini yayına alır |
| actions/configure-pages | v5 | Güncel | GitHub Pages ayarlarını yapılandırır |
| actions/github-script | v9.0.0 | 9 Nisan 2026 | JavaScript ile GitHub API erişimi |
| docker/build-push-action | v7.1.0 | 10 Nisan 2026 | Docker imajı derler ve depoya yükler |
| docker/setup-buildx-action | v4 | Güncel | Gelişmiş Docker Buildx kurar |
| docker/login-action | v3 | Güncel | Docker kayıt defterine oturum açar |
| peter-evans/create-pull-request | v8.1.1 | 10 Nisan 2026 | Otomatik çekme isteği oluşturur |
| actions/stale | v9 | Güncel | Eski sorun ve çekme isteklerini işaretler |
| github/super-linter | v7 | Güncel | Çoklu dil kod kalite denetimi |
| peaceiris/actions-gh-pages | v4 | Güncel | Statik siteyi Pages'e gönderir |
| actions/upload-release-asset | v1.0.2 | Eski (2021+) | GitHub Sürümüne dosya yükler |

> **Önemli Not:** `actions/setup-node` v6.3.0 sürümü, `devEngines` alanı desteği ve Node.js 24 uyumluluğu getirir. GitHub Actions çalışıcısının en az **v2.327** sürümünde olması gerekir.

---

## Derleme Gereksinimleri

Eaquel_Redirector'ı yerel ortamınızda derleyebilmek için aşağıdaki araçların kurulu olması gerekir:

| Araç | Minimum Sürüm | Nereden İndirilir |
|---|---|---|
| Android Studio | Meerkat (2024.3+) | developer.android.com/studio |
| Java Development Kit | 21 (LTS) | adoptium.net |
| Android Native Development Kit | r29.0.14206865 | Android Studio SDK Manager |
| CMake | 4.1.0+ | Android Studio SDK Manager |

---

## Derleme

### Komut Satırından Derleme

```bash
# Proje kök dizininde terminali açın

# Hata ayıklama (Debug) modunda derleme — imzasız, hata bilgisi zengin
./gradlew :Redirector:assembleDebug

# Yayın (Release) modunda derleme — optimize edilmiş, küçük boyutlu
./gradlew :Redirector:assembleRelease

# Yerel Maven deposuna yayımlama — başka projelerde kullanmak için
./gradlew :Redirector:publishToMavenLocal

# Tüm mimariler için tek seferde derleme
./gradlew :Redirector:assembleRelease \
    -PabiFilters="arm64-v8a,armeabi-v7a,x86_64"

# Önbellek temizleyerek temiz derleme
./gradlew clean :Redirector:assembleRelease
```

### GitHub Actions Otomatik Derleme Hattı

Deponun `.github/workflows/build.yml` dosyasında tanımlanan otomatik derleme hattı, `master` dalına her kod gönderiminde ve her çekme isteğinde otomatik olarak tetiklenir. Bu hat şunları yapar:

```
Tetikleyici: git push veya pull request
       │
       ▼
┌─────────────────────┐
│  Java 21 Kurulumu   │
└─────────────────────┘
       │
       ▼
┌─────────────────────────────────────┐
│  Gradle Bağımlılık Önbellekleme     │
│  ~/.gradle/caches — hızlı derleme  │
└─────────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────┐
│  CCache (C++ Derleyici Önbelleği)   │
│  — tekrarlanan derlemeler hızlanır  │
└─────────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────┐
│  ./gradlew :Redirector:assemble     │
│  arm64-v8a + armeabi-v7a + x86_64  │
└─────────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────┐
│  AAR dosyasını Yapay Eser olarak    │
│  GitHub Actions'a yükle            │
└─────────────────────────────────────┘
```

---

## Kurulum ve Entegrasyon

### Gradle Bağımlılığı (AAR ve Prefab)

Eaquel_Redirector, modern C++ projeleri için Prefab uyumlu olarak paketlenmiştir. Prefab, Android Gradle Eklentisinin C++ başlık dosyalarını ve paylaşılan kütüphaneleri AAR içinde dağıtma standardıdır.

Projenizin `build.gradle.kts` dosyasına şunu ekleyin:

```kotlin
android {
    buildFeatures {
        prefab = true   // Prefab desteğini etkinleştir
    }
}

dependencies {
    implementation("io.github.eaquel.redirector:eaquel_redirector:1.0.0-2026")
}
```

### CMakeLists.txt Bağlama

Projenizin `CMakeLists.txt` dosyasında bağlama işlemini gerçekleştirin:

```cmake
# Eaquel_Redirector paketini bul
find_package(eaquel_redirector REQUIRED CONFIG)

# Kendi hedefinize bağla
target_link_libraries(
    SizinKütüphaneniz
    PRIVATE
    eaquel_redirector::eaquel_redirector
)
```

---

## Hızlı Başlangıç ve API Kullanımı

### Temel Kanca Atma

Aşağıdaki örnek, `libc.so` kütüphanesinin `open` fonksiyonunu kancalayarak tüm dosya açma çağrılarını özel bir fonksiyona yönlendirir:

```cpp
#include "Redirector.hpp"
#include <sys/stat.h>
#include <fcntl.h>

// Orijinal fonksiyonun adresini saklayacak gösterici
static int (*orijinal_open)(const char* yol, int bayraklar, ...) = nullptr;

// Kanca fonksiyonumuz — open() çağrıldığında buraya gelir
int benim_open_kancam(const char* yol, int bayraklar, ...) {
    // İstediğimiz işlemi burada yapabiliriz
    // Örneğin: belirli bir dosyaya erişimi engelle
    if (strcmp(yol, "/data/blocked_file") == 0) {
        errno = EACCES;
        return -1;
    }

    // Diğer durumlarda orijinal fonksiyonu çağır
    return orijinal_open(yol, bayraklar);
}

void kankalari_kur() {
    // 1. Adım: Hedef sürecin bellek haritasını tara
    //          "self" anahtar kelimesi mevcut süreci ifade eder
    MapInfo* harita = redirector_scan_maps("self");

    // 2. Adım: Hedef kütüphanenin cihaz ve inode numarasını al
    //          Bu bilgiler kütüphanenin hangi dosyadan yüklendiğini tanımlar
    struct stat dosya_bilgisi;
    stat("/system/lib64/libc.so", &dosya_bilgisi);

    // 3. Adım: Kancayı kaydet
    //          Parametreler sırasıyla:
    //          - Cihaz numarası (st_dev)
    //          - İnode numarası (st_ino)
    //          - Kancalanacak sembol adı
    //          - Kanca fonksiyonunun adresi
    //          - Orijinal fonksiyon adresinin yazılacağı yer
    redirector_register_hook(
        dosya_bilgisi.st_dev,
        dosya_bilgisi.st_ino,
        "open",
        reinterpret_cast<void*>(benim_open_kancam),
        reinterpret_cast<void**>(&orijinal_open)
    );

    // 4. Adım: Kaydedilen tüm kancaları belleğe tek seferde uygula
    redirector_commit_hook_manual(harita);

    // 5. Adım: Bellek haritası nesnesini serbest bırak
    redirector_free_maps(harita);
}
```

### Global Offset Table Kancalama

```cpp
// GOT tablosuna doğrudan kanca — daha düşük seviyeli müdahale
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
// "ssl_" ön ekiyle başlayan tüm OpenSSL sembollerini kancala
struct stat ssl_bilgisi;
stat("/system/lib64/libssl.so", &ssl_bilgisi);

redirector_register_hook_by_prefix(
    ssl_bilgisi.st_dev,
    ssl_bilgisi.st_ino,
    "ssl_",                                          // Ön ek filtresi
    reinterpret_cast<void*>(genel_ssl_kancasi),
    reinterpret_cast<void**>(&orijinal_ssl_fonk)
);
```

### Kancayı Geri Alma

```cpp
// Kancayı kaldır ve orijinal fonksiyonu geri yükle
redirector_unhook(
    dosya_bilgisi.st_dev,
    dosya_bilgisi.st_ino,
    "open"
);
```

### ReZygisk ve CSOLoader için Optimum Kurulum

```cpp
// Zygisk modül girişinde bu fonksiyonu çağırın
// Otomatik olarak en iyi gizlilik seviyesi ve önbellek ayarlarını uygular
er_init_for_zygisk();
```

---

## API Referans Tablosu

| Fonksiyon İmzası | Açıklama |
|---|---|
| `redirector_scan_maps(pid)` | Belirtilen sürecin bellek haritasını güvenli `socketpair + clone` izolasyonu ile okur. `"self"` mevcut süreci ifade eder. |
| `redirector_free_maps(maps)` | `redirector_scan_maps` ile elde edilen bellek haritası nesnesini serbest bırakır ve bellek sızıntısını önler. |
| `redirector_register_hook(dev, inode, sembol, kanca, yedek)` | Belirtilen cihaz ve inode çiftiyle tanımlanan kütüphanede, verilen sembol adına Procedure Linkage Table kancası kaydeder. |
| `redirector_register_got_hook(dev, inode, sembol, kanca, yedek)` | Yukarıdakiyle aynı parametreler; ancak Procedure Linkage Table yerine Global Offset Table tablosuna doğrudan yazar. |
| `redirector_register_hook_by_prefix(dev, inode, önek, kanca, yedek)` | Belirtilen ön ek ile başlayan tüm sembolleri tek çağrıyla kancalar. |
| `redirector_register_hook_with_offset(dev, inode, sembol, başlangıç, bitiş, kanca, yedek)` | Bellek ofset aralığı filtresiyle yalnızca belirtilen aralıktaki sembolleri kancalar. |
| `redirector_commit_hook_manual(maps)` | Önceden `redirector_register_hook` ile kaydedilen tüm kancaları verilen bellek haritasına uygular. |
| `redirector_commit_hook()` | Kısa yol: mevcut sürecin haritasını otomatik tarayarak kaydedilen kancaları uygular. |
| `redirector_unhook(dev, inode, sembol)` | Belirtilen semboldeki kancayı kaldırır ve orijinal değeri yedekten geri yükler. |
| `redirector_invalidate_backups()` | Tüm sembol yedeklerini geçersiz kılar. Süreç çatallamadan (fork) sonra çağrılması gerekebilir. |
| `redirector_free_resources()` | Tüm dahili kaynakları, önbellekleri ve kayıtlı kancaları serbest bırakır. |
| `er_set_stealth_level(seviye)` | Gizlilik seviyesini değiştirir. `DIRECT_PATCH` modu genel kullanım için tavsiye edilir. |
| `er_init_for_zygisk()` | ReZygisk ve CSOLoader ortamları için en optimal gizlilik seviyesini ve önbellek parametrelerini otomatik olarak yapılandırır. |

---

## Stealth Modları

Eaquel_Redirector, farklı tehdit modellerine karşı üç ayrı gizlilik modu sunar:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          STEALTH MOD KARŞILAŞTIRMASI                        │
├───────────────────┬────────────────────────────────────────────────────────┤
│  DIRECT_PATCH     │ Tavsiye Edilen — Genel Kullanım                        │
│  ─────────────    │                                                         │
│  Açıklama:        │ Global Offset Table girişini doğrudan yeni adresle       │
│                   │ yazar. mremap veya anonim bellek izleri bırakmaz.        │
│                   │ Çoğu Anti-Cheat sistemi için görünmezdir.               │
│  İz Düzeyi:       │ ██░░░░░░░░ Çok Düşük                                   │
│  Performans:      │ ██████████ Maksimum                                     │
├───────────────────┼────────────────────────────────────────────────────────┤
│  TRAMPOLINE       │ Gelişmiş — Derin Analiz Ortamları                       │
│  ────────────     │                                                         │
│  Açıklama:        │ Orijinal PLT/GOT girişini koruyup, araya küçük bir       │
│                   │ sıçrama köprüsü (trampoline) kodu yerleştirir.          │
│                   │ Orijinal değer korunduğundan bütünlük denetçileri       │
│                   │ değişikliği fark edemez.                                │
│  İz Düzeyi:       │ ████░░░░░░ Düşük                                       │
│  Performans:      │ ████████░░ Yüksek                                      │
├───────────────────┼────────────────────────────────────────────────────────┤
│  BACKUP_FULL      │ Maksimum Gizlilik — Hassas Ortamlar                     │
│  ───────────      │                                                         │
│  Açıklama:        │ Kancalanan bellek bölgesinin tam yedeğini alır ve       │
│                   │ her denetim çağrısında orijinal değerleri gösterir.     │
│                   │ En kapsamlı koruma sağlar; hafif performans maliyeti    │
│                   │ vardır.                                                 │
│  İz Düzeyi:       │ ██████████ Sıfır (Denetçilere Görünmez)                │
│  Performans:      │ ██████░░░░ Orta                                        │
└───────────────────┴────────────────────────────────────────────────────────┘
```

Gizlilik modunu değiştirmek için:

```cpp
// Doğrudan yama modu (varsayılan, tavsiye edilen)
er_set_stealth_level(StealthLevel::DIRECT_PATCH);

// Sıçrama köprüsü modu
er_set_stealth_level(StealthLevel::TRAMPOLINE);

// Tam yedek modu (maksimum gizlilik)
er_set_stealth_level(StealthLevel::BACKUP_FULL);
```

---

## Lisans ve Yasal Uyarı

```
╔══════════════════════════════════════════════════════════════════════════════╗
║                          YASAL UYARI VE SORUMLULUK REDDİ                   ║
╠══════════════════════════════════════════════════════════════════════════════╣
║                                                                              ║
║  Eaquel_Redirector, aşağıdaki amaçlar için geliştirilmiştir:                ║
║                                                                              ║
║  • Tersine mühendislik araştırması                                           ║
║  • Güvenlik açığı analizi ve siber güvenlik araştırmaları                   ║
║  • Android sistem davranışının incelenmesi                                  ║
║  • Eğitim amaçlı kancalama mimarisi öğrenimi                                ║
║                                                                              ║
║  Bu projenin kötüye kullanımı, yetkisiz yazılım değişikliği,                ║
║  ticari hile araçları veya yasadışı amaçlarla kullanımından                 ║
║  doğacak tüm yasal ve hukuki sorumluluklar tamamen son kullanıcıya          ║
║  aittir. Geliştirici ve katkıda bulunanlar herhangi bir sorumluluğu         ║
║  kabul etmez.                                                               ║
║                                                                              ║
╚══════════════════════════════════════════════════════════════════════════════╝
```

---

<div align="center">

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

        Developed by  Eaquel [-] 2026 

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

[![Android](https://img.shields.io/badge/Android-11--16-1a73e8?style=flat-square&logo=android)](https://developer.android.com)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-9c27b0?style=flat-square&logo=cplusplus)](https://en.cppreference.com)
[![NDK](https://img.shields.io/badge/NDK-r29-00897b?style=flat-square)](https://developer.android.com/ndk)
[![Version](https://img.shields.io/badge/1.0.0--2026-Release-b71c1c?style=flat-square)](https://github.com/Eaquel/Eaquel_Redirector/releases)

</div>
