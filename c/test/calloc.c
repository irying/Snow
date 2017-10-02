#include<stdio.h>
#include<stdlib.h>

int main()
{
    int i,n;
    int *pData;
    printf("input number:");
    scanf("%d",&i);
    
    pData = (int *)calloc(i,sizeof(int));
    if(pData==NULL) exit(1);

    for(n=0;n<i;n++) {
        printf("input number into arr:");
	scanf("%d",&pData[n]);
    }
    printf ("你输入的数字为：");
    for (n=0;n<i;n++) printf ("%d ",pData[n]);
   
    free (pData);
    system("pause");
    return 0;   
}
