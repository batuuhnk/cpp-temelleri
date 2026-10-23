# C'den C++'a

C biliyorum, C++ bilmiyordum. PX4 uçuş yazılımı C++ ile yazıldığı için ([PX4 çalışmalarım](https://github.com/batuuhnk/px4-sitl-learning)), PX4 kodunda karşıma çıkan C++ kavramlarını tek tek çalıştım.

Her derste aynı şeyi önce C ile, sonra C++ ile yazdım. Böylece C++'ın getirdiği şeyin C'de neye karşılık geldiğini görebiliyorum. Her derste işlemcinin bellekte ne yaptığını (hangi adresten okuyup nereye yazdığını, kabaca kaç tick sürdüğünü) da not aldım. Her dersin sonunda o kavramın PX4 kodunda nerede geçtiğini de not aldım.

## Dersler

| # | Kavram | C'deki karşılığı |
|---|---|---|
| 1 | [class, public/private](01-class/NOT.md) | struct + nesnenin adresini (pointer) parametre alan fonksiyonlar |
| 2 | [constructor / destructor](02-constructor/NOT.md) | `init()` / `deinit()`, `malloc` + init / deinit + `free` |
| 3 | [referans ve const](03-referans-const/NOT.md) | NULL olamayan pointer, `const` pointer |
| 4 | [namespace](04-namespace/NOT.md) | isimlere modül öneki koymak (`gps_guncelle`) |
| 5 | [kalıtım](05-kalitim/NOT.md) | ortak struct'ı diğer struct'ların ilk alanı olarak gömmek |
| 6 | [virtual / override](06-virtual/NOT.md) | fonksiyon pointer tablosu (`w->tablo->run(w)`) |
| 7 | [template](07-template/NOT.md) | `void*` ile tipten bağımsız kod ya da makroyla her tip için kod üretmek |
| 8 | [küçük yardımcılar](08-kucuk-yardimcilar/NOT.md) | `enum`, `#define`, C tarzı cast, `NULL` yerine: enum class, constexpr, `_ms`, static_cast, nullptr |
| 9 | [bitirme: mini PX4](09-bitirme/NOT.md) | 1–8'in hepsi: work queue, virtual Run(), template'li topic'ler, GPS kaybı |

## Çalıştırma

Her ders klasöründe `c_versiyon.c`, `cpp_versiyon.cpp` ve `NOT.md` var (bitirme dersinde sadece `mini_px4.cpp`).

```bash
./calistir.sh 01-class/c_versiyon.c
./calistir.sh 01-class/cpp_versiyon.cpp
```

`calistir.sh` bilgisayarda g++ varsa onu, yoksa PX4'ün derleme container'ındaki g++'ı kullanıyor. Doğrudan derlemek için:

```bash
g++ -Wall -std=c++17 01-class/cpp_versiyon.cpp -o prog && ./prog
```
