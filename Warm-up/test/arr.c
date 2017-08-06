#include<stdio.h>

int main(){

    int ar[20] = {1,2,3};  //前3个元素分别是1，2，3
    int *p1 = ar; //将p1指向ar数组的第一个元素的地址，指向的是数组的元素，p1+1的增量为1个元素所占的字节空间
    int (*p2)[20] = &ar; //将p2指向ar数组的地址，即指向整个数组，p2+1的增量为&ar所代表的数组所占的字节空间
    int i;
   
    for(i=0; i<20; i++){
        printf("p1: %d\n", *(p1+i));
        printf("p2: %p\n", *(p2+i));
        printf("p22: %d\n", **(p2+i));
    }
    return 0;
}
