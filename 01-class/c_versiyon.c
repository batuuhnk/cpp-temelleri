// 1. ders, C hali: veri bir struct'ta, fonksiyonlar dışarıda ve struct'ın adresini parametre olarak alıyor.
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>   // offsetof

typedef struct {
    int32_t lat;          // derece x 10^7 (GPS modülleri genelde böyle gönderiyor)
    int32_t lon;
    uint8_t uydu_sayisi;
    bool    engelli;      // true ise yeni veri kabul edilmez (GPS kesik)
} GpsSim;

// p: struct'ın bellekteki adresi. İsmi önemli değil, "p", "gps", "self" de denebilirdi.
void gps_guncelle(GpsSim *p, int32_t lat, int32_t lon, uint8_t uydu)
{
    if (p->engelli) {     // "p adresinden 9 byte ilerideki değeri oku"
        return;
    }
    p->lat = lat;         // "p adresine lat'ı yaz"
    p->lon = lon;         // "p + 4 adresine lon'u yaz"
    p->uydu_sayisi = uydu;
}

void gps_engelle(GpsSim *p, bool engelli)
{
    p->engelli = engelli;
}

void gps_yazdir(const GpsSim *p)
{
    printf("lat=%.7f lon=%.7f uydu=%d\n", p->lat / 1e7, p->lon / 1e7, p->uydu_sayisi);
}

int main(void)
{
    GpsSim gps = {0};

    printf("gps nesnesinin adresi: %p\n", (void *)&gps);
    printf("alanlarin konumu: lat +%zu, lon +%zu, uydu_sayisi +%zu, engelli +%zu, toplam %zu byte\n",
           offsetof(GpsSim, lat), offsetof(GpsSim, lon),
           offsetof(GpsSim, uydu_sayisi), offsetof(GpsSim, engelli), sizeof(GpsSim));

    gps_guncelle(&gps, 473977420, 85455940, 12);
    gps_yazdir(&gps);

    gps_engelle(&gps, true);
    gps_guncelle(&gps, 474000000, 86000000, 10);   // engelli, değişmemeli
    gps_yazdir(&gps);

    gps.uydu_sayisi = 200;   // C'de kimse durdurmuyor: struct'ın içine doğrudan yazılabiliyor
    gps_yazdir(&gps);

    return 0;
}
