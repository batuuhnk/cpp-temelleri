# 3. referans ve const

C'de bir struct'ı fonksiyona vermenin iki yolu var: kopyasını vermek ya da adresini (pointer) vermek. C++ bunlara üçüncü bir yol ekliyor: **referans**. Bu derste üçünü karşılaştırdım; bellekte ne kopyalandığına, ne kadar sürdüğüne ve `const`'un ne işe yaradığına baktım. Kodların tamamı bu notun içinde.

## Örnek program

PX4'teki `sensor_gps` mesajına benzeyen bir struct kullandım: zaman, konum, irtifa, hata payları, hızlar, uydu sayısı, fix tipi. Gerçek `sensor_gps` mesajı 160 byte (`uorb top` çıktısındaki boyut sütunu); benimki 48 byte.

Bu struct'ı üç farklı fonksiyona veriyorum:

- `yazdir_kopya`: struct'ın kopyasını alıp yazdırıyor.
- `yazdir_pointer` / `yazdir_ref`: struct'ı sadece okuyup yazdırıyor, kopyalamadan.
- `sifirla`: struct'taki uydu sayısını ve fix tipini sıfırlıyor, yani asıl değişkeni değiştiriyor.

Her fonksiyon, eline geçen struct'ın bellekteki adresini de yazdırıyor. Böylece kopya mı yoksa asıl değişken mi olduğu görülebiliyor.

## C ile

Dosya: `c_versiyon.c`

```c
// 3. ders, C hali: bir struct'ı fonksiyona vermenin iki yolu var: kopyalayarak ya da adresini vererek.
#include <stdio.h>
#include <stdint.h>

// PX4'teki sensor_gps mesajına benzeyen, biraz büyük bir struct
typedef struct {
    uint64_t zaman_us;
    int32_t  lat;
    int32_t  lon;
    int32_t  irtifa_mm;
    float    yatay_hata_m;
    float    dikey_hata_m;
    float    hiz_kuzey;
    float    hiz_dogu;
    float    hiz_asagi;
    uint8_t  uydu_sayisi;
    uint8_t  fix_tipi;
} GpsVeri;

// 1) Kopyalayarak: fonksiyon struct'ın bir kopyasını alıyor
void yazdir_kopya(GpsVeri v)
{
    printf("  yazdir_kopya:   v'nin adresi %p, uydu=%d\n", (void *)&v, v.uydu_sayisi);
    v.uydu_sayisi = 0;   // sadece kopya değişiyor
}

// 2) Adresini vererek, sadece okumak için: const pointer
void yazdir_pointer(const GpsVeri *v)
{
    if (v == NULL) {
        printf("  yazdir_pointer: NULL geldi, hicbir sey yapilmadi\n");
        return;
    }
    printf("  yazdir_pointer: v'nin degeri  %p, uydu=%d\n", (const void *)v, v->uydu_sayisi);
}

// 3) Adresini vererek, değiştirmek için: pointer
void sifirla(GpsVeri *v)
{
    if (v == NULL) {
        return;
    }
    v->uydu_sayisi = 0;
    v->fix_tipi = 0;
}

int main(void)
{
    GpsVeri veri = {0};
    veri.uydu_sayisi = 12;
    veri.fix_tipi = 3;

    printf("GpsVeri boyutu: %zu byte\n", sizeof(GpsVeri));
    printf("veri'nin adresi:              %p\n", (void *)&veri);

    yazdir_kopya(veri);
    printf("yazdir_kopya sonrasi uydu = %d\n", veri.uydu_sayisi);

    yazdir_pointer(&veri);
    yazdir_pointer(NULL);

    sifirla(&veri);
    printf("sifirla sonrasi uydu = %d, fix = %d\n", veri.uydu_sayisi, veri.fix_tipi);
    return 0;
}
```

Ne yapıyor:

