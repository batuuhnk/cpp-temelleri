// Hata 1: namespace önekini yazmadan çağırmak
#include <cstdio>

namespace gps  { void guncelle() { printf("GPS\n"); } }
namespace baro { void guncelle() { printf("Baro\n"); } }

int main()
{
    guncelle();             // hangi namespace'teki?
    return 0;
}
