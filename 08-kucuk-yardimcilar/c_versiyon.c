// 8. ders, C hali: enum, #define, C tarzı cast, NULL ve süre makroları.
#include <stdio.h>
#include <stdint.h>

// ---- enum ----
enum FixTipi   { FIX_YOK, FIX_2D, FIX_3D };
enum UcusModu  { MOD_HOLD, MOD_LAND };

// ---- sabitler ve süreler ----
#define GPS_HZ            8
#define GPS_PERIYOT_US    (1000000 / GPS_HZ)
#define MS(x)             ((uint64_t)(x) * 1000)    // milisaniyeyi mikrosaniyeye çevir

// ---- NULL ile iki "farklı" fonksiyon (C'de aynı isimle olamıyor, ayrı isim gerekiyor) ----
void kaydet_sayi(int x)           { printf("  kaydet_sayi(%d)\n", x); }
void kaydet_metin(const char *s)  { printf("  kaydet_metin(%s)\n", s ? s : "NULL"); }

typedef struct { int32_t lat; int32_t lon; } GpsVeri;
typedef struct { float basinc; float sicaklik; } BaroVeri;

int main(void)
{
    enum FixTipi fix = FIX_2D;

    // enum aslında int: farklı bir enum'la ve sayıyla karşılaştırılabiliyor
    if (fix == MOD_LAND) {
        printf("enum: fix == MOD_LAND dogru cikti (fix=%d, MOD_LAND=%d)\n", fix, MOD_LAND);
    }
    printf("enum: sizeof(enum FixTipi) = %zu byte\n", sizeof(enum FixTipi));

    // #define: önişlemci metin olarak yerleştiriyor
    printf("define: GPS_PERIYOT_US = %d, MS(125) = %llu\n", GPS_PERIYOT_US, (unsigned long long)MS(125));

    // C tarzı cast: sayıyı çevirmek için de, pointer'ı başka tipe çevirmek için de aynı yazım
    double lat_derece = 47.3977420;
    int32_t lat = (int32_t)(lat_derece * 1e7);
    printf("cast: lat = %d\n", (int)lat);

    BaroVeri baro = { 1013.25f, 21.5f };
    GpsVeri *yanlis = (GpsVeri *)&baro;     // barometreyi GPS gibi okumak: C bunu sorunsuz kabul ediyor
    printf("cast: yanlis->lat = %d\n", (int)yanlis->lat);

    // NULL
    kaydet_sayi(0);
    kaydet_metin(NULL);
    return 0;
}
