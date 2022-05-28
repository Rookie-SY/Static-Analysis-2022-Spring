#include <stdio.h>

int main()
{
    int array[10];
    int t = array[5];
    while(t > 5)
        array[t] = t;
    int x,y = 85,a,b[10][5];
    int z = a;
    x = array[2] + z;
    array[8] = 9;
    x += array[3];
    array[3]++;
    x = b[9][4];
    if(y > x)
        x = y;
    else
        x = a;
    //y = 0;
    for(int i=0;i<10;i++){
        y = y + array[i];
    }
    return 0;
}

/*int main()
{
    int array[10][2] = {0,1,2,2,3,5};
    int x,y,a;
    x = array[0][2];
    array[8][0] = 9;
    if(y > x)
        x = y;
    else
        x = a;
    //y = 0;
    for(int i=0;i<10;i++){
        y = y + array[i][i];
    }
    return 0;
}*/
