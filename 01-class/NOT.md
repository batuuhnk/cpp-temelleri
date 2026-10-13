# 1. class

Bu derste C'deki struct + fonksiyon yapısının C++'taki `class`'a nasıl dönüştüğünü, işlemcinin gerçekte ne yaptığına kadar inerek inceledim. Assembly'yi STM32'deki Cortex-M4 için derleyip baktım (Pixhawk'ları derleyen `arm-none-eabi-gcc` ile).

## 1. Struct bellekte nasıl duruyor?

```c
typedef struct {
    int32_t lat;          // 4 byte
    int32_t lon;          // 4 byte
    uint8_t uydu_sayisi;  // 1 byte
    bool    engelli;      // 1 byte
} GpsSim;
```

Programın yazdırdığı yerleşim:

```
alanlarin konumu: lat +0, lon +4, uydu_sayisi +8, engelli +9, toplam 12 byte
```

Struct, bellekte art arda duran byte'lardan ibaret. Her alanın struct'ın başına göre sabit bir uzaklığı (offset) var. Nesne STM32'de RAM'in `0x20000100` adresinde olsaydı:

```
adres        içerik          alan
0x20000100   4 byte          lat           (+0)
0x20000104   4 byte          lon           (+4)
0x20000108   1 byte          uydu_sayisi   (+8)
0x20000109   1 byte          engelli       (+9)
0x2000010A   2 byte boşluk   (hizalama için, toplam 12 olsun diye)
```

Derleyici `gps.engelli` gördüğünde şunu anlıyor: "nesnenin adresine 9 ekle, oradaki 1 byte'ı oku". Alan isimleri sadece kaynak kodda var; işlemci sadece adres ve uzaklık görüyor.

## 2. `p` (ya da `self`) nedir?

C'de bir struct'ı değiştiren fonksiyona struct'ın kendisini değil **adresini** veriyoruz:

```c
void gps_guncelle(GpsSim *p, int32_t lat, int32_t lon, uint8_t uydu)
{
    if (p->engelli) return;
    p->lat = lat;
    ...
}

gps_guncelle(&gps, 473977420, 85455940, 12);
```

`p` sadece bir parametre adı, özel bir kelime değil. `gps`, `ptr`, `self` da denebilirdi; bazı C kodlarında `self` ya da `me` deniyor. İçinde nesnenin adresi var: `&gps`, yani yukarıdaki örnekte `0x20000100`.

`p->engelli` ifadesi `(*p).engelli` ile aynı: "p'deki adrese git, 9 byte ilerideki alanı al".

## 3. İşlemci bu çağrıyı nasıl yapıyor? (Cortex-M4)

ARM'ın fonksiyon çağırma kuralına (AAPCS) göre ilk 4 parametre sırayla `r0`, `r1`, `r2`, `r3` register'larına konuyor. Fazlası stack'e gidiyor; dönüş değeri `r0`'da geliyor.

`gps_guncelle(&gps, 473977420, 85455940, 12)` çağrısı için:

| Register | Değer | Ne |
|---|---|---|
| `r0` | `0x20000100` | `&gps`, nesnenin adresi (`p`) |
| `r1` | 473977420 | `lat` |
| `r2` | 85455940 | `lon` |
| `r3` | 12 | `uydu` |

`./asm.sh 01-class/asm/gps_c.c` çıktısı (açıklamaları ben ekledim):

**Çağıran taraf:**

```
ornek:
  push  {r3, lr}         dönüş adresini (lr) stack'e sakla
  movs  r3, #12          r3 = 12            (4. parametre: uydu)
  ldr   r2, [pc, #8]     r2 = 0x0517f444    (3. parametre: lon = 85455940)
  ldr   r1, [pc, #12]    r1 = 0x1c40524c    (2. parametre: lat = 473977420)
  ldr   r0, [pc, #12]    r0 = &gps          (1. parametre: nesnenin adresi)
  bl    gps_guncelle     fonksiyona atla, dönüş adresi lr'ye yazılır
  pop   {r3, pc}         geri dön
  .word 0x0517f444       sabit sayılar fonksiyonun hemen arkasında duruyor
  .word 0x1c40524c       (32 bitlik bir sayı tek komuta sığmadığı için
  .word 0x00000000        ldr ile buradan okunuyor; sonuncusu &gps, linker doldurur)
```

