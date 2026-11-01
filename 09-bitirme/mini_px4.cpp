// 9. ders, bitirme: PX4'ün çekirdek fikrinin oyuncak hali.
// Work queue, virtual Run(), template'li topic'ler, periyodik ve olayla tetiklenen modüller.
#include <cstdio>     // C'deki <stdio.h>'nin C++ adı: printf
#include <cstdint>    // C'deki <stdint.h>: uint64_t, int32_t ...

namespace mini        // namespace: isimleri bir "soyadı" altında toplar; dışarıdan mini::simdi diye erişilir
{                     // C'de bunun yerine her isme mini_ prefix'i yazardık

// ---------------- Zaman (8. ders: constexpr, süre yazımı) ----------------
using zaman_us = uint64_t;   // using = typedef'in C++ yazımı: typedef uint64_t zaman_us;
zaman_us simdi = 0;          // simülasyon saati, mikrosaniye (namespace içinde global değişken)

namespace time_literals      // süre literallerini ayrı bir namespace'e koyuyoruz; isteyen "using" ile açar
{
// operator ""_ms → kullanıcı tanımlı literal: kodda 125_ms yazınca bu fonksiyon çağrılır ve 125000 döner
// constexpr → sonuç derleme sırasında hesaplanır; programa doğrudan 125000 sayısı yazılır, çalışırken 0 tick
constexpr zaman_us operator ""_ms(unsigned long long ms) { return ms * 1000; }
constexpr zaman_us operator ""_s(unsigned long long s)   { return s * 1000000; }
} // namespace time_literals

using namespace time_literals;   // bu namespace'teki isimleri burada önek yazmadan kullan (4_ms gibi)

// ---------------- WorkItem (5. ders: kalıtım, 6. ders: virtual) ----------------
class WorkItem                   // class: veri + o veri üzerinde çalışan fonksiyonlar tek pakette (C'de struct + fonksiyonlar)
{
public:                          // public: dışarıdan erişilebilen kısım (API)
    // constructor: nesne oluşurken otomatik çalışır. ": _ad(ad)" = initializer list, üyeyi gövdeden önce ilk değerle kurar
    WorkItem(const char *ad) : _ad(ad) {}

    // virtual destructor: türeyen nesne WorkItem* üzerinden silinirse türeyenin destructor'ı da çalışsın diye
    virtual ~WorkItem() {}

    // pure virtual (= 0): WorkItem kendi Run'ını yazmıyor; her türeyen class yazmak ZORUNDA.
    // virtual → çağrı çalışma anında nesnenin gerçek tipine gider (C'deki struct içindeki fonksiyon pointer'ı gibi)
    virtual void Run() = 0;

    void ScheduleNow() { _bekliyor = true; }   // class içinde tanımlanan üye fonksiyon; _bekliyor bu nesnenin alanı

    void ScheduleOnInterval(zaman_us periyot)
    {
        _periyot = periyot;
        _sonraki = simdi + periyot;
    }

    // Work queue her adımda çağırıyor: zamanı geldiyse ya da tetiklendiyse Run()
    void gerekirse_calistir()
    {
        const bool zamani_geldi = (_periyot != 0 && simdi >= _sonraki);   // bool: C++'ta yerleşik tip (C'de stdbool.h)

        if (zamani_geldi || _bekliyor) {
            if (zamani_geldi) {
                _sonraki += _periyot;
            }
            _bekliyor = false;
            _calisma_sayisi++;
            Run();               // virtual çağrı: GpsSim ise GpsSim::Run, Ekf ise Ekf::Run çalışır
        }
    }

    // sondaki const: bu fonksiyon nesnenin alanlarını değiştirmez (derleyici kontrol eder)
    void durum_yazdir() const
    {
        if (_periyot != 0) {
            printf("  %-12s her %3u ms   %4u calisma\n", _ad, (unsigned)(_periyot / 1000), (unsigned)_calisma_sayisi);
        } else {
            printf("  %-12s olay ile     %4u calisma\n", _ad, (unsigned)_calisma_sayisi);
        }
    }

protected:                       // protected: dışarıya kapalı, ama türeyen class'lar (GpsSim, Ekf ...) erişebilir
    const char *_ad;

private:                         // private: yalnız bu class'ın kendi fonksiyonları erişebilir
    // {0} = üye başlangıç değeri (default member initializer): constructor yazmasan da bu değerle başlar
    zaman_us _periyot{0};
    zaman_us _sonraki{0};
    bool     _bekliyor{false};
    uint32_t _calisma_sayisi{0};
};

// ---------------- Topic, Publication, Subscription (7. ders: template, 3. ders: referans) ----------------
// template<typename T>: T yerine hangi tip verilirse (GpsVeri, EstimatorDurum ...) derleyici o tip için
// class'ın bir kopyasını üretir. C'de her mesaj tipi için ayrı struct + fonksiyon yazmak ya da void* kullanmak gerekirdi.
template<typename T>
class Topic
{
public:
    // const T &v: referans ile alıyor (kopya yok, pointer gibi ama NULL olamaz); const → fonksiyon v'yi değiştiremez
    void yayinla(const T &v)
    {
        _veri = v;               // struct ataması: T'nin tüm alanları kopyalanır
        _sayac++;
        for (int i = 0; i < _abone_sayisi; i++) {
            _aboneler[i]->ScheduleNow();      // callback'li aboneleri uyandır (-> : pointer üzerinden üye çağrısı, C'deki gibi)
        }
    }

