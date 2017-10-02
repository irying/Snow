#include <stdio.h>
typedef char (*PTR_TO_ARR)[30];
typedef int (*PTR_TO_FUNC)(int, int);
int max(int a, int b){
    return a>b ? a : b;
}
char str[30] = {'1','2','3'};
int main(){
    PTR_TO_ARR parr = &str;
    PTR_TO_FUNC pfunc = max;
    int i;
   
    printf("max: %d\n", (*pfunc)(10, 20));
    for(i=0; i<3; i++){
        printf("str[%d]: %c\n", i, (parr[0])[i]);
    }
    return 0;
}
