# 6. virtual ve override

5. derste (kalıtım) farklı türdeki sensörleri `Sensor *` listesinde tutabilmiştim, ama listedeki bir elemanın türüne özel fonksiyonunu çağıramamıştım. PX4'ün work queue'su ise tam olarak bunu yapıyor: bütün modülleri `WorkItem *` olarak tutuyor ve her birinin **kendi** `Run()` fonksiyonunu çağırıyor. Bunu sağlayan şey `virtual`. Bu derste C'de fonksiyon pointer tablosuyla elle yaptığım şeyi, C++'ın `virtual` ile nasıl kendiliğinden yaptığını inceledim. C++'ın gerçekten ekstra tick ve bellek harcadığı ilk yer de burası. Kodların tamamı bu notun içinde.

## Örnek program

PX4'ün work queue'sunun küçük bir kopyası:

- `WorkItem`: bütün işlerin ortak temeli. Adı ve kaç kez çalıştığı sayacı var. Her iş bir `Run()` fonksiyonuna sahip olmalı, ama WorkItem'ın kendisi ne yapacağını bilmiyor.
- `GpsSim`: her çalıştığında enlemi biraz değiştiriyor.
- `BaroSim`: her çalıştığında basıncı biraz düşürüyor.
- Bir kuyruk dizisi, işleri sadece "WorkItem" olarak tutuyor. Bir döngü kuyruktaki her işin `Run()`'ını çağırıyor; hangisinin GPS hangisinin barometre olduğunu bilmiyor.
- Son olarak heap'te bir GPS oluşturulup, temel pointer üzerinden siliniyor.

## C ile

Dosya: `c_versiyon.c`

```c
// 6. ders, C hali: her türün fonksiyonlarını bir tabloda tutmak ve nesneye o tablonun adresini koymak.
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct WorkItem WorkItem;

// Fonksiyon tablosu: her tür (GPS, baro) için bir tane, flash'ta sabit duruyor
typedef struct {
    void (*run)(WorkItem *self);
    void (*yok_et)(WorkItem *self);
} WorkItemTablo;

// Temel struct: ilk alanı tablonun adresi
struct WorkItem {
    const WorkItemTablo *tablo;
    const char          *ad;
    uint32_t             calisma_sayisi;
};

// ---- GPS ----
typedef struct {
    WorkItem temel;
    int32_t  lat;
} GpsSim;

static void gps_run(WorkItem *self)
{
    GpsSim *g = (GpsSim *)self;          // temel ilk alan olduğu için adres aynı
    g->temel.calisma_sayisi++;
    g->lat += 10;
    printf("  gps_run:  %u. calisma, lat=%d\n", (unsigned)g->temel.calisma_sayisi, (int)g->lat);
}

static void gps_yok_et(WorkItem *self)
{
    printf("  gps_yok_et (%s)\n", self->ad);
}

static const WorkItemTablo gps_tablosu = { gps_run, gps_yok_et };

void gps_init(GpsSim *g)
{
    g->temel.tablo = &gps_tablosu;
    g->temel.ad = "GPS";
    g->temel.calisma_sayisi = 0;
    g->lat = 473977420;
}

// ---- Barometre ----
typedef struct {
    WorkItem temel;
    float    basinc_hpa;
} BaroSim;

static void baro_run(WorkItem *self)
{
    BaroSim *b = (BaroSim *)self;
    b->temel.calisma_sayisi++;
    b->basinc_hpa -= 0.1f;
    printf("  baro_run: %u. calisma, basinc=%.1f\n", (unsigned)b->temel.calisma_sayisi, b->basinc_hpa);
}

static void baro_yok_et(WorkItem *self)
{
    printf("  baro_yok_et (%s)\n", self->ad);
}

static const WorkItemTablo baro_tablosu = { baro_run, baro_yok_et };

void baro_init(BaroSim *b)
{
    b->temel.tablo = &baro_tablosu;
    b->temel.ad = "BARO";
    b->temel.calisma_sayisi = 0;
    b->basinc_hpa = 1013.2f;
}

int main(void)
{
    GpsSim  gps;
    BaroSim baro;
    gps_init(&gps);
    baro_init(&baro);

    // Kuyruk: hepsi sadece "WorkItem"; hangisinin GPS hangisinin baro olduğunu döngü bilmiyor
    WorkItem *kuyruk[] = { &gps.temel, &baro.temel, &gps.temel };
    printf("Kuyruk calisiyor:\n");
    for (int i = 0; i < 3; i++) {
        WorkItem *w = kuyruk[i];
        w->tablo->run(w);                 // tablodan fonksiyonun adresini al, onu çağır
    }

    printf("gps'in tablosu  %p\n", (const void *)gps.temel.tablo);
    printf("baro'nun tablosu %p\n", (const void *)baro.temel.tablo);

    // Heap'teki bir nesneyi temel pointer üzerinden silmek
    printf("Heap ornegi:\n");
    GpsSim *yeni = malloc(sizeof(GpsSim));
    gps_init(yeni);
    WorkItem *w = &yeni->temel;
    w->tablo->yok_et(w);
    free(yeni);
    return 0;
}
```