    void callback_ekle(WorkItem *w)
    {
        if (_abone_sayisi < 4) {
            _aboneler[_abone_sayisi++] = w;
        }
    }

    uint32_t sayac() const { return _sayac; }
    const T &veri() const { return _veri; }   // const referans döndürür: okuyan kopya almaz, değiştiremez de

private:
    T         _veri{};           // {} = sıfırla başlat (C'deki = {0} gibi)
    uint32_t  _sayac{0};
    WorkItem *_aboneler[4] {};   // 4 elemanlı pointer dizisi, hepsi nullptr ile başlar
    int       _abone_sayisi{0};
};

template<typename T>
class Publication
{
public:
    // referans üye (_topic) mutlaka initializer list'te bağlanır; sonradan başka topic'e çevrilemez
    Publication(Topic<T> &topic) : _topic(topic) {}

    void publish(const T &v) { _topic.yayinla(v); }   // referans üzerinden çağrı: '.' ile (pointer olsaydı '->')

private:
    Topic<T> &_topic;            // referans: başka bir nesnenin takma adı; kopya değil, aynı Topic
};

template<typename T>
class Subscription
{
public:
    // callback verilirse, topic'e her yayında o modül uyandırılır
    // "= nullptr": default parametre; çağıran ikinci argümanı vermezse nullptr olur
    // nullptr: C++'ın tip güvenli NULL'u (8. ders)
    Subscription(Topic<T> &topic, WorkItem *callback = nullptr) : _topic(topic)
    {
        if (callback != nullptr) {
            _topic.callback_ekle(callback);
        }
    }

    bool updated() const { return _okunan != _topic.sayac(); }   // son okuduğumdan beri yeni yayın var mı?

    bool update(T *hedef)        // yeni veri varsa hedef'e kopyalar ve true döner
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
    // WorkItem* tutuyor ama içine GpsSim*, Ekf* ... konabiliyor: türeyen class pointer'ı temel class pointer'ına otomatik çevrilir
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
            simdi += 4_ms;       // 4_ms → derleme sırasında 4000'e dönüşür
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
struct GpsVeri {                 // C++'ta struct = varsayılanı public olan class; "struct GpsVeri" yazmadan GpsVeri kullanılır
    zaman_us zaman;
    int32_t  lat;
    int32_t  lon;
    uint8_t  uydu;
};

// enum class: değerler GpsDurumu:: önekiyle yazılır, int'e kendiliğinden çevrilmez, başka enum'la karıştırılamaz.
// ": uint8_t" → altta 1 byte tutulur (C enum'u genelde 4 byte)
enum class GpsDurumu : uint8_t { Bekleniyor, Kullaniliyor, Kayip };

struct EstimatorDurum {
    zaman_us  zaman;
    GpsDurumu gps;
};

// template'in kullanıldığı yer: derleyici burada Topic<GpsVeri> ve Topic<EstimatorDurum> class'larını üretir
Topic<GpsVeri>        gps_topic;
Topic<EstimatorDurum> estimator_topic;

} // namespace mini

// ---------------- Modüller (1. ders: class, 2. ders: constructor/destructor) ----------------
using namespace mini;                    // mini:: önekini yazmadan kullan
using namespace mini::time_literals;     // 125_ms, 1_s yazımları için

// GPS simülatörü: her 125 ms'de bir konum yayınlıyor. Engelliyken yayınlamıyor.
class GpsSim : public WorkItem           // ": public WorkItem" → kalıtım: GpsSim bir WorkItem'dır, onun üyelerini devralır
{
public:
    // önce temel class'ın constructor'ı çalışır (WorkItem("gps_sim")), sonra bu gövde
    GpsSim() : WorkItem("gps_sim")
    {
        ScheduleOnInterval(125_ms);      // devralınan fonksiyon, doğrudan çağrılabiliyor
    }

