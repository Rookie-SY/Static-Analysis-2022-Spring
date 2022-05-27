#include <stdio.h>

int main()
{
    //int array[10] = {0,1,2,2,3,5};
    int a,x,y = 85;
    int z = x;
    printf("%d\n",x);
    x ++;
    x += z;
    x = x + 1 + (y) + -(z);
    x = (x + 1)*(2+y+z);
    x = 1;
    //array[0] = 0;
    //x = array[8][0];
    if(y > x)
        x = y;
    else
        x = a;
    return 0;
}
