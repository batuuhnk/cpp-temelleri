// 3. ders, C++ hali: kopya, const referans ve referans.
#include <cstdio>
#include <cstdint>

struct GpsVeri {
    uint64_t zaman_us;
    int32_t  lat;
    int32_t  lon;
    int32_t  irtifa_mm;
    float    yatay_hata_m;
    float    dikey_hata_m;
    float    hiz_kuzey;
    float    hiz_dogu;
    float    hiz_asagi;
    uint8_t  uydu_sayisi;
    uint8_t  fix_tipi;
};

// 1) Kopyalayarak: C ile aynı
void yazdir_kopya(GpsVeri v)
{
    printf("  yazdir_kopya:   v'nin adresi %p, uydu=%d\n", (void *)&v, v.uydu_sayisi);
    v.uydu_sayisi = 0;   // sadece kopya değişiyor
}

// 2) Sadece okumak için: const referans. NULL kontrolü yok, erişim "." ile.
void yazdir_ref(const GpsVeri &v)
{
    printf("  yazdir_ref:     v'nin adresi %p, uydu=%d\n", (const void *)&v, v.uydu_sayisi);
}

// 3) Değiştirmek için: referans
void sifirla(GpsVeri &v)
{
    v.uydu_sayisi = 0;
    v.fix_tipi = 0;
}

int main()
{
    GpsVeri veri{};
    veri.uydu_sayisi = 12;
    veri.fix_tipi = 3;

    printf("GpsVeri boyutu: %zu byte\n", sizeof(GpsVeri));
    printf("veri'nin adresi:              %p\n", (void *)&veri);

    yazdir_kopya(veri);
    printf("yazdir_kopya sonrasi uydu = %d\n", veri.uydu_sayisi);

    yazdir_ref(veri);          // çağırırken & yazılmıyor, ama kopya da yapılmıyor

    // Referans, var olan bir değişkene ikinci bir isim: aynı adres
    GpsVeri &takma_ad = veri;
    printf("takma_ad'in adresi:           %p\n", (void *)&takma_ad);
    takma_ad.uydu_sayisi = 7;
    printf("takma_ad uzerinden 7 yazildi, veri.uydu = %d\n", veri.uydu_sayisi);

    sifirla(veri);
    printf("sifirla sonrasi uydu = %d, fix = %d\n", veri.uydu_sayisi, veri.fix_tipi);
    return 0;
}