Ne yapıyor:

- `WorkItemTablo` iki fonksiyon pointer'ı tutan bir struct: `run` ve `yok_et`. Her tür için bir tane var (`gps_tablosu`, `baro_tablosu`). `static const` oldukları için değişmiyorlar ve flash'ta duruyorlar.
- `WorkItem`'ın **ilk alanı** o tablonun adresi (`tablo`). `gps_init` bu alana `&gps_tablosu`, `baro_init` ise `&baro_tablosu` yazıyor. Böylece her nesne kendi türünün fonksiyonlarının nerede olduğunu biliyor.
- `gps_run(WorkItem *self)` gelen adresi `GpsSim *`'e çeviriyor. `temel` ilk alan olduğu için adres aynı (5. derste, kalıtımda görmüştüm).
- Kuyruk döngüsü `w->tablo->run(w)` diyor: "w'nin tablosuna bak, oradaki `run` adresini al, o fonksiyonu w ile çağır". GPS için `gps_run`, barometre için `baro_run` çalışıyor.

Çıktı (`./calistir.sh 06-virtual/c_versiyon.c`):

```
Kuyruk calisiyor:
  gps_run:  1. calisma, lat=473977430
  baro_run: 1. calisma, basinc=1013.1
  gps_run:  2. calisma, lat=473977440
gps'in tablosu  0x5b0cf81bbd80
baro'nun tablosu 0x5b0cf81bbd90
Heap ornegi:
  gps_yok_et (GPS)
```

| Çıktı | Hangi koddan geliyor | Anlamı |
|---|---|---|
| `gps_run: 1. calisma`, `baro_run: 1. calisma`, `gps_run: 2. calisma` | kuyruk döngüsündeki `w->tablo->run(w)` | Aynı satır, her eleman için o elemanın kendi fonksiyonunu çalıştırdı |
| `gps'in tablosu ...bd80`, `baro'nun tablosu ...bd90` | `printf` | İki tür, iki ayrı tablo |
| `gps_yok_et (GPS)` | `w->tablo->yok_et(w)` | Heap'teki GPS, temel pointer üzerinden doğru fonksiyonla kapatıldı |

Bu yöntem çalışıyor ama her şeyi elle yapıyorum: tabloyu yazmak, her init'te tabloyu bağlamak, `run` fonksiyonlarında adresi geri çevirmek. Birini unutursam (örneğin `tablo` alanını yazmayı) program yanlış bir adrese atlayıp çöküyor.

## C++ ile

Dosya: `cpp_versiyon.cpp`

