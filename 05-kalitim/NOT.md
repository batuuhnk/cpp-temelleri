# 5. kalıtım

Farklı sensörlerin ortak alanları ve fonksiyonları var: adı, güncelleme hızı, kaç okuma yaptığı. C'de bunları bir "temel" struct'a koyup, o struct'ı her sensör struct'ının ilk alanı olarak gömüyordum. C++'ta bunun karşılığı **kalıtım** (miras). Bu derste ikisini karşılaştırdım; bellekte nasıl durduklarına ve işlemcinin ne yaptığına baktım. Kodların tamamı bu notun içinde.

## Örnek program

- `Sensor`: bütün sensörlerde ortak olan kısım. Adı, hızı (Hz), okuma sayacı; okumayı sayan ve bilgileri yazdıran fonksiyonlar.
- `GpsSim`: Sensor'ın her şeyi + enlem, boylam ve bunları güncelleyen fonksiyon.
- `BaroSim`: Sensor'ın her şeyi + basınç ve onu güncelleyen fonksiyon.
- `main` GPS'i iki kez, barometreyi bir kez günceller. Sonra iki farklı türdeki sensörü **tek bir listede** tutup hepsinin bilgisini yazdırır.

## C ile

Dosya: `c_versiyon.c`

```c
// 5. ders, C hali: ortak alanları bir "temel" struct'a koyup, onu diğer struct'ların ilk alanı olarak gömmek.
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>   // offsetof

// ---- Bütün sensörlerde ortak olan kısım ----
typedef struct {
    const char *ad;
    uint8_t     hz;
    uint32_t    okuma_sayisi;
} Sensor;

void sensor_init(Sensor *s, const char *ad, uint8_t hz)
{
    s->ad = ad;
    s->hz = hz;
    s->okuma_sayisi = 0;
}

void sensor_okuma_say(Sensor *s)
{
    s->okuma_sayisi++;
}

void sensor_bilgi_yazdir(const Sensor *s)
{
    printf("  %-5s %3d Hz, %u okuma\n", s->ad, s->hz, (unsigned)s->okuma_sayisi);
}

// ---- GPS: Sensor + kendi alanları ----
typedef struct {
    Sensor  temel;      // ilk alan olmak zorunda (aşağıdaki listeye bakın)
    int32_t lat;
    int32_t lon;
} GpsSim;

void gps_init(GpsSim *g, uint8_t hz)
{
    sensor_init(&g->temel, "GPS", hz);
    g->lat = 0;
    g->lon = 0;
}

void gps_guncelle(GpsSim *g, int32_t lat, int32_t lon)
{
    g->lat = lat;
    g->lon = lon;
    sensor_okuma_say(&g->temel);
}

// ---- Barometre: Sensor + kendi alanı ----
typedef struct {
    Sensor temel;
    float  basinc_hpa;
} BaroSim;

void baro_init(BaroSim *b, uint8_t hz)
{
    sensor_init(&b->temel, "BARO", hz);
    b->basinc_hpa = 0.0f;
}

void baro_guncelle(BaroSim *b, float basinc)
{
    b->basinc_hpa = basinc;
    sensor_okuma_say(&b->temel);
}

int main(void)
{
    GpsSim  gps;
    BaroSim baro;
    gps_init(&gps, 8);
    baro_init(&baro, 20);

    gps_guncelle(&gps, 473977420, 85455940);
    gps_guncelle(&gps, 473977500, 85456000);
    baro_guncelle(&baro, 1013.2f);

    printf("GpsSim yerlesimi: temel +%zu, lat +%zu, lon +%zu, toplam %zu byte (Sensor kismi %zu byte)\n",
           offsetof(GpsSim, temel), offsetof(GpsSim, lat), offsetof(GpsSim, lon), sizeof(GpsSim), sizeof(Sensor));
    printf("&gps = %p, &gps.temel = %p\n", (void *)&gps, (void *)&gps.temel);

    // Farklı türde sensörleri tek bir listede tutmak: hepsinin başında bir Sensor var
    Sensor *liste[] = { &gps.temel, &baro.temel };
    printf("Butun sensorler:\n");
    for (int i = 0; i < 2; i++) {
        sensor_bilgi_yazdir(liste[i]);
    }
    return 0;
}
```