- `yazdir_kopya(GpsVeri v)`: parametre struct'ın kendisi. Çağrılınca 48 byte'lık struct'ın tamamı kopyalanıyor; fonksiyon bu kopyayla çalışıyor. İçeride `v.uydu_sayisi = 0;` yazmak sadece kopyayı değiştiriyor.
- `yazdir_pointer(const GpsVeri *v)`: parametre struct'ın adresi. Kopya yok. `const` "bu adresteki veriyi değiştirmeyeceğim" demek. Adres `NULL` olabileceği için önce kontrol ediliyor.
- `sifirla(GpsVeri *v)`: yine adres, ama `const` yok, çünkü veriyi değiştirecek. O da `NULL` kontrolü yapıyor.
- `main`'de `yazdir_pointer(NULL)` diye bilerek bir de `NULL` veriyorum.

Çıktı (`./calistir.sh 03-referans-const/c_versiyon.c`):

```
GpsVeri boyutu: 48 byte
veri'nin adresi:              0x7fff11ef11b0
  yazdir_kopya:   v'nin adresi 0x7fff11ef1180, uydu=12
yazdir_kopya sonrasi uydu = 12
  yazdir_pointer: v'nin degeri  0x7fff11ef11b0, uydu=12
  yazdir_pointer: NULL geldi, hicbir sey yapilmadi
sifirla sonrasi uydu = 0, fix = 0
```

| Çıktı | Hangi koddan geliyor | Anlamı |
|---|---|---|
| `GpsVeri boyutu: 48 byte` | `sizeof(GpsVeri)` | Kopyalanacak veri miktarı |
| `veri'nin adresi: ...11b0` | `main` | Asıl değişkenin adresi |
| `yazdir_kopya: v'nin adresi ...1180` | `yazdir_kopya` | **Farklı adres**: fonksiyon kopyayla çalışıyor |
| `yazdir_kopya sonrasi uydu = 12` | `main` | Fonksiyonun içinde 0 yazıldı ama asıl değişken değişmedi |
| `yazdir_pointer: v'nin degeri ...11b0` | `yazdir_pointer(&veri)` | **Aynı adres**: kopya yok, asıl değişken okunuyor |
| `yazdir_pointer: NULL geldi, ...` | `yazdir_pointer(NULL)` | `NULL` kontrolü olmasaydı program burada çökerdi |
| `sifirla sonrasi uydu = 0, fix = 0` | `sifirla(&veri)` | Pointer üzerinden asıl değişken değişti |

## Kopyalamak ne kadar sürüyor?

`GpsVeri`'nin bellekteki yerleşimi:

```
+0   zaman_us       8 byte
+8   lat            4 byte
+12  lon            4 byte
+16  irtifa_mm      4 byte
+20  yatay_hata_m   4 byte
+24  dikey_hata_m   4 byte
+28  hiz_kuzey      4 byte
+32  hiz_dogu       4 byte
+36  hiz_asagi      4 byte
+40  uydu_sayisi    1 byte
+41  fix_tipi       1 byte
+42  boş            6 byte (toplam 8'in katı olsun diye; en büyük alan 8 byte)
     toplam        48 byte
```

32 bitlik bir STM32 bellekle 4'er byte'lık parçalar halinde konuşuyor. Bu struct'ı kopyalamak 12 kez "4 byte oku, 4 byte yaz" demek:

| Yöntem | İşlemci ne yapıyor | Yaklaşık süre |
|---|---|---|
| Kopyalayarak | Stack'te 48 byte'lık yer ayırıyor, asıl struct'tan 12 kez okuyup 12 kez yazıyor, sonra kopyanın adresini fonksiyona veriyor | ~25–30 tick |
| Pointer ile | Asıl struct'ın adresini bir register'a koyuyor | ~1 tick |
| `NULL` kontrolü | Register'daki adresi 0 ile karşılaştırıp gerekirse atlıyor | ~1–2 tick |