```cpp
// 6. ders, C++ hali: virtual fonksiyonlar. Tabloyu derleyici kendisi oluşturuyor.
#include <cstdio>
#include <cstdint>

class WorkItem
{
public:
    WorkItem(const char *ad) : _ad(ad) {}

    virtual ~WorkItem()
    {
        printf("  ~WorkItem (%s)\n", _ad);
    }

    // "= 0": WorkItem'ın kendi Run'ı yok, miras alan her class yazmak zorunda
    virtual void Run() = 0;

protected:
    const char *_ad;
    uint32_t    _calisma_sayisi{0};
};

class GpsSim : public WorkItem
{
public:
    GpsSim() : WorkItem("GPS") {}

    ~GpsSim() override
    {
        printf("  ~GpsSim\n");
    }

    void Run() override
    {
        _calisma_sayisi++;
        _lat += 10;
        printf("  GpsSim::Run:  %u. calisma, lat=%d\n", (unsigned)_calisma_sayisi, (int)_lat);
    }

private:
    int32_t _lat{473977420};
};

class BaroSim : public WorkItem
{
public:
    BaroSim() : WorkItem("BARO") {}

    void Run() override
    {
        _calisma_sayisi++;
        _basinc_hpa -= 0.1f;
        printf("  BaroSim::Run: %u. calisma, basinc=%.1f\n", (unsigned)_calisma_sayisi, _basinc_hpa);
    }

private:
    float _basinc_hpa{1013.2f};
};

int main()
{
    GpsSim  gps;
    GpsSim  gps2;
    BaroSim baro;

    // Kuyruk: PX4'teki IntrusiveQueue<WorkItem *> gibi
    WorkItem *kuyruk[] = { &gps, &baro, &gps };
    printf("Kuyruk calisiyor:\n");
    for (WorkItem *work : kuyruk) {
        work->Run();                      // PX4'teki WorkQueue.cpp: work->Run();
    }

    // Nesnenin ilk 8 byte'ı (bu bilgisayarda bir adres): derleyicinin gizlice koyduğu tablo adresi
    printf("gps'in tablosu   %p\n", *(void **)&gps);
    printf("gps2'nin tablosu %p\n", *(void **)&gps2);
    printf("baro'nun tablosu %p\n", *(void **)&baro);
    printf("sizeof(WorkItem) = %zu, sizeof(GpsSim) = %zu\n", sizeof(WorkItem), sizeof(GpsSim));

    // Heap'teki bir nesneyi temel pointer üzerinden silmek
    printf("Heap ornegi:\n");
    WorkItem *w = new GpsSim();
    delete w;

    printf("main bitiyor:\n");
    return 0;
}
```

C sürümünden farkları:

- `WorkItemTablo` diye bir şey yok, `tablo` alanı yok, init'te tabloyu bağlamak yok. Bunların hepsini derleyici yapıyor.
- `virtual void Run() = 0;` WorkItem'da "her işin bir Run'ı var, hangisinin çalışacağı nesnenin gerçek türüne göre belirlensin" demek. Sonundaki `= 0`, "WorkItem'ın kendi Run'ı yok; miras alan her class yazmak zorunda" demek.
- GpsSim ve BaroSim'de `void Run() override` yazılıyor. `override`, "temel class'taki virtual Run'ın yerine bunu koyuyorum" demek.
- `GpsSim *g = (GpsSim *)self;` gibi elle adres çevirme yok. Fonksiyonun içinde `this` zaten doğru türde.
- Kuyruk döngüsü sadece `work->Run();` diyor. PX4'ün `WorkQueue.cpp`'sindeki satırın aynısı.
- Destructor da `virtual`: `virtual ~WorkItem()`. GpsSim'de `~GpsSim() override`.
- `*(void **)&gps` satırı nesnenin ilk alanını okuyup yazdırıyor. Derleyicinin gizlice koyduğu tablo adresini görmek için; normal kodda böyle bir şey yazılmıyor.

Çıktı (`./calistir.sh 06-virtual/cpp_versiyon.cpp`):

```
Kuyruk calisiyor:
  GpsSim::Run:  1. calisma, lat=473977430
  BaroSim::Run: 1. calisma, basinc=1013.1
  GpsSim::Run:  2. calisma, lat=473977440
gps'in tablosu   0x60eb7cc4bd00
gps2'nin tablosu 0x60eb7cc4bd00
baro'nun tablosu 0x60eb7cc4bcd8
sizeof(WorkItem) = 24, sizeof(GpsSim) = 24
Heap ornegi:
  ~GpsSim
  ~WorkItem (GPS)
main bitiyor:
  ~WorkItem (BARO)
  ~GpsSim
  ~WorkItem (GPS)
  ~GpsSim
  ~WorkItem (GPS)
```