Ne yapıyor:

- `Sensor` struct'ı ortak alanları tutuyor; `sensor_init`, `sensor_okuma_say`, `sensor_bilgi_yazdir` ortak fonksiyonlar.
- `GpsSim` ve `BaroSim` struct'larının **ilk alanı** bir `Sensor` (`temel`). Kendi alanları onun arkasından geliyor.
- `gps_init` önce `sensor_init(&g->temel, ...)` ile ortak kısmı başlatıyor, sonra kendi alanlarını. `gps_guncelle` de okumayı saymak için `sensor_okuma_say(&g->temel)` çağırıyor.
- `main`'deki `liste`, `Sensor *` tipinde iki adres tutuyor: `&gps.temel` ve `&baro.temel`. Döngü türlerine bakmadan hepsini `sensor_bilgi_yazdir` ile yazdırıyor.

Çıktı (`./calistir.sh 05-kalitim/c_versiyon.c`):

```
GpsSim yerlesimi: temel +0, lat +16, lon +20, toplam 24 byte (Sensor kismi 16 byte)
&gps = 0x7ffe9aab4d80, &gps.temel = 0x7ffe9aab4d80
Butun sensorler:
  GPS     8 Hz, 2 okuma
  BARO   20 Hz, 1 okuma
```

| Çıktı | Hangi koddan geliyor | Anlamı |
|---|---|---|
| `GpsSim yerlesimi: temel +0, lat +16, ...` | `offsetof` ve `sizeof` | Sensor kısmı en başta, GPS'in alanları arkasında (bilgisayarda; STM32 değerleri aşağıda *Bellekte nasıl duruyor?*) |
| `&gps = ...4d80, &gps.temel = ...4d80` | `main` | **Aynı adres**: `temel` ilk alan olduğu için GPS'in adresi aynı zamanda Sensor kısmının adresi |
| `GPS 8 Hz, 2 okuma`, `BARO 20 Hz, 1 okuma` | `liste` döngüsü | İki farklı türdeki sensör, ortak Sensor kısmı üzerinden aynı fonksiyonla yazdırıldı |

`temel`'in ilk alan olması şart değil ama çok işe yarıyor: GPS'in adresi ile Sensor kısmının adresi aynı oluyor, bu yüzden bir GPS'i "Sensor" gibi ele almak için ek bir hesap gerekmiyor.

## C++ ile

Dosya: `cpp_versiyon.cpp`

