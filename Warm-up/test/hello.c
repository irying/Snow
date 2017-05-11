#include <stdio.h>
#include "max.c"

int main()
{
    int s,n;
    scanf("%d,%d",&s,&n);
    int maxNum=max(s,n);
    printf("the max num is %d\n",maxNum);
    float v=s/n;
    printf("v=%f\n",v);
    return 0;
}
