# 4. namespace

C'de bir projede iki modül aynı isimli bir fonksiyon tanımlayamıyor. Bu yüzden isimlerin başına modülün adını koyuyordum: `gps_guncelle`, `baro_guncelle`. C++'ta bu öneklerin yerini **namespace** alıyor. Bu derste ikisini karşılaştırdım ve namespace'in bellekte ve işlemcide neye mal olduğuna baktım. Kodların tamamı bu notun içinde.

## Örnek program

İki sensör modülü var: GPS ve barometre. İkisinde de aynı işi yapan şeyler var:

- Bir güncelleme hızı sabiti (GPS 8 Hz, barometre 20 Hz).
- Kaç okuma yapıldığını sayan bir sayaç.
- Okuma yapan bir `guncelle` fonksiyonu.

İki modülde aynı isimleri kullanmak istiyorum, çünkü yaptıkları iş aynı.

## C ile

Dosya: `c_versiyon.c`

```c
// 4. ders, C hali: iki modülde aynı isimli fonksiyon olamıyor, isimlere önek koymak gerekiyor.
#include <stdio.h>

// ---- GPS modülü ----
#define GPS_HZ 8
static int gps_okuma_sayisi = 0;

void gps_guncelle(void)
{
    gps_okuma_sayisi++;
    printf("gps_guncelle:  %d. okuma (%d Hz)\n", gps_okuma_sayisi, GPS_HZ);
}

// ---- Barometre modülü ----
#define BARO_HZ 20
static int baro_okuma_sayisi = 0;

void baro_guncelle(void)
{
    baro_okuma_sayisi++;
    printf("baro_guncelle: %d. okuma (%d Hz)\n", baro_okuma_sayisi, BARO_HZ);
}

int main(void)
{
    gps_guncelle();
    gps_guncelle();
    baro_guncelle();
    printf("gps_okuma_sayisi = %d, baro_okuma_sayisi = %d\n", gps_okuma_sayisi, baro_okuma_sayisi);
    return 0;
}
```

Ne yapıyor:

- Her isim modülün adıyla başlıyor: `GPS_HZ`, `gps_okuma_sayisi`, `gps_guncelle` ve `BARO_HZ`, `baro_okuma_sayisi`, `baro_guncelle`.
- `main` GPS'i iki kez, barometreyi bir kez günceller ve sayaçları yazdırır.

Çıktı (`./calistir.sh 04-namespace/c_versiyon.c`):

```
gps_guncelle:  1. okuma (8 Hz)
gps_guncelle:  2. okuma (8 Hz)
baro_guncelle: 1. okuma (20 Hz)
gps_okuma_sayisi = 2, baro_okuma_sayisi = 1
```

| Çıktı | Hangi koddan geliyor | Anlamı |
|---|---|---|
| `gps_guncelle: 1. okuma`, `2. okuma` | iki `gps_guncelle()` çağrısı | GPS sayacı arttı |
| `baro_guncelle: 1. okuma (20 Hz)` | `baro_guncelle()` | Barometrenin ayrı sayacı ve sabiti var |
| `gps_okuma_sayisi = 2, baro_okuma_sayisi = 1` | `main`'deki son `printf` | İki modülün değişkenleri birbirine karışmadı |

## C'de iki modül aynı ismi kullanırsa

Öneki bırakıp iki fonksiyona da `guncelle` dedim: `hata_ornegi.c`

```c
// C'de iki modül aynı ismi kullanırsa ne oluyor?
#include <stdio.h>

void guncelle(void) { printf("GPS guncellendi\n"); }    // GPS modülü
void guncelle(void) { printf("Baro guncellendi\n"); }   // Barometre modülü, aynı isim

int main(void)
{
    guncelle();
    return 0;
}
```

Derleyici çıktısı (program hiç oluşmuyor):

```
hata_ornegi.c:5:6: error: redefinition of 'guncelle'
hata_ornegi.c:4:6: note: previous definition of 'guncelle' with type 'void(void)'
```

"`guncelle` ikinci kez tanımlanmış; ilki 4. satırda." C'de bir programdaki her fonksiyonun adı tek olmak zorunda. İki modül ayrı dosyalarda olsaydı, hata bu sefer derlemede değil, dosyalar birleştirilirken (linker) çıkardı. Önek koymak bu yüzden zorunlu.

## C++ ile

Dosya: `cpp_versiyon.cpp`

