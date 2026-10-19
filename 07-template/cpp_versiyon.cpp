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
