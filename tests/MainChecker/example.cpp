#include <stdio.h>
#include <iostream>
using namespace std;

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

int main(){
    int a, b;
    float c;
    struct A1 temp1,temp2,temp4;
    B1 btemp2 = {.m = a,.f = 2};
    temp4 = temp2;
    temp1.m = 2;
    struct A1 temp3 = temp1;
    btemp2.f = c;

    int* p = new int[a];
    delete p;

    int* q = p;
    delete[] q;

    double* r = (double* )malloc(sizeof(double) * 4);
    double* s = (double* )malloc(sizeof(double) * 4);
    r = s;
    free(s);
    free(r);
}