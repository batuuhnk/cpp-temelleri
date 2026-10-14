// Başlangıç listesine her alanı yazmak gerekiyor mu?
#include <cstdio>
#include <cstdint>

class GpsSim
{
public:
    // Sadece dışarıdan gelen değer listede. Diğer alanlar tanımdaki değeri alıyor.
    GpsSim(uint8_t guncelleme_hizi) : _hz(guncelleme_hizi) {}

    // Aynı sonuç, liste yerine gövdede atama ile
    GpsSim(uint8_t hz, int32_t lat) { _hz = hz; _lat = lat; }

    void yazdir() const
    {
        printf("lat=%d lon=%d uydu=%d hz=%d engelli=%d\n", (int)_lat, (int)_lon, _uydu_sayisi, _hz, _engelli);
    }

private:
    int32_t _lat{0};
    int32_t _lon{0};
    uint8_t _uydu_sayisi{0};
    uint8_t _hz{1};          // varsayılan 1 Hz; constructor isterse değiştiriyor
    bool    _engelli{false};
};

class SiraOrnegi
{
public:
    SiraOrnegi() : _b(1), _a(_b + 1) {}   // listede önce _b yazdım ama class'ta önce _a tanımlı

private:
    int _a;
    int _b;
};

int main()
{
    GpsSim a(8);
    GpsSim b(5, 473977420);
    a.yazdir();
    b.yazdir();
    return 0;
}