160 byte'lık gerçek `sensor_gps` mesajında kopyalama ~80–100 tick'e çıkıyor. Ayrıca kopya kadar stack yeri kaplıyor. Fonksiyon saniyede yüzlerce kez çağrılıyorsa (PX4'te sensör verisi 250 Hz) bu fark önem kazanıyor.

## C++ ile

Dosya: `cpp_versiyon.cpp`

```cpp
// 3. ders, C++ hali: kopya, const referans ve referans.
#include <cstdio>
#include <cstdint>

struct GpsVeri {
    uint64_t zaman_us;
    int32_t  lat;
    int32_t  lon;
    int32_t  irtifa_mm;
    float    yatay_hata_m;
    float    dikey_hata_m;
    float    hiz_kuzey;
    float    hiz_dogu;
    float    hiz_asagi;
    uint8_t  uydu_sayisi;
    uint8_t  fix_tipi;
};

// 1) Kopyalayarak: C ile aynı
void yazdir_kopya(GpsVeri v)
{
    printf("  yazdir_kopya:   v'nin adresi %p, uydu=%d\n", (void *)&v, v.uydu_sayisi);
    v.uydu_sayisi = 0;   // sadece kopya değişiyor
}

// 2) Sadece okumak için: const referans. NULL kontrolü yok, erişim "." ile.
void yazdir_ref(const GpsVeri &v)
{
    printf("  yazdir_ref:     v'nin adresi %p, uydu=%d\n", (const void *)&v, v.uydu_sayisi);
}

// 3) Değiştirmek için: referans
void sifirla(GpsVeri &v)
{
    v.uydu_sayisi = 0;
    v.fix_tipi = 0;
}

int main()
{
    GpsVeri veri{};
    veri.uydu_sayisi = 12;
    veri.fix_tipi = 3;

    printf("GpsVeri boyutu: %zu byte\n", sizeof(GpsVeri));
    printf("veri'nin adresi:              %p\n", (void *)&veri);

    yazdir_kopya(veri);
    printf("yazdir_kopya sonrasi uydu = %d\n", veri.uydu_sayisi);

    yazdir_ref(veri);          // çağırırken & yazılmıyor, ama kopya da yapılmıyor

    // Referans, var olan bir değişkene ikinci bir isim: aynı adres
    GpsVeri &takma_ad = veri;
    printf("takma_ad'in adresi:           %p\n", (void *)&takma_ad);
    takma_ad.uydu_sayisi = 7;
    printf("takma_ad uzerinden 7 yazildi, veri.uydu = %d\n", veri.uydu_sayisi);

    sifirla(veri);
    printf("sifirla sonrasi uydu = %d, fix = %d\n", veri.uydu_sayisi, veri.fix_tipi);
    return 0;
}
```

C sürümünden farkları:

- `typedef struct { ... } GpsVeri;` yerine `struct GpsVeri { ... };`. C++'ta `struct` ile `class` neredeyse aynı şey; tek fark, `struct`'ta her şey varsayılan olarak `public`. Sadece veri tutan yapılar için `struct` kullanılıyor. PX4'ün uORB mesajları da böyle: `struct sensor_gps_s`.
- `yazdir_kopya` C ile birebir aynı.
- `yazdir_pointer(const GpsVeri *v)` yerine `yazdir_ref(const GpsVeri &v)`: parametre bir **const referans**. Fonksiyonun içinde `v->uydu_sayisi` yerine `v.uydu_sayisi` yazılıyor, `NULL` kontrolü yok.
- `sifirla(GpsVeri *v)` yerine `sifirla(GpsVeri &v)`: parametre bir **referans**, `const` yok çünkü değiştiriyor.
- Çağırırken `&` yazılmıyor: `yazdir_ref(veri)`, `sifirla(veri)`. Görünüşte kopyalayan çağrıyla aynı, ama arka planda adres gidiyor.
- `GpsVeri &takma_ad = veri;` satırı, referansın fonksiyon parametresi olmadan da kullanılabildiğini gösteriyor.

Çıktı (`./calistir.sh 03-referans-const/cpp_versiyon.cpp`):

```
GpsVeri boyutu: 48 byte
veri'nin adresi:              0x7ffe0012f980
  yazdir_kopya:   v'nin adresi 0x7ffe0012f940, uydu=12
yazdir_kopya sonrasi uydu = 12
  yazdir_ref:     v'nin adresi 0x7ffe0012f980, uydu=12
takma_ad'in adresi:           0x7ffe0012f980
takma_ad uzerinden 7 yazildi, veri.uydu = 7
sifirla sonrasi uydu = 0, fix = 0
```

| Çıktı | Hangi koddan geliyor | Anlamı |
|---|---|---|
| `veri'nin adresi: ...f980` | `main` | Asıl değişkenin adresi |
| `yazdir_kopya: v'nin adresi ...f940` | `yazdir_kopya` | Farklı adres, kopya (C ile aynı) |
| `yazdir_ref: v'nin adresi ...f980` | `yazdir_ref(veri)` | **Aynı adres**: `&` yazmadan çağırdım ama kopya yapılmadı |
| `takma_ad'in adresi: ...f980` | `GpsVeri &takma_ad = veri;` | Referans yeni bir değişken değil, aynı belleğe ikinci isim |
| `takma_ad uzerinden 7 yazildi, veri.uydu = 7` | `takma_ad.uydu_sayisi = 7;` | Takma ada yazmak asıl değişkene yazmak |
| `sifirla sonrasi uydu = 0, fix = 0` | `sifirla(veri)` | Referans üzerinden asıl değişken değişti |

## Referans nedir?

Referans, var olan bir değişkene verilen **ikinci bir isim**. `GpsVeri &takma_ad = veri;` dedikten sonra `takma_ad` ile `veri` aynı bellek adresini gösteriyor; çıktıda ikisinin adresi aynı.

Arka planda referans bir adres. Derleyici `yazdir_ref(veri)` gördüğünde `veri`'nin adresini register'a koyuyor, tıpkı C'deki `yazdir_pointer(&veri)` gibi. Fonksiyonun içindeki `v.uydu_sayisi` de "o adres + 40'taki byte" oluyor. **İşlemci için referans ile pointer arasında fark yok; tick sayısı aynı.**

Farklar derleme aşamasında:

| | Pointer (`GpsVeri *p`) | Referans (`GpsVeri &r`) |
|---|---|---|
| `NULL` olabilir mi? | Evet, kontrol etmek gerekiyor | Hayır, her zaman bir değişkene bağlı |
| Oluştururken bir değişkene bağlamak zorunlu mu? | Hayır | Evet (aşağıda *Hata örneği*) |
| Sonradan başka değişkeni gösterebilir mi? | Evet, `p = &baska;` | Hayır, ömrü boyunca aynı değişken |
| Alana erişim | `p->uydu_sayisi` | `r.uydu_sayisi` |
| Çağırırken | `f(&veri)` | `f(veri)` |

`NULL` olamaması sayesinde C'deki `if (v == NULL)` kontrolüne gerek kalmıyor; o 1–2 tick ve bir hata ihtimali ortadan kalkıyor.

Bir dezavantajı var: çağrıya bakınca (`sifirla(veri)`) fonksiyonun değişkeni değiştireceği anlaşılmıyor. C'deki `sifirla(&veri)` yazımında `&` bunu belli ediyordu. PX4 bu yüzden bazı yerlerde hâlâ pointer kullanıyor (aşağıda *PX4'te*).

