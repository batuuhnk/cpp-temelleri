# 1. class

Bu derste C'de struct + fonksiyonlarla yazdığım bir kodu C++'ta `class` ile yazdım. İkisini karşılaştırıp işlemcinin bellekte gerçekte ne yaptığına baktım. Kodların tamamı bu notun içinde; ayrıca dosyalara bakmaya gerek yok.

## Örnek program

Basit bir GPS simülatörü:

- Konumu (enlem, boylam) ve uydu sayısını tutuyor.
- Konum güncellenebiliyor.
- GPS "engelli" yapılabiliyor (GPS kesik gibi); engelliyken güncelleme kabul edilmiyor.
- Konum ekrana yazdırılabiliyor.

Enlem ve boylamı `double` yerine `int32_t` olarak, derece × 10⁷ şeklinde tutuyorum. Gerçek GPS modülleri de (örneğin u-blox) konumu böyle gönderiyor: 47.3977420° → `473977420`.

## 1. C ile

Dosya: `c_versiyon.c`

```c
// 1. ders, C hali: veri bir struct'ta, fonksiyonlar dışarıda ve struct'ın adresini parametre olarak alıyor.
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>   // offsetof

typedef struct {
    int32_t lat;          // derece x 10^7 (GPS modülleri genelde böyle gönderiyor)
    int32_t lon;
    uint8_t uydu_sayisi;
    bool    engelli;      // true ise yeni veri kabul edilmez (GPS kesik)
} GpsSim;

// p: struct'ın bellekteki adresi. İsmi önemli değil, "p", "gps", "self" de denebilirdi.
void gps_guncelle(GpsSim *p, int32_t lat, int32_t lon, uint8_t uydu)
{
    if (p->engelli) {     // "p adresinden 9 byte ilerideki değeri oku"
        return;
    }
    p->lat = lat;         // "p adresine lat'ı yaz"
    p->lon = lon;         // "p + 4 adresine lon'u yaz"
    p->uydu_sayisi = uydu;
}

void gps_engelle(GpsSim *p, bool engelli)
{
    p->engelli = engelli;
}

void gps_yazdir(const GpsSim *p)
{
    printf("lat=%.7f lon=%.7f uydu=%d\n", p->lat / 1e7, p->lon / 1e7, p->uydu_sayisi);
}

int main(void)
{
    GpsSim gps = {0};

    printf("gps nesnesinin adresi: %p\n", (void *)&gps);
    printf("alanlarin konumu: lat +%zu, lon +%zu, uydu_sayisi +%zu, engelli +%zu, toplam %zu byte\n",
           offsetof(GpsSim, lat), offsetof(GpsSim, lon),
           offsetof(GpsSim, uydu_sayisi), offsetof(GpsSim, engelli), sizeof(GpsSim));

    gps_guncelle(&gps, 473977420, 85455940, 12);
    gps_yazdir(&gps);

    gps_engelle(&gps, true);
    gps_guncelle(&gps, 474000000, 86000000, 10);   // engelli, değişmemeli
    gps_yazdir(&gps);

    gps.uydu_sayisi = 200;   // C'de kimse durdurmuyor: struct'ın içine doğrudan yazılabiliyor
    gps_yazdir(&gps);

    return 0;
}
```

Ne yapıyor:

- `GpsSim` struct'ı veriyi tutuyor: enlem, boylam, uydu sayısı, engelli mi.
- Üç fonksiyon var: `gps_guncelle`, `gps_engelle`, `gps_yazdir`. Üçü de ilk parametre olarak struct'ın **adresini** (`GpsSim *p`) alıyor. Böylece hangi GPS nesnesi üzerinde çalışacaklarını biliyorlar.
- `main` önce struct'ın bellekte nasıl yerleştiğini yazdırıyor (`offsetof`: bir alanın struct'ın başından kaç byte ileride olduğu). Sonra sırayla: konumu güncelliyor, GPS'i engelleyip tekrar güncellemeye çalışıyor, en sonda struct'ın içine dışarıdan doğrudan `200` yazıyor.

Çıktı (`./calistir.sh 01-class/c_versiyon.c`):

