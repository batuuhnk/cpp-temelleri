# 7. template

PX4 kodunda en çok gördüğüm yazımlardan biri `uORB::Publication<sensor_gps_s>`. Sondaki `<...>` kısmı bir **template**: aynı kodu farklı veri tipleri için bir kez yazıp, her tip için ayrı bir kopyasını derleyiciye ürettirmek. C'de bunun için iki yol biliyordum: `void*` ile tipten bağımsız kod yazmak ya da makroyla her tip için kod üretmek. Bu derste üçünü karşılaştırdım. Kodların tamamı bu notun içinde.

## Örnek program

uORB topic'ine benzeyen basit bir **kanal**: bir modül veri yayınlıyor, diğeri en son yayınlanan veriyi okuyor. İki farklı veri tipi için kullanıyorum:

- `GpsVeri`: enlem, boylam, uydu sayısı (12 byte).
- `BaroVeri`: basınç, sıcaklık, zaman (12 byte).

İki tip için de aynı işi yapan bir kanal istiyorum: yayınla ve oku.

## C ile

Dosya: `c_versiyon.c`

```c
// 7. ders, C hali: farklı veri tipleri için aynı işi yapan kanal. İki yol: void* ve makro.
#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef struct { int32_t lat; int32_t lon; uint32_t uydu; } GpsVeri;     // 12 byte
typedef struct { float basinc; float sicaklik; uint32_t zaman; } BaroVeri; // 12 byte

// ---- Yol 1: void* ile tek bir kanal türü, her tipe çalışır ama tipi kontrol etmez ----
typedef struct {
    uint8_t  veri[64];     // en büyük mesaja yetecek kadar ham byte
    size_t   boyut;
    uint32_t sayac;
} KanalHam;

void kanal_ham_yayinla(KanalHam *k, const void *veri)
{
    memcpy(k->veri, veri, k->boyut);   // kaç byte olduğunu kanal biliyor, ne olduğunu bilmiyor
    k->sayac++;
}

void kanal_ham_oku(const KanalHam *k, void *hedef)
{
    memcpy(hedef, k->veri, k->boyut);
}

// ---- Yol 2: makro ile her tip için ayrı kod üretmek ----
#define KANAL_TANIMLA(TIP)                                              \
    typedef struct { TIP veri; uint32_t sayac; } Kanal_##TIP;           \
    static void kanal_##TIP##_yayinla(Kanal_##TIP *k, const TIP *v)     \
    {                                                                   \
        k->veri = *v;                                                   \
        k->sayac++;                                                     \
    }                                                                   \
    static void kanal_##TIP##_oku(const Kanal_##TIP *k, TIP *hedef)     \
    {                                                                   \
        *hedef = k->veri;                                               \
    }

KANAL_TANIMLA(GpsVeri)    // Kanal_GpsVeri, kanal_GpsVeri_yayinla, kanal_GpsVeri_oku oluşur
KANAL_TANIMLA(BaroVeri)   // Kanal_BaroVeri, kanal_BaroVeri_yayinla, kanal_BaroVeri_oku oluşur

int main(void)
{
    GpsVeri  gps  = { 473977420, 85455940, 12 };
    BaroVeri baro = { 1013.25f, 21.5f, 1000 };

    // Yol 1
    KanalHam gps_ham = { .boyut = sizeof(GpsVeri) };
    kanal_ham_yayinla(&gps_ham, &gps);
    GpsVeri okunan;
    kanal_ham_oku(&gps_ham, &okunan);
    printf("void* kanal:  lat=%d uydu=%u (sayac=%u)\n", (int)okunan.lat, (unsigned)okunan.uydu, (unsigned)gps_ham.sayac);

    kanal_ham_yayinla(&gps_ham, &baro);   // YANLIŞ: GPS kanalına barometre verisi. Derleyici bir şey demiyor.
    kanal_ham_oku(&gps_ham, &okunan);
    printf("yanlis tip:   lat=%d uydu=%u (sayac=%u)\n", (int)okunan.lat, (unsigned)okunan.uydu, (unsigned)gps_ham.sayac);

    // Yol 2
    Kanal_GpsVeri  gps_kanali  = {0};
    Kanal_BaroVeri baro_kanali = {0};
    kanal_GpsVeri_yayinla(&gps_kanali, &gps);
    kanal_BaroVeri_yayinla(&baro_kanali, &baro);
    GpsVeri  g;
    BaroVeri b;
    kanal_GpsVeri_oku(&gps_kanali, &g);
    kanal_BaroVeri_oku(&baro_kanali, &b);
    printf("makro kanal:  lat=%d uydu=%u, basinc=%.2f sicaklik=%.1f\n", (int)g.lat, (unsigned)g.uydu, b.basinc, b.sicaklik);
    return 0;
}
```

