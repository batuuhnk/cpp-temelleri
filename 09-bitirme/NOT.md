# 9. bitirme: mini PX4

İlk 8 derste öğrendiklerimi tek bir programda birleştirdim: PX4'ün çekirdek fikrinin düz C++ ile yazılmış oyuncak hali. İçinde bir work queue, `virtual Run()` ile çalışan modüller, template'li topic'ler, periyodik çalışan ve bir topic'e veri gelince uyanan modüller var. Bunlarla PX4 çalışmamdaki GPS kaybı deneyini küçük ölçekte yeniden yaptım.

Bu derste C sürümü yok; her parçanın C karşılığı ilgili derste. Kodun tamamı bu notun içinde.

## Program ne yapıyor?

Üç modül, bir work queue ve iki topic var:

```
gps_sim  (her 125 ms)  ──gps_topic──►  ekf  (her 50 ms)  ──estimator_topic──►  gps_monitor  (olayla)
                                                                                 durum değişince yazar
```

- **gps_sim**: Her 125 ms'de `gps_topic`'e bir konum yayınlıyor. "Engelli" yapılınca yayını kesiyor; PX4'teki `sensor_gps_sim stop`'un ya da `failure gps off`'un karşılığı.
- **ekf**: Her 50 ms'de çalışıyor. `gps_topic`'te yeni veri varsa son GPS zamanını güncelliyor. 1 saniyedir yeni GPS gelmediyse GPS'i "kayıp" sayıyor. Sonucu her seferinde `estimator_topic`'e yayınlıyor. PX4'teki EKF2'nin `cs_gnss_pos` bayrağının basit hali.
- **gps_monitor**: Periyodik değil. `estimator_topic`'e her yayın yapıldığında uyanıyor; GPS durumu değişmişse ekrana yazıyor.
- **Work queue**: Simülasyon saatini 4 ms'lik adımlarla (250 Hz) ilerletiyor ve her adımda modüllere "zamanın geldiyse ya da biri seni uyandırdıysa çalış" diyor.

`main` 2 saniye normal çalıştırıyor, sonra GPS'i kesip 3 saniye çalıştırıyor, sonra GPS'i geri açıp 1 saniye daha çalıştırıyor. En sonda work queue'nun durumunu yazdırıyor.

## Kod

Dosya: `mini_px4.cpp`

