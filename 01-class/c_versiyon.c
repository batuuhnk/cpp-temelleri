// 1. ders, C hali: veri bir struct'ta, fonksiyonlar dışarıda ve struct'ın adresini parametre olarak alıyor.
#include <stdio.h>
#include <stdbool.h>

typedef struct {
    double lat;
    double lon;
    int    uydu_sayisi;
    bool   engelli;      // true ise yeni veri kabul edilmez (GPS kesik)
} GpsSim;

void gps_guncelle(GpsSim *self, double lat, double lon, int uydu)
{
    if (self->engelli) {
        return;
    }
    self->lat = lat;
    self->lon = lon;
    self->uydu_sayisi = uydu;
}

void gps_engelle(GpsSim *self, bool engelli)
{
    self->engelli = engelli;
}

void gps_yazdir(const GpsSim *self)
{
    printf("lat=%.6f lon=%.6f uydu=%d\n", self->lat, self->lon, self->uydu_sayisi);
}

int main(void)
{
    GpsSim gps = {0};

    gps_guncelle(&gps, 47.397742, 8.545594, 12);
    gps_yazdir(&gps);

    gps_engelle(&gps, true);
    gps_guncelle(&gps, 47.400000, 8.600000, 10);   // engelli, değişmemeli
    gps_yazdir(&gps);

    gps.uydu_sayisi = -5;   // C'de kimse durdurmuyor: struct'ın içine doğrudan yazılabiliyor
    gps_yazdir(&gps);

    printf("struct boyutu: %zu byte\n", sizeof(GpsSim));
    return 0;
}
