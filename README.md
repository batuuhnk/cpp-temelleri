# C'den C++'a

C biliyorum, C++ bilmiyordum. PX4 uçuş yazılımı C++ ile yazıldığı için ([PX4 çalışmalarım](https://github.com/batuuhnk/px4-sitl-learning)), PX4 kodunda karşıma çıkan C++ kavramlarını tek tek çalıştım.

Her derste aynı şeyi önce C ile, sonra C++ ile yazdım. Böylece C++'ın getirdiği şeyin C'de neye karşılık geldiğini görebiliyorum. İkisini STM32 (Cortex-M4) için derleyip assembly'lerini de karşılaştırdım: işlemci tarafında gerçekte ne değiştiğini görmek için. Her dersin sonunda o kavramın PX4 kodunda nerede geçtiğini de not aldım.

## Dersler

| # | Kavram | C'deki karşılığı |
|---|---|---|
| 1 | [class, public/private](01-class/NOT.md) | struct + `self` pointer'ı alan fonksiyonlar |
| 2 | constructor / destructor | `init()` / `deinit()` |
| 3 | referans ve const | NULL olamayan pointer |
| 4 | namespace | isim önekleri |
| 5 | kalıtım | base struct'ı gömmek |
| 6 | virtual / override | fonksiyon pointer'ı |
| 7 | template | makroyla kod üretmek |
| 8 | küçük yardımcılar | enum class, constexpr, static_cast, nullptr |
| 9 | bitirme: minik publish/subscribe sistemi | |

## Çalıştırma

Her ders klasöründe `c_versiyon.c`, `cpp_versiyon.cpp`, `NOT.md` ve assembly karşılaştırması için `asm/` var.

```bash
./calistir.sh 01-class/c_versiyon.c
./calistir.sh 01-class/cpp_versiyon.cpp
```

STM32 (Cortex-M4) assembly'sini görmek için:

```bash
./asm.sh 01-class/asm/gps_c.c
./asm.sh 01-class/asm/gps_cpp.cpp
```

`calistir.sh` bilgisayarda g++ varsa onu, yoksa PX4'ün derleme container'ındaki g++'ı kullanıyor. `asm.sh` aynı container'daki `arm-none-eabi-gcc`'yi kullanıyor (Pixhawk firmware'ini derleyen derleyici). Doğrudan derlemek için:

```bash
g++ -Wall -std=c++17 01-class/cpp_versiyon.cpp -o prog && ./prog
```