```cpp
// 5. ders, C++ hali: GpsSim ve BaroSim, Sensor'dan miras alıyor.
#include <cstdio>
#include <cstdint>

// ---- Temel class: bütün sensörlerde ortak olan kısım ----
class Sensor
{
public:
    Sensor(const char *ad, uint8_t hz) : _ad(ad), _hz(hz)
    {
        printf("  Sensor constructor (%s)\n", _ad);
    }

    ~Sensor()
    {
        printf("  Sensor destructor (%s)\n", _ad);
    }

    void bilgi_yazdir() const
    {
        printf("  %-5s %3d Hz, %u okuma\n", _ad, _hz, (unsigned)_okuma_sayisi);
    }

protected:
    // protected: dışarıdan erişilemez, ama bu class'tan miras alanlar erişebilir
    void okuma_say() { _okuma_sayisi++; }

    const char *_ad;
    uint8_t     _hz;
    uint32_t    _okuma_sayisi{0};
};

// ---- GpsSim, Sensor'dan miras alıyor ----
class GpsSim : public Sensor
{
public:
    GpsSim(uint8_t hz) : Sensor("GPS", hz)   // önce temel class'ın constructor'ı
    {
        printf("  GpsSim constructor\n");
    }

    ~GpsSim()
    {
        printf("  GpsSim destructor\n");
    }

    void guncelle(int32_t lat, int32_t lon)
    {
        _lat = lat;
        _lon = lon;
        okuma_say();       // Sensor'dan miras kalan fonksiyon
    }

    void adres_yazdir() const
    {
        printf("this = %p, _lat'in adresi = %p (this + %td)\n",
               (const void *)this, (const void *)&_lat, (const char *)&_lat - (const char *)this);
    }

private:
    int32_t _lat{0};
    int32_t _lon{0};
};

// ---- BaroSim, Sensor'dan miras alıyor ----
class BaroSim : public Sensor
{
public:
    BaroSim(uint8_t hz) : Sensor("BARO", hz) {}

    void guncelle(float basinc)
    {
        _basinc_hpa = basinc;
        okuma_say();
    }

private:
    float _basinc_hpa{0.0f};
};

int main()
{
    printf("Nesneler olusturuluyor:\n");
    GpsSim  gps(8);
    BaroSim baro(20);

    gps.guncelle(473977420, 85455940);
    gps.guncelle(473977500, 85456000);
    baro.guncelle(1013.2f);

    printf("GpsSim boyutu %zu byte, Sensor boyutu %zu byte\n", sizeof(GpsSim), sizeof(Sensor));
    gps.adres_yazdir();

    // GpsSim'in adresini Sensor pointer'ına çevirmek: aynı adres
    Sensor *s = &gps;
    printf("&gps = %p, Sensor olarak = %p\n", (void *)&gps, (void *)s);

    // Farklı türde sensörleri tek bir listede tutmak
    Sensor *liste[] = { &gps, &baro };
    printf("Butun sensorler:\n");
    for (Sensor *sensor : liste) {
        sensor->bilgi_yazdir();
    }

    printf("main bitiyor:\n");
    return 0;
}
```

C sürümünden farkları:

- `class GpsSim : public Sensor` satırı "GpsSim, Sensor'dan miras alıyor" demek. GpsSim'e `temel` diye bir alan eklemiyorum; Sensor'ın bütün alanları ve fonksiyonları GpsSim'in parçası oluyor.
- GpsSim'in constructor'ı başlangıç listesinde Sensor'ın constructor'ını çağırıyor: `GpsSim(uint8_t hz) : Sensor("GPS", hz)`. C'deki `gps_init` içinden `sensor_init` çağırmanın karşılığı.
- `guncelle` içinde `sensor_okuma_say(&g->temel)` yerine sadece `okuma_say()` yazılıyor. Miras kalan fonksiyon, GpsSim'in kendi fonksiyonuymuş gibi çağrılıyor.
- `gps.bilgi_yazdir()` gibi bir çağrı da mümkün; `bilgi_yazdir` Sensor'da tanımlı ama GpsSim'e miras kalmış.
- `Sensor *s = &gps;` C'deki `&gps.temel` yazımı yerine geçiyor; derleyici GpsSim adresini kendiliğinden Sensor adresi olarak kabul ediyor.
- Sensor'ın alanları ve `okuma_say` `protected:` altında (aşağıda *protected*).
- Constructor ve destructor'lara, ne zaman çalıştıklarını görmek için `printf` koydum.

Çıktı (`./calistir.sh 05-kalitim/cpp_versiyon.cpp`):

```
Nesneler olusturuluyor:
  Sensor constructor (GPS)
  GpsSim constructor
  Sensor constructor (BARO)
GpsSim boyutu 24 byte, Sensor boyutu 16 byte
this = 0x7fff1ccc56b0, _lat'in adresi = 0x7fff1ccc56c0 (this + 16)
&gps = 0x7fff1ccc56b0, Sensor olarak = 0x7fff1ccc56b0
Butun sensorler:
  GPS     8 Hz, 2 okuma
  BARO   20 Hz, 1 okuma
main bitiyor:
  Sensor destructor (BARO)
  GpsSim destructor
  Sensor destructor (GPS)
```