```
gps nesnesinin adresi: 0x7ffe10aefe7c
alanlarin konumu: lat +0, lon +4, uydu_sayisi +8, engelli +9, toplam 12 byte
lat=47.3977420 lon=8.5455940 uydu=12
lat=47.3977420 lon=8.5455940 uydu=12
lat=47.3977420 lon=8.5455940 uydu=200
```

- 3. satır: ilk güncelleme yapıldı.
- 4. satır: GPS engelliyken güncelleme denendi, konum değişmedi.
- 5. satır: `gps.uydu_sayisi = 200;` satırı struct'ın içine doğrudan yazdı. C'de bunu engelleyen bir şey yok; `gps_guncelle`'deki "engelliyse kabul etme" kontrolü de atlanmış oldu.

## 2. Struct bellekte nasıl duruyor?

Çıktının 2. satırı struct'ın yerleşimini veriyor: `lat +0, lon +4, uydu_sayisi +8, engelli +9, toplam 12 byte`.

Struct, bellekte art arda duran byte'lardan ibaret. Her alanın struct'ın başına göre sabit bir uzaklığı var. Nesne STM32'de RAM'in `0x20000100` adresinde olsaydı:

```
adres        alan            uzaklık
0x20000100   lat             +0   (4 byte)
0x20000104   lon             +4   (4 byte)
0x20000108   uydu_sayisi     +8   (1 byte)
0x20000109   engelli         +9   (1 byte)
0x2000010A   boş             2 byte, toplam 12 olsun diye (hizalama)
```

Alan isimleri sadece kaynak kodda var. Derleyici `engelli` gördüğünde onu "nesnenin adresi + 9" diye çeviriyor; işlemci sadece adres görüyor.

## 3. `p` nedir?

`gps_guncelle(GpsSim *p, ...)` içindeki `p` özel bir kelime değil, sadece parametrenin adı (`gps`, `ptr`, `self` de denebilirdi). Çağırırken `&gps` veriyorum, yani `p`'nin içinde nesnenin adresi var; örnekte `0x20000100`.

`p->engelli` demek "p'deki adrese git, 9 byte ileriye bak" demek. `p->lat = lat` demek "p'deki adrese lat'ı yaz" demek.

## 4. İşlemci bu çağrıda ne yapıyor?

İşlemcinin içinde register denen birkaç hızlı saklama yeri var. İşlemci hesap yaparken veriyi önce register'a alıyor, işini yapıyor, sonucu belleğe yazıyor. ARM'da (STM32) bir fonksiyonun ilk 4 parametresi sırayla ilk 4 register'a konuyor.

`gps_guncelle(&gps, 473977420, 85455940, 12)` çağrısı adım adım:

| Adım | İşlemci ne yapıyor | Yaklaşık süre |
|---|---|---|
| 1 | 4 parametreyi register'lara koyuyor: 1. register'a `&gps` (`0x20000100`), 2.'ye lat, 3.'ye lon, 4.'ye 12 | her biri ~1–2 tick |
| 2 | Dönüş adresini saklayıp fonksiyona atlıyor | ~2–4 tick |
| 3 | `p->engelli`: `0x20000100 + 9 = 0x20000109` adresinden 1 byte okuyup bir register'a alıyor | ~2 tick (bellekten okuma) |
| 4 | Okunan değer 0 değilse (engelli) fonksiyondan çıkıyor | ~1 tick, atlarsa birkaç tick daha |
| 5 | `p->lat = lat`: lat'ı register'dan `0x20000100` adresine yazıyor | ~1 tick |
| 6 | `p->lon = lon`: `0x20000104` adresine yazıyor | ~1 tick |
| 7 | `p->uydu_sayisi = uydu`: `0x20000108` adresine 1 byte yazıyor | ~1 tick |
| 8 | Saklanan dönüş adresine geri dönüyor | ~2–4 tick |

Toplam 15 tick civarı. Pixhawk 6X'teki gibi 480 MHz'lik bir STM32H7'de bu 0.03 µs ediyor. Süreler, RAM'e beklemesiz erişildiği durum için yaklaşık değerler.

Lat, lon ve uydu ara bir yere kopyalanmıyor; register'da hazır duran değer doğrudan nesnenin içindeki yerine yazılıyor. Her alan için tek bir yazma işlemi var.

## 5. C++ ile

Dosya: `cpp_versiyon.cpp`

