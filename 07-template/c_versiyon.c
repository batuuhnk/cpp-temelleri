// 7. ders, C hali: farklı veri tipleri için aynı işi yapan kanal. İki yol: void* ve makro.
#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef struct { int32_t lat; int32_t lon; uint32_t uydu; } GpsVeri;     // 12 byte
typedef struct { float basinc; float sicaklik; uint32_t zaman; } BaroVeri; // 12 byte

// ---- Yol 1: void* ile tek bir kanal türü, her tipe çalışır ama tipi kontrol etmez ----
typedef struct {
    uint8_t  veri[64];     // en büyük mesaja yetecek kadar ham byte
    size_t   boyut;
    uint32_t sayac;
} KanalHam;

void kanal_ham_yayinla(KanalHam *k, const void *veri)
{
    memcpy(k->veri, veri, k->boyut);   // kaç byte olduğunu kanal biliyor, ne olduğunu bilmiyor
    k->sayac++;
}

void kanal_ham_oku(const KanalHam *k, void *hedef)
{
    memcpy(hedef, k->veri, k->boyut);
}

// ---- Yol 2: makro ile her tip için ayrı kod üretmek ----
#define KANAL_TANIMLA(TIP)                                              \
    typedef struct { TIP veri; uint32_t sayac; } Kanal_##TIP;           \
    static void kanal_##TIP##_yayinla(Kanal_##TIP *k, const TIP *v)     \
    {                                                                   \
        k->veri = *v;                                                   \
        k->sayac++;                                                     \
    }                                                                   \
    static void kanal_##TIP##_oku(const Kanal_##TIP *k, TIP *hedef)     \
    {                                                                   \
        *hedef = k->veri;                                               \
    }

KANAL_TANIMLA(GpsVeri)    // Kanal_GpsVeri, kanal_GpsVeri_yayinla, kanal_GpsVeri_oku oluşur
KANAL_TANIMLA(BaroVeri)   // Kanal_BaroVeri, kanal_BaroVeri_yayinla, kanal_BaroVeri_oku oluşur

int main(void)
{
    GpsVeri  gps  = { 473977420, 85455940, 12 };
    BaroVeri baro = { 1013.25f, 21.5f, 1000 };

    // Yol 1
    KanalHam gps_ham = { .boyut = sizeof(GpsVeri) };
    kanal_ham_yayinla(&gps_ham, &gps);
    GpsVeri okunan;
    kanal_ham_oku(&gps_ham, &okunan);
    printf("void* kanal:  lat=%d uydu=%u (sayac=%u)\n", (int)okunan.lat, (unsigned)okunan.uydu, (unsigned)gps_ham.sayac);

    kanal_ham_yayinla(&gps_ham, &baro);   // YANLIŞ: GPS kanalına barometre verisi. Derleyici bir şey demiyor.
    kanal_ham_oku(&gps_ham, &okunan);
    printf("yanlis tip:   lat=%d uydu=%u (sayac=%u)\n", (int)okunan.lat, (unsigned)okunan.uydu, (unsigned)gps_ham.sayac);

    // Yol 2
    Kanal_GpsVeri  gps_kanali  = {0};
    Kanal_BaroVeri baro_kanali = {0};
    kanal_GpsVeri_yayinla(&gps_kanali, &gps);
    kanal_BaroVeri_yayinla(&baro_kanali, &baro);
    GpsVeri  g;
    BaroVeri b;
    kanal_GpsVeri_oku(&gps_kanali, &g);
    kanal_BaroVeri_oku(&baro_kanali, &b);
    printf("makro kanal:  lat=%d uydu=%u, basinc=%.2f sicaklik=%.1f\n", (int)g.lat, (unsigned)g.uydu, b.basinc, b.sicaklik);
    return 0;
}
