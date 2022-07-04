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
        ConstructStmtCFG();
        dumpStmtCFG();
        //writeDotFile(funDecl->getQualifiedNameAsString());
        InitTreeNodeVector();
        ConstructFDT();
        Getidom();
        dumpFDT();
        dumpStmtFDT();
        blockcfg_forest.push_back(blockcfg);
        FDT_forest.push_back(FDT);
        vector<BlockInfo>().swap(blockcfg);
        vector<TreeNode>().swap(FDT);
    }
}

void ForwardDominanceTree::completeCfgRelation(unique_ptr<CFG>& cfg){
    clang::CFG::iterator blockIter;
    for(blockIter = cfg->begin(); blockIter != cfg->end(); blockIter++){
        int blockstmtnum = 0;
        CFGBlock* block = *blockIter;
        blockNum++;
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
        BumpVector<CFGElement>::reverse_iterator elementIter;
        for(elementIter = block->begin(); elementIter != block->end(); elementIter++){
            CFGElement element = *elementIter;
            if(element.getKind() == clang::CFGElement::Kind::Statement){
                llvm::Optional<CFGStmt> stmt = element.getAs<CFGStmt>();
                if(stmt.hasValue() == true){
                    Stmt* statement = const_cast<Stmt* >(stmt.getValue().getStmt());
                    CFGInfo cfginfo;
                    cfginfo.statement = statement;
                    cfginfo.statementid = blockstmtnum;
                    blockstmtnum++;
                    blockelement.blockstatement.push_back(cfginfo);
                }
            }
            else if(element.getKind() == clang::CFGElement::Kind::Constructor){
                // may have no use
                std::cout <<"Check here.\n" << std::endl;
            }   
        }
        blockcfg.push_back(blockelement);
        FDT.push_back(treenode);
    }
    std::cout << blockNum << std::endl;
}

void ForwardDominanceTree::ConstructStmtCFG(){
    allStmtNum = 0;
    for(int i=0;i<blockNum;i++){
        allStmtNum = allStmtNum + blockcfg[i].blockstatement.size();
    }
    for(int i=0;i<blockNum;i++){
        if(blockcfg[i].blockstatement.size() == 0){
            if(blockcfg[i].pred.size() != 0 && blockcfg[i].succ.size() == 0){
                CFGInfo cfginfo;
                cfginfo.statementid = 0;
                cfginfo.statement = NULL;
                Edge edge;
                edge.blockid = blockcfg[i].pred[0];
                edge.statementid = blockcfg[blockcfg[i].pred[0]].blockstatement.size()-1;
                cfginfo.pred.push_back(edge);
                blockcfg[i].blockstatement.push_back(cfginfo);
            }//exit
            else if(blockcfg[i].succ.size() != 0 && blockcfg[i].pred.size() == 0){
                CFGInfo cfginfo;
                cfginfo.statementid = 0;
                cfginfo.statement = NULL;
                Edge edge;
                edge.blockid = blockcfg[i].succ[0];
                edge.statementid = 0;
                cfginfo.succ.push_back(edge);
                blockcfg[i].blockstatement.push_back(cfginfo);
            }//entry
            else{
                CFGInfo cfginfo;
                cfginfo.statementid = 0;
                cfginfo.statement = NULL;
                Edge edge;
                edge.blockid = blockcfg[i].succ[0];
                edge.statementid = 0;
                cfginfo.succ.push_back(edge);
                Edge edge1;
                edge1.blockid = blockcfg[i].pred[0];
                edge1.statementid = blockcfg[blockcfg[i].pred[0]].blockstatement.size()-1;
                cfginfo.pred.push_back(edge1);
                blockcfg[i].blockstatement.push_back(cfginfo);
            }
        }// entry or exit
        else{
            for(int j=0;j<blockcfg[i].blockstatement.size();j++){  
                if(j==0){
                    for(int k=0;k<blockcfg[i].pred.size();k++){
                        Edge edge;
                        edge.blockid = blockcfg[i].pred[k];
                        if(edge.blockid == blockNum-1)
                            edge.statementid = 0;
                        else
                            edge.statementid = blockcfg[edge.blockid].blockstatement.size()-1;
                        blockcfg[i].blockstatement[j].pred.push_back(edge);
                    }
                }
                else{
                    Edge edge;
                    edge.blockid = i;
                    edge.statementid = j-1;
                    blockcfg[i].blockstatement[j].pred.push_back(edge);
                }
                if(j == blockcfg[i].blockstatement.size()-1)
                {
                    for(int k=0;k<blockcfg[i].succ.size();k++){
                        Edge edge;
                        edge.blockid = blockcfg[i].succ[k];
                        edge.statementid = 0;
                        blockcfg[i].blockstatement[j].succ.push_back(edge);
                    }
                }
                else{
                    Edge edge;
                    edge.blockid = i;
                    edge.statementid = j+1;
                    blockcfg[i].blockstatement[j].succ.push_back(edge);
                }
            }
        }
    }
}

