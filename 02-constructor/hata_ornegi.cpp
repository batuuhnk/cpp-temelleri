// Constructor bir parametre istiyorsa, parametresiz nesne oluşturulabilir mi?
#include <cstdint>

class GpsSim
{
public:
    GpsSim(uint8_t hz) : _hz(hz) {}

private:
    uint8_t _hz;
};

int main()
{
    GpsSim gps;   // hz verilmedi, hata bekliyoruz
    return 0;
}