```cpp
// 9. ders, bitirme: PX4'ün çekirdek fikrinin oyuncak hali.
// Work queue, virtual Run(), template'li topic'ler, periyodik ve olayla tetiklenen modüller.
#include <cstdio>
#include <cstdint>

namespace mini
{

// ---------------- Zaman (8. ders: constexpr, süre yazımı) ----------------
using zaman_us = uint64_t;
zaman_us simdi = 0;     // simülasyon saati, mikrosaniye

namespace time_literals
{
constexpr zaman_us operator ""_ms(unsigned long long ms) { return ms * 1000; }
constexpr zaman_us operator ""_s(unsigned long long s)   { return s * 1000000; }
} // namespace time_literals

using namespace time_literals;

// ---------------- WorkItem (5. ders: kalıtım, 6. ders: virtual) ----------------
class WorkItem
{
public:
    WorkItem(const char *ad) : _ad(ad) {}
    virtual ~WorkItem() {}

    virtual void Run() = 0;     // her modül kendi Run'ını yazmak zorunda

    void ScheduleNow() { _bekliyor = true; }

    void ScheduleOnInterval(zaman_us periyot)
    {
        _periyot = periyot;
        _sonraki = simdi + periyot;
    }

    // Work queue her adımda çağırıyor: zamanı geldiyse ya da tetiklendiyse Run()
    void gerekirse_calistir()
    {
        const bool zamani_geldi = (_periyot != 0 && simdi >= _sonraki);

        if (zamani_geldi || _bekliyor) {
            if (zamani_geldi) {
                _sonraki += _periyot;
            }
            _bekliyor = false;
            _calisma_sayisi++;
            Run();
        }
    }

    void durum_yazdir() const
    {
        if (_periyot != 0) {
            printf("  %-12s her %3u ms   %4u calisma\n", _ad, (unsigned)(_periyot / 1000), (unsigned)_calisma_sayisi);
        } else {
            printf("  %-12s olay ile     %4u calisma\n", _ad, (unsigned)_calisma_sayisi);
        }
    }

protected:
    const char *_ad;

private:
    zaman_us _periyot{0};
    zaman_us _sonraki{0};
    bool     _bekliyor{false};
    uint32_t _calisma_sayisi{0};
};

// ---------------- Topic, Publication, Subscription (7. ders: template, 3. ders: referans) ----------------
template<typename T>
class Topic
{
public:
    void yayinla(const T &v)
    {
        _veri = v;
        _sayac++;
        for (int i = 0; i < _abone_sayisi; i++) {
            _aboneler[i]->ScheduleNow();      // callback'li aboneleri uyandır
        }
    }

    void callback_ekle(WorkItem *w)
    {
        if (_abone_sayisi < 4) {
            _aboneler[_abone_sayisi++] = w;
        }
    }

    uint32_t sayac() const { return _sayac; }
    const T &veri() const { return _veri; }

private:
    T         _veri{};
    uint32_t  _sayac{0};
    WorkItem *_aboneler[4] {};
    int       _abone_sayisi{0};
};

template<typename T>
class Publication
{
public:
    Publication(Topic<T> &topic) : _topic(topic) {}

    void publish(const T &v) { _topic.yayinla(v); }

private:
    Topic<T> &_topic;
};

template<typename T>
class Subscription
{
public:
    // callback verilirse, topic'e her yayında o modül uyandırılır
    Subscription(Topic<T> &topic, WorkItem *callback = nullptr) : _topic(topic)
    {
        if (callback != nullptr) {
            _topic.callback_ekle(callback);
        }
    }

    bool updated() const { return _okunan != _topic.sayac(); }

    bool update(T *hedef)
    {
        if (!updated()) {
            return false;
        }
        *hedef = _topic.veri();
        _okunan = _topic.sayac();
        return true;
    }

private:
    Topic<T> &_topic;
    uint32_t  _okunan{0};
};

// ---------------- Work queue ----------------
class WorkQueue
{
public:
    void ekle(WorkItem *w)
    {
        if (_sayi < 8) {
            _isler[_sayi++] = w;
        }
    }

    // Saati 4 ms'lik adımlarla (250 Hz) ilerletip her adımda işlere bakıyor
    void calistir(zaman_us sure)
    {
        const zaman_us bitis = simdi + sure;

        while (simdi < bitis) {
            simdi += 4_ms;
            for (int i = 0; i < _sayi; i++) {
                _isler[i]->gerekirse_calistir();
            }
        }
    }

    void status() const
    {
        for (int i = 0; i < _sayi; i++) {
            _isler[i]->durum_yazdir();
        }
    }

private:
    WorkItem *_isler[8] {};
    int       _sayi{0};
};

// ---------------- Mesajlar (8. ders: enum class) ----------------
struct GpsVeri {
    zaman_us zaman;
    int32_t  lat;
    int32_t  lon;
    uint8_t  uydu;
};

enum class GpsDurumu : uint8_t { Bekleniyor, Kullaniliyor, Kayip };

struct EstimatorDurum {
    zaman_us  zaman;
    GpsDurumu gps;
};

Topic<GpsVeri>        gps_topic;
Topic<EstimatorDurum> estimator_topic;

} // namespace mini

// ---------------- Modüller (1. ders: class, 2. ders: constructor/destructor) ----------------
using namespace mini;
using namespace mini::time_literals;

// GPS simülatörü: her 125 ms'de bir konum yayınlıyor. Engelliyken yayınlamıyor.
class GpsSim : public WorkItem
{
public:
    GpsSim() : WorkItem("gps_sim")
    {
        ScheduleOnInterval(125_ms);
    }

    ~GpsSim() override
    {
        printf("  gps_sim kapandi\n");
    }

    void engelle(bool engelli) { _engelli = engelli; }

    void Run() override
    {
        if (_engelli) {
            return;
        }

        GpsVeri v{};
        v.zaman = simdi;
        v.lat   = 473977420 + static_cast<int32_t>(simdi / 1000000);   // saniyede bir biraz kaysın
        v.lon   = 85455940;
        v.uydu  = 12;
        _gps_pub.publish(v);
    }

private:
    Publication<GpsVeri> _gps_pub{gps_topic};
    bool _engelli{false};
};

// Tahminci: her 50 ms'de GPS'e bakıyor, 1 saniyedir yeni GPS yoksa "kayıp" diyor.
class Ekf : public WorkItem
{
public:
    Ekf() : WorkItem("ekf")
    {
        ScheduleOnInterval(50_ms);
    }

    void Run() override
    {
        GpsVeri gps;
        if (_gps_sub.update(&gps)) {
            _son_gps_zamani = gps.zaman;
        }

        GpsDurumu durum = GpsDurumu::Bekleniyor;
        if (_son_gps_zamani != 0) {
            durum = (simdi - _son_gps_zamani > GPS_ZAMAN_ASIMI) ? GpsDurumu::Kayip : GpsDurumu::Kullaniliyor;
        }

        _durum_pub.publish(EstimatorDurum{simdi, durum});
    }

private:
    static constexpr zaman_us GPS_ZAMAN_ASIMI = 1_s;    // PX4 EKF2'de bu süre 7 saniye

    Subscription<GpsVeri>       _gps_sub{gps_topic};
    Publication<EstimatorDurum> _durum_pub{estimator_topic};
    zaman_us                    _son_gps_zamani{0};
};

// GPS monitörü: periyodik değil; estimator her yayın yaptığında uyanıyor, durum değişince yazıyor.
class GpsMonitor : public WorkItem
{
public:
    GpsMonitor() : WorkItem("gps_monitor") {}

    void Run() override
    {
        EstimatorDurum d;
        if (_durum_sub.update(&d) && d.gps != _onceki) {
            printf("[%6.3f s] gps_monitor: %s -> %s\n", d.zaman / 1e6, ad(_onceki), ad(d.gps));
            _onceki = d.gps;
        }
    }

private:
    static const char *ad(GpsDurumu g)
    {
        switch (g) {
        case GpsDurumu::Bekleniyor:   return "bekleniyor";
        case GpsDurumu::Kullaniliyor: return "kullaniliyor";
        case GpsDurumu::Kayip:        return "KAYIP";
        }
        return "?";
    }

    Subscription<EstimatorDurum> _durum_sub{estimator_topic, this};   // callback: this
    GpsDurumu _onceki{GpsDurumu::Bekleniyor};
};

int main()
{
    GpsSim     gps;
    Ekf        ekf;
    GpsMonitor monitor;

    WorkQueue wq;
    wq.ekle(&gps);
    wq.ekle(&ekf);
    wq.ekle(&monitor);

    printf("[%6.3f s] basladi\n", simdi / 1e6);
    wq.calistir(2_s);

    printf("[%6.3f s] GPS kesiliyor\n", simdi / 1e6);
    gps.engelle(true);
    wq.calistir(3_s);

    printf("[%6.3f s] GPS geri aciliyor\n", simdi / 1e6);
    gps.engelle(false);
    wq.calistir(1_s);

    printf("[%6.3f s] work queue durumu:\n", simdi / 1e6);
    wq.status();

    printf("program bitiyor:\n");
    return 0;
}
```

