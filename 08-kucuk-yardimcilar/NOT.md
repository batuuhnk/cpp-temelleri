# 8. küçük yardımcılar

PX4 kodunda sık geçen beş küçük şeyi bu derste topladım: `enum class`, `constexpr`, `125_ms` gibi süre yazımları, `static_cast` ve `nullptr`. Her biri C'de bildiğim bir şeyin (`enum`, `#define`, C tarzı cast, `NULL`) daha güvenli hali. Hepsinin ortak noktası: işlerini derleme sırasında yapıyorlar, program çalışırken ek tick harcamıyorlar. Kodların tamamı bu notun içinde.

## Örnek program

GPS ile ilgili küçük parçalar:

- GPS'in fix tipi (yok, 2D, 3D) ve aracın uçuş modu (hold, land) için iki enum.
- GPS'in güncelleme hızından (8 Hz) periyodunu (125 ms) hesaplamak ve 125 ms'yi mikrosaniyeye çevirmek.
- Dereceyi `int32_t`'ye çevirmek (enlem × 10⁷) ve bir barometre struct'ını GPS struct'ı gibi okumaya çalışmak.
- Aynı isimli iki fonksiyondan birine `NULL` / `nullptr` vermek.

## C ile

Dosya: `c_versiyon.c`

```c
// 8. ders, C hali: enum, #define, C tarzı cast, NULL ve süre makroları.
#include <stdio.h>
#include <stdint.h>

// ---- enum ----
enum FixTipi   { FIX_YOK, FIX_2D, FIX_3D };
enum UcusModu  { MOD_HOLD, MOD_LAND };

// ---- sabitler ve süreler ----
#define GPS_HZ            8
#define GPS_PERIYOT_US    (1000000 / GPS_HZ)
#define MS(x)             ((uint64_t)(x) * 1000)    // milisaniyeyi mikrosaniyeye çevir

// ---- NULL ile iki "farklı" fonksiyon (C'de aynı isimle olamıyor, ayrı isim gerekiyor) ----
void kaydet_sayi(int x)           { printf("  kaydet_sayi(%d)\n", x); }
void kaydet_metin(const char *s)  { printf("  kaydet_metin(%s)\n", s ? s : "NULL"); }

typedef struct { int32_t lat; int32_t lon; } GpsVeri;
typedef struct { float basinc; float sicaklik; } BaroVeri;

int main(void)
{
    enum FixTipi fix = FIX_2D;

    // enum aslında int: farklı bir enum'la ve sayıyla karşılaştırılabiliyor
    if (fix == MOD_LAND) {
        printf("enum: fix == MOD_LAND dogru cikti (fix=%d, MOD_LAND=%d)\n", fix, MOD_LAND);
    }
    printf("enum: sizeof(enum FixTipi) = %zu byte\n", sizeof(enum FixTipi));

    // #define: önişlemci metin olarak yerleştiriyor
    printf("define: GPS_PERIYOT_US = %d, MS(125) = %llu\n", GPS_PERIYOT_US, (unsigned long long)MS(125));

    // C tarzı cast: sayıyı çevirmek için de, pointer'ı başka tipe çevirmek için de aynı yazım
    double lat_derece = 47.3977420;
    int32_t lat = (int32_t)(lat_derece * 1e7);
    printf("cast: lat = %d\n", (int)lat);

    BaroVeri baro = { 1013.25f, 21.5f };
    GpsVeri *yanlis = (GpsVeri *)&baro;     // barometreyi GPS gibi okumak: C bunu sorunsuz kabul ediyor
    printf("cast: yanlis->lat = %d\n", (int)yanlis->lat);

    // NULL
    kaydet_sayi(0);
    kaydet_metin(NULL);
    return 0;
}
```

Derleyici bir uyarı verdi, program yine de derlendi:

```
c_versiyon.c:26:13: warning: comparison between 'enum FixTipi' and 'enum UcusModu' [-Wenum-compare]
```

Çıktı (`./calistir.sh 08-kucuk-yardimcilar/c_versiyon.c`):

```
enum: fix == MOD_LAND dogru cikti (fix=1, MOD_LAND=1)
enum: sizeof(enum FixTipi) = 4 byte
define: GPS_PERIYOT_US = 125000, MS(125) = 125000
cast: lat = 473977420
cast: yanlis->lat = 1149063168
  kaydet_sayi(0)
  kaydet_metin(NULL)
```

