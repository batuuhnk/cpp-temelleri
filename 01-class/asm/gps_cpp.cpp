// STM32 (Cortex-M4) assembly'sini görmek için C++ hali.
#include <cstdint>

class GpsSim
{
public:
    void guncelle(int32_t lat, int32_t lon, uint8_t uydu);

private:
    int32_t _lat = 0;
    int32_t _lon = 0;
    uint8_t _uydu_sayisi = 0;
    bool    _engelli = false;
};

void GpsSim::guncelle(int32_t lat, int32_t lon, uint8_t uydu)
{
    if (_engelli) {
        return;
    }
    _lat = lat;
    _lon = lon;
    _uydu_sayisi = uydu;
}

GpsSim gps;

void ornek()
{
    gps.guncelle(473977420, 85455940, 12);
}