## Çıktı

`./calistir.sh 09-bitirme/mini_px4.cpp`:

```
[ 0.000 s] basladi
[ 0.152 s] gps_monitor: bekleniyor -> kullaniliyor
[ 2.000 s] GPS kesiliyor
[ 3.052 s] gps_monitor: kullaniliyor -> KAYIP
[ 5.000 s] GPS geri aciliyor
[ 5.152 s] gps_monitor: KAYIP -> kullaniliyor
[ 6.000 s] work queue durumu:
  gps_sim      her 125 ms     48 calisma
  ekf          her  50 ms    120 calisma
  gps_monitor  olay ile      120 calisma
program bitiyor:
  gps_sim kapandi
```

| Çıktı | Ne oldu |
|---|---|
| `[0.152 s] bekleniyor -> kullaniliyor` | İlk GPS 0.128 s'de yayınlandı; EKF bir sonraki çalışmasında (0.152 s) gördü ve durumu yayınladı; gps_monitor aynı adımda uyanıp değişikliği yazdı |
| `[2.000 s] GPS kesiliyor` | `gps.engelle(true)`. Son GPS yayını tam 2.000 s'de yapılmıştı |
| `[3.052 s] kullaniliyor -> KAYIP` | EKF 3.000 s'de çalıştığında son GPS'in üzerinden tam 1.000 s geçmişti, sınırı aşmamıştı. 3.052 s'deki çalışmasında 1.052 s geçmişti → kayıp |
| `[5.000 s] GPS geri aciliyor` | `gps.engelle(false)` |
| `[5.152 s] KAYIP -> kullaniliyor` | GPS 5.128 s'de yeniden yayınladı, EKF 5.152 s'de gördü |
| `gps_sim her 125 ms 48 calisma` | 6 saniye / 125 ms = 48. GPS kesikken de Run() çağrıldı ama yayın yapmadı |
| `ekf her 50 ms 120 calisma` | 6 saniye / 50 ms = 120 |
| `gps_monitor olay ile 120 calisma` | EKF'nin her yayını onu bir kez uyandırdı: 120. Ama sadece 3 kez yazdı, çünkü sadece 3 kez durum değişti |
| `gps_sim kapandi` | `main` biterken GpsSim'in destructor'ı çalıştı |

