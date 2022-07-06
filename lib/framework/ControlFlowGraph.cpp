#include "framework/ControlFlowGraph.h"

#include <iostream>

using namespace clang;
using namespace std;

ControlFlowGraph::ControlFlowGraph(ASTManager *manager, ASTResource *resource, CallGraph *call_graph) {
    this->manager = manager;
    this->resource = resource;
    this->call_graph = call_graph;
}

void ControlFlowGraph::drawCfg(){
    for(int i = 0;i<call_graph->allFunctions.size();i++){
        FunctionDecl *funDecl = manager->getFunctionDecl(call_graph->allFunctions[i]);
        std::cout << "The function is: "
                        << funDecl->getQualifiedNameAsString() << std::endl;
        LangOptions LangOpts;
        LangOpts.CPlusPlus = true;
        std::unique_ptr<CFG>& cfg = manager->getCFG(call_graph->allFunctions[i]);
        completeCfgRelation(cfg);
        writeDotFile(funDecl->getQualifiedNameAsString());
        vector<BlockInfo>().swap(blockcfg);
    }
}

void ControlFlowGraph::completeCfgRelation(unique_ptr<CFG>& cfg){
    clang::CFG::iterator blockIter;
    for(blockIter = cfg->begin(); blockIter != cfg->end(); blockIter++){
        CFGBlock* block = *blockIter;
        BlockInfo blockelement;
        blockelement.blockid = block->getBlockID();
        if(block->succ_empty() == false){
            for(auto succ_iter = block->succ_begin(); succ_iter != block->succ_end();succ_iter++ ){
                //succ_iter->getReachableBlock()->dump();
                if(*succ_iter != NULL){
                    if(succ_iter->isReachable()){
                        blockelement.succ.push_back(succ_iter->getReachableBlock()->getBlockID());
                    }
                    else{
                        blockelement.succ.push_back(succ_iter->getPossiblyUnreachableBlock()->getBlockID());
                    }
                }
            }
        }
        blockcfg.push_back(blockelement);
    }
}

void ControlFlowGraph::writeDotFile(string funcname){
    string command = "mkdir -p cfg";
    system(command.c_str());
    string prefix = "cfg/";
    string suffix = ".dot";
    string filename =  prefix + funcname + suffix;
    std::fstream out(filename, ios::out);
    std::string head = "digraph \"Control Flow Graph\" {";
    std::string end = "}";
    std::string label = "    label=\"Control Flow Graph\"";
    if (out.is_open()) {
        out << head << std::endl;
        out << label << std::endl << std::endl;
    }
    auto it = blockcfg.begin();
    for (; it != blockcfg.end(); it++) {
        //if((*it).succ.size() != 0){
            //std::cout <<(*it).succ[0] << endl;
            writeNodeDot(out, (*it).blockid,(*it).succ);
        //}
    }
    out << end << std::endl;
    out.close();
}

void ControlFlowGraph::writeNodeDot(std::ostream& out,int blockid,vector<int> succ){
    //char id = '0' + blockid;
    string nodeid = "0x";
    nodeid = nodeid + to_string(blockid);
    out << "    Node" << nodeid << " [shape=record,label=\"{";
    out << "Block "<< blockid << "}\"];" << endl;
    for(int i=0;i<succ.size();i++){
        string prefix = "0x";
        //char succid = '0' + succ[i];
        prefix = prefix + to_string(succ[i]);
        out << "    Node" << nodeid << " -> " << "Node" << prefix << endl;
    }
}