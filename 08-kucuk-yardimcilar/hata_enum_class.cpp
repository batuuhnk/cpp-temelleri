// enum class'ı başka bir enum class'la ve sayıyla karşılaştırmak
#include <cstdint>

enum class FixTipi  : uint8_t { Yok, Fix2D, Fix3D };
enum class UcusModu : uint8_t { Hold, Land };

int main()
{
    FixTipi fix = FixTipi::Fix3D;
    bool a = (fix == UcusModu::Land);   // hata 1: farklı enum'lar
    bool b = (fix == 2);                // hata 2: enum class ile sayı
    return a || b;
}
