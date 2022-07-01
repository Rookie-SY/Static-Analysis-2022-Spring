#ifndef PROGRAM_DEPENDENCY_GRAPH_H
#define PROGRAM_DEPENDENCY_GRAPH_H

#include <unordered_map>
#include <queue>
#include <vector>
#include <algorithm>

#include "ASTManager.h"
#include "CallGraph.h"

using namespace std;
using namespace clang;

class ForwardDominanceTree{
private:
    struct TreeNode{
        int blockid;
        int idom;
        vector<int> postdom;
        vector<int> Invector;
        vector<int> Outvector;
        vector<int> reverse_pred;
        vector<int> reverse_succ;
        vector<int> real_succ;
        bool isvisit;
    };
    struct BlockInfo{
        int blockid;
        vector<int> succ;
        vector<int> pred;
    };
    int blockNum;
    vector<BlockInfo> blockcfg;
    vector<TreeNode> FDT;
public:
    ASTResource *resource;
    ASTManager *manager;
    CallGraph *call_graph;
public:
    ForwardDominanceTree(ASTManager *manager, ASTResource *resource, CallGraph *call_graph);
    void ConstructFDTFromCfg();
    void completeCfgRelation(unique_ptr<CFG>& cfg);
    void InitTreeNodeVector();
    void ConstructFDT();
    void calculate_block(int num);
    void Getidom();
    void dumpFDT();

    void writeDotFile(string funcname);
    void writeNodeDot(std::ostream& out,int blockid,vector<int> succ);
};


#endif