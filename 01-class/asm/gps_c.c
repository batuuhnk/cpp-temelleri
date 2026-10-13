// STM32 (Cortex-M4) assembly'sini görmek için C hali.
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int32_t lat;          // derece x 10^7
    int32_t lon;
    uint8_t uydu_sayisi;
    bool    engelli;
} GpsSim;

GpsSim gps;               // global nesne: adresi derleme sonunda belli

void gps_guncelle(GpsSim *p, int32_t lat, int32_t lon, uint8_t uydu)
{
    if (p->engelli) {
        return;
    }
    p->lat = lat;
    p->lon = lon;
    p->uydu_sayisi = uydu;
}

void ornek(void)
{
    gps_guncelle(&gps, 473977420, 85455940, 12);
}