void ForwardDominanceTree::dumpStmtCFG(){
    for(int i=0;i<blockNum;i++){
        for(int j=0;j<blockcfg[i].blockstatement.size();j++){
            std::cout << "Block " << blockcfg[i].blockid << " ";
            std::cout << "Stmt " << blockcfg[i].blockstatement[j].statementid << " \n";
            std::cout << "   pred:";
            if(blockcfg[i].blockstatement[j].pred.size() == 0)
                cout << "NULL\n";
            else{
                for(int k=0;k<blockcfg[i].blockstatement[j].pred.size();k++){
                    std::cout << "block " << blockcfg[i].blockstatement[j].pred[k].blockid << " " << "stmt " << blockcfg[i].blockstatement[j].pred[k].statementid <<" ";
                }
            }
            std::cout << "\n   succ:";
            if(blockcfg[i].blockstatement[j].succ.size() == 0)
                cout << "NULL\n";
            else{
                for(int k=0;k<blockcfg[i].blockstatement[j].succ.size();k++){
                    std::cout << "block " << blockcfg[i].blockstatement[j].succ[k].blockid << " " << "stmt " << blockcfg[i].blockstatement[j].succ[k].statementid <<" ";
                }
            }
            std::cout << endl;
        }
        std::cout << endl;
    }
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
            //std::cout <<i <<"temp\n";
            //for(int j=0;j<temp.size();j++)
             //   std::cout<< temp[j];
            //std::cout <<endl;
            //std::cout <<"temp\n";
            if(FDT[trypost].postdom.size() == temp.size()){
                //std::cout <<"post = " << trypost << " "<<endl;
                bool issame = true;
                for(int k=0;k<FDT[trypost].postdom.size();k++){
                    //std::cout << "trypost " << FDT[trypost].postdom[k] << " " << "cur" << temp[k]<<endl;
                    if(FDT[trypost].postdom[k] == temp[k]){

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
    /*for(int i=0;i<blockNum;i++){
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
    }*/
    for(int i=0;i<blockNum;i++){
        if(i!=0){
            FDT[FDT[i].idom].real_succ.push_back(FDT[i].blockid);
        }
    }
    for(int i=0;i<blockNum;i++){
        FDT[i].blockstatement = blockcfg[i].blockstatement;
    }
    for(int i=0;i<blockNum;i++){
        if(i == 0){
            vector<Edge>().swap(FDT[0].blockstatement[0].pred);
            vector<Edge>().swap(FDT[0].blockstatement[0].succ);
            for(int j=0;j<FDT[i].real_succ.size();j++){
                Edge edge;
                edge.blockid = FDT[i].real_succ[j];
                edge.statementid = blockcfg[edge.blockid].blockstatement.size()-1;
                FDT[0].blockstatement[0].succ.push_back(edge);
            }
        }
        else if(i == blockNum - 1){
            vector<Edge>().swap(FDT[i].blockstatement[0].pred);
            vector<Edge>().swap(FDT[i].blockstatement[0].succ);
            Edge edge;
            edge.blockid = FDT[i].idom;
            edge.statementid = 0;
            FDT[i].blockstatement[0].pred.push_back(edge);
        }
        else{
            for(int j=0;j<FDT[i].blockstatement.size();j++){
                vector<Edge>().swap(FDT[i].blockstatement[j].pred);
                vector<Edge>().swap(FDT[i].blockstatement[j].succ);
                if(j==0){
                    for(int k=0;k<FDT[i].real_succ.size();k++){
                        Edge edge;
                        edge.blockid = FDT[i].real_succ[k];
                        edge.statementid = blockcfg[edge.blockid].blockstatement.size()-1;
                        FDT[i].blockstatement[j].succ.push_back(edge);
                    }
                }
                else{
                    Edge edge;
                    edge.blockid = i;
                    edge.statementid = j-1;
                    FDT[i].blockstatement[j].succ.push_back(edge);
                }
                if(j==FDT[i].blockstatement.size()-1){
                    Edge edge;
                    edge.blockid = FDT[i].idom;
                    edge.statementid = 0;
                    FDT[i].blockstatement[j].pred.push_back(edge);
                }
                else{
                    Edge edge;
                    edge.blockid = i;
                    edge.statementid = j+1;
                    FDT[i].blockstatement[j].pred.push_back(edge);
                }
            }
        }
    }
    vector<BlockStmt> temp;
    for(int i=0;i<blockNum;i++){
        BlockStmt blockstmt;
        blockstmt.stmtnum = FDT[i].blockstatement.size();
        for(int j=0;j<blockstmt.stmtnum;j++)
            blockstmt.fpostdom.push_back(0);
        temp.push_back(blockstmt);
    }
    for(int i=0;i<blockNum;i++){
        for(int j=0;j<FDT[i].blockstatement.size();j++){
            vector<BlockStmt> tempb_stmt = temp;
            std::cout << "block " << i << " stmt " << j << endl;
            for(int k=0;k<FDT[i].postdom.size();k++){
                std::cout <<"fdt " << FDT[i].postdom[k] << " ";
                for(int t=0;t<tempb_stmt[FDT[i].postdom[k]].stmtnum;t++){
                    tempb_stmt[FDT[i].postdom[k]].fpostdom[t] = 1;
                }
            }
            int cur_stmtid = FDT[i].blockstatement[j].statementid;
            //cout << "\ncur_stmtid = " << cur_stmtid << " " << FDT[i].blockstatement.size()<< endl;
            for(int k=cur_stmtid;k < FDT[i].blockstatement.size();k++){
                std::cout << "stmtid " << k << " ";
                tempb_stmt[FDT[i].blockid].fpostdom[k] = 1;
            }
            FDT[i].blockstatement[j].postdom_stmt = tempb_stmt;
            //std::cout << "good\n";
            std::cout << endl;
        }
    }
    // TODO: construct stmtcfg edge;
}

void ForwardDominanceTree::dumpFDT(){
    std::cout << endl;
    for(int i=0;i<blockNum;i++){
        std::cout << "BLOCK " << FDT[i].blockid << " " << FDT[i].blockstatement.size() << std::endl;
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

void ForwardDominanceTree::dumpStmtFDT(){
    std::cout << endl;
    for(int i=0;i<blockNum;i++){
        for(int j=0;j<FDT[i].blockstatement.size();j++){
            std::cout << "Block " << FDT[i].blockid << " ";
            std::cout << "Stmt " << FDT[i].blockstatement[j].statementid << " \n";
            std::cout << "   pred:";
            if(FDT[i].blockstatement[j].pred.size() == 0)
                cout << "NULL";
            else{
                for(int k=0;k<FDT[i].blockstatement[j].pred.size();k++){
                    std::cout << "block " << FDT[i].blockstatement[j].pred[k].blockid << " " << "stmt " << FDT[i].blockstatement[j].pred[k].statementid <<" ";
                }
            }
            std::cout << "\n   succ:";
            if(FDT[i].blockstatement[j].succ.size() == 0)
                cout << "NULL";
            else{
                for(int k=0;k<FDT[i].blockstatement[j].succ.size();k++){
                    std::cout << "block " << FDT[i].blockstatement[j].succ[k].blockid << " " << "stmt " << FDT[i].blockstatement[j].succ[k].statementid <<" ";
                }
            }
            std::cout << endl;
        }
    }
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
            BlockStmt blockstmt;
            blockstmt.stmtnum = tempcfg[j].blockstatement.size();
            for(int k=0;k<blockstmt.stmtnum;k++)
                blockstmt.fpostdom.push_back(0);
            Postdom.push_back(blockstmt);
        }
        for(int j=0;j<tempcfg.size();j++){
            CDGNode cdgnode;
            cdgnode.blockid = tempcfg[j].blockid;
            cdgnode.blockstatement = tempfdt[j].blockstatement;
            CalculateControlDependent(&cdgnode,i);
            CDG.push_back(cdgnode);
        }
        FillPred();
        //CompleteCFGEdge();
        CDG_forest.push_back(CDG);
        vector<CDGNode>().swap(CDG);
    }
}

void ControlDependenceGraph::CalculateControlDependent(CDGNode *cdgnode,int treenum){
    //std::cout << cdgnode->blockid << endl;
    for(int i=0;i<cdgnode->blockstatement.size();i++){
        vector<BlockStmt> UnionPostDom = CalculateStmtUnionSuccPostdom(cdgnode->blockid,cdgnode->blockstatement[i].statementid,treenum);
        vector<TreeNode> tempfdt = forward_dominance_tree->FDT_forest[treenum];
        vector<BlockInfo> tempcfg = forward_dominance_tree->blockcfg_forest[treenum];
        vector<Edge>().swap(cdgnode->blockstatement[i].succ);
        vector<Edge>().swap(cdgnode->blockstatement[i].pred);
        int blockNum = tempcfg.size();
        for(int j=0;j<tempcfg[cdgnode->blockid].blockstatement[cdgnode->blockstatement[i].statementid].succ.size();j++){
            for(int k=0;k<blockNum;k++){
                Edge edge = tempcfg[cdgnode->blockid].blockstatement[cdgnode->blockstatement[i].statementid].succ[j];
                vector<int> tempfdom = tempfdt[edge.blockid].blockstatement[edge.statementid].postdom_stmt[k].fpostdom;
                for(int t=0;t<tempfdom.size();t++){
                    if(tempfdom[t] == 1){
                        if(UnionPostDom[k].fpostdom[t] == 0){
                            Edge edge1;
                            edge1.blockid = k;
                            edge1.statementid = t;
                            cdgnode->blockstatement[i].succ.push_back(edge1);
                        }
                    }
                }
            }
        }
    }
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

vector<BlockStmt> ControlDependenceGraph::CalculateStmtUnionSuccPostdom(int blockid,int stmtid,int treenum){
    vector<TreeNode> tempfdt = forward_dominance_tree->FDT_forest[treenum];
    vector<BlockInfo> tempcfg = forward_dominance_tree->blockcfg_forest[treenum];
    int blockNum = tempfdt.size();
    vector<BlockStmt> Unionpost;
    for(int i=0;i<blockNum;i++){
        BlockStmt blockstmt;
        blockstmt.stmtnum = tempfdt[i].blockstatement.size();
        for(int j=0;j<blockstmt.stmtnum;j++)
            blockstmt.fpostdom.push_back(1);
        Unionpost.push_back(blockstmt);
    }
    for(int i=0;i<tempcfg[blockid].blockstatement[stmtid].succ.size();i++){
        for(int j=0;j<blockNum;j++){
            vector<int> tempfdom = tempfdt[tempcfg[blockid].blockstatement[stmtid].succ[i].blockid].blockstatement[tempcfg[blockid].blockstatement[stmtid].succ[i].statementid].postdom_stmt[j].fpostdom;
            for(int k=0;k<tempfdom.size();k++){
                if(tempfdom[k] == 0){
                    Unionpost[j].fpostdom[k] = 0;
                }
            }
        }
    }
    return Unionpost;
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
    for(int i=0;i<CDG.size();i++){
        for(int j=0;j<CDG[i].blockstatement.size();j++){
            for(int k=0;k<CDG[i].blockstatement[j].succ.size();k++){
                Edge edge = CDG[i].blockstatement[j].succ[k];
                Edge predge;
                predge.blockid = i;
                predge.statementid = j;
                CDG[edge.blockid].blockstatement[edge.statementid].pred.push_back(predge);
            }
        }
    }
}

void ControlDependenceGraph::CompleteCFGEdge(){
    for(int i=0;i<CDG.size();i++){
        for(int j=0;j<CDG[i].blockstatement.size();j++){
                if(j != 0){
                    Edge edge;
                    edge.blockid = i;
                    edge.statementid = j - 1;
                    CDG[i].blockstatement[j].pred.push_back(edge);
                }
                if(j != CDG[i].blockstatement.size()-1){
                    Edge edge;
                    edge.blockid = i;
                    edge.statementid = j+1;
                    CDG[i].blockstatement[j].succ.push_back(edge);
                }
        }
    }
}

void ControlDependenceGraph::dumpCDG(){
    for(int i=0;i<CDG_forest.size();i++){
        vector<CDGNode> tempcdg = CDG_forest[i];
        for(int j=0;j<tempcdg.size();j++){
            std::cout << "BLOCK " << tempcdg[j].blockid << " " << tempcdg[j].blockstatement.size()<< std::endl;
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
    std::cout << endl;
}

int ControlDependenceGraph::CalculateStmtNo(Edge edge,int treenum){
    int res = 0;
    for(int i=0;i<edge.blockid;i++){
        res = res + CDG_forest[treenum][i].blockstatement.size();
    }
    res = res + edge.statementid + 1 - 1;
    return res;
}

void ControlDependenceGraph::dumpStmtCDG(){
    std::cout << "dumpstmt\n";
     for(int i=0;i<CDG_forest.size();i++){
        vector<CDGNode> tempcdg = CDG_forest[i];
        for(int j=0;j<tempcdg.size();j++){
            for(int k=0;k<tempcdg[j].blockstatement.size();k++){
                std::cout << "BLOCK " << tempcdg[j].blockid << " Stmt" << tempcdg[j].blockstatement[k].statementid<< std::endl;
                std::cout << "   pred: ";
                if(tempcdg[j].blockstatement[k].pred.size() != 0){
                    for(int m=0;m<tempcdg[j].blockstatement[k].pred.size();m++){
                        Edge edge = tempcdg[j].blockstatement[k].pred[m];
                        std::cout << "block " << edge.blockid << " stmt " << edge.statementid << " ";
                    }
                    std::cout << endl;
                }
                else{
                    std::cout << "NULL\n";
                }
                std::cout << "   succ: ";
                if(tempcdg[j].blockstatement[k].succ.size() != 0){
                    for(int m=0;m<tempcdg[j].blockstatement[k].succ.size();m++){
                        Edge edge = tempcdg[j].blockstatement[k].succ[m];
                        std::cout << "block " << edge.blockid << " stmt " << edge.statementid << " ";
                    }
                    std::cout << endl;
                }
                else{
                    std::cout << "NULL\n";
                }
            }
        }
     }
}

DataDependenceGraph::DataDependenceGraph(ASTManager *manager, ASTResource *resource, CallGraph *call_graph,ForwardDominanceTree *forwarddominancetree){
    this->manager = manager;
    this->resource = resource;
    this->call_graph = call_graph;
    this->forward_dominance_tree = forwarddominancetree;
}

void DataDependenceGraph::ConstructDDGForest(){
    for(int i=0;i<this->forward_dominance_tree->blockcfg_forest.size();i++){
        blockcfg = this->forward_dominance_tree->blockcfg_forest[i];
        ConstructDDG();
        vector<BlockInfo>().swap(blockcfg);
    }
}

void DataDependenceGraph::ConstructDDG(){
    this->blocknum = blockcfg.size();
    this->allstmtnum = 0;
    for(int i=0;i<this->blocknum;i++){
        this->allstmtnum = this->allstmtnum + blockcfg[i].blockstatement.size();
    }
    CompleteStmtCFG();
}

void DataDependenceGraph::CompleteStmtCFG(){
    int stmtid = 0;
    for(int i=0;i<blocknum;i++){
        for(int j=0;j<blockcfg[i].blockstatement.size();j++){
            StmtBitVector stmtbitvector;
            stmtbitvector.stmtid = stmtid;
            stmtid++;
            stmtbitvector.edge.blockid = i;
            stmtbitvector.edge.statementid = j;
            stmtbitvector.statement = blockcfg[i].blockstatement[j].statement;
            stmtbitvector.edge_pred = blockcfg[i].blockstatement[j].pred;
            stmtbitvector.edge_succ = blockcfg[i].blockstatement[j].succ;
            for(int k=0;k<stmtbitvector.edge_pred.size();k++){
                int add = SwitchEdgeToStmtid(stmtbitvector.edge_pred[k]);
                stmtbitvector.pred.push_back(add);
            }
            for(int k=0;k<stmtbitvector.edge_succ.size();k++){
                int add = SwitchEdgeToStmtid(stmtbitvector.edge_succ[k]);
                stmtbitvector.succ.push_back(add);
            }
            stmtbitvector.isvisit = false;
            for(int k=0;k<allstmtnum;k++){
                stmtbitvector.Invector.push_back(0);
                stmtbitvector.Outvector.push_back(0);
                stmtbitvector.Genvector.push_back(0);
                stmtbitvector.Killvector.push_back(0);
            }
            stmtcfg.push_back(stmtbitvector);
        }
    }
    assert(stmtid == allstmtnum);
}

int DataDependenceGraph::SwitchEdgeToStmtid(Edge edge){
    int res = 0;
    for(int i=0;i<edge.blockid;i++){
        res = res + blockcfg[i].blockstatement.size();
    }
    res = res + edge.statementid;
    return res;
}

void DataDependenceGraph::AnalyzeStmt(){
    for(int i=0;i<allstmtnum;i++){
        clang::Stmt* statement = stmtcfg[i].statement;
        if(statement == nullptr){
            stmtcfg[i].isusefulstmt = false;
        }
        else{
            if(statement->getStmtClass() == clang::Stmt::StmtClass::DeclStmtClass){
                stmtcfg[i].isusefulstmt = true;
                clang::DeclStmt* declstmt = static_cast<clang::DeclStmt*>(statement);
                if(declstmt->isSingleDecl()){
                    clang::Decl* oneDecl = declstmt->getSingleDecl();
                    GetStmtLvalue(statement);   
                    if(oneDecl->getKind() == clang::Decl::Kind::Var){
                        clang::VarDecl* vardecl = static_cast<clang::VarDecl*>(oneDecl);
                    }
                }
                else{
                    std::cout << "multiple def\n";
                }
            }// def
            else if(statement->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass){
                clang::BinaryOperator* binaryIter = static_cast<clang::BinaryOperator*>(statement);
                if(binaryIter->isAssignmentOp()){
                    stmtcfg[i].isusefulstmt = true;           
                }// x = 1
                else if(binaryIter->isCompoundAssignmentOp()){
                    stmtcfg[i].isusefulstmt = true;  
                }// ?
                else{
                    stmtcfg[i].isusefulstmt = false;
                }
            }
                        else if(statement->getStmtClass() == clang::Stmt::StmtClass::CompoundAssignOperatorClass){
                            StatementInfo statementPair;
                            statementPair.statementno = statementNum;
                            statementNum++;
                            statementPair.stmt = statement;
                            statementPair.isdummy = false;
                            blockInfo.usefulBlockStatement.push_back(statementPair);
                        }// x += 1
                        else if(statement->getStmtClass() == clang::Stmt::StmtClass::UnaryOperatorClass){
                            clang::UnaryOperator* unaryIter = static_cast<clang::UnaryOperator*>(statement);
                            if(unaryIter->isIncrementDecrementOp()){
                                StatementInfo statementPair;
                                statementPair.statementno = statementNum;
                                statementNum++;
                                statementPair.stmt = statement;
                                statementPair.isdummy = false;
                                blockInfo.usefulBlockStatement.push_back(statementPair);
                            }
                            else if(unaryIter->isArithmeticOp()){
                                StatementInfo statementPair;
                                statementPair.statementno = uselessStatementNum;
                                uselessStatementNum++;
                                statementPair.stmt = statement;
                                statementPair.isdummy = false;
                                uselessBlockInfo.usefulBlockStatement.push_back(statementPair);
                            }
                        }//x ++ or x-- or ++x or --x
                        else if(statement->getStmtClass() == clang::Stmt::StmtClass::CXXOperatorCallExprClass){
                            clang::CXXOperatorCallExpr* cxxopIter = static_cast<clang::CXXOperatorCallExpr*>(statement);
                            auto iter = cxxopIter->child_begin();
                            if((*iter) != NULL){
                                clang::ImplicitCastExpr* impliIter = static_cast<clang::ImplicitCastExpr*>((*iter));
                                assert(impliIter->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass);
                                clang::DeclRefExpr* declIter = static_cast<clang::DeclRefExpr*>(impliIter->getSubExpr());
                                if(declIter->getNameInfo().getAsString() == "operator="){
                                    StatementInfo statementPair;
                                    statementPair.statementno = statementNum;
                                    statementNum++;
                                    statementPair.stmt = statement;
                                    statementPair.isdummy = false;
                                    blockInfo.usefulBlockStatement.push_back(statementPair);
                                }
                            }
                            
                        }
        }
    }
}

string DataDependenceGraph::GetStmtLvalue(clang::Stmt* statement){

}