#include <stdio.h>
struct weapon{
    int price;
    int atk;
    struct weapon *next;
};

int main()
{
    struct weapon a,b,c,*head;
    a.price = 100;
    a.atk = 200;
    b.price = 260;
    b.atk = 300;
    c.price = 310;
    c.atk = 500;
    head = &a;
    a.next = &b;
    b.next = &c;
    c.next = NULL;

    struct weapon *p;
    p = head;
    while(p != NULL){
      	printf("%d,%d\n",p->atk,p->price);
        p=p->next;
    }

    return 0;
}