```cpp
// 1. ders, C++ hali: veri ve onu kullanan fonksiyonlar aynı class'ın içinde.
#include <cstdio>
#include <cstdint>

class GpsSim
{
public:
    void guncelle(int32_t lat, int32_t lon, uint8_t uydu)
    {
        if (_engelli) {          // derleyici bunu this->_engelli olarak okuyor
            return;
        }
        _lat = lat;              // this->_lat = lat;
        _lon = lon;
        _uydu_sayisi = uydu;
    }

    void engelle(bool engelli)
    {
        this->_engelli = engelli;   // "this" açıkça da yazılabilir
    }

    void yazdir() const
    {
        printf("lat=%.7f lon=%.7f uydu=%d\n", _lat / 1e7, _lon / 1e7, _uydu_sayisi);
    }

    void this_yazdir() const
    {
        printf("this = %p\n", (const void *)this);
    }

private:
    int32_t _lat = 0;
    int32_t _lon = 0;
    uint8_t _uydu_sayisi = 0;
    bool    _engelli = false;
};

int main()
{
    GpsSim gps;

    printf("gps nesnesinin adresi: %p\n", (void *)&gps);
    gps.this_yazdir();
    printf("toplam %zu byte\n", sizeof(GpsSim));

    gps.guncelle(473977420, 85455940, 12);
    gps.yazdir();

    gps.engelle(true);
    gps.guncelle(474000000, 86000000, 10);   // engelli, değişmemeli
    gps.yazdir();

    return 0;
}
```

C sürümünden farkları:

- Veri (`_lat`, `_lon`, ...) ve fonksiyonlar (`guncelle`, `engelle`, `yazdir`) aynı `class GpsSim { ... };` bloğunun içinde.
- Fonksiyonlarda `GpsSim *p` parametresi yok. Fonksiyonun içinde alanlara `p->lat` yerine doğrudan `_lat` diye erişiliyor.
- Çağırırken `gps_guncelle(&gps, ...)` yerine `gps.guncelle(...)` yazılıyor.
- Alanlar `private:` altında, fonksiyonlar `public:` altında (7. bölüm).
- Alanların başlangıç değeri tanımın yanında veriliyor (`int32_t _lat = 0;`). Bunu 2. derste ayrıntılı göreceğim.
- Değişken isimleri `_` ile başlıyor. Bu C++'ın bir kuralı değil, PX4'ün yazım alışkanlığı: üye değişkenleri yerel değişkenlerden ayırmak için.
- C sürümündeki "dışarıdan 200 yazma" satırı yok, çünkü C++'ta derlenmiyor (7. bölüm).

Çıktı (`./calistir.sh 01-class/cpp_versiyon.cpp`):

```
gps nesnesinin adresi: 0x7ffeccae0d6c
this = 0x7ffeccae0d6c
toplam 12 byte
lat=47.3977420 lon=8.5455940 uydu=12
lat=47.3977420 lon=8.5455940 uydu=12
```

- 1. ve 2. satır aynı sayı: `this`, nesnenin adresi (6. bölüm).
- Boyut C ile aynı, 12 byte.
- Son iki satır C ile aynı davranış: engelliyken güncelleme kabul edilmedi.

(Adresler bilgisayardaki stack'ten geliyor ve her çalıştırmada değişiyor; STM32'de `0x2000....` gibi bir RAM adresi olurdu.)

## 6. `this`

C++'ta parametre listesinde nesnenin adresi yazmıyor, ama derleyici onu **gizlice ilk parametre olarak ekliyor**. Bu gizli parametrenin adı `this`:

- `gps.guncelle(473977420, 85455940, 12)` çağrısı, derleyici için `guncelle(&gps, 473977420, 85455940, 12)` demek.
- Fonksiyonun içindeki `_engelli`, derleyici için `this->_engelli` demek. C'deki `p->engelli` ile aynı; `p` yerine `this` var ve yazmak zorunda değilim. `engelle()` fonksiyonunda `this->_engelli` diye açıkça da yazdım, ikisi aynı şey.

Çıktıdaki ilk iki satır bunu gösteriyor: `main`'de yazdırılan `&gps` ile fonksiyonun içinde yazdırılan `this` aynı sayı.

