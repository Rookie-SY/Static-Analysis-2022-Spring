#include <iostream>
using namespace std;

class A1{
public:
  int m;
  float array[10];  
  struct B1{
    int t1;
  }b;
  int add(int a1,int b1){
    return a1+b1;
  }
};

int main()
{
    int a,b;
    int array[10][5];
    A1 temp1,temp2,temp4;
    temp4 = temp2;
    temp1.m = 2;
    A1 temp3 = temp1;
    temp2.array[2] = 0.5;
    temp2.b.t1 = 5;
    int t;
    array[t][a] = temp1.m + temp2.m + temp3.array[a] + array[b][2];
    t = temp3.add(a,b);
    return 0;
}