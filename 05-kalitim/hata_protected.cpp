// protected bir üyeye dışarıdan erişmek
#include <cstdint>

class Sensor
{
protected:
    uint32_t _okuma_sayisi{0};
};

class GpsSim : public Sensor
{
public:
    void sifirla() { _okuma_sayisi = 0; }   // miras alan class erişebilir
};

int main()
{
    GpsSim gps;
    gps.sifirla();
    gps._okuma_sayisi = 5;   // dışarıdan erişilemez, hata bekliyoruz
    return 0;
}