| Çıktı | Hangi koddan geliyor | Anlamı |
|---|---|---|
| `GpsSim::Run`, `BaroSim::Run`, `GpsSim::Run` | `work->Run();` | `work` bir `WorkItem *`, ama her seferinde nesnenin gerçek türünün Run'ı çalıştı. C'dekiyle aynı sonuç |
| `gps'in tablosu ...bd00`, `gps2'nin tablosu ...bd00` | nesnelerin ilk alanı | **Aynı adres**: GpsSim türünün tek bir tablosu var, bütün GpsSim nesneleri onu gösteriyor |
| `baro'nun tablosu ...bcd8` | baro'nun ilk alanı | BaroSim'in ayrı bir tablosu var |
| `sizeof(WorkItem) = 24, ...` | `sizeof` | Tablo adresi nesneye yer ekliyor (bu bilgisayarda; STM32 değerleri aşağıda *Bellekte ne değişiyor?*) |
| `~GpsSim` sonra `~WorkItem (GPS)` | `delete w;` | `w` bir `WorkItem *` olduğu halde önce GpsSim'in, sonra WorkItem'ın destructor'ı çalıştı (aşağıda *virtual destructor*) |
| `main bitiyor:` sonrası | `main`'in sonu | Yerel nesneler ters sırayla yok edildi: baro, gps2, gps |

## virtual nedir?

Bir fonksiyon `virtual` olunca, onu bir temel class pointer'ı üzerinden çağırdığımda **hangi fonksiyonun çalışacağı program çalışırken belirleniyor**, nesnenin gerçek türüne bakılarak.

- `virtual` yoksa: `work->Run()` derleme sırasında `WorkItem::Run`'a bağlanır. Derleyici sadece pointer'ın türüne (`WorkItem *`) bakar.
- `virtual` varsa: `work->Run()` "work'ün gösterdiği nesnenin türünün Run'ı" demek. GPS ise GpsSim'inki, barometre ise BaroSim'inki.

Arka planda C sürümündeki yapının aynısı kuruluyor:

| C'de elle yaptığım | C++'ta derleyicinin yaptığı |
|---|---|
| `WorkItemTablo gps_tablosu = { gps_run, gps_yok_et };` | Her class için bir **virtual tablo** (vtable): o class'ın virtual fonksiyonlarının adresleri. Flash'ta, class başına bir tane |
| `WorkItem`'ın ilk alanı `tablo` | Her nesnenin başına gizli bir **tablo pointer'ı** (vptr) |
| `gps_init` içinde `g->temel.tablo = &gps_tablosu;` | Constructor çalışırken vptr'ye o class'ın tablosunun adresi yazılıyor |
| `w->tablo->run(w)` | `work->Run()` |

Çıktıdaki `gps` ve `gps2`'nin aynı tablo adresini göstermesi bunun kanıtı: tablo class başına bir tane, nesneler sadece onun adresini tutuyor.

## override ve "= 0"

**`= 0` (saf virtual):** `virtual void Run() = 0;` WorkItem'ın Run'ı olmadığını söylüyor. Böyle en az bir fonksiyonu olan class'a **soyut class** deniyor; ondan doğrudan nesne oluşturulamıyor. Denedim: `hata_soyut.cpp`

```cpp
// Run'ı "= 0" olan bir class'tan doğrudan nesne oluşturmak
class WorkItem
{
public:
    virtual ~WorkItem() {}
    virtual void Run() = 0;
};

int main()
{
    WorkItem w;     // hata bekliyoruz: Run'ı olmayan bir nesne çalıştırılamaz
    return 0;
}
```

```
hata_soyut.cpp:11:14: error: cannot declare variable 'w' to be of abstract type 'WorkItem'
hata_soyut.cpp:2:7: note:   because the following virtual functions are pure within 'WorkItem':
hata_soyut.cpp:6:18: note:     'virtual void WorkItem::Run()'
```