```cpp
// 4. ders, C++ hali: her modül kendi namespace'inde, isimler çakışmıyor.
#include <cstdio>

namespace gps
{
const int HZ = 8;
int okuma_sayisi = 0;

void guncelle()
{
    okuma_sayisi++;      // aynı namespace'in içinden önek yazmaya gerek yok
    printf("gps::guncelle:  %d. okuma (%d Hz)\n", okuma_sayisi, HZ);
}
} // namespace gps

namespace baro
{
const int HZ = 20;
int okuma_sayisi = 0;

void guncelle()
{
    okuma_sayisi++;
    printf("baro::guncelle: %d. okuma (%d Hz)\n", okuma_sayisi, HZ);
}
} // namespace baro

// İç içe namespace
namespace sensorler
{
namespace imu
{
const int HZ = 250;

void guncelle()
{
    printf("sensorler::imu::guncelle (%d Hz)\n", HZ);
}
} // namespace imu
} // namespace sensorler

int main()
{
    gps::guncelle();
    gps::guncelle();
    baro::guncelle();
    sensorler::imu::guncelle();
    printf("gps::okuma_sayisi = %d, baro::okuma_sayisi = %d\n", gps::okuma_sayisi, baro::okuma_sayisi);

    {
        using namespace baro;   // bu blok boyunca baro:: yazmadan kullanabilirim
        guncelle();
        printf("bu blokta HZ = %d\n", HZ);
    }

    return 0;
}
```

C sürümünden farkları:

