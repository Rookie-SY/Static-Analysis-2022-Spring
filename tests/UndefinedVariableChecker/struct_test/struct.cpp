#include <stdio.h>
struct A1{
  int m;
  float array[10];  
  struct B1{
    int t1;
  }b;
};

typedef struct {
  int m;
  float f;
  //B1(int init_m,float init_t):m(init_m),f(init_t){}
}B1;

int main()
{
    int a,b;
    float c;
    int array[10][5];
    struct A1 temp1,temp2,temp4;
    //struct B1 btemp1(1,c);
    B1 btemp2 = {.m = a,.f = 2};
    temp4 = temp2;
    temp1.m = 2;
    struct A1 temp3 = temp1;
    temp2.array[2] = 0.5;
    temp2.b.t1 = 5;
    int t;
    array[t][a] = temp1.m + temp2.m + temp3.array[a] + array[b][2];
    return 0;
}