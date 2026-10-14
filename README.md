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
| 5 | kalıtım | base struct'ı gömmek |
| 6 | virtual / override | fonksiyon pointer'ı |
| 7 | template | makroyla kod üretmek |
| 8 | küçük yardımcılar | enum class, constexpr, static_cast, nullptr |
| 9 | bitirme: minik publish/subscribe sistemi | |

## Çalıştırma

Her ders klasöründe `c_versiyon.c`, `cpp_versiyon.cpp`, ve `NOT.md` var.

```bash
./calistir.sh 01-class/c_versiyon.c
./calistir.sh 01-class/cpp_versiyon.cpp
```

`calistir.sh` bilgisayarda g++ varsa onu, yoksa PX4'ün derleme container'ındaki g++'ı kullanıyor. Doğrudan derlemek için:

```bash
g++ -Wall -std=c++17 01-class/cpp_versiyon.cpp -o prog && ./prog
```