| Çıktı | Hangi koddan geliyor | Anlamı |
|---|---|---|
| `fix == MOD_LAND dogru cikti (fix=1, MOD_LAND=1)` | `if (fix == MOD_LAND)` | GPS fix tipiyle uçuş modu "eşit" çıktı; ikisi de arka planda 1 sayısı. Anlamsız bir karşılaştırma, ama sadece uyarı aldı |
| `sizeof(enum FixTipi) = 4 byte` | `sizeof` | C enum'u bir `int`; 3 değer için 4 byte |
| `GPS_PERIYOT_US = 125000, MS(125) = 125000` | `#define`'lar | Doğru, ama bunlar tipi olmayan metin parçaları |
| `cast: lat = 473977420` | `(int32_t)(lat_derece * 1e7)` | Sayı çevirme, doğru |
| `cast: yanlis->lat = 1149063168` | `(GpsVeri *)&baro` | Barometre struct'ı GPS gibi okundu; 1013.25 basıncının float byte'ları. C tarzı cast buna izin veriyor |
| `kaydet_sayi(0)`, `kaydet_metin(NULL)` | iki ayrı fonksiyon | C'de aynı isimli iki fonksiyon olamadığı için ayrı isimler gerekti |

## C++ ile

Dosya: `cpp_versiyon.cpp`

```cpp
// 8. ders, C++ hali: enum class, constexpr, static_cast, nullptr ve süre literalleri.
#include <cstdio>
#include <cstdint>

// ---- enum class ----
enum class FixTipi  : uint8_t { Yok, Fix2D, Fix3D };   // ": uint8_t" -> 1 byte
enum class UcusModu : uint8_t { Hold, Land };

// ---- constexpr ----
constexpr uint32_t GPS_HZ = 8;

constexpr uint32_t hz_to_us(uint32_t hz)
{
    return 1000000 / hz;
}

constexpr uint32_t GPS_PERIYOT_US = hz_to_us(GPS_HZ);
static_assert(GPS_PERIYOT_US == 125000, "8 Hz 125 ms etmeli");   // derleme sırasında kontrol

// ---- süre literali: PX4'teki 125_ms yazımı ----
constexpr uint64_t operator ""_ms(unsigned long long ms)
{
    return ms * 1000;
}

// ---- aynı isimli iki fonksiyon (C++'ta parametre tipi farklıysa olabiliyor) ----
void kaydet(int x)           { printf("  kaydet(int %d)\n", x); }
void kaydet(const char *s)   { printf("  kaydet(const char* %s)\n", s ? s : "nullptr"); }

struct GpsVeri  { int32_t lat; int32_t lon; };
struct BaroVeri { float basinc; float sicaklik; };

int main()
{
    FixTipi fix = FixTipi::Fix3D;
    if (fix == FixTipi::Fix3D) {
        printf("enum class: fix 3D (sayi degeri %d)\n", static_cast<int>(fix));
    }
    printf("enum class: sizeof(FixTipi) = %zu byte\n", sizeof(FixTipi));

    printf("constexpr: GPS_PERIYOT_US = %u, 125_ms = %llu\n",
           (unsigned)GPS_PERIYOT_US, (unsigned long long)125_ms);

    int dizi[GPS_HZ];      // constexpr bir sabit dizi boyutu olarak kullanılabiliyor
    printf("constexpr: dizi %zu elemanli\n", sizeof(dizi) / sizeof(dizi[0]));

    double lat_derece = 47.3977420;
    int32_t lat = static_cast<int32_t>(lat_derece * 1e7);
    printf("static_cast: lat = %d\n", (int)lat);

    float param1 = 3.9f;                                       // MAVLink komut parametresi gibi
    int birim = static_cast<int>(param1 + 0.5f);               // PX4'teki failure_unit hesabı
    printf("static_cast: param1=%.1f -> birim=%d\n", param1, birim);

    kaydet(0);
    kaydet(nullptr);
    return 0;
}
```

Çıktı (`./calistir.sh 08-kucuk-yardimcilar/cpp_versiyon.cpp`):

```
enum class: fix 3D (sayi degeri 2)
enum class: sizeof(FixTipi) = 1 byte
constexpr: GPS_PERIYOT_US = 125000, 125_ms = 125000
constexpr: dizi 8 elemanli
static_cast: lat = 473977420
static_cast: param1=3.9 -> birim=4
  kaydet(int 0)
  kaydet(const char* nullptr)
```