Ne yapıyor:

**Yol 1, `void*`:** `KanalHam` tek bir kanal türü. Verinin ne olduğunu bilmiyor, sadece kaç byte olduğunu (`boyut`) biliyor. `kanal_ham_yayinla` gelen adresten `boyut` kadar byte'ı `memcpy` ile kendi içine kopyalıyor. `void *` "herhangi bir şeyin adresi" demek; her tipte veri kabul ediyor.

**Yol 2, makro:** `KANAL_TANIMLA(TIP)` makrosu, verilen tip için bir struct ve iki fonksiyon üretiyor. `##` önişlemcinin "yapıştır" işareti: `Kanal_##TIP` → `Kanal_GpsVeri`. `KANAL_TANIMLA(GpsVeri)` satırı `Kanal_GpsVeri`, `kanal_GpsVeri_yayinla`, `kanal_GpsVeri_oku` üretiyor; `KANAL_TANIMLA(BaroVeri)` de barometre için aynılarını.

`main` önce `void*` kanalına GPS verisi yayınlayıp okuyor. Sonra bilerek **yanlış** bir şey yapıyor: GPS kanalına barometre verisi yayınlıyor. En son makro kanallarını kullanıyor.

Çıktı (`./calistir.sh 07-template/c_versiyon.c`):

```
void* kanal:  lat=473977420 uydu=12 (sayac=1)
yanlis tip:   lat=1149063168 uydu=1000 (sayac=2)
makro kanal:  lat=473977420 uydu=12, basinc=1013.25 sicaklik=21.5
```

| Çıktı | Hangi koddan geliyor | Anlamı |
|---|---|---|
| `void* kanal: lat=473977420 uydu=12` | ilk `kanal_ham_yayinla` ve `kanal_ham_oku` | Doğru kullanımda çalışıyor |
| `yanlis tip: lat=1149063168 uydu=1000` | `kanal_ham_yayinla(&gps_ham, &baro)` | **Derleyici uyarı bile vermedi.** Barometrenin byte'ları GPS gibi okundu: `1149063168`, 1013.25 basıncının float olarak bellekteki byte'larının tam sayı gibi okunmuş hali; `uydu=1000` aslında barometrenin `zaman` alanı |
| `makro kanal: ...` | `kanal_GpsVeri_...` ve `kanal_BaroVeri_...` | Her tip kendi fonksiyonunu kullanıyor, tipler karışmıyor |

İki yolun da sorunu var:

- **`void*`**: tip kontrolü yok. Yanlış veri sessizce kabul ediliyor ve sorun ancak uçuşta saçma bir değer olarak ortaya çıkıyor. Ayrıca `memcpy` her seferinde boyutu çalışırken okuyan genel bir fonksiyon.
- **Makro**: tip kontrolü var, ama makronun içi derleyici için tek uzun bir satır. İçinde bir hata olursa hata mesajı makroyu kullandığım satırı gösteriyor, makronun hangi kısmında olduğunu değil. Debugger'da makronun içine adım adım girilemiyor.

## C++ ile

Dosya: `cpp_versiyon.cpp`

