#include<stdio.h>

int main(){
    int x,y,z;
    x = 1;
    y = 2;
    do{
        x = x + 1;
        if(2 > 1){
            x = z;
            y = x;
        }
        else{
            x = z;
            z = 2 * x;
            y = x + 1;
        }
        z = x + y;
    }while(z > 1);
    z = x * y;
    return 0;
}