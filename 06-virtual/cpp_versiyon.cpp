// 6. ders, C++ hali: virtual fonksiyonlar. Tabloyu derleyici kendisi oluşturuyor.
#include <cstdio>
#include <cstdint>

class WorkItem
{
public:
    WorkItem(const char *ad) : _ad(ad) {}

    virtual ~WorkItem()
    {
        printf("  ~WorkItem (%s)\n", _ad);
    }

    // "= 0": WorkItem'ın kendi Run'ı yok, miras alan her class yazmak zorunda
    virtual void Run() = 0;

protected:
    const char *_ad;
    uint32_t    _calisma_sayisi{0};
};

class GpsSim : public WorkItem
{
public:
    GpsSim() : WorkItem("GPS") {}

    ~GpsSim() override
    {
        printf("  ~GpsSim\n");
    }

    void Run() override
    {
        _calisma_sayisi++;
        _lat += 10;
        printf("  GpsSim::Run:  %u. calisma, lat=%d\n", (unsigned)_calisma_sayisi, (int)_lat);
    }

private:
    int32_t _lat{473977420};
};

class BaroSim : public WorkItem
{
public:
    BaroSim() : WorkItem("BARO") {}

    void Run() override
    {
        _calisma_sayisi++;
        _basinc_hpa -= 0.1f;
        printf("  BaroSim::Run: %u. calisma, basinc=%.1f\n", (unsigned)_calisma_sayisi, _basinc_hpa);
    }

private:
    float _basinc_hpa{1013.2f};
};

int main()
{
    GpsSim  gps;
    GpsSim  gps2;
    BaroSim baro;

    // Kuyruk: PX4'teki IntrusiveQueue<WorkItem *> gibi
    WorkItem *kuyruk[] = { &gps, &baro, &gps };
    printf("Kuyruk calisiyor:\n");
    for (WorkItem *work : kuyruk) {
        work->Run();                      // PX4'teki WorkQueue.cpp: work->Run();
    }

    // Nesnenin ilk 8 byte'ı (bu bilgisayarda bir adres): derleyicinin gizlice koyduğu tablo adresi
    printf("gps'in tablosu   %p\n", *(void **)&gps);
    printf("gps2'nin tablosu %p\n", *(void **)&gps2);
    printf("baro'nun tablosu %p\n", *(void **)&baro);
    printf("sizeof(WorkItem) = %zu, sizeof(GpsSim) = %zu\n", sizeof(WorkItem), sizeof(GpsSim));

    // Heap'teki bir nesneyi temel pointer üzerinden silmek
    printf("Heap ornegi:\n");
    WorkItem *w = new GpsSim();
    delete w;

    printf("main bitiyor:\n");
    return 0;
}