Bu yüzden 4. bölümdeki tablo C++ için de aynen geçerli: adres 1. register'a gidiyor, `_engelli` adres + 9'dan okunuyor, alanlar aynı adreslere yazılıyor. **C++'ın class'ı ekstra tick harcamıyor.** İkisini STM32 için derleyip karşılaştırdım; işlemciye giden komutlar birebir aynı çıktı.

C++'ın gerçekten ekstra tick harcadığı bir yeri ileride göreceğim: `virtual` fonksiyonlar (6. ders). Orada fonksiyonun adresini bulmak için bellekten bir okuma daha yapılıyor.

## 7. public ve private

- `public:` altındakiler dışarıdan çağrılabiliyor.
- `private:` altındakilere sadece class'ın kendi fonksiyonları erişebiliyor.

C'de `gps.uydu_sayisi = 200;` sorunsuz çalışmıştı. C++'ta aynısını denediğim dosya: `hata_ornegi.cpp`

```cpp
// private bir üyeye dışarıdan yazmaya çalışınca derleyici ne diyor?
#include <cstdio>

class GpsSim
{
public:
    void yazdir() const { printf("uydu=%d\n", _uydu_sayisi); }

private:
    int _uydu_sayisi = 0;
};

int main()
{
    GpsSim gps;
    gps._uydu_sayisi = 200;   // hata bekliyoruz
    gps.yazdir();
    return 0;
}
```

Derleyici çıktısı (program hiç oluşmuyor):

```
hata_ornegi.cpp:16:9: error: 'int GpsSim::_uydu_sayisi' is private within this context
   16 |     gps._uydu_sayisi = 200;   // hata bekliyoruz
```

Yani GPS verisi sadece `guncelle()` üzerinden değişebiliyor, ve `guncelle()` engelli durumu kontrol ediyor. C'de bu sadece bir anlaşmaydı, C++'ta kuralı derleyici uyguluyor.

Bu kontrol **sadece derleme sırasında** yapılıyor. Program çalışırken işlemci için public ya da private diye bir şey yok; ikisi de "adres + uzaklık". Private hiç tick harcamıyor, sadece yanlış kodun derlenmesini engelliyor.

## 8. const fonksiyon

`void yazdir() const` sonundaki `const`, bu fonksiyonun üye değişkenleri değiştirmeyeceği anlamına geliyor. C'deki `void gps_yazdir(const GpsSim *p)` ile aynı şey: `yazdir()` içinde `_lat = 0;` yazsaydım derleme hatası alırdım. Bu da sadece derleme sırasında kontrol ediliyor.

## 9. Boyut

İkisi de 12 byte. Fonksiyonlar nesnenin içinde saklanmıyor; flash'ta tek kopya olarak duruyorlar ve hangi nesne üzerinde çalışacaklarını kendilerine verilen adresten (`p` ya da `this`) öğreniyorlar. Class'a fonksiyon eklemek RAM'de yer kaplamıyor. Aynı class'tan 10 nesne oluşturursam RAM'de 10 × 12 byte yer tutuyor, fonksiyonlar flash'ta yine tek kopya.

## PX4'te

PX4'ün GPS simülatörü `src/modules/simulation/sensor_gps_sim/SensorGpsSim.hpp` dosyasında aynı düzende yazılmış (kısaltılmış hali):

```cpp
class SensorGpsSim : public ModuleBase<SensorGpsSim>, public ModuleParams, public px4::ScheduledWorkItem
{
public:
	SensorGpsSim();
	bool init();
	...
private:
	void Run() override;
	uORB::PublicationMulti<sensor_gps_s> _sensor_gps_pub{ORB_ID(sensor_gps)};
	...
};
```

- `public` kısmında dışarıdan çağrılanlar var (`init()`), `private` kısmında iç işler ve `_` ile başlayan üye değişkenler.
- Work queue bu modülün `Run()` fonksiyonunu çağırırken arka planda olan şey bu derstekiyle aynı: nesnenin adresi `this` olarak veriliyor, `Run()` içindeki her `_` ile başlayan değişken "this + uzaklık" adresinden okunuyor.
- Henüz görmediğim kısımlar: `: public ...` kalıtım (5. ders), `override` (6. ders), `<sensor_gps_s>` template (7. ders), `SensorGpsSim();` constructor (2. ders).
