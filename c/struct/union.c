#include <stdio.h>
struct data{
    int a;
    char b;
    int c;
};
union data2{
    int a;
    char b;
    int c;
};
int main()
{
    union data2 data_1;
    data_1.b = 'c';
    data_1.a = 10;
    printf("%lu",sizeof(struct data));
    printf("%p\n,%p\n.%p\n",&data_1.a,&data_1.b,&data_1.c);
    printf("%d\n",data_1.b);

}
