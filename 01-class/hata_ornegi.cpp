// private bir üyeye dışarıdan yazmaya çalışınca derleyici ne diyor?
#include <cstdio>

class GpsSim
{
public:
    void yazdir() const { printf("uydu=%d\n", _uydu_sayisi); }

private:
    int _uydu_sayisi = 0;
};

int main()
{
    GpsSim gps;
    gps._uydu_sayisi = -5;   // hata bekliyoruz
    gps.yazdir();
    return 0;
}
