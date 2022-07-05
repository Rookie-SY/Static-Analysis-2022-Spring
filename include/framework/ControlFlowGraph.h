#ifndef CONTROL_FLOW_GRAPH_H
#define CONTROL_FLOW_GRAPH_H

#include <unordered_map>

#include "ASTManager.h"
#include "CallGraph.h"

using namespace std;
using namespace clang;

class ControlFlowGraph{
private:
    struct BlockInfo{
        int blockid;
        vector<int> succ;
    };
    vector<BlockInfo> blockcfg;
public:
    ASTResource *resource;
    ASTManager *manager;
    CallGraph *call_graph;
public:
    ControlFlowGraph(ASTManager *manager, ASTResource *resource, CallGraph *call_graph);
    void drawCfg();
    void completeCfgRelation(unique_ptr<CFG>& cfg);
    void writeDotFile(string funcname);
    void writeNodeDot(std::ostream& out,int blockid,vector<int> succ);
};


#endif