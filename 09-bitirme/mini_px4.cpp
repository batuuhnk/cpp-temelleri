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
