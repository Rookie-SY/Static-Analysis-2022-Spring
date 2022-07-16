#include <stdio.h>
#include <stdlib.h>
int add(int t,int m){
    int a1;
    int b1 = 2;
    b1 = a1 + b1;
    return b1;
}

int main(){
    int array[10];
    int t = array[5];
    int b = add(1, 2);
    array[2] = b;
    int y = 1;
    for(int i = 0; i < 10; i++)
        y = y + array[i];

    double* p = (double*)malloc(8 * 4);
    p++;
    int* q = (int* )p;
    free(q);
    q -= 2;
    free(q);
    p--;
    free(p);
    return 0;
}