#include <stdio.h>
#include <iostream>
using namespace std;

struct TreeNode{
    int nodenum;
};

struct Tree{
    char name;
    int num;
    float m;
    TreeNode node1,node2;
};

int foo(Tree node, int te,float m){
    node.m = m;
    node.m += m;
    if(te > 10)
        return node.m + te;
}

struct Tree add(int a11, int b11){
    if(a11 - b11 <= b11 + 2){
        Tree temp1;
        TreeNode node1;
        temp1.node1 = node1;
        return temp1;
    }
    else{
        Tree temp2;
        if(foo(temp2,a11,0.5) > 0)
            return temp2;
    }
}

int main(){
    int a1,b1,c1;
    float a2,b2,c2 = 0.5;
    char a3,b3,c3 = 'c';
    a1 = a3;
    //c1++;
    b2 = (b1+c1)*(-a1);
    Tree tree1,tree2,tree4;
    Tree tree3 = tree1;
    tree4 = tree1;
    tree1.num = a1;
    tree2.m = tree3.m + c2;

    Tree treelist1[20],treelist2[15];
    float array[10];
    for(int i=0;i<10;i++){
        treelist1[i].num = array[i];
    }

    while(!(b1 > a1+c3)){
        if(a3 > 'a'){
            printf("%d\n",a3);
            array[a3] = 0;
        }
        else{
            if(b2 > -a2){
                cout << a2 << endl;
            }
            else if(b2 == a2){
                while(treelist2[a1].num > c2){
                    treelist2[c1].m = b1;
                }
            }
        }
    }
    add(10,add(a1,b1).num);
    return 0;
}