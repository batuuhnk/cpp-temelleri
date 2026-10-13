# 1. class

Bu derste C'deki struct + fonksiyon yapısının C++'taki `class`'a nasıl dönüştüğünü ve işlemcinin bellekte gerçekte ne yaptığını inceledim.

## 1. Struct bellekte nasıl duruyor?

```c
typedef struct {
    int32_t lat;          // 4 byte, derece x 10^7
    int32_t lon;          // 4 byte
    uint8_t uydu_sayisi;  // 1 byte
    bool    engelli;      // 1 byte
} GpsSim;
```

Programın yazdırdığı yerleşim:

```
alanlarin konumu: lat +0, lon +4, uydu_sayisi +8, engelli +9, toplam 12 byte
```

Struct, bellekte art arda duran byte'lardan ibaret. Her alanın struct'ın başına göre sabit bir uzaklığı var. Nesne STM32'de RAM'in `0x20000100` adresinde olsaydı:

```
adres        alan            uzaklık
0x20000100   lat             +0   (4 byte)
0x20000104   lon             +4   (4 byte)
0x20000108   uydu_sayisi     +8   (1 byte)
0x20000109   engelli         +9   (1 byte)
0x2000010A   boş             2 byte, toplam 12 olsun diye (hizalama)
```

Alan isimleri sadece kaynak kodda var. Derleyici `engelli` gördüğünde onu "nesnenin adresi + 9" diye çeviriyor; işlemci sadece adres görüyor.

## 2. `p` nedir?

C'de struct'ı değiştiren fonksiyona struct'ın kendisini değil **adresini** veriyoruz:

```c
void gps_guncelle(GpsSim *p, int32_t lat, int32_t lon, uint8_t uydu)
{
    if (p->engelli) return;
    p->lat = lat;
    p->lon = lon;
    p->uydu_sayisi = uydu;
}

gps_guncelle(&gps, 473977420, 85455940, 12);
```

`p` özel bir kelime değil, sadece parametrenin adı (`gps`, `ptr`, `self` de denebilirdi). İçinde nesnenin adresi var, örnekte `0x20000100`. `p->engelli` demek "p'deki adrese git, 9 byte ileriye bak" demek.

## 3. İşlemci bu çağrıda ne yapıyor?

İşlemcinin içinde register denen birkaç hızlı saklama yeri var. İşlemci hesap yaparken veriyi önce register'a alıyor, işini yapıyor, sonucu belleğe yazıyor. ARM'da (STM32) bir fonksiyonun ilk 4 parametresi sırayla ilk 4 register'a konuyor.

`gps_guncelle(&gps, 473977420, 85455940, 12)` çağrısı adım adım:

| Adım | İşlemci ne yapıyor | Yaklaşık süre |
|---|---|---|
| 1 | 4 parametreyi register'lara koyuyor: 1. register'a `&gps` (`0x20000100`), 2.'ye lat, 3.'ye lon, 4.'ye 12 | her biri ~1–2 tick |
| 2 | Dönüş adresini saklayıp fonksiyona atlıyor | ~2–4 tick |
| 3 | `p->engelli`: `0x20000100 + 9 = 0x20000109` adresinden 1 byte okuyup bir register'a alıyor | ~2 tick (bellekten okuma) |
| 4 | Okunan değer 0 değilse (engelli) fonksiyondan çıkıyor | ~1 tick, atlarsa birkaç tick daha |
| 5 | `p->lat = lat`: lat'ı register'dan `0x20000100` adresine yazıyor | ~1 tick |
| 6 | `p->lon = lon`: `0x20000104` adresine yazıyor | ~1 tick |
| 7 | `p->uydu_sayisi = uydu`: `0x20000108` adresine 1 byte yazıyor | ~1 tick |
| 8 | Saklanan dönüş adresine geri dönüyor | ~2–4 tick |

Toplam 15 tick civarı. Pixhawk 6X'teki gibi 480 MHz'lik bir STM32H7'de bu 0.03 µs ediyor. Süreler Cortex-M'de RAM'e bekleme olmadan erişildiği durum için yaklaşık değerler.

Dikkat çeken şey: lat, lon ve uydu ayrı ayrı kopyalanmıyor, register'da hazır duran değer doğrudan nesnenin içindeki yerine yazılıyor. Her alan için tek bir yazma işlemi var.