```cpp
// 7. ders, C++ hali: template. Kanal'ı bir kez yazıyorum, derleyici her tip için ayrı kodunu üretiyor.
#include <cstdio>
#include <cstdint>

struct GpsVeri  { int32_t lat; int32_t lon; uint32_t uydu; };
struct BaroVeri { float basinc; float sicaklik; uint32_t zaman; };

// T: "bir tip". Kanal<GpsVeri> yazılınca her T'nin yerine GpsVeri geçiyor.
template<typename T>
class Kanal
{
public:
    void yayinla(const T &v)
    {
        _veri = v;
        _sayac++;
    }

    // Yeni veri varsa hedef'e kopyalayıp true döndürür, yoksa false
    bool guncelle(T &hedef)
    {
        if (_okunan == _sayac) {
            return false;
        }
        hedef = _veri;
        _okunan = _sayac;
        return true;
    }

private:
    T        _veri{};
    uint32_t _sayac{0};
    uint32_t _okunan{0};
};

// Fonksiyon template'i: PX4'teki math::constrain'in sade hali
template<typename T>
T sinirla(T deger, T alt, T ust)
{
    if (deger < alt) return alt;
    if (deger > ust) return ust;
    return deger;
}

int main()
{
    Kanal<GpsVeri>  gps_kanali;
    Kanal<BaroVeri> baro_kanali;

    gps_kanali.yayinla(GpsVeri{473977420, 85455940, 12});
    baro_kanali.yayinla(BaroVeri{1013.25f, 21.5f, 1000});

    GpsVeri  g{};
    BaroVeri b{};
    bool yeni = gps_kanali.guncelle(g);
    printf("gps:  yeni=%d lat=%d uydu=%u\n", yeni, (int)g.lat, (unsigned)g.uydu);
    yeni = gps_kanali.guncelle(g);
    printf("gps tekrar: yeni=%d (arada yayinlanan yok)\n", yeni);
    yeni = baro_kanali.guncelle(b);
    printf("baro: yeni=%d basinc=%.2f sicaklik=%.1f\n", yeni, b.basinc, b.sicaklik);

    printf("sizeof(Kanal<GpsVeri>) = %zu, sizeof(Kanal<BaroVeri>) = %zu\n", sizeof(Kanal<GpsVeri>), sizeof(Kanal<BaroVeri>));

    printf("sinirla(15, 0, 10)       = %d\n", sinirla(15, 0, 10));
    printf("sinirla(0.003f, ...)     = %.6f\n", sinirla(0.003f, 0.000125f, 0.02f));
    printf("sinirla(0.00001f, ...)   = %.6f\n", sinirla(0.00001f, 0.000125f, 0.02f));
    return 0;
}
```

C sürümünden farkları:

- `template<typename T>` satırı "aşağıdaki class'ta `T` bir tip yerine geçiyor" demek. `Kanal`'ı bir kez, normal bir class gibi yazıyorum; `T` yazdığım her yer, kullanırken hangi tipi verirsem o oluyor.
- `Kanal<GpsVeri> gps_kanali;` yazınca derleyici `T` yerine `GpsVeri` koyarak bir class üretiyor; `Kanal<BaroVeri>` için de ayrı bir class. Makronun yaptığı işin aynısı, ama derleyici tarafından ve tip kontrolüyle.
- `yayinla(const T &v)`: 3. dersteki (referans ve const) const referans. Veri kopyalanmadan veriliyor.
- `guncelle(T &hedef)` sadece yeni veri varsa kopyalıyor ve `true` döndürüyor. `_sayac` her yayında artıyor; `_okunan` en son hangi yayını okuduğumu tutuyor. PX4'teki `update()` de buna benzer çalışıyor.
- `sinirla` bir **fonksiyon template'i**: değeri alt ve üst sınır arasında tutuyor. `int` ile de `float` ile de çalışıyor. PX4'teki `math::constrain`'in sade hali.

Çıktı (`./calistir.sh 07-template/cpp_versiyon.cpp`):

```
gps:  yeni=1 lat=473977420 uydu=12
gps tekrar: yeni=0 (arada yayinlanan yok)
baro: yeni=1 basinc=1013.25 sicaklik=21.5
sizeof(Kanal<GpsVeri>) = 20, sizeof(Kanal<BaroVeri>) = 20
sinirla(15, 0, 10)       = 10
sinirla(0.003f, ...)     = 0.003000
sinirla(0.00001f, ...)   = 0.000125
```

