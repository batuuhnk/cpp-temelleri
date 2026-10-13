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
