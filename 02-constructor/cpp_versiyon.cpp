// 2. ders, C++ hali: constructor nesne oluşurken, destructor nesne yok olurken kendiliğinden çağrılıyor.
#include <cstdio>
#include <cstdint>

static int acik_gps_sayisi = 0;

class GpsSim
{
public:
    // Constructor: adı class'la aynı, dönüş tipi yok.
    // ": _hz(hz)" kısmı başlangıç listesi: _hz alanına hz değerini yazıyor.
    GpsSim(uint8_t hz) : _hz(hz)
    {
        acik_gps_sayisi++;
        printf("constructor: %d Hz GPS acildi,  acik GPS sayisi = %d\n", _hz, acik_gps_sayisi);
    }

    // Destructor: adının başında ~ var, parametresi yok.
    ~GpsSim()
    {
        acik_gps_sayisi--;
        printf("destructor:  %d Hz GPS kapandi, acik GPS sayisi = %d\n", _hz, acik_gps_sayisi);
    }

    void yazdir() const
    {
        printf("GPS: %d Hz, uydu=%d\n", _hz, _uydu_sayisi);
    }

private:
    int32_t _lat{0};          // {0} ile "= 0" aynı şey; PX4 genelde {} kullanıyor
    int32_t _lon{0};
    uint8_t _uydu_sayisi{0};
    uint8_t _hz;              // değerini constructor veriyor
    bool    _engelli{false};
};

void ucus()
{
    GpsSim ana(8);            // burada ana'nın constructor'ı çağrılıyor
    GpsSim yedek(5);          // burada yedek'in constructor'ı çağrılıyor

    ana.yazdir();
    yedek.yazdir();

    printf("ucus fonksiyonunun sonu\n");
}                             // burada önce yedek'in, sonra ana'nın destructor'ı çağrılıyor

int main()
{
    printf("ucus basliyor\n");
    ucus();
    printf("ucus bitti, acik GPS sayisi = %d\n", acik_gps_sayisi);

    // Heap'te oluşturmak: new = bellek ayır + constructor, delete = destructor + belleği geri ver.
    // PX4'te "sensor_gps_sim start" new ile, "sensor_gps_sim stop" delete ile çalışıyor.
    printf("heap ornegi basliyor\n");
    GpsSim *g = new GpsSim(10);
    g->yazdir();
    delete g;
    printf("heap ornegi bitti, acik GPS sayisi = %d\n", acik_gps_sayisi);
    return 0;
}
