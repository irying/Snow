#include <stdio.h>
struct weapon{
    char name[20];
    int atk;
    int price;
};
// 2.global var
struct power{
    char name[10];
}power;

int main()
{
    struct weapon weapon_1 = {"first",100,200};
    struct weapon weapon_2[2] = {{"one",600,200},{"two",900,300}};
    struct weapon *w=&weapon_1; 
    struct weapon *p=weapon_2;
    printf("%s\n,%d\n",(*w).name,++(*w).atk);
    printf("%s\n,%d\n",weapon_1.name,++weapon_1.atk);
    printf("%s\n,%d\n",weapon_2[0].name,weapon_2[1].atk);
    printf("%s\n,%d\n",p->name,p->atk);
    p++;
    printf("%s\n,%d\n",p->name,p->atk);
    return 0;

}

