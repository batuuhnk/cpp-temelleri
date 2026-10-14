# 2. constructor ve destructor

C'de bir nesneyi kullanmadan önce `init`, işim bitince `deinit` fonksiyonunu elle çağırıyordum. C++'ta bunların karşılığı constructor ve destructor; farkı, derleyicinin onları benim yerime doğru yerde kendiliğinden çağırması. Bu derste bunu, ve işlemcinin bu sırada bellekte ne yaptığını inceledim. Kodların tamamı bu notun içinde.

## Örnek program

1. dersteki GPS simülatörünü biraz değiştirdim:

- GPS'e bir de güncelleme hızı (`hz`, saniyede kaç konum) ekledim. Bu değer GPS oluşturulurken veriliyor.
- Her GPS açılırken bir kaynak alıyor, kapanırken geri veriyor. Gerçekte bu bir UART portu, DMA kanalı ya da bellek olurdu; burada sadece bir sayaçla (`acik_gps_sayisi`) temsil ediyorum. Açılınca sayaç 1 artıyor, kapanınca 1 azalıyor. Her şey düzgün kapanırsa sayaç sonunda 0 olmalı.
- `ucus()` fonksiyonu içinde iki GPS kullanılıyor: ana (8 Hz) ve yedek (5 Hz).
- Son olarak bir GPS de heap'te (`malloc` / `new` ile) oluşturulup siliniyor.

## C ile

Dosya: `c_versiyon.c`

```c
// 2. ders, C hali: nesneyi kullanmadan önce init, işi bitince deinit elle çağrılmalı.
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>   // malloc, free

// GPS'e ayrılmış kaynak sayısı (gerçekte UART portu, DMA kanalı, bellek gibi bir şey olurdu)
static int acik_gps_sayisi = 0;

typedef struct {
    int32_t lat;
    int32_t lon;
    uint8_t uydu_sayisi;
    uint8_t hz;           // saniyede kaç konum verecek
    bool    engelli;
} GpsSim;

void gps_init(GpsSim *p, uint8_t hz)
{
    p->lat = 0;
    p->lon = 0;
    p->uydu_sayisi = 0;
    p->hz = hz;
    p->engelli = false;
    acik_gps_sayisi++;
    printf("gps_init:   %d Hz GPS acildi,  acik GPS sayisi = %d\n", hz, acik_gps_sayisi);
}

void gps_deinit(GpsSim *p)
{
    acik_gps_sayisi--;
    printf("gps_deinit: %d Hz GPS kapandi, acik GPS sayisi = %d\n", p->hz, acik_gps_sayisi);
}

void gps_yazdir(const GpsSim *p)
{
    printf("GPS: %d Hz, uydu=%d\n", p->hz, p->uydu_sayisi);
}

void ucus(void)
{
    GpsSim ana;
    GpsSim yedek;

    gps_init(&ana, 8);
    gps_init(&yedek, 5);

    gps_yazdir(&ana);
    gps_yazdir(&yedek);

    gps_deinit(&ana);
    // gps_deinit(&yedek);   // unutuldu
}

int main(void)
{
    printf("ucus basliyor\n");
    ucus();
    printf("ucus bitti, acik GPS sayisi = %d\n", acik_gps_sayisi);

    // Heap'te oluşturmak: bellek ayır, sonra init. Silerken önce deinit, sonra bellek geri.
    printf("heap ornegi basliyor\n");
    GpsSim *g = malloc(sizeof(GpsSim));
    gps_init(g, 10);
    gps_yazdir(g);
    gps_deinit(g);
    free(g);
    printf("heap ornegi bitti, acik GPS sayisi = %d\n", acik_gps_sayisi);
    return 0;
}
```

Ne yapıyor:

- `gps_init` alanlara başlangıç değerlerini yazıyor ve sayacı artırıyor. `gps_deinit` sayacı azaltıyor.
- `ucus()` içinde iki GPS oluşturuluyor, ikisi de `gps_init` ile açılıyor. Kapatırken `ana` için `gps_deinit` çağrılmış, `yedek` için **unutulmuş** (yorum satırı olarak bıraktım).
- `main` en sonda bir GPS'i heap'te oluşturuyor: önce `malloc` ile bellek, sonra `gps_init`. Silerken önce `gps_deinit`, sonra `free`. Burada sıra önemli; ikisini de elle doğru sırada yazmam gerekiyor.

Çıktı (`./calistir.sh 02-constructor/c_versiyon.c`):