Her satırın açıklaması aşağıda, ilgili başlığın altında.

## enum class

```cpp
enum class FixTipi : uint8_t { Yok, Fix2D, Fix3D };
FixTipi fix = FixTipi::Fix3D;
```

C'deki `enum`'dan üç farkı var:

- **Değerlere tipin adıyla erişiliyor:** `FIX_3D` yerine `FixTipi::Fix3D`. C'de değer isimleri global olduğu için başlarına `FIX_`, `MOD_` önekleri koyuyordum (4. dersteki, namespace'teki sorunun aynısı).
- **Başka bir şeyle karıştırılamıyor:** C'deki "fix tipi == uçuş modu" karşılaştırması C++'ta derlenmiyor (aşağıda *Hata örnekleri*). Sayıya çevirmek gerekiyorsa açıkça yazılıyor: `static_cast<int>(fix)` → çıktıdaki `sayi degeri 2`.
- **Boyutu seçilebiliyor:** `: uint8_t` yazınca 1 byte (çıktıda `sizeof(FixTipi) = 1 byte`). C enum'u 4 byte'tı. uORB mesajları gibi saniyede yüzlerce kez kopyalanan struct'larda bu 3 byte önemli.

İşlemci için `FixTipi::Fix3D` sadece 2 sayısı; karşılaştırması C'deki gibi tek bir işlem. Ek tick yok.

## constexpr

```cpp
constexpr uint32_t GPS_HZ = 8;
constexpr uint32_t hz_to_us(uint32_t hz) { return 1000000 / hz; }
constexpr uint32_t GPS_PERIYOT_US = hz_to_us(GPS_HZ);
static_assert(GPS_PERIYOT_US == 125000, "8 Hz 125 ms etmeli");
```

`constexpr` "bu değer derleme sırasında hesaplanabilir" demek.

- `GPS_PERIYOT_US`'yi hesaplamak için `hz_to_us(8)` **derleyici tarafından** çağrılıyor. Programın içine 1000000 / 8 bölmesi değil, doğrudan 125000 sayısı yazılıyor. Bölme işlemi STM32'de 2–12 tick sürebiliyor; burada çalışma sırasında hiç yapılmıyor.
- `#define GPS_PERIYOT_US (1000000 / GPS_HZ)` de çoğu zaman aynı sonucu verir, ama `#define`'ın tipi yok, namespace'e uymuyor ve önişlemci onu sadece metin olarak yapıştırıyor. `constexpr`'ın tipi var (`uint32_t`) ve normal bir değişken gibi davranıyor.
- `static_assert` derleme sırasında bir koşulu kontrol ediyor. Koşul yanlışsa program derlenmiyor (aşağıda *Hata örnekleri*). Çalışma zamanında 0 tick, 0 byte.
- Constexpr bir sabit dizi boyutu olarak kullanılabiliyor: `int dizi[GPS_HZ];` → çıktıdaki `dizi 8 elemanli`.

## Süre yazımı: `125_ms`

```cpp
constexpr uint64_t operator ""_ms(unsigned long long ms) { return ms * 1000; }
```

Bu satır, sayıların sonuna `_ms` yazılabilmesini sağlıyor: `125_ms` yazınca derleyici bu fonksiyonu 125 ile çağırıyor ve sonuç olarak 125000 (mikrosaniye) koyuyor. Çıktıdaki `125_ms = 125000` bu. Fonksiyon `constexpr` olduğu için hesap derleme sırasında yapılıyor; programın içinde sadece 125000 sayısı var.

C'deki karşılığı `MS(125)` makrosu. İkisi de aynı sayıyı üretiyor; `_ms` sadece daha okunur ve tipi belli.

## static_cast

```cpp
int32_t lat = static_cast<int32_t>(lat_derece * 1e7);
int birim   = static_cast<int>(param1 + 0.5f);
```

`static_cast<HEDEF_TIP>(deger)`, C'deki `(HEDEF_TIP)deger` ile aynı işi yapıyor, ama sadece **mantıklı** çevirmelere izin veriyor: sayıdan sayıya, enum'dan sayıya, kalıtımda türeyen class'tan temel class'a (5. ders) gibi.