- Her modülün isimleri bir `namespace` bloğunun içinde: `namespace gps { ... }` ve `namespace baro { ... }`. İki modülde de isimler aynı: `HZ`, `okuma_sayisi`, `guncelle`.
- Namespace'in dışından çağırırken önüne namespace'in adı ve `::` yazılıyor: `gps::guncelle()`, `baro::okuma_sayisi`.
- Namespace'in içinden önek yazmaya gerek yok: `gps::guncelle` içinde `okuma_sayisi` ve `HZ` doğrudan `gps`'in kendi isimlerini gösteriyor.
- `#define GPS_HZ 8` yerine `const int HZ = 8;`. `#define` derleyiciden önce çalışan önişlemcinin işi; namespace'i tanımıyor. `const` ise namespace'in içinde kalıyor. (8. derste, küçük yardımcılarda `constexpr`'ı göreceğim.)
- `sensorler::imu` iç içe bir namespace.
- `main`'in sonundaki blokta `using namespace baro;` ile o blok boyunca `baro::` yazmadan `guncelle()` ve `HZ` kullanılıyor.

Çıktı (`./calistir.sh 04-namespace/cpp_versiyon.cpp`):

```
gps::guncelle:  1. okuma (8 Hz)
gps::guncelle:  2. okuma (8 Hz)
baro::guncelle: 1. okuma (20 Hz)
sensorler::imu::guncelle (250 Hz)
gps::okuma_sayisi = 2, baro::okuma_sayisi = 1
baro::guncelle: 2. okuma (20 Hz)
bu blokta HZ = 20
```

| Çıktı | Hangi koddan geliyor | Anlamı |
|---|---|---|
| ilk üç satır | `gps::guncelle()` iki kez, `baro::guncelle()` bir kez | Aynı isimli iki fonksiyon, her biri kendi sayacını ve `HZ`'sini kullanıyor |
| `sensorler::imu::guncelle (250 Hz)` | `sensorler::imu::guncelle()` | İç içe namespace |
| `gps::okuma_sayisi = 2, baro::okuma_sayisi = 1` | `main` | İki `okuma_sayisi` ayrı değişkenler |
| `baro::guncelle: 2. okuma`, `bu blokta HZ = 20` | `using namespace baro;` bloğundaki `guncelle()` ve `HZ` | Önek yazmadan çağrıldı ve barometreninkiler kullanıldı |

## Namespace nedir?

Namespace, isimlerin önüne derleyicinin taktığı bir **etiket**. `gps::guncelle` derleyici için "`gps` etiketli `guncelle`" demek. C'de `gps_guncelle` diye elle yaptığım şeyi derleyici benim yerime yapıyor.

- `::` "içindeki" anlamına geliyor: `sensorler::imu::guncelle` = "sensorler'in içindeki imu'nun içindeki guncelle".
- Aynı `::` class'larda da kullanılıyor. 2. dersin (constructor) PX4 kısmındaki `SensorGpsSim::SensorGpsSim()` yazımı "SensorGpsSim class'ının içindeki constructor" demekti.
- `using namespace X;` yazıldığı blok boyunca X'in isimlerini öneksiz kullanmayı sağlıyor. Kolaylık getiriyor ama aşağıdaki ikinci hatadaki gibi çakışma riski de getiriyor. Genelde sadece dar bir blokta ya da isimleri çakışmayacağı belli olan küçük namespace'ler için kullanılıyor.

## İki hata örneği

**Önek yazmamak:** `hata_onek_yok.cpp`

```cpp
// Hata 1: namespace önekini yazmadan çağırmak
#include <cstdio>

namespace gps  { void guncelle() { printf("GPS\n"); } }
namespace baro { void guncelle() { printf("Baro\n"); } }

int main()
{
    guncelle();             // hangi namespace'teki?
    return 0;
}
```

```
hata_onek_yok.cpp:9:5: error: 'guncelle' was not declared in this scope
hata_onek_yok.cpp:9:5: note: suggested alternatives:
hata_onek_yok.cpp:4:23: note:   'gps::guncelle'
hata_onek_yok.cpp:5:23: note:   'baro::guncelle'
```

"Böyle bir isim yok; şunlardan birini mi kastettin: `gps::guncelle`, `baro::guncelle`?" Namespace'in içindeki isim, önek olmadan dışarıdan görünmüyor.

**Aynı isimleri içeren iki namespace'i birden açmak:** `hata_iki_using.cpp`

```cpp
// Hata 2: aynı isimleri içeren iki namespace'i birden açmak
#include <cstdio>

namespace gps  { void guncelle() { printf("GPS\n"); } }
namespace baro { void guncelle() { printf("Baro\n"); } }

int main()
{
    using namespace gps;
    using namespace baro;
    guncelle();             // ikisi de açık, hangisi?
    return 0;
}
```

```
hata_iki_using.cpp:11:13: error: call of overloaded 'guncelle()' is ambiguous
hata_iki_using.cpp:5:23: note: candidate: 'void baro::guncelle()'
hata_iki_using.cpp:4:23: note: candidate: 'void gps::guncelle()'
```

"`guncelle()` çağrısı belirsiz; iki aday var." `using namespace` ile ikisini birden açınca C'deki isim çakışması geri geliyor. Derleyici yine de yanlış olanı sessizce seçmiyor, hata veriyor.

## Bellekte ve işlemcide ne oluyor?

**Hiçbir şey.** Namespace sadece derleme sırasında var olan bir isim etiketi:

- Hiç tick harcamıyor: `gps::guncelle()` çağrısı, C'deki `gps_guncelle()` çağrısıyla aynı işlemci işi.
- Hiç bellek kaplamıyor: `gps::okuma_sayisi` de `gps_okuma_sayisi` de RAM'de aynı 4 byte.

Etiketin nereye gittiğini derlenmiş dosyanın sembol tablosunda gördüm. Sembol tablosu, derlenmiş dosyadaki fonksiyon ve değişken isimlerinin listesi; linker parçaları birleştirirken bu isimleri kullanıyor. (`nm` komutuyla listeledim.)

```
C'de:                       C++'ta (ham hali):               C++'ta (okunur hali, nm -C):
gps_guncelle                _ZN3gps8guncelleEv               gps::guncelle()
baro_guncelle               _ZN4baro8guncelleEv              baro::guncelle()
                            _ZN9sensorler3imu8guncelleEv     sensorler::imu::guncelle()
```

C++ derleyicisi namespace'in adını fonksiyonun adına gömüyor: `_ZN3gps8guncelleEv` = "3 harflik `gps`, içinde 8 harflik `guncelle`". Yani sonuçta ortaya çıkan şey, C'de elle yaptığım önekleme ile aynı: iki ayrı isim. Linker için de iki ayrı fonksiyon.

`const int HZ = 8;` de genelde bellekte yer tutmuyor; derleyici kullanıldığı yere 8 değerini doğrudan koyuyor, tıpkı `#define` gibi. Farkı, namespace'e uyması ve bir tipinin (`int`) olması.

## PX4'te

PX4'te namespace her yerde. Daha önce gördüğüm kodlarda geçenler:

| Yazım | Nerede tanımlı | Anlamı |
|---|---|---|
| `uORB::Publication<...>`, `uORB::Subscription` | `platforms/common/uORB/Publication.hpp` (`namespace uORB`) | uORB'ye ait class'lar |
| `px4::ScheduledWorkItem` | PX4'ün work queue kodu (`namespace px4`) | PX4 altyapısına ait class |
| `px4::wq_configurations::hp_default` | `px4_work_queue/WorkQueueManager.hpp` | İç içe namespace: `px4`'ün içinde `wq_configurations`, onun içinde `hp_default` thread ayarı |
| `px4::params::SIM_GPS_USED` | parametre sistemi | Parametre isimleri de bir namespace'in içinde |
| `matrix::Vector3f` | matematik kütüphanesi (`namespace matrix`) | 3 boyutlu vektör tipi |

`SensorGpsSim`'de `using namespace` iki yerde geçiyor:

```cpp
// SensorGpsSim.hpp
using namespace time_literals;

// SensorGpsSim.cpp
using namespace matrix;
```

`time_literals`, `src/drivers/drv_hrt.h` içinde tanımlı küçük bir namespace. `125_ms` gibi süre yazımlarını sağlayan şey onun içinde. `using namespace time_literals;` yazıldığı için `SensorGpsSim` constructor'ında `ScheduleOnInterval(125_ms)` diye öneksiz yazılabiliyor. (`_ms` yazımının kendisi 8. dersin, küçük yardımcıların konusu.) Bu namespace'in içinde sadece birkaç süre birimi var, başka bir şeyle çakışma riski yok; `using namespace`'in güvenli kullanıldığı durum bu.
