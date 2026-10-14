// 4. ders, C hali: iki modülde aynı isimli fonksiyon olamıyor, isimlere önek koymak gerekiyor.
#include <stdio.h>

// ---- GPS modülü ----
#define GPS_HZ 8
static int gps_okuma_sayisi = 0;

void gps_guncelle(void)
{
    gps_okuma_sayisi++;
    printf("gps_guncelle:  %d. okuma (%d Hz)\n", gps_okuma_sayisi, GPS_HZ);
}

// ---- Barometre modülü ----
#define BARO_HZ 20
static int baro_okuma_sayisi = 0;

void baro_guncelle(void)
{
    baro_okuma_sayisi++;
    printf("baro_guncelle: %d. okuma (%d Hz)\n", baro_okuma_sayisi, BARO_HZ);
}

int main(void)
{
    gps_guncelle();
    gps_guncelle();
    baro_guncelle();
    printf("gps_okuma_sayisi = %d, baro_okuma_sayisi = %d\n", gps_okuma_sayisi, baro_okuma_sayisi);
    return 0;
}
