// Template kanalına yanlış tipte veri yayınlamak
#include <cstdint>

struct GpsVeri  { int32_t lat; int32_t lon; uint32_t uydu; };
struct BaroVeri { float basinc; float sicaklik; uint32_t zaman; };

template<typename T>
class Kanal
{
public:
    void yayinla(const T &v) { _veri = v; }
private:
    T _veri{};
};

int main()
{
    Kanal<GpsVeri> gps_kanali;
    BaroVeri baro{1013.25f, 21.5f, 1000};
    gps_kanali.yayinla(baro);    // hata bekliyoruz: GPS kanalına barometre verisi
    return 0;
}
