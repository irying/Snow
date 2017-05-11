#include <stdio.h>
#define COUNTOFKEY 10
#define N(n) n*10
#define ADD(a,b) (a+b)  // add ()

int main()
{

    int a=COUNTOFKEY;
    int b=N(a);
    printf("b=%d\n",b);
    float s,n;
    scanf("%f,%f",&s,&n);
    float sum = ADD(s,n);
    printf("sum=%f\n",sum);
    return 0;
}
