#include<stdio.h>
#include<iostream>
#include<stdlib.h>

// int a, b;
// int* pop;
// // bool GetString(char* p, int n)
// // {
// //     return true;
// // }

// // void MemoryLeak(int nSize)
// // {
// //     char* p= new char[nSize];
// //     if( !GetString( p, nSize ) )
// //     {
// //         printf("Error\n");
// //         return;   //there is a memory leak point
// //     }
// //   //using the string pointed by p;
// //     delete[] p;
// // }
// // //Memory leak demo two
// // unsigned char* Func(void)
// // {
// //     unsigned char *stra;
// //     stra = (unsigned char *)malloc(10);
// //     return stra;
// // }
// void my_test(int* p)
// {

// }
// int* test()
// {
//     int* p = new int[5];
//     return p;
// }
// int as()
// {
//     return 4;
// }

int main()
{
    int* p = (int* )malloc(sizeof(int) * 4);
    p++;
    free(p);
    // int* g = (int* )malloc(4 * sizeof(int));
    // int* e = (int* )malloc(sizeof(int));
    // pop = new int[4];
    // double* h = new double[4];
    // double* i = new double;
    // int* j = (int* )malloc(as());
    // my_test((int* )malloc(4 * sizeof(int)));
    // int* p;
    // p = (int* )malloc(16);
    // int* q;
    // int a, b;
    // if(a > b)
    // {
    //     q = p++;
    //     p--;
    //     free(p);
    // }
    // else
    // {
    //     free(p);
    // }
    // int *p_seexp = (int *)malloc(4*sizeof(int));
    // int i_seexp = 3;
    // while (i_seexp--) {
    //     free(p_seexp); // seexp, warning
    // }
    // int leaknum = 10;
    // int *seexp_p = NULL;
    // while (leaknum--) {
    //     seexp_p = (int*)malloc(10*sizeof(int)); // seexp, warning
    // }
    // seexp_p[0] = 520;
    // int x, y;
    // x = x + 1;
    // y = y + x;
    // free(f);
    // int* r = p++;
    // int* q;
    // q = p;
    // p = (int* )malloc(4);
    // int a, b;
    // if(a > b)
    // {
    //     int* q = (int* )malloc(8);
    // }
    // else
    // {
    // q = 1 + p;
    // }
    // q = 1 + (double* )p;
    // p += 2;
    // p++;
    // p--;
    // p -= 2;
    // p = p - 2;
    // p = p + 1;
    // q = (double* )( 1 + test());
    // p = test() + 1;
    // p = 1 + test();
    // q = (double* )test() + 1;
    // q = (double* )test();
    // q = 1 + (double* )test();
    // p = test() + 1;
    // p = test() + sizeof(int);
    // p = 2 + test(); 
    // q = (double* )p++;
    // double* q;
    // q = (double* )p + 1;
    // q = (double* )(p + 1);
    // p += sizeof(int);
    // q = p - sizeof(int);
    // int* r;
    // r = test();
    // if(a > b)
    // {
    //     free(p);
    //     MemoryLeak(4);
    // }
    // else
    // {
    //     MemoryLeak(8);
    // }
    return 0;
}
// void GetString(char* p, int n) 
// {
//     if(!GetStringFromInput(p, n)) {
//         MessageBox(¡°Error¡±);
//         delete(p);
//         return;
//     }
// }
// void DoubleFree() 
// {
//     char* p = new char[10];
//     char* q = p;
//     GetString(q, 10);
//     delete(p);
// }