**Çağrılan fonksiyon:**

```
gps_guncelle:
  push  {r4}             r4'ü kullanacağım, eski değerini sakla
  ldrb  r4, [r0, #9]     r4 = (r0 + 9) adresindeki 1 byte  → p->engelli
  cbnz  r4, son          r4 sıfır değilse (engelli) "son"a atla
  str   r1, [r0, #0]     (r0 + 0) adresine 4 byte yaz: r1  → p->lat = lat
  str   r2, [r0, #4]     (r0 + 4) adresine 4 byte yaz: r2  → p->lon = lon
  strb  r3, [r0, #8]     (r0 + 8) adresine 1 byte yaz: r3  → p->uydu_sayisi = uydu
son:
  pop   {r4}             r4'ü geri yükle
  bx    lr               çağırana dön
```

Yani işlemci için olan şey şu: nesnenin adresi `r0`'a gidiyor, fonksiyon her alana `r0 + uzaklık` adresinden erişiyor. `[r0, #9]` içindeki 9, bölüm 1'deki `engelli +9` değerinin ta kendisi.

## 4. C++'ta aynısı: `this`

```cpp
class GpsSim
{
public:
    void guncelle(int32_t lat, int32_t lon, uint8_t uydu)
    {
        if (_engelli) return;
        _lat = lat;
        ...
    }
private:
    int32_t _lat = 0;
    int32_t _lon = 0;
    uint8_t _uydu_sayisi = 0;
    bool    _engelli = false;
};

gps.guncelle(473977420, 85455940, 12);
```

Parametre listesinde nesnenin adresi yok, ama derleyici onu **gizlice ilk parametre olarak ekliyor**. Bu gizli parametrenin adı `this`. Yani:

- `gps.guncelle(...)` çağrısı, derleyici için `GpsSim::guncelle(&gps, ...)` demek.
- Fonksiyonun içindeki `_engelli`, derleyici için `this->_engelli` demek. C'deki `p->engelli` ile aynı şey; sadece `p` yerine `this` var ve yazmak zorunda değilim.

Programın çıktısı bunu doğruluyor; nesnenin adresi ile fonksiyon içindeki `this` aynı sayı:

```
gps nesnesinin adresi: 0x7ffc88d08bbc
this = 0x7ffc88d08bbc
_lat +0, _lon +4, _uydu_sayisi +8, _engelli +9
```

(Bu adres bilgisayardaki stack'ten; STM32'de `0x2000....` gibi bir RAM adresi olurdu.)

`./asm.sh 01-class/asm/gps_cpp.cpp` çıktısı, C ile yan yana:

```
C: gps_guncelle               C++: GpsSim::guncelle
  push  {r4}                    push  {r4}
  ldrb  r4, [r0, #9]            ldrb  r4, [r0, #9]      this->_engelli
  cbnz  r4, son                 cbnz  r4, son
  str   r1, [r0, #0]            str   r1, [r0, #0]      this->_lat = lat
  str   r2, [r0, #4]            str   r2, [r0, #4]      this->_lon = lon
  strb  r3, [r0, #8]            strb  r3, [r0, #8]      this->_uydu_sayisi = uydu
  pop   {r4}                    pop   {r4}
  bx    lr                      bx    lr
```

**Birebir aynı.** C++'ta `this` da `r0`'da geliyor, `lat` `r1`'de. Çağıran taraf da aynı: `r0`'a `&gps` yükleyip `bl` ile atlıyor. C++'ın `class`'ı işlemci için yeni bir şey değil; C'de elle yaptığım "adresi ilk parametre olarak ver" işini derleyici benim yerime yapıyor.

### Tek fark: fonksiyonun adı

Assembly'de C fonksiyonunun adı `gps_guncelle`, C++'takinin adı `_ZN6GpsSim8guncelleEllh`. C++ derleyicisi fonksiyonun hangi class'a ait olduğunu ve parametre tiplerini isme gömüyor (name mangling):

