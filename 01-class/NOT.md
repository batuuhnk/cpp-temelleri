# 1. class

## Fikir

C'de veriyi bir `struct`'ta tutup, onu kullanan fonksiyonlara struct'ın adresini veriyordum:

```c
gps_guncelle(&gps, 47.39, 8.54, 12);
```

C++'ta `class` veriyi ve o veriyi kullanan fonksiyonları bir arada tutuyor:

```cpp
gps.guncelle(47.39, 8.54, 12);
```

Arka planda olan şey aynı. Derleyici `guncelle` fonksiyonuna gizli bir parametre ekliyor: nesnenin adresi. C'de bu parametreye `self` diyordum, C++'ta adı `this`. Class'ın içinde `_engelli` yazmak `this->_engelli` yazmakla aynı.

## public ve private

- `public:` altındakiler dışarıdan çağrılabiliyor.
- `private:` altındakilere sadece class'ın kendi fonksiyonları erişebiliyor.

C'de struct'ın içine dışarıdan her şey yazılabiliyordu. C sürümünde `gps.uydu_sayisi = -5;` sorunsuz derlendi ve çalıştı. C++ sürümünde aynı satır derlenmiyor:

```
error: 'int GpsSim::_uydu_sayisi' is private within this context
```

Yani GPS verisi sadece `guncelle()` üzerinden değişebiliyor, ve `guncelle()` engelli durumu kontrol ediyor. Kuralı derleyici uyguluyor; C'de bu sadece bir anlaşmaydı.

## const fonksiyon

`void yazdir() const` sonundaki `const`, bu fonksiyonun üye değişkenleri değiştirmeyeceği anlamına geliyor. C'deki `void gps_yazdir(const GpsSim *self)` ile aynı şey.

## Çıktılar

C (`c_versiyon.c`):
```
lat=47.397742 lon=8.545594 uydu=12
lat=47.397742 lon=8.545594 uydu=12
lat=47.397742 lon=8.545594 uydu=-5
struct boyutu: 24 byte
```

C++ (`cpp_versiyon.cpp`):
```
lat=47.397742 lon=8.545594 uydu=12
lat=47.397742 lon=8.545594 uydu=12
class boyutu: 24 byte
```

- İkinci satırlar değişmedi: GPS engelliyken güncelleme kabul edilmedi.
- C'de üçüncü satırda uydu sayısı -5 oldu, C++'ta bu satır derlenmediği için yok.
- İkisi de 24 byte. Fonksiyonlar nesnenin içinde saklanmıyor, kodda bir kez duruyor; class'a fonksiyon eklemek bellekte yer kaplamıyor. Gömülü tarafta önemli bir nokta.

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

Aynı düzen: `public` kısmında dışarıdan çağrılanlar (`init()`), `private` kısmında iç işler ve `_` ile başlayan üye değişkenler. `: public ...` kısmı kalıtım (5. ders), `override` 6. ders, `<sensor_gps_s>` template (7. ders).