## const

`const` "bu değişmeyecek" demek, ve derleyici buna uyulup uyulmadığını kontrol ediyor:

- `const GpsVeri &v`: fonksiyon `v` üzerinden hiçbir alanı değiştiremiyor. Denersem derleme hatası (aşağıda *Hata örneği*).
- C'deki `const GpsVeri *v` ile aynı anlam.
- 1. dersteki `void yazdir() const` da aynı fikir: `this` üzerinden değişiklik yapılamıyor.

`const` sadece derleme sırasında kontrol ediliyor; işlemci için hiçbir şey değiştirmiyor, tick harcamıyor.

**Const referans**, büyük bir veriyi sadece okumak için en iyi yol: kopya yok (referans olduğu için) ve yanlışlıkla değiştirme yok (`const` olduğu için).

## Hangisi ne zaman?

| Durum | Ne kullanılıyor | Neden |
|---|---|---|
| Küçük tipler (`int`, `float`, `bool`) | Kopyalayarak: `void f(int x)` | Zaten bir register'a sığıyor, adres vermek daha hızlı değil |
| Büyük bir struct'ı sadece okumak | Const referans: `void f(const GpsVeri &v)` | Kopya yok, değiştirilemez |
| Büyük bir struct'ı değiştirmek | Referans `void f(GpsVeri &v)` ya da pointer `void f(GpsVeri *v)` | Asıl değişken değişsin; pointer'da çağrıdaki `&` bunu belli ediyor |
| "Hiç veri yok" durumu olabiliyorsa | Pointer | `NULL` ile "yok" denebiliyor; referansla denemez |