```
ucus basliyor
gps_init:   8 Hz GPS acildi,  acik GPS sayisi = 1
gps_init:   5 Hz GPS acildi,  acik GPS sayisi = 2
GPS: 8 Hz, uydu=0
GPS: 5 Hz, uydu=0
gps_deinit: 8 Hz GPS kapandi, acik GPS sayisi = 1
ucus bitti, acik GPS sayisi = 1
heap ornegi basliyor
gps_init:   10 Hz GPS acildi,  acik GPS sayisi = 2
GPS: 10 Hz, uydu=0
gps_deinit: 10 Hz GPS kapandi, acik GPS sayisi = 1
heap ornegi bitti, acik GPS sayisi = 1
```

| Çıktı | Hangi koddan geliyor | Anlamı |
|---|---|---|
| `gps_init: 8 Hz ... = 1` ve `gps_init: 5 Hz ... = 2` | iki `gps_init` çağrısı | İki GPS açıldı, iki kaynak alındı |
| `GPS: 8 Hz, uydu=0`, `GPS: 5 Hz, uydu=0` | `gps_yazdir` | Başlangıç değerleri doğru yazılmış |
| `gps_deinit: 8 Hz ... = 1` | `gps_deinit(&ana)` | Sadece ana kapandı |
| `ucus bitti, acik GPS sayisi = 1` | `main` | **Yedek GPS'in kaynağı hiç geri verilmedi** |
| `heap ornegi ...` ile başlayan kısım | `malloc` + `gps_init`, `gps_deinit` + `free` | Heap'teki GPS düzgün açılıp kapandı, ama sayaç yine 1'de bitiyor: yedek'ten kalan kaynak hâlâ açık |

C'de iki tür hata yapılabiliyor:

- **deinit'i unutmak:** Burada olan bu. Kaynak geri verilmiyor (kaynak sızıntısı). Aylarca çalışan gömülü bir cihazda bu, bir süre sonra portların ya da belleğin bitmesi demek.
- **init'i unutmak:** `GpsSim ana;` satırından sonra `gps_init` çağrılmasaydı, alanlarda o bellek adresinde daha önceden ne kaldıysa o olurdu (aşağıda *Nesne bellekte nerede oluşuyor?*). Bunu programda denemedim, çünkü sonucu her çalıştırmada farklı ve tanımsız.

## Nesne bellekte nerede oluşuyor?

Struct'ın yerleşimi (1. dersteki gibi `offsetof` ile ölçtüm): `lat +0, lon +4, uydu_sayisi +8, hz +9, engelli +10, toplam 12 byte`.

`ucus()` içindeki `GpsSim ana;` ve `GpsSim yedek;` yerel değişkenler, yani **stack**'te duruyorlar:

- Fonksiyon başlarken işlemci stack pointer'ı (stack'in nerede bittiğini tutan register) tüm yerel değişkenlerin toplam boyutu kadar aşağı kaydırıyor. İki GPS için 24 byte. Bu tek bir işlem, yaklaşık 1 tick, ve kaç değişken olursa olsun bir kez yapılıyor.
- Bu 24 byte'a **hiçbir şey yazılmıyor**. İçinde o adreste daha önce çalışmış başka bir fonksiyondan kalan değerler duruyor. Değerleri anlamlı hale getiren tek şey `gps_init`; unutulursa çöp kalıyor.
- Fonksiyon biterken stack pointer geri kaydırılıyor (yine ~1 tick). Bellek "geri verilmiş" oluyor ama silinmiyor. `gps_deinit` çağrılmamışsa sayaç gibi dış kaynaklar da açık kalıyor.

Heap'te (`malloc`) durum farklı: `malloc` heap'te uygun büyüklükte boş bir yer arıyor. Bu arama heap'in ne kadar dolu olduğuna göre onlarca ile yüzlerce tick arası sürebiliyor ve süresi önceden kesin bilinemiyor. Bu yüzden gömülü sistemlerde heap'ten bellek genelde sadece açılışta bir kez alınıyor, zamanlaması önemli döngülerin içinde alınmıyor.

## C++ ile

Dosya: `cpp_versiyon.cpp`