```
_Z  N  6GpsSim  8guncelle  E  l     l     h
    │  │        │          │  │     │     └ uint8_t (unsigned char)
    │  │        │          │  └─────┴ int32_t, int32_t (ARM'da long)
    │  │        │          └ isim bitti
    │  │        └ 8 harflik isim: guncelle
    │  └ 6 harflik isim: GpsSim
    └ iç içe isim başlıyor
```

Böylece iki farklı class'ta aynı isimli `guncelle` fonksiyonu olabiliyor, linker onları karıştırmıyor. C'de bu yüzden `gps_guncelle`, `baro_guncelle` diye önek koymak zorundaydım.

## 5. public ve private

- `public:` altındakiler dışarıdan çağrılabiliyor.
- `private:` altındakilere sadece class'ın kendi fonksiyonları erişebiliyor.

C'de `gps.uydu_sayisi = 200;` sorunsuz derlendi; çıktının üçüncü satırında uydu sayısı 200 oldu. C++'ta aynı satır derlenmiyor:

```
error: 'int GpsSim::_uydu_sayisi' is private within this context
```

Bu kontrol **sadece derleme sırasında** yapılıyor. Assembly'de public ya da private'tan hiç iz yok; işlemci için hepsi `r0 + uzaklık`. Yani private çalışma zamanında hiçbir maliyet getirmiyor, sadece yanlış kodun derlenmesini engelliyor.

## 6. const fonksiyon

`void yazdir() const` sonundaki `const`, bu fonksiyonun üye değişkenleri değiştirmeyeceği anlamına geliyor. Derleyici için `this` burada `const GpsSim *` oluyor; C'deki `void gps_yazdir(const GpsSim *p)` ile aynı şey.

## 7. Boyut

İkisi de 12 byte. Fonksiyonlar nesnenin içinde saklanmıyor; flash'ta tek kopya olarak duruyorlar ve hangi nesne üzerinde çalışacaklarını `r0`'daki adresten öğreniyorlar. Class'a fonksiyon eklemek RAM'de yer kaplamıyor.

## Çıktılar

C (`./calistir.sh 01-class/c_versiyon.c`):
```
gps nesnesinin adresi: 0x7ffe86471b3c
alanlarin konumu: lat +0, lon +4, uydu_sayisi +8, engelli +9, toplam 12 byte
lat=47.3977420 lon=8.5455940 uydu=12
lat=47.3977420 lon=8.5455940 uydu=12
lat=47.3977420 lon=8.5455940 uydu=200
```

C++ (`./calistir.sh 01-class/cpp_versiyon.cpp`):
```
gps nesnesinin adresi: 0x7ffc88d08bbc
this = 0x7ffc88d08bbc
_lat +0, _lon +4, _uydu_sayisi +8, _engelli +9
toplam 12 byte
lat=47.3977420 lon=8.5455940 uydu=12
lat=47.3977420 lon=8.5455940 uydu=12
```

- İkinci satırlarda konum değişmedi: GPS engelliyken güncelleme kabul edilmedi (`cbnz` ile atlandı).
- C'de struct'a dışarıdan 200 yazılabildi; C++'ta bu satır derlenmedi.

## PX4'te

`src/modules/simulation/sensor_gps_sim/SensorGpsSim.hpp`:

```cpp
class SensorGpsSim : public ModuleBase<SensorGpsSim>, public ModuleParams, public px4::ScheduledWorkItem
{
public:
	SensorGpsSim();
	bool init();
	...
private:
	void Run() override;
	uORB::PublicationMulti<sensor_gps_s> _sensor_gps_pub{ORB_ID(sensor_gps)};
	...
};
```

Aynı düzen: `public` kısmında dışarıdan çağrılanlar (`init()`), `private` kısmında iç işler ve `_` ile başlayan üye değişkenler. Work queue `Run()` fonksiyonunu çağırırken de arka planda olan şey bu derstekiyle aynı: nesnenin adresi `r0`'da, `Run()` içindeki her `_` ile başlayan değişken `r0 + uzaklık`. `: public ...` kısmı kalıtım (5. ders), `override` 6. ders, `<sensor_gps_s>` template (7. ders).
