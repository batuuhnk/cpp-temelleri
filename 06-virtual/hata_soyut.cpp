// Run'ı "= 0" olan bir class'tan doğrudan nesne oluşturmak
class WorkItem
{
public:
    virtual ~WorkItem() {}
    virtual void Run() = 0;
};

int main()
{
    WorkItem w;     // hata bekliyoruz: Run'ı olmayan bir nesne çalıştırılamaz
    return 0;
}