PX4 çalışmamdaki gerçek deneyde EKF2'nin GPS'i bırakması 7.9 saniye sürmüştü; orada zaman aşımı 7 saniye. Burada 1 saniye koydum, akış aynı.

## Hangi parça hangi dersten?

| Kodda | Ne yapıyor | Ders |
|---|---|---|
| `class GpsSim`, `class Ekf`, `_ad`, `_engelli` gibi alanlar, `public`/`private` | Her modül veriyi ve fonksiyonlarını bir arada tutuyor | 1. class |
| `GpsSim() : WorkItem("gps_sim") { ScheduleOnInterval(125_ms); }`, `~GpsSim()` | Modül oluşurken kendini zamanlıyor; program biterken destructor çalışıyor | 2. constructor/destructor |
| `Publication(Topic<T> &topic) : _topic(topic)`, `publish(const T &v)`, `update(T *hedef)` | Topic'e referansla bağlanmak; veriyi kopyasız vermek; okurken pointer'la hedefe yazmak | 3. referans ve const |
| `namespace mini`, `namespace time_literals`, `using namespace` | Altyapı isimleri bir namespace'te, süre yazımları ayrı küçük bir namespace'te | 4. namespace |
| `class GpsSim : public WorkItem` | Her modül WorkItem'ın zamanlama işlerini miras alıyor (`ScheduleOnInterval`, `ScheduleNow`) | 5. kalıtım |
| `virtual void Run() = 0;`, `void Run() override`, `_isler[i]->gerekirse_calistir()` içindeki `Run()` | Work queue modüllerin türünü bilmeden her birinin kendi Run'ını çalıştırıyor | 6. virtual |
| `Topic<T>`, `Publication<T>`, `Subscription<T>` | Her mesaj tipi için ayrı, tip güvenli topic ve yayıncı/okuyucu | 7. template |
| `enum class GpsDurumu : uint8_t`, `constexpr`, `125_ms`, `1_s`, `static_cast`, `nullptr` | Durum değerleri, süre sabitleri, tip çevirme, "callback yok" varsayılanı | 8. küçük yardımcılar |

## Bir adımda ne oluyor?

0.152 s'deki adım, ilk "kullaniliyor" mesajının yazıldığı an:

1. Work queue saati 0.148'den 0.152'ye ilerletiyor ve sırayla üç modülün `gerekirse_calistir()` fonksiyonunu çağırıyor.
2. **gps_sim**: bir sonraki çalışma zamanı 0.250, henüz gelmedi; hiçbir şey yapmıyor.
3. **ekf**: çalışma zamanı 0.150 idi, geçti. `Run()` çağrılıyor (virtual, 6. ders).
   - `_gps_sub.update(&gps)`: topic'in sayacı (1) okuduğum sayaçtan (0) farklı → yeni veri var. GPS verisi `gps`'e kopyalanıyor, son GPS zamanı 0.128 oluyor.
   - 0.152 − 0.128 = 0.024 s < 1 s → durum "kullaniliyor".
   - `_durum_pub.publish(...)`: `estimator_topic`'e yazılıyor. Topic, callback listesindeki **gps_monitor**'un `ScheduleNow()`'ını çağırıyor; gps_monitor'un "bekliyor" bayrağı true oluyor.
4. **gps_monitor**: bekliyor bayrağı true → aynı adımda `Run()` çağrılıyor. `update(&d)` yeni durumu okuyor; önceki "bekleniyor"du, şimdi "kullaniliyor" → ekrana yazıyor.

gps_monitor kuyrukta EKF'den sonra olduğu için EKF'nin yayınını aynı adımda gördü. Önce olsaydı, bir sonraki adımda (4 ms sonra) görecekti.