C tarzı cast her şeye izin veriyordu: C sürümünde `(GpsVeri *)&baro` ile barometre struct'ını GPS gibi okudum ve saçma bir enlem aldım. Aynısını `static_cast` ile denediğimde program derlenmedi (aşağıda *Hata örnekleri*). Gerçekten byte'ları başka bir tip gibi okumak gerekiyorsa C++'ta bunun ayrı ve göze batan bir yazımı var (`reinterpret_cast`); böylece tehlikeli çevirmeler kodda kolayca bulunabiliyor.

`static_cast<int>(param1 + 0.5f)`: 3.9 + 0.5 = 4.4, tam sayıya çevrilince 4 (çıktıdaki `birim=4`). Ondalıklı bir sayıyı en yakın tam sayıya yuvarlamanın kısa yolu.

İşlemci için: float'tan int'e çevirmek STM32'nin FPU'sunda tek bir komut (~1 tick); C tarzı cast'le aynı. Pointer çevirmek 0 tick, adres değişmiyor. Yani `static_cast` ek tick getirmiyor, sadece derleme sırasında kontrol ekliyor.

## nullptr

C++'ta aynı isimde, parametre tipleri farklı birden fazla fonksiyon olabiliyor (1. derste, isim listesinde parametre tiplerinin fonksiyon adına gömüldüğünü görmüştüm; bu yüzden mümkün):

```cpp
void kaydet(int x);
void kaydet(const char *s);
```

Hangisinin çalışacağını derleyici verilen değerin tipine bakarak seçiyor: `kaydet(0)` → `int` olan, `kaydet(nullptr)` → pointer olan. Çıktıdaki son iki satır bu.

C'deki `NULL` aslında 0 sayısı. C++'ta `kaydet(NULL)` yazınca derleyici "bu bir sayı mı, adres mi?" diye karar veremiyor (aşağıda *Hata örnekleri*). `nullptr` ise tipi "boş adres" olan bir değer; sayı ile karışmıyor. İşlemci için ikisi de 0 adresi; fark yok.

## Hata örnekleri

**enum class'ı başka bir enum'la ve sayıyla karşılaştırmak:** `hata_enum_class.cpp`

```cpp
// enum class'ı başka bir enum class'la ve sayıyla karşılaştırmak
#include <cstdint>

enum class FixTipi  : uint8_t { Yok, Fix2D, Fix3D };
enum class UcusModu : uint8_t { Hold, Land };

int main()
{
    FixTipi fix = FixTipi::Fix3D;
    bool a = (fix == UcusModu::Land);   // hata 1: farklı enum'lar
    bool b = (fix == 2);                // hata 2: enum class ile sayı
    return a || b;
}
```

```
hata_enum_class.cpp:10:19: error: no match for 'operator==' (operand types are 'FixTipi' and 'UcusModu')
hata_enum_class.cpp:11:19: error: no match for 'operator==' (operand types are 'FixTipi' and 'int')
```

İkisi de "bu iki tip birbiriyle karşılaştırılamaz". C'de uyarıyla geçen anlamsız karşılaştırma burada derlenmiyor.

**İlgisiz pointer tiplerini static_cast ile çevirmek:** `hata_static_cast.cpp`

```cpp
// static_cast ile birbiriyle ilgisi olmayan pointer tiplerini çevirmek
#include <cstdint>

struct GpsVeri  { int32_t lat; int32_t lon; };
struct BaroVeri { float basinc; float sicaklik; };

int main()
{
    BaroVeri baro{1013.25f, 21.5f};
    GpsVeri *g = static_cast<GpsVeri *>(&baro);   // hata bekliyoruz
    return g->lat;
}
```

```
hata_static_cast.cpp:10:18: error: invalid 'static_cast' from type 'BaroVeri*' to type 'GpsVeri*'
```

C'de saçma bir enlemle sonuçlanan çevirme burada derlenmiyor.

**static_assert ile yanlış bir sabiti yakalamak:** `hata_static_assert.cpp`

```cpp
// static_assert: derleme sırasında yanlış bir sabiti yakalamak
#include <cstdint>

constexpr uint32_t hz_to_us(uint32_t hz) { return 1000000 / hz; }

constexpr uint32_t GPS_HZ = 7;                     // biri yanlışlıkla 7 yazmış
static_assert(hz_to_us(GPS_HZ) == 125000, "GPS periyodu 125 ms olmali");

int main() { return 0; }
```

