// 8. ders, C++ hali: enum class, constexpr, static_cast, nullptr ve süre literalleri.
#include <cstdio>
#include <cstdint>

// ---- enum class ----
enum class FixTipi  : uint8_t { Yok, Fix2D, Fix3D };   // ": uint8_t" -> 1 byte
enum class UcusModu : uint8_t { Hold, Land };

// ---- constexpr ----
constexpr uint32_t GPS_HZ = 8;

constexpr uint32_t hz_to_us(uint32_t hz)
{
    return 1000000 / hz;
}

constexpr uint32_t GPS_PERIYOT_US = hz_to_us(GPS_HZ);
static_assert(GPS_PERIYOT_US == 125000, "8 Hz 125 ms etmeli");   // derleme sırasında kontrol

// ---- süre literali: PX4'teki 125_ms yazımı ----
constexpr uint64_t operator ""_ms(unsigned long long ms)
{
    return ms * 1000;
}

// ---- aynı isimli iki fonksiyon (C++'ta parametre tipi farklıysa olabiliyor) ----
void kaydet(int x)           { printf("  kaydet(int %d)\n", x); }
void kaydet(const char *s)   { printf("  kaydet(const char* %s)\n", s ? s : "nullptr"); }

struct GpsVeri  { int32_t lat; int32_t lon; };
struct BaroVeri { float basinc; float sicaklik; };

int main()
{
    FixTipi fix = FixTipi::Fix3D;
    if (fix == FixTipi::Fix3D) {
        printf("enum class: fix 3D (sayi degeri %d)\n", static_cast<int>(fix));
    }
    printf("enum class: sizeof(FixTipi) = %zu byte\n", sizeof(FixTipi));

    printf("constexpr: GPS_PERIYOT_US = %u, 125_ms = %llu\n",
           (unsigned)GPS_PERIYOT_US, (unsigned long long)125_ms);

    int dizi[GPS_HZ];      // constexpr bir sabit dizi boyutu olarak kullanılabiliyor
    printf("constexpr: dizi %zu elemanli\n", sizeof(dizi) / sizeof(dizi[0]));

    double lat_derece = 47.3977420;
    int32_t lat = static_cast<int32_t>(lat_derece * 1e7);
    printf("static_cast: lat = %d\n", (int)lat);

    float param1 = 3.9f;                                       // MAVLink komut parametresi gibi
    int birim = static_cast<int>(param1 + 0.5f);               // PX4'teki failure_unit hesabı
    printf("static_cast: param1=%.1f -> birim=%d\n", param1, birim);

    kaydet(0);
    kaydet(nullptr);
    return 0;
}
