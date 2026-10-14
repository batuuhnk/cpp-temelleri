// Hata 2: aynı isimleri içeren iki namespace'i birden açmak
#include <cstdio>

namespace gps  { void guncelle() { printf("GPS\n"); } }
namespace baro { void guncelle() { printf("Baro\n"); } }

int main()
{
    using namespace gps;
    using namespace baro;
    guncelle();             // ikisi de açık, hangisi?
    return 0;
}
