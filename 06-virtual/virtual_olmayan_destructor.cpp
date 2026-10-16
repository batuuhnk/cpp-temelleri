// Temel class'ın destructor'ı virtual değilse ne oluyor?
#include <cstdio>

class WorkItem
{
public:
    ~WorkItem() { printf("  ~WorkItem\n"); }      // virtual yok
    virtual void Run() = 0;
};

class GpsSim : public WorkItem
{
public:
    ~GpsSim() { printf("  ~GpsSim (GPS portu kapatildi)\n"); }
    void Run() override {}
};

int main()
{
    WorkItem *w = new GpsSim();
    delete w;       // hangi destructor'lar çalışacak?
    return 0;
}
