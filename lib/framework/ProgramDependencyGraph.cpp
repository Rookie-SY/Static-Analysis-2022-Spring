#include "framework/ProgramDependencyGraph.h"

#include <iostream>

using namespace clang;
using namespace std;

ForwardDominanceTree::ForwardDominanceTree(ASTManager *manager, ASTResource *resource, CallGraph *call_graph) {
    this->manager = manager;
    this->resource = resource;
    this->call_graph = call_graph;
}

void ForwardDominanceTree::ConstructFDTFromCfg(){
    for(int i = 0;i<call_graph->allFunctions.size();i++){
        blockNum = 0;
        FunctionDecl *funDecl = manager->getFunctionDecl(call_graph->allFunctions[i]);
        std::cout << "The function is: "
                        << funDecl->getQualifiedNameAsString() << std::endl;
        LangOptions LangOpts;
        LangOpts.CPlusPlus = true;
        std::unique_ptr<CFG>& cfg = manager->getCFG(call_graph->allFunctions[i]);
        completeCfgRelation(cfg);
        //writeDotFile(funDecl->getQualifiedNameAsString());
        InitTreeNodeVector();
        ConstructFDT();
        Getidom();
        dumpFDT();
        blockcfg_forest.push_back(blockcfg);
        FDT_forest.push_back(FDT);
        vector<BlockInfo>().swap(blockcfg);
        vector<TreeNode>().swap(FDT);
    }
}

void ForwardDominanceTree::completeCfgRelation(unique_ptr<CFG>& cfg){
    clang::CFG::iterator blockIter;
    for(blockIter = cfg->begin(); blockIter != cfg->end(); blockIter++){
        blockNum++;
        CFGBlock* block = *blockIter;
        BlockInfo blockelement;
        blockelement.blockid = block->getBlockID();
        TreeNode treenode;
        treenode.blockid  = block->getBlockID();
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
        if(block->pred_empty() == false){
            for(auto pred_iter = block->pred_begin(); pred_iter != block->pred_end();pred_iter++ ){
                if(*pred_iter != NULL){
                    if(pred_iter->isReachable())
                        blockelement.pred.push_back(pred_iter->getReachableBlock()->getBlockID());
                    else
                        blockelement.pred.push_back(pred_iter->getPossiblyUnreachableBlock()->getBlockID());
                }
            }
        }
        blockcfg.push_back(blockelement);
        FDT.push_back(treenode);
    }
    std::cout << blockNum << std::endl;
}


void ForwardDominanceTree::InitTreeNodeVector(){
    for(int i=0;i<blockNum;i++){
        //std::cout<<"pred\n";
        if(blockcfg[i].pred.size() != 0){
            for(int j=0;j<blockcfg[i].pred.size();j++){
                FDT[i].reverse_succ.push_back(blockcfg[i].pred[j]);
            }
        }
        //std::cout<<"succ\n";
        if(blockcfg[i].succ.size() != 0){
            for(int j=0;j<blockcfg[i].succ.size();j++){
                FDT[i].reverse_pred.push_back(blockcfg[i].succ[j]);
            }
        }
        FDT[i].isvisit = false;
        if(FDT[i].blockid == 0){
            FDT[i].Outvector.push_back(1);
            for(int j=1;j<blockNum;j++){
                FDT[i].Outvector.push_back(0);
            }
        }
        else{
            for(int j=0;j<blockNum;j++){
                FDT[i].Outvector.push_back(1);
            }
        }
        for(int j=0;j<blockNum;j++){
            FDT[i].Invector.push_back(0);
        }
        //std::cout <<"i=" << i <<std::endl;
    }
}

void ForwardDominanceTree::calculate_block(int num){
    if(FDT[num].isvisit == true) return;
    queue<TreeNode> q;
    q.push(FDT[num]);
    FDT[num].isvisit = true;
    while(!q.empty()){
        TreeNode tempblock = q.front();
        vector<int> temp;
        for(int i=0;i<blockNum;i++)
            temp.push_back(1);
        if(tempblock.reverse_pred.empty() == false){
            for(int i=0;i<tempblock.reverse_pred.size();i++){
                for(int j=0;j<blockNum;j++){
                    if(FDT[tempblock.reverse_pred[i]].Outvector[j] == 0)
                        temp[j] = 0;
                }
            }
            tempblock.Invector = temp;
        }
        vector<int> res;
        for(int i=0;i<blockNum;i++)
            res.push_back(tempblock.Invector[i]);
        res[tempblock.blockid] = 1;
        FDT[tempblock.blockid].Outvector = res;
        for(int i=0;i<tempblock.reverse_succ.size();i++){
            if(FDT[tempblock.reverse_succ[i]].isvisit == false){
                q.push(FDT[tempblock.reverse_succ[i]]);
                FDT[tempblock.reverse_succ[i]].isvisit = true;
            }
        }
        q.pop();
    }
}