| Çıktı | Hangi koddan geliyor | Anlamı |
|---|---|---|
| `gps: yeni=1 lat=473977420 uydu=12` | ilk `gps_kanali.guncelle(g)` | Yeni veri vardı, okundu |
| `gps tekrar: yeni=0` | ikinci `gps_kanali.guncelle(g)` | Arada yayın olmadığı için okunacak yeni veri yok |
| `baro: yeni=1 basinc=1013.25 ...` | `baro_kanali.guncelle(b)` | Aynı kod, farklı tip |
| `sizeof(...) = 20` | `sizeof` | 12 byte veri + iki sayaç (4 + 4). Template ek bellek getirmiyor |
| `sinirla(15, 0, 10) = 10` | `sinirla<int>` | 15, üst sınır 10'a indirildi |
| `sinirla(0.003f, ...) = 0.003000` | `sinirla<float>` | Sınırların içinde, değişmedi |
| `sinirla(0.00001f, ...) = 0.000125` | `sinirla<float>` | Alt sınıra çıkarıldı. PX4'teki `dt` hesabında kullanılan sınırların aynısı |

## Template nedir?

Template, derleyiciye verilen bir **kalıp**. Kendisi kod değil; `Kanal<GpsVeri>` gibi bir tiple kullanıldığında derleyici kalıbı o tiple doldurup gerçek kodu üretiyor.

Derlenmiş dosyanın sembol tablosuna (fonksiyon isimleri listesi) baktım (`nm -C` ile):

```
Kanal<GpsVeri>::yayinla(GpsVeri const&)
Kanal<GpsVeri>::guncelle(GpsVeri&)
Kanal<BaroVeri>::yayinla(BaroVeri const&)
Kanal<BaroVeri>::guncelle(BaroVeri&)
int sinirla<int>(int, int, int)
float sinirla<float>(float, float, float)
```

Kaynak kodda bir tane `Kanal` ve bir tane `sinirla` yazdım; derlenmiş dosyada her biri **kullandığım her tip için ayrı** bir fonksiyon olarak var. Makronun ürettiği `kanal_GpsVeri_yayinla` ve `kanal_BaroVeri_yayinla`'nın aynısı.

Makrodan farkları:

- Tip kontrolü var (aşağıda *Hata örneği*).
- Template'in içi normal kod gibi yazılıyor ve okunuyor; hata mesajları doğru satırı gösteriyor, debugger'da içine girilebiliyor.
- Fonksiyon template'lerinde tipi yazmak gerekmiyor: `sinirla(15, 0, 10)` yazınca derleyici `T`'nin `int` olduğunu parametrelerden kendisi anlıyor.

## Hata örneği

C'deki `void*` yönteminde sessizce geçen hatayı template ile denedim: `hata_yanlis_tip.cpp`

```cpp
// Template kanalına yanlış tipte veri yayınlamak
#include <cstdint>

struct GpsVeri  { int32_t lat; int32_t lon; uint32_t uydu; };
struct BaroVeri { float basinc; float sicaklik; uint32_t zaman; };

template<typename T>
class Kanal
{
public:
    void yayinla(const T &v) { _veri = v; }
private:
    T _veri{};
};

int main()
{
    Kanal<GpsVeri> gps_kanali;
    BaroVeri baro{1013.25f, 21.5f, 1000};
    gps_kanali.yayinla(baro);    // hata bekliyoruz: GPS kanalına barometre verisi
    return 0;
}
```

```
hata_yanlis_tip.cpp:20:24: error: cannot convert 'BaroVeri' to 'const GpsVeri&'
hata_yanlis_tip.cpp:11:27: note:   initializing argument 1 of 'void Kanal<T>::yayinla(const T&) [with T = GpsVeri]'
```

"`BaroVeri`, `const GpsVeri&`'e çevrilemez." İkinci satır da hangi template'in hangi tiple kullanıldığını gösteriyor: `[with T = GpsVeri]`. C'de uçuşta saçma bir enlem olarak ortaya çıkacak hata, burada program derlenmeden yakalanıyor.

## İşlemci ne yapıyor?

Template'in bütün işi derleme sırasında bitiyor. Program çalışırken `Kanal<GpsVeri>::yayinla` sıradan bir fonksiyon; içinde "T neydi?" diye bakılan hiçbir şey yok.

`gps_kanali.yayinla(v)` çağrısı:

| Adım | İşlemci ne yapıyor | Yaklaşık süre |
|---|---|---|
| 1 | `this` = kanalın adresi, `v`'nin adresi register'lara konup fonksiyona atlanıyor | ~4–6 tick |
| 2 | `v`'den 12 byte okunup kanalın `_veri` alanına yazılıyor: derleyici boyutun 12 olduğunu bildiği için 3 okuma + 3 yazma | ~6 tick |
| 3 | `_sayac` okunup 1 eklenip yazılıyor | ~3–4 tick |
| 4 | Geri dönülüyor | ~2–4 tick |