"WorkItem soyut bir tür, ondan değişken oluşturulamaz; çünkü `Run()` saf virtual." Mantıklı: Run'ı olmayan bir iş kuyruğa girip çalıştırılsaydı ne yapacaktı? C'de `tablo` alanını boş bırakıp çağırmak programı çökertirdi; burada derlenmiyor bile.

**`override`:** Temel class'taki virtual bir fonksiyonun yerine geçtiğimi derleyiciye söylüyor; derleyici de gerçekten öyle bir fonksiyon olup olmadığını kontrol ediyor. İsmi yanlış yazmayı denedim: `hata_override.cpp`

```cpp
// override yazıp ismi yanlış yazmak
class WorkItem
{
public:
    virtual ~WorkItem() {}
    virtual void Run() = 0;
};

class GpsSim : public WorkItem
{
public:
    void run() override {}      // küçük harf "run": hata bekliyoruz
};

int main()
{
    return 0;
}
```

```
hata_override.cpp:12:10: error: 'void GpsSim::run()' marked 'override', but does not override
```

"`run()` override olarak işaretlenmiş ama hiçbir şeyin yerine geçmiyor." Temel class'taki `Run` büyük harfle başlıyor. `override` yazmasaydım bu kod sessizce derlenirdi: GpsSim'de alakasız yeni bir `run` fonksiyonu oluşurdu, kuyruk ise hiç çağrılmayan bir şeyi beklerdi. `override` bu tür hataları derleme aşamasında yakalıyor.

## virtual destructor

Temel class pointer'ı üzerinden `delete` yapılıyorsa, destructor da virtual olmalı. Virtual olmazsa ne olduğunu denedim: `virtual_olmayan_destructor.cpp`

```cpp
// Temel class'ın destructor'ı virtual değilse ne oluyor?
#include <cstdio>

class WorkItem
{
public:
    ~WorkItem() { printf("  ~WorkItem\n"); }      // virtual yok
    virtual void Run() = 0;
};

class GpsSim : public WorkItem
{
public:
    ~GpsSim() { printf("  ~GpsSim (GPS portu kapatildi)\n"); }
    void Run() override {}
};

int main()
{
    WorkItem *w = new GpsSim();
    delete w;       // hangi destructor'lar çalışacak?
    return 0;
}
```

Derleyici uyardı, program yine de derlendi ve çalıştı:

```
warning: deleting object of abstract class type 'WorkItem' which has non-virtual destructor will cause undefined behavior [-Wdelete-non-virtual-dtor]
```

Çıktı:

```
  ~WorkItem
```

`~GpsSim` **hiç çalışmadı**; "GPS portu kapatıldı" mesajı yok. `delete w` sadece pointer'ın türüne (`WorkItem *`) baktı ve sadece WorkItem'ın destructor'ını çağırdı. GPS'in kaynakları geri verilmedi, 2. dersteki (constructor) kaynak sızıntısının aynısı.

Ana programda destructor virtual olduğu için `delete w;` doğru sırayla `~GpsSim` ve `~WorkItem`'ı çalıştırdı. Kural: virtual fonksiyonu olan bir temel class'ın destructor'ı da virtual olmalı.

## Bellekte ne değişiyor?

Her nesnenin başına bir tablo pointer'ı ekleniyor. STM32'de (pointer 4 byte) ölçtüğüm boyutlar:

| | virtual yokken | virtual varken |
|---|---|---|
| WorkItem | 8 byte | 12 byte |
| GpsSim | 12 byte | 16 byte |

```
GpsSim nesnesi, STM32'de 16 byte:

+0    tablo pointer'ı (vptr)   4 byte   ← derleyici ekledi, GpsSim'in tablosunu gösteriyor
+4    _ad                       4 byte
+8    _calisma_sayisi           4 byte
+12   _lat                      4 byte
```

