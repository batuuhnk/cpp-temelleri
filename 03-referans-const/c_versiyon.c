// 3. ders, C hali: bir struct'ı fonksiyona vermenin iki yolu var: kopyalayarak ya da adresini vererek.
#include <stdio.h>
#include <stdint.h>

// PX4'teki sensor_gps mesajına benzeyen, biraz büyük bir struct
typedef struct {
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
} GpsVeri;

// 1) Kopyalayarak: fonksiyon struct'ın bir kopyasını alıyor
void yazdir_kopya(GpsVeri v)
{
    printf("  yazdir_kopya:   v'nin adresi %p, uydu=%d\n", (void *)&v, v.uydu_sayisi);
    v.uydu_sayisi = 0;   // sadece kopya değişiyor
}

// 2) Adresini vererek, sadece okumak için: const pointer
void yazdir_pointer(const GpsVeri *v)
{
    if (v == NULL) {
        printf("  yazdir_pointer: NULL geldi, hicbir sey yapilmadi\n");
        return;
    }
    printf("  yazdir_pointer: v'nin degeri  %p, uydu=%d\n", (const void *)v, v->uydu_sayisi);
}

// 3) Adresini vererek, değiştirmek için: pointer
void sifirla(GpsVeri *v)
{
    if (v == NULL) {
        return;
    }
    v->uydu_sayisi = 0;
    v->fix_tipi = 0;
}

int main(void)
{
    GpsVeri veri = {0};
    veri.uydu_sayisi = 12;
    veri.fix_tipi = 3;

    printf("GpsVeri boyutu: %zu byte\n", sizeof(GpsVeri));
    printf("veri'nin adresi:              %p\n", (void *)&veri);

    yazdir_kopya(veri);
    printf("yazdir_kopya sonrasi uydu = %d\n", veri.uydu_sayisi);

    yazdir_pointer(&veri);
    yazdir_pointer(NULL);

    sifirla(&veri);
    printf("sifirla sonrasi uydu = %d, fix = %d\n", veri.uydu_sayisi, veri.fix_tipi);
    return 0;
}