C'deki `void*` sürümünde 2. adım `memcpy` çağrısı: ayrı bir fonksiyona atlama, boyutu kanaldan okuma ve byte'ları bir döngüyle kopyalama. Template'te derleyici tipi ve boyutu bildiği için kopyalamayı doğrudan yazıyor. Yani template, `void*`'dan **daha az** tick harcıyor, makroyla ise aynı.

6. dersteki `virtual`'dan farkı: virtual'da hangi fonksiyonun çalışacağı **çalışırken** tablodan okunuyor (~4 tick ek maliyet). Template'te her şey **derlerken** belli; tablo yok, ek tick yok.

Template'in bir maliyeti var: **flash**. Her tip için kodun ayrı bir kopyası üretiliyor. 2 tip için 2 kopya, 50 tip için 50 kopya; makroda da aynısı olurdu. RAM'e etkisi yok: `Kanal<GpsVeri>` 20 byte, içinde sadece veri ve sayaçlar var.

## PX4'te

**`uORB::Publication<T>`** (`platforms/common/uORB/Publication.hpp`):

```cpp
template<typename T>
class Publication : public PublicationBase
{
	...
	bool publish(const T &data)
	...
};
```

Bu dersteki `Kanal<T>`'nin gerçek hali. `SensorGpsSim`'de `uORB::PublicationMulti<sensor_gps_s> _sensor_gps_pub` yazınca, derleyici `T` yerine `sensor_gps_s` koyarak GPS mesajına özel bir yayıncı üretiyor. `publish(const T &data)` 3. dersteki `publish(const sensor_gps_s &data)` oluyor; GPS yayıncısına barometre mesajı vermeye çalışırsam program derlenmiyor.

**Eski C arayüzü** (`platforms/common/uORB/uORB.h`):

```cpp
int orb_publish(const struct orb_metadata *meta, orb_advert_t handle, const void *data);
```

Bu dersteki `void*` yolunun aynısı: veri `const void *` olarak veriliyor, boyutu `meta` biliyor, tip kontrolü yok. PX4'ün eski modüllerinde bu arayüz kullanılıyor; PX4 çalışmamda değiştirdiğim `px4_simple_app` örneği de bunu kullanıyordu (`orb_publish(ORB_ID(vehicle_attitude), att_pub, &att)`). Yeni modüller template'li `Publication<T>`'yi kullanıyor.

**`math::constrain`** (`src/lib/mathlib/math/Limits.hpp`):

```cpp
template<typename _Tp>
constexpr _Tp constrain(_Tp val, _Tp min_val, _Tp max_val)
```

Bu dersteki `sinirla`. `MulticopterRateControl::Run()` içinde `dt` hesaplanırken kullanılıyor: `math::constrain(((now - _last_run) * 1e-6f), 0.000125f, 0.02f)`. Çıktıdaki son iki satır bu sınırlarla denediğim değerler.

**`matrix::Vector3f`** (`src/lib/matrix/matrix/Vector3.hpp`):

```cpp
using Vector3f = Vector3<float>;
```

PX4'ün matris kütüphanesi baştan sona template. `Vector3<float>` float'lardan oluşan 3'lü vektör; `Vector3f` sadece kısa adı.

**`ModuleBase<SensorGpsSim>`** (`platforms/common/include/px4_platform_common/module.h`):

```cpp
template<class T>
class ModuleBase
{
	...
	T *object = T::instantiate(argc, argv);
	...
	int ret = T::custom_command(argc - 1, argv + 1);
```

Burada ilginç bir kullanım var: `SensorGpsSim`, template parametresi olarak **kendini** veriyor: `class SensorGpsSim : public ModuleBase<SensorGpsSim>`. Böylece `ModuleBase` içindeki `T::custom_command(...)` gibi çağrılar `SensorGpsSim::custom_command(...)` oluyor. `start`, `stop`, `status` komutlarını işleyen ortak kod bir kez yazılmış; her modül kendini vererek bu kodun kendine özel bir kopyasını alıyor. 6. dersteki virtual tablosuna gerek kalmadan, derleme sırasında.