## 4. C++'ta aynısı: `this`

```cpp
class GpsSim
{
public:
    void guncelle(int32_t lat, int32_t lon, uint8_t uydu)
    {
        if (_engelli) return;
        _lat = lat;
        _lon = lon;
        _uydu_sayisi = uydu;
    }
private:
    int32_t _lat = 0;
    int32_t _lon = 0;
    uint8_t _uydu_sayisi = 0;
    bool    _engelli = false;
};

gps.guncelle(473977420, 85455940, 12);
```

Parametre listesinde nesnenin adresi yok, ama derleyici onu **gizlice ilk parametre olarak ekliyor**. Bu gizli parametrenin adı `this`:

- `gps.guncelle(...)` çağrısı, derleyici için `guncelle(&gps, ...)` demek.
- Fonksiyonun içindeki `_engelli`, derleyici için `this->_engelli` demek. C'deki `p->engelli` ile aynı; `p` yerine `this` var ve yazmak zorunda değilim.

Programın çıktısı bunu doğruluyor; nesnenin adresi ile fonksiyon içindeki `this` aynı sayı:

```
gps nesnesinin adresi: 0x7ffc88d08bbc
this = 0x7ffc88d08bbc
_lat +0, _lon +4, _uydu_sayisi +8, _engelli +9
```

(Bu adres bilgisayardaki stack'ten; STM32'de `0x2000....` gibi bir RAM adresi olurdu.)

Bu yüzden işlemci tarafında 3. bölümdeki tablo C++ için de aynen geçerli: adres 1. register'a gidiyor, `_engelli` adres + 9'dan okunuyor, alanlar aynı adreslere yazılıyor. **C++'ın class'ı ekstra tick harcamıyor.** İkisini STM32 için derleyip karşılaştırdım; işlemciye giden komutlar birebir aynı çıktı.

İleride C++'ın gerçekten ekstra maliyet getirdiği bir yer göreceğim: `virtual` fonksiyonlar (6. ders). Orada fonksiyonun adresini bulmak için bellekten bir okuma daha yapılıyor.

## 5. public ve private

- `public:` altındakiler dışarıdan çağrılabiliyor.
- `private:` altındakilere sadece class'ın kendi fonksiyonları erişebiliyor.

C'de `gps.uydu_sayisi = 200;` sorunsuz derlendi; çıktının üçüncü satırında uydu sayısı 200 oldu. C++'ta aynı satır derlenmiyor:

```
error: 'int GpsSim::_uydu_sayisi' is private within this context
```

Bu kontrol **sadece derleme sırasında** yapılıyor. Program çalışırken işlemci için public ya da private diye bir şey yok; ikisi de "adres + uzaklık". Yani private hiç tick harcamıyor, sadece yanlış kodun derlenmesini engelliyor.

## 6. const fonksiyon

`void yazdir() const` sonundaki `const`, bu fonksiyonun üye değişkenleri değiştirmeyeceği anlamına geliyor. C'deki `void gps_yazdir(const GpsSim *p)` ile aynı şey. Bu da sadece derleme sırasında kontrol ediliyor.

## 7. Boyut

İkisi de 12 byte. Fonksiyonlar nesnenin içinde saklanmıyor; flash'ta tek kopya olarak duruyorlar ve hangi nesne üzerinde çalışacaklarını kendilerine verilen adresten öğreniyorlar. Class'a fonksiyon eklemek RAM'de yer kaplamıyor. Aynı class'tan 10 nesne oluşturursam RAM'de 10 × 12 byte yer tutuyor, fonksiyonlar flash'ta yine tek kopya.

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

- İkinci satırlarda konum değişmedi: GPS engelliyken 4. adımda fonksiyondan çıkıldı.
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

Aynı düzen: `public` kısmında dışarıdan çağrılanlar (`init()`), `private` kısmında iç işler ve `_` ile başlayan üye değişkenler. Work queue `Run()` fonksiyonunu çağırırken de arka planda olan şey bu derstekiyle aynı: nesnenin adresi `this` olarak veriliyor, `Run()` içindeki her `_` ile başlayan değişken "this + uzaklık" adresinden okunuyor. `: public ...` kısmı kalıtım (5. ders), `override` 6. ders, `<sensor_gps_s>` template (7. ders).