void ForwardDominanceTree::ConstructFDT(){
    bool ischanging = true;
    std::vector<TreeNode> pre_blockbitvector;
    for(int i=0;i<blockNum;i++){
        pre_blockbitvector.push_back(FDT[i]);
    }
    int count = 0;
    while(ischanging){
        calculate_block(0);
        //std::cout << count << std::endl;
        //blockvector_output();
        for(int i=0;i<blockNum;i++){
            if(FDT[i].isvisit == false){
                //std::cout << i <<std::endl;
                std::cout<< "wrong bfs" <<std::endl;
                for(int j=0;j<blockNum;j++){
                    FDT[i].Invector[j] = 0;
                    FDT[i].Outvector[j] = 1;
                }
            }
        }
        bool allchange = false;
        for(int i=0;i<blockNum;i++){
            if(pre_blockbitvector[i].Outvector != FDT[i].Outvector){
                allchange = true;
            }
        }
        if(allchange == true){
            ischanging = true;
            for(int i=0;i<blockNum;i++)
                FDT[i].isvisit = false;
            for(int i=0;i<blockNum;i++)
                pre_blockbitvector[i] = FDT[i];
        }
        else{
            ischanging = false;
            for(int i=0;i<blockNum;i++)
                FDT[i].isvisit = false;
        }
        count++;
    }
    std::cout<< "iteration time: " << count <<std::endl;
}

void ForwardDominanceTree::Getidom(){
    for(int i=0;i<blockNum;i++){
        for(int j=0;j<blockNum;j++){
            if(FDT[i].Outvector[j] == 1){
                if(j != FDT[i].blockid){
                    FDT[i].postdom.push_back(j);
                }
            }
        }
    }

    /*for(int i=0;i<blockNum;i++){
        std::cout << "BLOCK " << FDT[i].blockid <<"   "<< std::endl;
        for(int j=0;j<FDT[i].postdom.size();j++){
            std::cout << FDT[i].postdom[j] <<" ";
        }
        std::cout <<endl;
    }*/

    for(int i=0;i<blockNum;i++){
        for(auto iter= FDT[i].postdom.begin();iter != FDT[i].postdom.end();iter++){
            int trypost = *iter;
            std::vector<int> temp(FDT[i].postdom.begin(),FDT[i].postdom.end());
            auto newiter = std::remove(std::begin(temp),std::end(temp),trypost);
            temp.erase(newiter,std::end(temp));
            
            if(FDT[trypost].postdom.size() == temp.size()){
                bool issame = true;
                for(int k=0;k<FDT[trypost].postdom.size();k++){
                    if(FDT[trypost].postdom[k] == FDT[i].postdom[k]){

                    }
                    else
                        issame = false;
                }
                if(issame == true){
                    FDT[i].idom = trypost;
                    break;
                }
                else
                    continue;
            }
            else
                continue;
        }
    }
    for(int i=0;i<blockNum;i++){
        if(i !=0){
            std::cout << "BLOCK " << FDT[i].blockid <<"   "<< std::endl;
            std::cout << FDT[i].idom;
            std::cout << std::endl;
        }
        else {
            std::cout << "BLOCK " << FDT[i].blockid <<"   "<< std::endl;
            std::cout << "";
            std::cout << std::endl;
        }
    }
    for(int i=0;i<blockNum;i++){
        if(i!=0){
            FDT[FDT[i].idom].real_succ.push_back(FDT[i].blockid);
        }
    }
}