```cpp
// 2. ders, C++ hali: constructor nesne oluşurken, destructor nesne yok olurken kendiliğinden çağrılıyor.
#include <cstdio>
#include <cstdint>

static int acik_gps_sayisi = 0;

class GpsSim
{
public:
    // Constructor: adı class'la aynı, dönüş tipi yok.
    // ": _hz(hz)" kısmı başlangıç listesi: _hz alanına hz değerini yazıyor.
    GpsSim(uint8_t hz) : _hz(hz)
    {
        acik_gps_sayisi++;
        printf("constructor: %d Hz GPS acildi,  acik GPS sayisi = %d\n", _hz, acik_gps_sayisi);
    }

    // Destructor: adının başında ~ var, parametresi yok.
    ~GpsSim()
    {
        acik_gps_sayisi--;
        printf("destructor:  %d Hz GPS kapandi, acik GPS sayisi = %d\n", _hz, acik_gps_sayisi);
    }

    void yazdir() const
    {
        printf("GPS: %d Hz, uydu=%d\n", _hz, _uydu_sayisi);
    }

private:
    int32_t _lat{0};          // {0} ile "= 0" aynı şey; PX4 genelde {} kullanıyor
    int32_t _lon{0};
    uint8_t _uydu_sayisi{0};
    uint8_t _hz;              // değerini constructor veriyor
    bool    _engelli{false};
};

void ucus()
{
    GpsSim ana(8);            // burada ana'nın constructor'ı çağrılıyor
    GpsSim yedek(5);          // burada yedek'in constructor'ı çağrılıyor

    ana.yazdir();
    yedek.yazdir();

    printf("ucus fonksiyonunun sonu\n");
}                             // burada önce yedek'in, sonra ana'nın destructor'ı çağrılıyor

int main()
{
    printf("ucus basliyor\n");
    ucus();
    printf("ucus bitti, acik GPS sayisi = %d\n", acik_gps_sayisi);

    // Heap'te oluşturmak: new = bellek ayır + constructor, delete = destructor + belleği geri ver.
    // PX4'te "sensor_gps_sim start" new ile, "sensor_gps_sim stop" delete ile çalışıyor.
    printf("heap ornegi basliyor\n");
    GpsSim *g = new GpsSim(10);
    g->yazdir();
    delete g;
    printf("heap ornegi bitti, acik GPS sayisi = %d\n", acik_gps_sayisi);
    return 0;
}
```

C sürümünden farkları:

- `gps_init` yerine **constructor** var: `GpsSim(uint8_t hz)`. Adı class'ın adıyla aynı ve dönüş tipi yok.
- `gps_deinit` yerine **destructor** var: `~GpsSim()`. Adının başında `~` var, parametre almıyor.
- `ucus()` içinde hiçbir yerde init ya da deinit çağrılmıyor. `GpsSim ana(8);` yazmak hem yer ayırıyor hem constructor'ı çağırıyor; fonksiyonu kapatan süslü parantez destructor'ları çağırıyor.
- Alanların başlangıç değerleri tanımda: `int32_t _lat{0};`. `{0}` ile `= 0` aynı şey; PX4 kodunda genelde süslü parantezli yazım kullanılıyor.
- Heap'te `malloc` + `gps_init` yerine `new GpsSim(10)`, `gps_deinit` + `free` yerine `delete g` var.
- Heap'teki nesnenin fonksiyonu `g->yazdir()` ile çağrılıyor. `g` bir adres (pointer) olduğu için `.` yerine `->` kullanılıyor; C'deki `p->alan` ile aynı mantık.

Çıktı (`./calistir.sh 02-constructor/cpp_versiyon.cpp`):

```
ucus basliyor
constructor: 8 Hz GPS acildi,  acik GPS sayisi = 1
constructor: 5 Hz GPS acildi,  acik GPS sayisi = 2
GPS: 8 Hz, uydu=0
GPS: 5 Hz, uydu=0
ucus fonksiyonunun sonu
destructor:  5 Hz GPS kapandi, acik GPS sayisi = 1
destructor:  8 Hz GPS kapandi, acik GPS sayisi = 0
ucus bitti, acik GPS sayisi = 0
heap ornegi basliyor
constructor: 10 Hz GPS acildi,  acik GPS sayisi = 1
GPS: 10 Hz, uydu=0
destructor:  10 Hz GPS kapandi, acik GPS sayisi = 0
heap ornegi bitti, acik GPS sayisi = 0
```