(Programın bilgisayardaki çıktısında 24 byte yazıyor; orada pointer'lar 8 byte.)

- **RAM'de:** nesne başına 4 byte. 30 modüllük bir sistemde 120 byte.
- **Flash'ta:** class başına bir tablo; her virtual fonksiyon için tabloda 4 byte'lık bir adres.

## İşlemci ne yapıyor?

`work->Run()` çağrısında, `work` bir GpsSim'i gösterirken:

| Adım | İşlemci ne yapıyor | Yaklaşık süre |
|---|---|---|
| 1 | `work`'teki adresten ilk 4 byte'ı okuyor: tablo pointer'ı (GpsSim'in tablosunun adresi) | ~2 tick (RAM'den okuma) |
| 2 | Tablonun içinden Run'ın sırasındaki adresi okuyor: `GpsSim::Run`'ın adresi | ~2 tick (flash'tan okuma; flash yavaşsa daha fazla) |
| 3 | `this` = `work` olarak register'a koyup o adrese atlıyor | ~2–4 tick |
| 4 | `GpsSim::Run` çalışıyor | (fonksiyonun kendi işi) |

Normal (virtual olmayan) bir çağrıda 1. ve 2. adımlar yok; derleyici fonksiyonun adresini derleme sırasında biliyor ve doğrudan atlıyor. Yani virtual çağrının **ek maliyeti yaklaşık 4 tick**, artı iki bellek okuması. Bir de, derleyici hangi fonksiyonun çalışacağını bilmediği için küçük fonksiyonları çağrıldıkları yere yerleştiremiyor.

Bu ne kadar önemli? 480 MHz'lik bir STM32H7'de 4 tick yaklaşık 8 nanosaniye. PX4'te 30 civarında modülün her biri saniyede 250 kez çalışsa, saniyede 7500 virtual çağrı eder; toplam ek maliyet saniyede ~30.000 tick, işlemci zamanının on binde birinden az. Karşılığında, yeni bir modül eklemek için work queue'nun koduna hiç dokunmak gerekmiyor.

C sürümünde `w->tablo->run(w)` da aynı iki okumayı ve dolaylı atlamayı yapıyor. Yani virtual, C'de elle yapacağım şeyden daha pahalı değil; sadece benim yerime yazılıyor ve hata yapma ihtimalini ortadan kaldırıyor.

## PX4'te

PX4'ün work queue'su bu dersteki örneğin aynısı.

`WorkItem.hpp`:

```cpp
virtual ~WorkItem();
...
virtual void Run() = 0;
```

WorkItem soyut bir class: kendi Run'ı yok, her modül yazmak zorunda. Destructor virtual, çünkü modüller temel pointer üzerinden siliniyor (2. derste, constructor'da gördüğüm `delete _object.load();`).

`SensorGpsSim.hpp`:

```cpp
~SensorGpsSim() override;
...
void Run() override;
```

GPS simülatörü WorkItem'ın Run'ının yerine kendi Run'ını koyuyor (`ScheduledWorkItem` üzerinden, 5. dersteki zincir).

`WorkQueue.hpp` ve `WorkQueue.cpp`:

```cpp
IntrusiveQueue<WorkItem *> _q;     // kuyruk: sadece WorkItem adresleri
...
work->Run();                        // sırası gelen işi çalıştır
```

Work queue, kuyruktaki modülün GPS simülatörü mü, EKF2 mi, rate kontrolcüsü mü olduğunu bilmiyor; sadece `work->Run()` diyor. Tablo pointer'ı sayesinde her seferinde doğru modülün Run'ı çalışıyor. PX4 çalışmamda `work_queue status` ile gördüğüm listedeki her satır, böyle bir kuyruktaki bir WorkItem'dı.

PX4'ün gerçek kart için derleme ayarlarında (`cmake/px4_add_common_flags.cmake`, `platforms/nuttx/cmake/px4_impl_os.cmake`) `-fno-rtti` ve `-fno-exceptions` var. Yani PX4, C++'ın çalışma zamanında tür bilgisi ve hata fırlatma özelliklerini kapatıyor; kullandığı tek çalışma zamanı mekanizması bu dersteki virtual tablo.
