#include<stdio.h>

int add2(int c, int d){
    c = c+1;
    d++;
    if(c+d < 10)
        return add2(c,d);
    else
        return 10+c;
}

int add1(int a, int b){
    if(a > b)
        return a-b;
    else    
        return add2(a+b,b);
}

int main(){
    int a,b,c;
    a = 10;
    if(a > 5)
        b = a + c;
    else if(a < -5)
        add1(a,2);
    else
        add2(b,2);
    return 0;
}