| Çıktı | Hangi koddan geliyor | Anlamı |
|---|---|---|
| `constructor: 8 Hz ...`, `constructor: 5 Hz ...` | `GpsSim ana(8);` ve `GpsSim yedek(5);` | Nesne tanımlandığı anda constructor çalıştı |
| `ucus fonksiyonunun sonu` | `ucus()`'taki son `printf` | Destructor'lar bundan **sonra** geliyor |
| `destructor: 5 Hz ...` sonra `destructor: 8 Hz ...` | `ucus()`'u kapatan süslü parantez | İki nesne için de destructor kendiliğinden çağrıldı, **ters sırayla**: önce yedek, sonra ana |
| `ucus bitti, acik GPS sayisi = 0` | `main` | Hiçbir kaynak açık kalmadı |
| `heap ornegi ...` ile başlayan kısım | `new GpsSim(10)` ve `delete g` | `new` constructor'ı, `delete` destructor'ı çağırdı |

## Constructor

- Nesne oluşturulduğu anda çağrılıyor. Kod yazarken bunu atlamanın yolu yok.
- `GpsSim(uint8_t hz) : _hz(hz)` içindeki `: _hz(hz)` kısmına **başlangıç listesi** deniyor. Constructor'ın gövdesi çalışmadan önce alanlara değer yazıyor. Bu örnekte gövdede `_hz = hz;` yazmakla aynı sonucu veriyor. PX4'te bu listede, aşağıda göreceğim gibi, miras alınan class'ların constructor'ları da çağrılıyor.
- Tanımda başlangıç değeri olan alanlar (`_lat{0}` gibi) da constructor'ın başında yazılıyor.
- Constructor bir parametre istiyorsa, nesneyi parametresiz oluşturmak mümkün değil. Bu, C'deki "init'i unutmak" hatasını derleme aşamasında engelliyor (aşağıda *Hata örneği*).

## Destructor

- Nesnenin ömrü bittiği anda çağrılıyor:
  - Yerel değişken için: tanımlandığı bloğu kapatan süslü parantezde.
  - `new` ile oluşturulan için: `delete` yazıldığında.
