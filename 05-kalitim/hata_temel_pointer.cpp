// Sensor pointer'ı üzerinden GpsSim'e özel bir fonksiyonu çağırmak
#include <cstdint>

class Sensor
{
public:
    void bilgi_yazdir() const {}
};

class GpsSim : public Sensor
{
public:
    void guncelle(int32_t lat, int32_t lon) { _lat = lat; _lon = lon; }

private:
    int32_t _lat{0};
    int32_t _lon{0};
};

int main()
{
    GpsSim gps;
    Sensor *s = &gps;
    s->bilgi_yazdir();          // olur: Sensor'da var
    s->guncelle(1, 2);          // hata bekliyoruz: Sensor'da guncelle yok
    return 0;
}
