// C'de iki modül aynı ismi kullanırsa ne oluyor?
#include <stdio.h>

void guncelle(void) { printf("GPS guncellendi\n"); }    // GPS modülü
void guncelle(void) { printf("Baro guncellendi\n"); }   // Barometre modülü, aynı isim

int main(void)
{
    guncelle();
    return 0;
}
