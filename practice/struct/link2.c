#include <stdio.h>
#include <malloc.h>

struct weapon{
    int price;
    int atk;
    struct weapon *next;
};

struct weapon * create(){
    struct weapon *head;
    struct weapon *p1,*p2;
    int n=0;
    p1=p2=(struct weapon*)malloc(sizeof(struct weapon));
    scanf("%d,%d",&p1->price,&p1->atk);
    head=NULL;
    while(p1->price!=0){
     	n++;
	if(n==1) head=p1;
	else p2->next= p1;

        p2=p1;
        p1=(struct weapon*)malloc(sizeof(struct weapon));
	scanf("%d,%d",&p1->price,&p1->atk);
    } 
    p2->next = NULL;
    return (head);
}



int main()
{
    struct weapon *p;
    p=create();
    printf("%d,%d",p->price,p->atk);
    return 0;
}
