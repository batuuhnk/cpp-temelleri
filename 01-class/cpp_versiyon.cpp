// 1. ders, C++ hali: veri ve onu kullanan fonksiyonlar aynı class'ın içinde.
#include <cstdio>
#include <cstdint>

class GpsSim
{
public:
    // C'deki "GpsSim *p" parametresi yazılmıyor. Derleyici onu kendisi ekliyor ve adı "this".
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

    void yazdir() const          // const: bu fonksiyon üye değişkenleri değiştirmeyecek
    {
        printf("lat=%.7f lon=%.7f uydu=%d\n", _lat / 1e7, _lon / 1e7, _uydu_sayisi);
    }

    void adresleri_yazdir() const
    {
        printf("this = %p\n", (const void *)this);
        printf("_lat +%td, _lon +%td, _uydu_sayisi +%td, _engelli +%td\n",
               (const char *)&_lat - (const char *)this, (const char *)&_lon - (const char *)this,
               (const char *)&_uydu_sayisi - (const char *)this, (const char *)&_engelli - (const char *)this);
    }

private:
    // Sadece class'ın kendi fonksiyonları erişebilir. PX4'te de üye değişkenler "_" ile başlıyor.
    // Başlangıç değerleri (= 0 gibi) 2. derste ayrıntılı.
    int32_t _lat = 0;
    int32_t _lon = 0;
    uint8_t _uydu_sayisi = 0;
    bool    _engelli = false;
};

int main()
{
    GpsSim gps;

    printf("gps nesnesinin adresi: %p\n", (void *)&gps);
    gps.adresleri_yazdir();
    printf("toplam %zu byte\n", sizeof(GpsSim));

    gps.guncelle(473977420, 85455940, 12);   // C'deki gps_guncelle(&gps, ...) ile aynı
    gps.yazdir();

    gps.engelle(true);
    gps.guncelle(474000000, 86000000, 10);   // engelli, değişmemeli
    gps.yazdir();

    // gps._uydu_sayisi = 200;   // derlenmez: _uydu_sayisi private (bkz. hata_ornegi.cpp)

    return 0;
}