## Neden 0.150 değil de 0.152?

EKF her 50 ms'de çalışacak şekilde zamanlandı, ama ilk çalışması 0.152 s'de oldu. Sebebi, work queue'nun saati 4 ms'lik adımlarla ilerletmesi: 0.148'de zamanı gelmemişti, bir sonraki adım 0.152. Yani her modül, zamanı geldikten sonraki ilk adımda çalışıyor; 0–4 ms arası bir gecikme oluyor.

Gerçek sistemlerde de buna benzer bir gecikme var. PX4'te `work_queue status` çıktısında EKF2'nin `249.5 Hz` (tam 250 değil) ve aralığının `4009 us` (tam 4000 değil) çıkması bu türden küçük zamanlama sapmalarını gösteriyordu.

## PX4'teki karşılıkları

| Mini PX4'te | PX4'te | Dosya |
|---|---|---|
| `WorkItem`, `virtual void Run() = 0;` | `WorkItem`, `virtual void Run() = 0;` | `px4_work_queue/WorkItem.hpp` |
| `ScheduleOnInterval(125_ms)` | `ScheduleOnInterval(125_ms)` | `px4_work_queue/ScheduledWorkItem.hpp` |
| `WorkQueue::calistir` içindeki `gerekirse_calistir()` → `Run()` | `WorkQueue::Run()` içindeki `work->Run();` | `px4_work_queue/WorkQueue.cpp` |
| `Topic<T>` | uORB topic'i (`ORB_ID(sensor_gps)` gibi) | `uORB/` |
| `Publication<T>::publish(const T &)` | `uORB::Publication<T>::publish(const T &)` | `uORB/Publication.hpp` |
| `Subscription<T>::update(T *)` | `uORB::Subscription::update(void *)` | `uORB/Subscription.hpp` |
| `Subscription<T>(topic, this)` ile callback | `uORB::SubscriptionCallbackWorkItem` | `uORB/SubscriptionCallback.hpp` |
| `namespace time_literals`, `_ms`, `_s` | `namespace time_literals`, `_ms`, `_s` | `drivers/drv_hrt.h` |
| `GpsSim` | `SensorGpsSim` | `modules/simulation/sensor_gps_sim/` |
| `Ekf`'teki 1 s zaman aşımı ve `GpsDurumu` | EKF2'deki 7 s `reset_timeout_max` ve `cs_gnss_pos` | `modules/ekf2/EKF/` |
| `gps.engelle(true)` | `sensor_gps_sim stop` ya da `failure gps off` | |
| `wq.status()` | `pxh> work_queue status` | |

## Neyi basitleştirdim?

Mini PX4 fikri doğru gösteriyor, ama gerçek PX4'ün yaptığı birçok şeyi yapmıyor:

- **Tek thread:** Bütün modüller tek bir döngüde sırayla çalışıyor. PX4'te birden fazla work queue thread'i var, her birinin önceliği farklı (`wq:rate_ctrl` en yüksek); işletim sistemi aralarında geçiş yapıyor.
- **Gerçek zaman yok:** Saat sadece bir sayı ve döngü onu ilerletiyor; program aslında milisaniyeler içinde bitiyor. PX4 SITL'deki lockstep fikrine benziyor, ama PX4'te zaman simülatörle birlikte ilerliyor.
- **Kilit yok:** Topic'e aynı anda iki thread yazıp okuyamadığı için kilit (mutex) gerekmedi. PX4'te uORB bunu kendisi hallediyor.
- **Topic'ler global değişken:** PX4'te topic'ler isimleriyle (`ORB_ID`) bulunuyor; modüller birbirinin değişkenlerini bilmiyor.
- **Kuyruk derinliği hep 1:** PX4'te bazı topic'lerin kuyruğu daha derin (`sensor_gyro` için 8).

Bunlar eksik olsa da asıl fikir aynı: modüller birbirini çağırmıyor, sadece topic'lere yazıp okuyor; work queue da her birinin `Run()`'ını, türünü bilmeden çalıştırıyor.

## Sırada

C++ hazırlığı burada bitiyor. Bundan sonra PX4'ün gerçek `SensorGpsSim` kodunu satır satır okuyup her satırı bu derslerden birine bağlayacağım. Ardından da PX4 çalışmamda bulduğum eksikliği kapatacağım: SIH'in GPS simülatörüne `failure gps off` desteği eklemek.
