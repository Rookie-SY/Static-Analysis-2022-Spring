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

struct BlockStmt{
    int stmtnum;
    vector<int> fpostdom;
};

struct Edge{
    int blockid;
    int statementid;
};

struct CFGInfo{
    int statementid;
    clang::Stmt* statement;
    string sourcecode;
    vector<Edge> pred;
    vector<Edge> succ;
    vector<BlockStmt> postdom_stmt;
};

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
    vector<CFGInfo> blockstatement;
};
struct BlockInfo{
    int blockid;
    vector<int> succ;
    vector<int> pred;
    vector<CFGInfo> blockstatement;
};


class ControlDependenceGraph;
class DataDependenceGraph;

class ForwardDominanceTree{
private:
    int blockNum;
    int allStmtNum;
    vector<BlockInfo> blockcfg;
    vector<TreeNode> FDT;
    vector<vector<BlockInfo>> blockcfg_forest;
    vector<vector<TreeNode>> FDT_forest;
    clang::SourceManager *SM;
public:
    ASTResource *resource;
    ASTManager *manager;
    CallGraph *call_graph;
public:
    ForwardDominanceTree(ASTManager *manager, ASTResource *resource, CallGraph *call_graph);
    void ConstructFDTFromCfg();
    void completeCfgRelation(unique_ptr<CFG>& cfg);
    void ConstructStmtCFG();
    void InitTreeNodeVector();
    void ConstructFDT();
    void calculate_block(int num);
    void Getidom();
    void dumpFDT();
    void dumpStmtFDT();
    void dumpStmtCFG();
    void writeDotFile(string funcname);
    void writeNodeDot(std::ostream& out,int blockid,vector<int> succ);
    friend class ControlDependenceGraph;
    friend class DataDependenceGraph;
};

class ControlDependenceGraph{
private:
    struct CDGNode{
        int blockid;
        vector<int> controldependent;
        vector<int> pred;
        vector<int> succ;
        vector<CFGInfo> blockstatement;
    };
    vector<BlockStmt> Postdom;
    vector<CDGNode> CDG;
    vector<vector<CDGNode>> CDG_forest;
private:
    ASTResource *resource;
    ASTManager *manager;
    CallGraph *call_graph;
    ForwardDominanceTree *forward_dominance_tree;
    clang::SourceManager *SM;
public:
    ControlDependenceGraph(ASTManager *manager, ASTResource *resource, CallGraph *call_graph,ForwardDominanceTree *forwarddominancetree);
    void ConstructCDG();
    void CalculateControlDependent(CDGNode *cdgnode,int treenum);
    void FillPred();
    vector<int> CalculateUnionSuccPostdom(int blockid,int treenum);
    vector<BlockStmt> CalculateStmtUnionSuccPostdom(int blockid,int stmtid,int treenum);
    void CompleteCFGEdge();
    int CalculateStmtNo(Edge edge,int treenum);
    void dumpCDG();
    void dumpStmtCDG();
};

struct StmtBitVector{
    Edge edge;
    int stmtid;
    clang::Stmt* statement;
    bool isusefulstmt;
    string variable;
    vector<string> rvalue;
    vector<int> Invector;
    vector<int> Outvector;
    vector<int> Genvector;
    vector<int> Killvector;
    vector<int> pred;// pred based on stmt_cfg
    vector<int> succ;// succ based on cfg_graph
    vector<Edge> edge_pred;// pred based on cfg_graph
    vector<Edge> edge_succ;// succ based on cfg_graph
    bool isvisit;
};

class DataDependenceGraph{
private:
    int allstmtnum;
    int blocknum;
    vector<BlockInfo> blockcfg;
    vector<StmtBitVector> stmtcfg;
private:
    ASTResource *resource;
    ASTManager *manager;
    CallGraph *call_graph;
    ForwardDominanceTree *forward_dominance_tree;
public:
    DataDependenceGraph(ASTManager *manager, ASTResource *resource, CallGraph *call_graph,ForwardDominanceTree *forwarddominancetree);
    void ConstructDDGForest();
    void ConstructDDG();
    void CompleteStmtCFG();
    void AnalyzeStmt();
    int SwitchEdgeToStmtid(Edge edge);
    string GetStmtLvalue(clang::Stmt* statement);
};

#endif