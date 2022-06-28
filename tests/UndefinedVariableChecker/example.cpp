#include <stdio.h>

int global;

struct A1{
    int m;
    int array1[5];
};

int add(int a, int b,struct A1 c){
    int d;
    if(a > 2 || b > 3)
        return a+b;
    else   
        return add(a+b,c.m + a,c);  
}

int main()
{
    //int array[10] = {0,1,2,2,3,5};
    struct A1 c;
    int a,x,y = 85;
    a = global;
    if(x > 0)
        printf("%d\n",a);
    int z = -((x+y)*a);
    int m = 0.5;
    int t;
    bool p = x > 1;
    int temp;
    temp = add(t,add(a,2,c),c);
    printf("%d\n",x);
    while(!2)
        return a;
    x ++;
    x += z;
    x = x + 1 + (y) + -(z);
    x = (x + 1)*(2+y+z);
    x = -(x+y)/z;
    x = 1;
    x <<= 1;
    x = add(1,add(2,y,c),c);
    //add(1,t);
    //array[0] = 0;
    //x = array[8][0];
    if(y > x && !x)
        x = y;
    else
        x = a;
    return 0;
}
