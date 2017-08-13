#include <stdio.h>
#include "max.h"
#include "min.h"

int main()
{
    int s,n;
    scanf("%d,%d",&s,&n);
    int maxNum=max(s,n);
    int minNum=min(s,n);
    printf("the max num is %d\n",maxNum);
    printf("the min num is %d\n",minNum);
    float v=s/n;
    printf("v=%f\n",v);
    return 0;
}
