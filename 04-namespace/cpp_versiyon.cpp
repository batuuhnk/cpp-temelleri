// 4. ders, C++ hali: her modül kendi namespace'inde, isimler çakışmıyor.
#include <cstdio>

namespace gps
{
const int HZ = 8;
int okuma_sayisi = 0;

void guncelle()
{
    okuma_sayisi++;      // aynı namespace'in içinden önek yazmaya gerek yok
    printf("gps::guncelle:  %d. okuma (%d Hz)\n", okuma_sayisi, HZ);
}
} // namespace gps

namespace baro
{
const int HZ = 20;
int okuma_sayisi = 0;

void guncelle()
{
    okuma_sayisi++;
    printf("baro::guncelle: %d. okuma (%d Hz)\n", okuma_sayisi, HZ);
}
} // namespace baro

// İç içe namespace
namespace sensorler
{
namespace imu
{
const int HZ = 250;

void guncelle()
{
    printf("sensorler::imu::guncelle (%d Hz)\n", HZ);
}
} // namespace imu
} // namespace sensorler

int main()
{
    gps::guncelle();
    gps::guncelle();
    baro::guncelle();
    sensorler::imu::guncelle();
    printf("gps::okuma_sayisi = %d, baro::okuma_sayisi = %d\n", gps::okuma_sayisi, baro::okuma_sayisi);

    {
        using namespace baro;   // bu blok boyunca baro:: yazmadan kullanabilirim
        guncelle();
        printf("bu blokta HZ = %d\n", HZ);
    }

    return 0;
}
