// 1. ders, C++ hali: veri ve onu kullanan fonksiyonlar aynı class'ın içinde.
#include <cstdio>

class GpsSim
{
public:
    // Dışarıdan çağrılabilen fonksiyonlar. C'deki "GpsSim *self" parametresi yok:
    // derleyici onu gizlice "this" adıyla kendisi ekliyor.
    void guncelle(double lat, double lon, int uydu)
    {
        if (_engelli) {          // aslında this->_engelli
            return;
        }
        _lat = lat;
        _lon = lon;
        _uydu_sayisi = uydu;
    }

    void engelle(bool engelli)
    {
        this->_engelli = engelli;   // "this" açıkça da yazılabilir, C'deki self->engelli ile aynı
    }

    void yazdir() const          // const: bu fonksiyon üye değişkenleri değiştirmeyecek
    {
        printf("lat=%.6f lon=%.6f uydu=%d\n", _lat, _lon, _uydu_sayisi);
    }

private:
    // Sadece class'ın kendi fonksiyonları erişebilir. PX4'te de üye değişkenler "_" ile başlıyor.
    // Başlangıç değerleri (= 0.0 gibi) 2. derste ayrıntılı.
    double _lat = 0.0;
    double _lon = 0.0;
    int    _uydu_sayisi = 0;
    bool   _engelli = false;
};

int main()
{
    GpsSim gps;

    gps.guncelle(47.397742, 8.545594, 12);   // C'deki gps_guncelle(&gps, ...) ile aynı
    gps.yazdir();

    gps.engelle(true);
    gps.guncelle(47.400000, 8.600000, 10);   // engelli, değişmemeli
    gps.yazdir();

    // gps._uydu_sayisi = -5;   // derlenmez: _uydu_sayisi private (bkz. hata_ornegi.cpp)

    printf("class boyutu: %zu byte\n", sizeof(GpsSim));
    return 0;
}