```
hata_static_assert.cpp:7:32: error: static assertion failed: GPS periyodu 125 ms olmali
hata_static_assert.cpp:7:32: note: the comparison reduces to '(142857 == 125000)'
```

Derleyici `hz_to_us(7)`'yi kendisi hesapladı (142857) ve koşulun tutmadığını söyledi. Yanlış sabit karta yüklenmeden yakalanıyor.

**C++'ta NULL ile aynı isimli fonksiyon çağırmak:** `hata_null.cpp`

```cpp
// C++'ta NULL ile aşırı yüklenmiş (aynı isimli) fonksiyon çağırmak
#include <cstdio>
#include <cstddef>

void kaydet(int x)          { printf("int %d\n", x); }
void kaydet(const char *s)  { printf("metin %s\n", s); }

int main()
{
    kaydet(NULL);    // NULL bir sayı mı, pointer mı?
    return 0;
}
```

```
hata_null.cpp:10:11: error: call of overloaded 'kaydet(NULL)' is ambiguous
hata_null.cpp:5:6: note: candidate: 'void kaydet(int)'
hata_null.cpp:6:6: note: candidate: 'void kaydet(const char*)'
```

"`kaydet(NULL)` belirsiz; iki aday var." `NULL` hem sayı hem adres gibi görünebildiği için derleyici seçemiyor. `nullptr` ile bu sorun yok.

## Özet: bellekte ve işlemcide

| C'deki | C++'taki | Çalışma zamanında fark |
|---|---|---|
| `enum` (4 byte) | `enum class : uint8_t` | 0 tick; RAM'de 3 byte daha az |
| `#define`, makro | `constexpr`, `_ms` | 0 tick; ikisi de derlenmiş kodda sadece bir sayı |
| — | `static_assert` | 0 tick, 0 byte; sadece derlemede |
| `(TIP)deger` | `static_cast<TIP>(deger)` | aynı komutlar |
| `NULL` | `nullptr` | ikisi de 0 adresi |

Hiçbiri çalışma zamanında maliyet getirmiyor; hepsi hataları derleme aşamasına çekiyor.

## PX4'te

**`static_cast`**: PX4 çalışmamda `failure gps off`'un neden çalışmadığını araştırırken okuduğum `SimulatorMavlink::check_failure_injections()` fonksiyonunda:

```cpp
const int failure_unit = static_cast<int>(vehicle_command.param1 + 0.5f);
const int failure_type = static_cast<int>(vehicle_command.param2 + 0.5f);
```

MAVLink komut parametreleri `float` geliyor; hata birimi (GPS, ivmeölçer...) ve tipi (off, stuck...) tam sayıya yuvarlanıyor. Bu dersteki `param1=3.9 -> birim=4` örneğinin aynısı.

**`nullptr`**: `SensorGpsSim` constructor'ında `ModuleParams(nullptr)` ("üst parametre grubu yok"), `print_usage(const char *reason = nullptr)`, ve modül silinince `_object.store(nullptr)`.

**`constexpr`**: SIH simülatöründe (`simulator_sih/sih.hpp`):

```cpp
static constexpr uint16_t NUM_ACTUATORS_MAX = 9;
...
float _u[NUM_ACTUATORS_MAX] {};   // motor sinyalleri
```

Constexpr sabit, dizi boyutu olarak kullanılıyor; bu dersteki `int dizi[GPS_HZ]` gibi.

**`_ms` ve `_s`**: `src/drivers/drv_hrt.h`, `time_literals` namespace'i içinde:

```cpp
constexpr hrt_abstime operator ""_ms(unsigned long long milliseconds)
{
	return hrt_abstime(milliseconds * 1000ULL);
}
```

Bu dersteki `_ms`'nin aynısı. `SensorGpsSim::init()` içindeki `ScheduleOnInterval(125_ms)`, derlenmiş kodda sadece 125000 sayısı. 4. dersteki (namespace) `using namespace time_literals;` satırı da bu yazımı öneksiz kullanabilmek içindi.

**`enum class`**: failsafe ayarlarında (`commander/failsafe/failsafe.h`):

```cpp
enum class LowBatteryAction : int32_t {
	Warning = 0,
	Return = 1,
	Land = 2,
	ReturnOrLand = 3
};
```

Düşük bataryada ne yapılacağı. `: int32_t` çünkü değeri bir PX4 parametresinden (32 bitlik tam sayı) geliyor.
