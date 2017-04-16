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
    struct weapon weapon_2[2] = {{"one",900,200},{"two",900,300}};
    
    printf("%s\n,%d\n",weapon_1.name,++weapon_1.atk);
    printf("%s\n,%d\n",weapon_2[0].name,weapon_2[1].atk);
    return 0;

}