- Fonksiyondan erken çıkılsa bile (örneğin ortada bir `return` olsa) derleyici destructor çağrısını o çıkışa da ekliyor. C'de her `return`'den önce `deinit` yazmayı hatırlamam gerekirdi.
- Birden fazla nesne varsa, **oluşturulma sırasının tersiyle** yok ediliyor: önce yedek, sonra ana. Sebebi: sonra oluşturulan bir nesne, önce oluşturulana bağlı olabilir (örneğin yedek GPS, ana GPS'in açtığı bir portu kullanıyor olabilir). Önce ona bağlı olan kapanmalı.

## İşlemci ne yapıyor?

C++ sürümündeki `ucus()` fonksiyonu adım adım:

| Adım | İşlemci ne yapıyor | Yaklaşık süre |
|---|---|---|
| 1 | Stack pointer'ı 24 byte aşağı kaydırıyor: ana ve yedek için yer ayrıldı, içleri henüz çöp | ~1 tick |
| 2 | `GpsSim ana(8)`: constructor'ı normal bir fonksiyon gibi çağırıyor. 1. register'a `this` = ana'nın stack'teki adresi, 2. register'a 8 | ~1–2 tick + atlama ~2–4 tick |
| 3 | Constructor içinde: `this+0`'a 0 (lat), `this+4`'e 0 (lon), `this+8`'e 0 (uydu), `this+9`'a 8 (hz), `this+10`'a 0 (engelli) yazıyor | her biri ~1 tick |
| 4 | `acik_gps_sayisi++`: global sayacı RAM'den okuyor, 1 ekliyor, geri yazıyor | ~3–4 tick |
| 5 | Aynısı `yedek` için (`this` = yedek'in adresi, hz = 5) | 2–4. adımlar kadar |
| 6 | `yazdir()` çağrıları ve son `printf` | (`printf` yüzlerce tick; sadece göstermek için var) |
| 7 | Fonksiyonun sonuna gelince yedek'in destructor'ı (`this` = yedek): sayaç okunup 1 azaltılıp yazılıyor | çağrı + ~3–4 tick |
| 8 | ana'nın destructor'ı, aynı şekilde | çağrı + ~3–4 tick |
| 9 | Stack pointer geri kaydırılıyor, fonksiyondan dönülüyor | ~1 tick + dönüş |

Önemli olan: derleyici constructor ve destructor çağrılarını tam olarak benim C'de `gps_init` ve `gps_deinit` yazdığım yerlere koyuyor. İşlemci aynı işi yapıyor, **ekstra tick yok**. Fark sadece çağrıyı unutma ihtimalinin ortadan kalkması.

`new` ve `delete` için de aynısı geçerli. `new GpsSim(10)` = heap'ten 12 byte iste (`malloc` gibi, süresi belirsiz) + constructor'ı çağır. `delete g` = destructor'ı çağır + belleği geri ver (`free` gibi). Ek maliyet C++'tan değil, heap'in kendisinden geliyor.

## Hata örneği

Constructor parametre istiyorsa nesneyi parametresiz oluşturmayı denedim: `hata_ornegi.cpp`

```cpp
// Constructor bir parametre istiyorsa, parametresiz nesne oluşturulabilir mi?
#include <cstdint>

class GpsSim
{
public:
    GpsSim(uint8_t hz) : _hz(hz) {}

private:
    uint8_t _hz;
};

int main()
{
    GpsSim gps;   // hz verilmedi, hata bekliyoruz
    return 0;
}
```

Derleyici çıktısı (program hiç oluşmuyor; `15:12`, dosyanın 15. satırının 12. karakteri demek):

```
hata_ornegi.cpp:15:12: error: no matching function for call to 'GpsSim::GpsSim()'
   15 |     GpsSim gps;   // hz verilmedi, hata bekliyoruz
hata_ornegi.cpp:7:5: note: candidate: 'GpsSim::GpsSim(uint8_t)'
hata_ornegi.cpp:7:5: note:   candidate expects 1 argument, 0 provided
```

Derleyici "parametresiz bir constructor yok; bulduğum constructor 1 parametre istiyor, sen 0 verdin" diyor. Yani `hz` belirtmeden GPS nesnesi oluşturmak mümkün değil. C'de `GpsSim gps;` yazıp `gps_init`'i unutabiliyordum; burada değerleri verilmemiş bir nesne hiç oluşamıyor.

(Derleyicinin tam çıktısında bir de "copy constructor" adlı ikinci bir aday geçiyor: bir nesneyi başka bir nesneden kopyalayarak oluşturmak için. Şimdilik konum değil.)

## PX4'te

PX4'ün GPS simülatörü (`src/modules/simulation/sensor_gps_sim/`) bu dersteki her şeyi kullanıyor.

Constructor (`SensorGpsSim.cpp`):

```cpp
SensorGpsSim::SensorGpsSim() :
	ModuleParams(nullptr),
	ScheduledWorkItem(MODULE_NAME, px4::wq_configurations::hp_default)
{
}
```

Gövdesi boş; bütün iş başlangıç listesinde. Burada alanlara değer yazmak yerine, miras aldığı class'ların constructor'ları çağrılıyor. Örneğin `ScheduledWorkItem(...)` bu modülü `wq:hp_default` work queue thread'ine kaydediyor. (Miras, 5. dersin yani kalıtımın konusu.)

Kaynak alma ve geri verme (`SensorGpsSim.hpp` ve `.cpp`):

```cpp
// .hpp: alanın başlangıç değeri; constructor çalışırken bir ölçüm sayacı ayırıyor
perf_counter_t _loop_perf{perf_alloc(PC_ELAPSED, MODULE_NAME": cycle")};

// .cpp: destructor; nesne silinirken o sayacı geri veriyor
SensorGpsSim::~SensorGpsSim()
{
	perf_free(_loop_perf);
}
```

Bu dersteki `acik_gps_sayisi++` / `acik_gps_sayisi--` düzeninin gerçek hali: oluşurken kaynak al, yok olurken geri ver.

Modülün oluşturulması ve silinmesi (`SensorGpsSim::task_spawn()` ve PX4'ün modül altyapısı `module.h`):

```cpp
SensorGpsSim *instance = new SensorGpsSim();   // "sensor_gps_sim start" komutu
...
delete _object.load();                          // "sensor_gps_sim stop" komutu
```

- `pxh>` konsolunda `sensor_gps_sim start` yazılınca nesne `new` ile heap'te oluşturuluyor: constructor çalışıyor, modül work queue'ya kaydoluyor, ölçüm sayacı ayrılıyor.
- `sensor_gps_sim stop` yazılınca nesne `delete` ile siliniyor: destructor çalışıyor, sayaç geri veriliyor.
- PX4 çalışmamdaki GPS kaybı deneyinde GPS'i `sensor_gps_sim stop` ile kesmiştim. Yani orada aslında bu dersteki destructor'ı çalıştırmışım.
- `new` sadece modül başlatılırken bir kez kullanılıyor. GPS verisini her 125 ms'de bir üreten `Run()` fonksiyonunun içinde heap'ten bellek alınmıyor; yukarıda *Nesne bellekte nerede oluşuyor?* kısmında yazdığım, süresi belirsiz heap işlemlerini döngüden uzak tutma kuralı.