void ForwardDominanceTree::dumpFDT(){
    std::cout << endl;
    for(int i=0;i<blockNum;i++){
        std::cout << "BLOCK " << FDT[i].blockid << std::endl;
        std::cout << "   succ: ";
        if(FDT[i].real_succ.size() == 0){
            std::cout << "NULL" << std::endl;
        }
        else{
            for(int j=0;j<FDT[i].real_succ.size();j++){
                std::cout << FDT[i].real_succ[j] << " ";
            }
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;
    /*for(int i=0;i<blockNum;i++){
        std::cout << "BLOCK " << FDT[i].blockid << std::endl;
        for(int j=0;j<blockNum;j++){
            std::cout << FDT[i].Outvector[j] <<" ";
        }
        std::cout << endl;
    }*/
}

ControlDependenceGraph::ControlDependenceGraph(ASTManager *manager, ASTResource *resource, CallGraph *call_graph,ForwardDominanceTree *forwarddominancetree){
    this->manager = manager;
    this->resource = resource;
    this->call_graph = call_graph;
    this->forward_dominance_tree = forwarddominancetree;
}

void ControlDependenceGraph::ConstructCDG(){
    for(int i=0;i<forward_dominance_tree->blockcfg_forest.size();i++){
        vector<BlockInfo> tempcfg = forward_dominance_tree->blockcfg_forest[i];
        vector<TreeNode> tempfdt = forward_dominance_tree->FDT_forest[i];
        for(int j=0;j<tempcfg.size();j++){
            CDGNode cdgnode;
            cdgnode.blockid = tempcfg[j].blockid;
            CalculateControlDependent(&cdgnode,i);
            CDG.push_back(cdgnode);
        }
        FillPred();
        CDG_forest.push_back(CDG);
        vector<CDGNode>().swap(CDG);
    }
}

void ControlDependenceGraph::CalculateControlDependent(CDGNode *cdgnode,int treenum){
    vector<int> UnionPostDom = CalculateUnionSuccPostdom(cdgnode->blockid,treenum);
    vector<TreeNode> tempfdt = forward_dominance_tree->FDT_forest[treenum];
    vector<BlockInfo> tempcfg = forward_dominance_tree->blockcfg_forest[treenum];
    int blockNum = tempfdt.size();
    for(int i=0;i<blockNum;i++){
        cdgnode->controldependent.push_back(0);
    }
    for(int i=0;i<tempcfg[cdgnode->blockid].succ.size();i++){
        for(int j=0;j<blockNum;j++){
            if(tempfdt[tempcfg[cdgnode->blockid].succ[i]].Outvector[j] == 1){
                if(UnionPostDom[j] == 0){
                    cdgnode->controldependent[j] = 1;
                }
            }
        }
    }
    for(int i=0;i<cdgnode->controldependent.size();i++){
        if(cdgnode->controldependent[i] == 1){
            cdgnode->succ.push_back(i);
        }
    }
}

vector<int> ControlDependenceGraph::CalculateUnionSuccPostdom(int blockid,int treenum){
    vector<TreeNode> tempfdt = forward_dominance_tree->FDT_forest[treenum];
    vector<BlockInfo> tempcfg = forward_dominance_tree->blockcfg_forest[treenum];
    int blockNum = tempfdt.size();
    vector<int> UnionPost;
    for(int i=0;i<blockNum;i++)
        UnionPost.push_back(1);
    for(int i=0;i<tempcfg[blockid].succ.size();i++){
        for(int j=0;j<blockNum;j++){
            if(tempfdt[tempcfg[blockid].succ[i]].Outvector[j] == 0){
                UnionPost[j] = 0;
            }
        }
    }
    return UnionPost;
}

void ControlDependenceGraph::FillPred(){
    for(int i=0;i<CDG.size();i++){
        for(int j=0;j<CDG[i].succ.size();j++){
            CDG[CDG[i].succ[j]].pred.push_back(i);
        }
    }
}

void ControlDependenceGraph::dumpCDG(){
    for(int i=0;i<CDG_forest.size();i++){
        vector<CDGNode> tempcdg = CDG_forest[i];
        for(int j=0;j<tempcdg.size();j++){
            std::cout << "BLOCK " << tempcdg[j].blockid << std::endl;
            std::cout << "   pred: ";
            if(tempcdg[j].pred.size() == 0){
                std::cout << "NULL" << std::endl;
            }
            else{
                for(int k=0;k<tempcdg[j].pred.size();k++){
                    std::cout << tempcdg[j].pred[k] << " ";
                }
                std::cout << std::endl;
            }
            std::cout << "   succ: ";
            if(tempcdg[j].succ.size() == 0){
                std::cout << "NULL" << std::endl;
            }
            else{
                for(int k=0;k<tempcdg[j].succ.size();k++){
                    std::cout << tempcdg[j].succ[k] << " ";
                }
                std::cout << std::endl;
            }
        }
    }
}