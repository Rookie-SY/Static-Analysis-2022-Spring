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

class ControlDependenceGraph;

class ForwardDominanceTree{
private:
    int blockNum;
    vector<BlockInfo> blockcfg;
    vector<TreeNode> FDT;
    vector<vector<BlockInfo>> blockcfg_forest;
    vector<vector<TreeNode>> FDT_forest;
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
    friend class ControlDependenceGraph;
};

class ControlDependenceGraph{
private:
    struct CDGNode{
        int blockid;
        vector<int> controldependent;
        vector<int> pred;
        vector<int> succ;
    };
    vector<CDGNode> CDG;
    vector<vector<CDGNode>> CDG_forest;
private:
    ASTResource *resource;
    ASTManager *manager;
    CallGraph *call_graph;
    ForwardDominanceTree *forward_dominance_tree;
public:
    ControlDependenceGraph(ASTManager *manager, ASTResource *resource, CallGraph *call_graph,ForwardDominanceTree *forwarddominancetree);
    void ConstructCDG();
    void CalculateControlDependent(CDGNode *cdgnode,int treenum);
    void FillPred();
    vector<int> CalculateUnionSuccPostdom(int blockid,int treenum);
    void dumpCDG();
};


#endif