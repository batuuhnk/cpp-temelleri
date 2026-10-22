// static_cast ile birbiriyle ilgisi olmayan pointer tiplerini çevirmek
#include <cstdint>

struct GpsVeri  { int32_t lat; int32_t lon; };
struct BaroVeri { float basinc; float sicaklik; };

int main()
{
    BaroVeri baro{1013.25f, 21.5f};
    GpsVeri *g = static_cast<GpsVeri *>(&baro);   // hata bekliyoruz
    return g->lat;
}
