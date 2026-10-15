// 5. ders, C++ hali: GpsSim ve BaroSim, Sensor'dan miras alıyor.
#include <cstdio>
#include <cstdint>

// ---- Temel class: bütün sensörlerde ortak olan kısım ----
class Sensor
{
public:
    Sensor(const char *ad, uint8_t hz) : _ad(ad), _hz(hz)
    {
        printf("  Sensor constructor (%s)\n", _ad);
    }

    ~Sensor()
    {
        printf("  Sensor destructor (%s)\n", _ad);
    }

    void bilgi_yazdir() const
    {
        printf("  %-5s %3d Hz, %u okuma\n", _ad, _hz, (unsigned)_okuma_sayisi);
    }

protected:
    // protected: dışarıdan erişilemez, ama bu class'tan miras alanlar erişebilir
    void okuma_say() { _okuma_sayisi++; }

    const char *_ad;
    uint8_t     _hz;
    uint32_t    _okuma_sayisi{0};
};

// ---- GpsSim, Sensor'dan miras alıyor ----
class GpsSim : public Sensor
{
public:
    GpsSim(uint8_t hz) : Sensor("GPS", hz)   // önce temel class'ın constructor'ı
    {
        printf("  GpsSim constructor\n");
    }

    ~GpsSim()
    {
        printf("  GpsSim destructor\n");
    }

    void guncelle(int32_t lat, int32_t lon)
    {
        _lat = lat;
        _lon = lon;
        okuma_say();       // Sensor'dan miras kalan fonksiyon
    }

    void adres_yazdir() const
    {
        printf("this = %p, _lat'in adresi = %p (this + %td)\n",
               (const void *)this, (const void *)&_lat, (const char *)&_lat - (const char *)this);
    }

private:
    int32_t _lat{0};
    int32_t _lon{0};
};

// ---- BaroSim, Sensor'dan miras alıyor ----
class BaroSim : public Sensor
{
public:
    BaroSim(uint8_t hz) : Sensor("BARO", hz) {}

    void guncelle(float basinc)
    {
        _basinc_hpa = basinc;
        okuma_say();
    }

private:
    float _basinc_hpa{0.0f};
};

int main()
{
    printf("Nesneler olusturuluyor:\n");
    GpsSim  gps(8);
    BaroSim baro(20);

    gps.guncelle(473977420, 85455940);
    gps.guncelle(473977500, 85456000);
    baro.guncelle(1013.2f);

    printf("GpsSim boyutu %zu byte, Sensor boyutu %zu byte\n", sizeof(GpsSim), sizeof(Sensor));
    gps.adres_yazdir();

    // GpsSim'in adresini Sensor pointer'ına çevirmek: aynı adres
    Sensor *s = &gps;
    printf("&gps = %p, Sensor olarak = %p\n", (void *)&gps, (void *)s);

    // Farklı türde sensörleri tek bir listede tutmak
    Sensor *liste[] = { &gps, &baro };
    printf("Butun sensorler:\n");
    for (Sensor *sensor : liste) {
        sensor->bilgi_yazdir();
    }

    printf("main bitiyor:\n");
    return 0;
}
