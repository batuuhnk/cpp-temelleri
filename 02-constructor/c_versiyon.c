// 2. ders, C hali: nesneyi kullanmadan önce init, işi bitince deinit elle çağrılmalı.
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>   // malloc, free

// GPS'e ayrılmış kaynak sayısı (gerçekte UART portu, DMA kanalı, bellek gibi bir şey olurdu)
static int acik_gps_sayisi = 0;

typedef struct {
    int32_t lat;
    int32_t lon;
    uint8_t uydu_sayisi;
    uint8_t hz;           // saniyede kaç konum verecek
    bool    engelli;
} GpsSim;

void gps_init(GpsSim *p, uint8_t hz)
{
    p->lat = 0;
    p->lon = 0;
    p->uydu_sayisi = 0;
    p->hz = hz;
    p->engelli = false;
    acik_gps_sayisi++;
    printf("gps_init:   %d Hz GPS acildi,  acik GPS sayisi = %d\n", hz, acik_gps_sayisi);
}

void gps_deinit(GpsSim *p)
{
    acik_gps_sayisi--;
    printf("gps_deinit: %d Hz GPS kapandi, acik GPS sayisi = %d\n", p->hz, acik_gps_sayisi);
}

void gps_yazdir(const GpsSim *p)
{
    printf("GPS: %d Hz, uydu=%d\n", p->hz, p->uydu_sayisi);
}

void ucus(void)
{
    GpsSim ana;
    GpsSim yedek;

    gps_init(&ana, 8);
    gps_init(&yedek, 5);

    gps_yazdir(&ana);
    gps_yazdir(&yedek);

    gps_deinit(&ana);
    // gps_deinit(&yedek);   // unutuldu
}

int main(void)
{
    printf("ucus basliyor\n");
    ucus();
    printf("ucus bitti, acik GPS sayisi = %d\n", acik_gps_sayisi);

    // Heap'te oluşturmak: bellek ayır, sonra init. Silerken önce deinit, sonra bellek geri.
    printf("heap ornegi basliyor\n");
    GpsSim *g = malloc(sizeof(GpsSim));
    gps_init(g, 10);
    gps_yazdir(g);
    gps_deinit(g);
    free(g);
    printf("heap ornegi bitti, acik GPS sayisi = %d\n", acik_gps_sayisi);
    return 0;
}
