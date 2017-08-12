#include<stdio.h>
#include<stdlib.h>

int main()
{
    int input,n;
    int count = 0;
    int *numbers = NULL;
    int *more_numbers = NULL;

    do{
       printf("enter number:");
       scanf("%d", &input);
       count++;

       more_numbers = (int *)realloc(numbers,count * sizeof(int));
       if(more_numbers != NULL){
          numbers = more_numbers;
          numbers[count-1] = input;
       }else {
          free(numbers);
          puts("error");
          exit(1);
       }
    }while(input!=0);

    printf("number enter:");
    for(n=0;n<count;n++) printf("%d", numbers[n]);
    free(numbers);
    system("pause");
    return 0;
}