## Hata örneği

Referansla ilgili iki hatayı tek dosyada denedim: `hata_ornegi.cpp`

```cpp
// Referansla ilgili iki hata: const referans üzerinden yazmak ve başlatılmamış referans.
#include <cstdint>

struct GpsVeri {
    uint8_t uydu_sayisi;
};

void yazdir_ref(const GpsVeri &v)
{
    v.uydu_sayisi = 0;   // hata 1: v const, değiştirilemez
}

int main()
{
    GpsVeri &r;          // hata 2: referans bir değişkene bağlanmadan oluşturulamaz
    return 0;
}
```

Derleyici çıktısı (program hiç oluşmuyor):

```
hata_ornegi.cpp:10:19: error: assignment of member 'GpsVeri::uydu_sayisi' in read-only object
   10 |     v.uydu_sayisi = 0;   // hata 1: v const, değiştirilemez
hata_ornegi.cpp:15:14: error: 'r' declared as reference but not initialized
   15 |     GpsVeri &r;          // hata 2: referans bir değişkene bağlanmadan oluşturulamaz
```

| Hata | Anlamı |
|---|---|
| `assignment of member ... in read-only object` | `v` const referans; "salt okunur nesnenin alanına atama yapılamaz" |
| `'r' declared as reference but not initialized` | Referans oluşturulurken hangi değişkenin takma adı olduğu söylenmeli. Bu yüzden "boş" ya da `NULL` bir referans olamıyor |

## PX4'te

PX4'te iki yönde iki farklı yöntem kullanılıyor.

**Veri yayınlarken const referans** (`platforms/common/uORB/Publication.hpp`):

```cpp
bool publish(const T &data)
```

`SensorGpsSim::Run()` içinde GPS verisi hazırlanıp şöyle yayınlanıyor:

```cpp
sensor_gps_s sensor_gps{};
...
_sensor_gps_pub.publish(sensor_gps);
```

160 byte'lık `sensor_gps` fonksiyona kopyalanmıyor; `publish` sadece adresini alıyor ve veriyi değiştiremiyor. `publish` veriyi uORB'nin kendi tamponuna bir kez kopyalıyor, o kadar. (`T` yerine burada `sensor_gps_s` geçiyor; bu yazımı 7. derste, template'te göreceğim.)

**Veri okurken pointer** (`platforms/common/uORB/Subscription.hpp`):

```cpp
bool update(void *dst)
```

Modüllerde şöyle çağrılıyor: `_sub.update(&veri)`. Burada referans yerine pointer kullanılıyor. Çağrıdaki `&`, okuyana "bu fonksiyon `veri`'nin içine yazacak" diye açıkça gösteriyor.

Küçük tiplerde bile const referans görülebiliyor, örneğin `EKF2::PublishAttitude(const hrt_abstime &timestamp)`. `hrt_abstime` 64 bitlik bir sayı; 32 bitlik işlemcide kopyalayarak verilirse iki register kaplıyor, referansla tek register (adres) yetiyor.