    // override: "temel class'taki virtual fonksiyonu eziyorum" — isim/imza yanlışsa derleyici hata verir
    ~GpsSim() override
    {
        printf("  gps_sim kapandi\n");   // destructor: nesnenin ömrü bitince (main'den çıkarken) otomatik çalışır
    }

    void engelle(bool engelli) { _engelli = engelli; }

    void Run() override                  // WorkItem::Run'ın GpsSim hali; work queue zamanı gelince bunu çağırır
    {
        if (_engelli) {
            return;
        }

        GpsVeri v{};                     // {} → tüm alanlar sıfırla başlar
        v.zaman = simdi;
        // static_cast: kontrollü tip çevirme, C'deki (int32_t)(...) ile aynı komut; sadece derlemede kontrol ekler
        v.lat   = 473977420 + static_cast<int32_t>(simdi / 1000000);   // saniyede bir biraz kaysın
        v.lon   = 85455940;
        v.uydu  = 12;
        _gps_pub.publish(v);
    }

private:
    Publication<GpsVeri> _gps_pub{gps_topic};   // üye nesne, {gps_topic} ile constructor'ına topic veriliyor
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
        if (_gps_sub.update(&gps)) {     // yeni GPS geldiyse gps'e kopyalanır
            _son_gps_zamani = gps.zaman;
        }

        GpsDurumu durum = GpsDurumu::Bekleniyor;   // enum class değeri: önekiyle yazılmak zorunda
        if (_son_gps_zamani != 0) {
            durum = (simdi - _son_gps_zamani > GPS_ZAMAN_ASIMI) ? GpsDurumu::Kayip : GpsDurumu::Kullaniliyor;
        }

        // EstimatorDurum{simdi, durum}: geçici struct'ı alan sırasıyla doldurup doğrudan gönderiyor
        _durum_pub.publish(EstimatorDurum{simdi, durum});
    }

private:
    // static: tüm Ekf nesneleri için tek kopya; constexpr: derleme sırasında 1000000 olur (#define'ın tip güvenli hali)
    static constexpr zaman_us GPS_ZAMAN_ASIMI = 1_s;    // PX4 EKF2'de bu süre 7 saniye

    Subscription<GpsVeri>       _gps_sub{gps_topic};          // callback yok → Ekf GPS gelince uyanmaz, kendi periyodunda bakar
    Publication<EstimatorDurum> _durum_pub{estimator_topic};
    zaman_us                    _son_gps_zamani{0};
};

// GPS monitörü: periyodik değil; estimator her yayın yaptığında uyanıyor, durum değişince yazıyor.
class GpsMonitor : public WorkItem
{
public:
    GpsMonitor() : WorkItem("gps_monitor") {}   // ScheduleOnInterval çağırmıyor → periyodik değil

    void Run() override
    {
        EstimatorDurum d;
        if (_durum_sub.update(&d) && d.gps != _onceki) {   // enum class karşılaştırması: aynı tipler arasında serbest
            printf("[%6.3f s] gps_monitor: %s -> %s\n", d.zaman / 1e6, ad(_onceki), ad(d.gps));
            _onceki = d.gps;
        }
    }

private:
    // static üye fonksiyon: nesneye (this'e) ihtiyaç duymaz; C'deki dosya içi static fonksiyon gibi
    static const char *ad(GpsDurumu g)
    {
        switch (g) {
        case GpsDurumu::Bekleniyor:   return "bekleniyor";
        case GpsDurumu::Kullaniliyor: return "kullaniliyor";
        case GpsDurumu::Kayip:        return "KAYIP";
        }
        return "?";
    }

    // this: bu nesnenin kendi adresi (GpsMonitor*). Topic'e "yayın gelince beni uyandır" diye kendini veriyor
    Subscription<EstimatorDurum> _durum_sub{estimator_topic, this};   // callback: this
    GpsDurumu _onceki{GpsDurumu::Bekleniyor};
};

int main()
{
    // nesneler stack'te oluşuyor; burada constructor'lar çalışır, main bitince destructor'lar ters sırayla çalışır
    GpsSim     gps;
    Ekf        ekf;
    GpsMonitor monitor;

    WorkQueue wq;
    wq.ekle(&gps);               // &gps: GpsSim* → WorkItem* olarak saklanır (kalıtım sayesinde)
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
    return 0;                    // buradan sonra monitor, ekf, gps destructor'ları çalışır → "gps_sim kapandi"
}
