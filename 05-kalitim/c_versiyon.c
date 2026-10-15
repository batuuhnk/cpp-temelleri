// 5. ders, C hali: ortak alanları bir "temel" struct'a koyup, onu diğer struct'ların ilk alanı olarak gömmek.
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>   // offsetof

// ---- Bütün sensörlerde ortak olan kısım ----
typedef struct {
    const char *ad;
    uint8_t     hz;
    uint32_t    okuma_sayisi;
} Sensor;

void sensor_init(Sensor *s, const char *ad, uint8_t hz)
{
    s->ad = ad;
    s->hz = hz;
    s->okuma_sayisi = 0;
}

void sensor_okuma_say(Sensor *s)
{
    s->okuma_sayisi++;
}

void sensor_bilgi_yazdir(const Sensor *s)
{
    printf("  %-5s %3d Hz, %u okuma\n", s->ad, s->hz, (unsigned)s->okuma_sayisi);
}

// ---- GPS: Sensor + kendi alanları ----
typedef struct {
    Sensor  temel;      // ilk alan olmak zorunda (aşağıdaki listeye bakın)
    int32_t lat;
    int32_t lon;
} GpsSim;

void gps_init(GpsSim *g, uint8_t hz)
{
    sensor_init(&g->temel, "GPS", hz);
    g->lat = 0;
    g->lon = 0;
}

void gps_guncelle(GpsSim *g, int32_t lat, int32_t lon)
{
    g->lat = lat;
    g->lon = lon;
    sensor_okuma_say(&g->temel);
}

// ---- Barometre: Sensor + kendi alanı ----
typedef struct {
    Sensor temel;
    float  basinc_hpa;
} BaroSim;

void baro_init(BaroSim *b, uint8_t hz)
{
    sensor_init(&b->temel, "BARO", hz);
    b->basinc_hpa = 0.0f;
}

void baro_guncelle(BaroSim *b, float basinc)
{
    b->basinc_hpa = basinc;
    sensor_okuma_say(&b->temel);
}

int main(void)
{
    GpsSim  gps;
    BaroSim baro;
    gps_init(&gps, 8);
    baro_init(&baro, 20);

    gps_guncelle(&gps, 473977420, 85455940);
    gps_guncelle(&gps, 473977500, 85456000);
    baro_guncelle(&baro, 1013.2f);

    printf("GpsSim yerlesimi: temel +%zu, lat +%zu, lon +%zu, toplam %zu byte (Sensor kismi %zu byte)\n",
           offsetof(GpsSim, temel), offsetof(GpsSim, lat), offsetof(GpsSim, lon), sizeof(GpsSim), sizeof(Sensor));
    printf("&gps = %p, &gps.temel = %p\n", (void *)&gps, (void *)&gps.temel);

    // Farklı türde sensörleri tek bir listede tutmak: hepsinin başında bir Sensor var
    Sensor *liste[] = { &gps.temel, &baro.temel };
    printf("Butun sensorler:\n");
    for (int i = 0; i < 2; i++) {
        sensor_bilgi_yazdir(liste[i]);
    }
    return 0;
}
