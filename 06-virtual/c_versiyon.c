// 6. ders, C hali: her türün fonksiyonlarını bir tabloda tutmak ve nesneye o tablonun adresini koymak.
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct WorkItem WorkItem;

// Fonksiyon tablosu: her tür (GPS, baro) için bir tane, flash'ta sabit duruyor
typedef struct {
    void (*run)(WorkItem *self);
    void (*yok_et)(WorkItem *self);
} WorkItemTablo;

// Temel struct: ilk alanı tablonun adresi
struct WorkItem {
    const WorkItemTablo *tablo;
    const char          *ad;
    uint32_t             calisma_sayisi;
};

// ---- GPS ----
typedef struct {
    WorkItem temel;
    int32_t  lat;
} GpsSim;

static void gps_run(WorkItem *self)
{
    GpsSim *g = (GpsSim *)self;          // temel ilk alan olduğu için adres aynı
    g->temel.calisma_sayisi++;
    g->lat += 10;
    printf("  gps_run:  %u. calisma, lat=%d\n", (unsigned)g->temel.calisma_sayisi, (int)g->lat);
}

static void gps_yok_et(WorkItem *self)
{
    printf("  gps_yok_et (%s)\n", self->ad);
}

static const WorkItemTablo gps_tablosu = { gps_run, gps_yok_et };

void gps_init(GpsSim *g)
{
    g->temel.tablo = &gps_tablosu;
    g->temel.ad = "GPS";
    g->temel.calisma_sayisi = 0;
    g->lat = 473977420;
}

// ---- Barometre ----
typedef struct {
    WorkItem temel;
    float    basinc_hpa;
} BaroSim;

static void baro_run(WorkItem *self)
{
    BaroSim *b = (BaroSim *)self;
    b->temel.calisma_sayisi++;
    b->basinc_hpa -= 0.1f;
    printf("  baro_run: %u. calisma, basinc=%.1f\n", (unsigned)b->temel.calisma_sayisi, b->basinc_hpa);
}

static void baro_yok_et(WorkItem *self)
{
    printf("  baro_yok_et (%s)\n", self->ad);
}

static const WorkItemTablo baro_tablosu = { baro_run, baro_yok_et };

void baro_init(BaroSim *b)
{
    b->temel.tablo = &baro_tablosu;
    b->temel.ad = "BARO";
    b->temel.calisma_sayisi = 0;
    b->basinc_hpa = 1013.2f;
}

int main(void)
{
    GpsSim  gps;
    BaroSim baro;
    gps_init(&gps);
    baro_init(&baro);

    // Kuyruk: hepsi sadece "WorkItem"; hangisinin GPS hangisinin baro olduğunu döngü bilmiyor
    WorkItem *kuyruk[] = { &gps.temel, &baro.temel, &gps.temel };
    printf("Kuyruk calisiyor:\n");
    for (int i = 0; i < 3; i++) {
        WorkItem *w = kuyruk[i];
        w->tablo->run(w);                 // tablodan fonksiyonun adresini al, onu çağır
    }

    printf("gps'in tablosu  %p\n", (const void *)gps.temel.tablo);
    printf("baro'nun tablosu %p\n", (const void *)baro.temel.tablo);

    // Heap'teki bir nesneyi temel pointer üzerinden silmek
    printf("Heap ornegi:\n");
    GpsSim *yeni = malloc(sizeof(GpsSim));
    gps_init(yeni);
    WorkItem *w = &yeni->temel;
    w->tablo->yok_et(w);
    free(yeni);
    return 0;
}
