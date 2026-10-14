// Referansla ilgili iki hata: const referans üzerinden yazmak ve başlatılmamış referans.
#include <cstdint>

struct GpsVeri {
    uint8_t uydu_sayisi;
};

void yazdir_ref(const GpsVeri &v)
{
    v.uydu_sayisi = 0;   // hata 1: v const, değiştirilemez
}

int main()
{
    GpsVeri &r;          // hata 2: referans bir değişkene bağlanmadan oluşturulamaz
    return 0;
}
