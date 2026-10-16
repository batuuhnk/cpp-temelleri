// override yazıp ismi yanlış yazmak
class WorkItem
{
public:
    virtual ~WorkItem() {}
    virtual void Run() = 0;
};

class GpsSim : public WorkItem
{
public:
    void run() override {}      // küçük harf "run": hata bekliyoruz
};

int main()
{
    return 0;
}
