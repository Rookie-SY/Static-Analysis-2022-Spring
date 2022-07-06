#include <stdio.h>

int add(int a,int b,int c1, int d){
  int c;
  a = c1 + d;
  if(c > a)
    c = a;
  else
    c = a + b;
  d++;
  return 0;
}

int add1(){
  int c,d;
  return c+d;
}

int main()
{
    int a,b;
    float c;
    int array[10][5];
    if(a > b && a >> 1){
      a = a + b;
      while(a < b){
        a++;
      }
    }
    else
      a = a-b;
    return 0;
}

/*int main()
{
    int a,b;
    float c;
    int array[10][5];
    if(a > b){
      a = a + b;
      return 1;
    }
    else
      a = a-b;
    return 0;
}*/