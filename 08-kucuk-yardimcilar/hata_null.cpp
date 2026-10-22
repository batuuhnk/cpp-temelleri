// C++'ta NULL ile aşırı yüklenmiş (aynı isimli) fonksiyon çağırmak
#include <cstdio>
#include <cstddef>

void kaydet(int x)          { printf("int %d\n", x); }
void kaydet(const char *s)  { printf("metin %s\n", s); }

int main()
{
    kaydet(NULL);    // NULL bir sayı mı, pointer mı?
    return 0;
}