| Çıktı | Hangi koddan geliyor | Anlamı |
|---|---|---|
| `Sensor constructor (GPS)` sonra `GpsSim constructor` | `GpsSim gps(8);` | Önce temel class'ın constructor'ı, sonra GpsSim'inki çalıştı |
| `Sensor constructor (BARO)` | `BaroSim baro(20);` | BaroSim'in kendi constructor'ında `printf` yok, sadece Sensor'ınki göründü |
| `GpsSim boyutu 24 byte, Sensor boyutu 16 byte` | `sizeof` | C ile aynı boyutlar |
| `this = ...56b0, _lat'in adresi = ...56c0 (this + 16)` | `gps.adres_yazdir()` | GpsSim'in kendi alanı Sensor kısmının arkasında, C ile aynı yerde |
| `&gps = ...56b0, Sensor olarak = ...56b0` | `Sensor *s = &gps;` | Sensor'a çevirmek adresi değiştirmedi |
| `GPS 8 Hz, 2 okuma`, `BARO 20 Hz, 1 okuma` | `liste` döngüsü | C ile aynı: farklı türler tek listede, ortak fonksiyonla |
| `Sensor destructor (BARO)` | `main`'in sonu | baro yok edildi (BaroSim'in kendi destructor'ı yok, sadece Sensor'ınki) |
| `GpsSim destructor` sonra `Sensor destructor (GPS)` | `main`'in sonu | gps yok edildi: önce GpsSim'in destructor'ı, sonra Sensor'ınki |

## Kalıtım nedir?

`class GpsSim : public Sensor` "her GpsSim aynı zamanda bir Sensor'dır" demek. GpsSim:

- Sensor'ın bütün alanlarına sahip (`_ad`, `_hz`, `_okuma_sayisi`),
- Sensor'ın bütün fonksiyonlarına sahip (`bilgi_yazdir`, `okuma_say`),
- üstüne kendi alanlarını ve fonksiyonlarını ekliyor (`_lat`, `_lon`, `guncelle`).

Sensor'a **temel class** (base class), GpsSim'e **türeyen class** (derived class) deniyor. Ortak kod bir kez yazılıyor, her sensör onu kullanıyor. Sensor'a yeni bir ortak özellik eklersem, bütün sensörlere kendiliğinden geliyor.

`public` kelimesi, Sensor'ın public olan şeylerinin GpsSim'de de public kalmasını sağlıyor; yani dışarıdan `gps.bilgi_yazdir()` çağrılabiliyor. PX4'te hep bu şekil kullanılıyor.

## Bellekte nasıl duruyor?

Bellek yerleşimi C'deki "temel struct'ı ilk alan olarak gömme" ile **birebir aynı**. STM32'de (pointer 4 byte) ölçtüğüm değerler:

```
GpsSim nesnesi, STM32'de 20 byte:

+0    _ad              4 byte  ┐
+4    _hz              1 byte  │ Sensor kısmı (12 byte)
+5    boş              3 byte  │ (hizalama)
+8    _okuma_sayisi    4 byte  ┘
+12   _lat             4 byte  ┐ GpsSim'in kendi alanları
+16   _lon             4 byte  ┘
```

