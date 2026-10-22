// static_assert: derleme sırasında yanlış bir sabiti yakalamak
#include <cstdint>

constexpr uint32_t hz_to_us(uint32_t hz) { return 1000000 / hz; }

constexpr uint32_t GPS_HZ = 7;                     // biri yanlışlıkla 7 yazmış
static_assert(hz_to_us(GPS_HZ) == 125000, "GPS periyodu 125 ms olmali");

int main() { return 0; }