(Programın bilgisayardaki çıktısında 16 ve 24 byte yazıyor, çünkü 64 bitlik bilgisayarda `_ad` pointer'ı 8 byte.)

Sensor kısmı her zaman en başta. Bu yüzden:

- **GpsSim'in adresi = Sensor kısmının adresi.** Çıktıdaki `&gps` ve `Sensor olarak` değerleri bu yüzden aynı.
- Miras kalan bir fonksiyon (`okuma_say`) GpsSim için çağrıldığında, ona verilen `this` GpsSim'in adresi; fonksiyon `this + 8`'de `_okuma_sayisi`'ni buluyor. Sensor için de GpsSim için de aynı uzaklık, aynı fonksiyon.

## protected

1. derste `public` ve `private`'ı görmüştüm. Kalıtımla birlikte üçüncüsü geliyor:

| | Class'ın kendisi | Miras alan class | Dışarısı (`main` gibi) |
|---|---|---|---|
| `public` | erişir | erişir | erişir |
| `protected` | erişir | erişir | **erişemez** |
| `private` | erişir | **erişemez** | **erişemez** |

Sensor'ın alanlarını ve `okuma_say`'i `protected` yaptım: GpsSim ve BaroSim kullanabilsin, ama `main` gibi dışarıdaki kodlar okuma sayacını değiştiremesin.

Dışarıdan erişmeyi denedim: `hata_protected.cpp`

```cpp
// protected bir üyeye dışarıdan erişmek
#include <cstdint>

class Sensor
{
protected:
    uint32_t _okuma_sayisi{0};
};

class GpsSim : public Sensor
{
public:
    void sifirla() { _okuma_sayisi = 0; }   // miras alan class erişebilir
};

int main()
{
    GpsSim gps;
    gps.sifirla();
    gps._okuma_sayisi = 5;   // dışarıdan erişilemez, hata bekliyoruz
    return 0;
}
```

```
hata_protected.cpp:20:9: error: 'uint32_t Sensor::_okuma_sayisi' is protected within this context
hata_protected.cpp:7:14: note: declared protected here
```

`GpsSim::sifirla()` içindeki `_okuma_sayisi = 0;` sorunsuz derlendi; hata sadece `main`'deki satırda. Bu kontrol de `private` gibi sadece derleme sırasında yapılıyor, tick harcamıyor.

## Temel class pointer'ı: farklı türleri tek listede tutmak

`Sensor *liste[] = { &gps, &baro };` satırı iki farklı türdeki nesneyi tek listede tutuyor. Bu, birçok modülü tek bir döngüyle yönetmenin yolu.

Ama bir sınır var: liste elemanı bir `Sensor *`. Derleyici o adreste bir GpsSim mi, BaroSim mi olduğunu bilmiyor; sadece Sensor'da tanımlı olanları çağırmaya izin veriyor. Denedim: `hata_temel_pointer.cpp`

```cpp
// Sensor pointer'ı üzerinden GpsSim'e özel bir fonksiyonu çağırmak
#include <cstdint>

class Sensor
{
public:
    void bilgi_yazdir() const {}
};

class GpsSim : public Sensor
{
public:
    void guncelle(int32_t lat, int32_t lon) { _lat = lat; _lon = lon; }

private:
    int32_t _lat{0};
    int32_t _lon{0};
};

int main()
{
    GpsSim gps;
    Sensor *s = &gps;
    s->bilgi_yazdir();          // olur: Sensor'da var
    s->guncelle(1, 2);          // hata bekliyoruz: Sensor'da guncelle yok
    return 0;
}
```

```
hata_temel_pointer.cpp:25:8: error: 'class Sensor' has no member named 'guncelle'
```

"Sensor'da `guncelle` diye bir şey yok." Gerçekten de yok: GpsSim'in `guncelle(lat, lon)`'u var, BaroSim'in `guncelle(basinc)`'ı var, ama Sensor'ın yok.

Peki listedeki her sensörü, türü ne olursa olsun, **kendi** güncelleme fonksiyonuyla çalıştırmak istersem? Bu 6. dersin (virtual) konusu. PX4'ün work queue'su da tam olarak bu sorunu çözüyor (aşağıda *PX4'te*).

## Constructor ve destructor sırası

Çıktıda görüldüğü gibi:

- **Oluşturulurken:** önce temel class (Sensor), sonra türeyen class (GpsSim). GpsSim'in constructor'ı çalışırken Sensor kısmı hazır olmalı, çünkü GpsSim onu kullanabilir.
- **Yok edilirken:** tersi. Önce GpsSim, sonra Sensor. GpsSim'in destructor'ı çalışırken Sensor kısmı hâlâ duruyor olmalı.

2. dersteki "sonra oluşturulan önce yok edilir" kuralının aynısı.

## İşlemci ne yapıyor?

`gps.guncelle(473977420, 85455940)` çağrısı:

| Adım | İşlemci ne yapıyor | Yaklaşık süre |
|---|---|---|
| 1 | `this` = gps'in adresi, lat ve lon register'lara konup `GpsSim::guncelle`'ye atlanıyor | ~4–6 tick |
| 2 | lat `this + 12`'ye, lon `this + 16`'ya yazılıyor | ~2 tick |
| 3 | `okuma_say()` çağrılıyor. `this` **olduğu gibi** veriliyor, çünkü Sensor kısmı aynı adreste başlıyor | atlama ~2–4 tick |
| 4 | `okuma_say` içinde `this + 8`'deki sayaç okunup 1 eklenip geri yazılıyor | ~3–4 tick |
| 5 | İki fonksiyondan geri dönülüyor | ~4–8 tick |

Kalıtımın kendisi **ekstra tick harcamıyor**:

- GpsSim adresini Sensor adresine çevirmek 0 tick, adres aynı.
- Miras kalan fonksiyonu çağırmak, normal bir fonksiyon çağrısı. C'de `sensor_okuma_say(&g->temel)` yazmakla aynı iş.
- Temel class'ın constructor'ını çağırmak da normal bir fonksiyon çağrısı. C'deki `gps_init` içinden `sensor_init` çağırmakla aynı.
- Derleyici küçük fonksiyonları (`okuma_say` gibi) çağırmak yerine çağrıldığı yere doğrudan yerleştirebiliyor; o zaman 3. ve 5. adımlardaki atlamalar da kalkıyor.

Bir istisna: bir class birden fazla class'tan miras alıyorsa (PX4'te olduğu gibi), sadece ilk temel class en başta durabiliyor. İkinci temel class nesnenin içinde biraz ileride oluyor; onun adresine çevirmek için adrese sabit bir sayı ekleniyor (~1 tick).

## PX4'te

PX4'ün GPS simülatörü üç class'tan miras alıyor (`SensorGpsSim.hpp`):

```cpp
class SensorGpsSim : public ModuleBase<SensorGpsSim>, public ModuleParams, public px4::ScheduledWorkItem
```

| Temel class | Ne veriyor |
|---|---|
| `ModuleBase<SensorGpsSim>` | `start`, `stop`, `status` komutları; 2. dersteki `new` / `delete` işleri burada (`<SensorGpsSim>` kısmı 7. dersin, template'in konusu) |
| `ModuleParams` | Parametreleri okuma: `updateParams()` |
| `px4::ScheduledWorkItem` | Work queue'ya kaydolma ve periyodik çalışma: `ScheduleOnInterval()`, `ScheduleNow()` |

`ScheduledWorkItem`'ın kendisi de `WorkItem`'dan miras alıyor (`ScheduledWorkItem.hpp`: `class ScheduledWorkItem : public WorkItem`). Yani zincir şöyle: `SensorGpsSim` → `ScheduledWorkItem` → `WorkItem`.

Miras kalan fonksiyonun kullanımı, `SensorGpsSim::init()`:

```cpp
bool SensorGpsSim::init()
{
	ScheduleOnInterval(125_ms); // 8 Hz
	return true;
}
```

`ScheduleOnInterval`, SensorGpsSim'de yazılmamış; `ScheduledWorkItem`'dan miras kalmış. Bu dersteki `guncelle` içinden `okuma_say()` çağırmanın aynısı. Temel class constructor'ının başlangıç listesinde çağrılması da 2. dersin (constructor) PX4 kısmında görmüştüm: `ScheduledWorkItem(MODULE_NAME, px4::wq_configurations::hp_default)`.

**Work queue'nun modülleri tutması**, bu dersteki `Sensor *liste[]` ile aynı fikir. `WorkQueue.hpp` içinde:

```cpp
IntrusiveQueue<WorkItem *> _q;
```

Work queue, sırası gelen modülleri `WorkItem *` tipinde adresler olarak bir kuyrukta tutuyor. GPS simülatörü, barometre simülatörü, batarya simülatörü... hepsi bu kuyrukta sadece "bir WorkItem" olarak duruyor. Sırası gelince (`WorkQueue.cpp`):

```cpp
work->Run();
```

`work` bir `WorkItem *`. Ama çalışan şey `WorkItem`'ın değil, `SensorGpsSim`'in kendi `Run()` fonksiyonu. Bu dersteki hata örneğinde `Sensor *` üzerinden GpsSim'e özel bir fonksiyon çağıramamıştım; PX4 bunu nasıl yapıyor? Cevap `WorkItem.hpp`'deki şu satırda: `virtual void Run() = 0;`. Bu `virtual` kelimesi 6. dersin konusu.
