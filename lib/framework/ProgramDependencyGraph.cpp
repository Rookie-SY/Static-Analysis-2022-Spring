#include "framework/ProgramDependencyGraph.h"

#include <iostream>

using namespace clang;
using namespace std;

ForwardDominanceTree::ForwardDominanceTree(ASTManager *manager, ASTResource *resource, CallGraph *call_graph)
{
    this->manager = manager;
    this->resource = resource;
    this->call_graph = call_graph;
}

void ForwardDominanceTree::ConstructFDTFromCfg()
{
    for (int i = 0; i < call_graph->allFunctions.size(); i++)
    {
        blockNum = 0;
        FunctionDecl *funDecl = manager->getFunctionDecl(call_graph->allFunctions[i]);
        /*std::cout << "The function is: "
                       << funDecl->getQualifiedNameAsString() << std::endl;*/
        LangOptions LangOpts;
        LangOpts.CPlusPlus = true;
        int param_num = funDecl->getNumParams();
        for (int j = 0; j < param_num; j++)
        {
            clang::ParmVarDecl *paramvar = funDecl->getParamDecl(j);
            string paramname = paramvar->getNameAsString();
            param.push_back(paramname);
        }
        std::unique_ptr<CFG> &cfg = manager->getCFG(call_graph->allFunctions[i]);
        completeCfgRelation(cfg);
        ConstructStmtCFG();
        // dumpStmtCFG();
        // writeDotFile(funDecl->getQualifiedNameAsString());
        InitTreeNodeVector();
        ConstructFDT();
        Getidom();
        // dumpFDT();
        // dumpStmtFDT();
        blockcfg_forest.push_back(blockcfg);
        FDT_forest.push_back(FDT);
        vector<BlockInfo>().swap(blockcfg);
        vector<TreeNode>().swap(FDT);
        vector<string>().swap(param);
    }
}

void ForwardDominanceTree::completeCfgRelation(unique_ptr<CFG> &cfg)
{
    clang::CFG::iterator blockIter;
    for (blockIter = cfg->begin(); blockIter != cfg->end(); blockIter++)
    {
        int blockstmtnum = 0;
        CFGBlock *block = *blockIter;
        blockNum++;
        BlockInfo blockelement;
        blockelement.blockid = block->getBlockID();
        TreeNode treenode;
        treenode.blockid = block->getBlockID();
        /*if(block->getTerminatorStmt() != nullptr){
        if(IfStmt *if_stmt = dyn_cast<IfStmt>(block->getTerminatorStmt())){
            std::cout << "******************************\n";
        }
        }*/
        if (block->succ_empty() == false)
        {
            for (auto succ_iter = block->succ_begin(); succ_iter != block->succ_end(); succ_iter++)
            {
                // succ_iter->getReachableBlock()->dump();
                if (*succ_iter != NULL)
                {
                    if (succ_iter->isReachable())
                    {
                        blockelement.succ.push_back(succ_iter->getReachableBlock()->getBlockID());
                    }
                    else
                    {
                        blockelement.succ.push_back(succ_iter->getPossiblyUnreachableBlock()->getBlockID());
                    }
                }
            }
        }
        if (block->pred_empty() == false)
        {
            for (auto pred_iter = block->pred_begin(); pred_iter != block->pred_end(); pred_iter++)
            {
                if (*pred_iter != NULL)
                {
                    if (pred_iter->isReachable())
                        blockelement.pred.push_back(pred_iter->getReachableBlock()->getBlockID());
                    else
                        blockelement.pred.push_back(pred_iter->getPossiblyUnreachableBlock()->getBlockID());
                }
            }
        }
        BumpVector<CFGElement>::reverse_iterator elementIter;
        for (elementIter = block->begin(); elementIter != block->end(); elementIter++)
        {
            CFGElement element = *elementIter;
            if (element.getKind() == clang::CFGElement::Kind::Statement)
            {
                llvm::Optional<CFGStmt> stmt = element.getAs<CFGStmt>();
                if (stmt.hasValue() == true)
                {
                    Stmt *statement = const_cast<Stmt *>(stmt.getValue().getStmt());
                    CFGInfo cfginfo;
                    cfginfo.statement = statement;
                    cfginfo.statementid = blockstmtnum;
                    blockstmtnum++;
                    blockelement.blockstatement.push_back(cfginfo);
                }
            }
            else if (element.getKind() == clang::CFGElement::Kind::Constructor)
            {
                // may have no use
                std::cout << "Check here.\n"
                          << std::endl;
            }
        }
        blockcfg.push_back(blockelement);
        FDT.push_back(treenode);
    }
    // std::cout << blockNum << std::endl;
}

void ForwardDominanceTree::ConstructStmtCFG()
{
    allStmtNum = 0;
    for (int i = 0; i < blockNum; i++)
    {
        allStmtNum = allStmtNum + blockcfg[i].blockstatement.size();
    }
    for (int i = 0; i < blockNum; i++)
    {
        if (blockcfg[i].blockstatement.size() == 0)
        {
            if (blockcfg[i].pred.size() != 0 && blockcfg[i].succ.size() == 0)
            {
                CFGInfo cfginfo;
                cfginfo.statementid = 0;
                cfginfo.statement = NULL;
                Edge edge;
                edge.blockid = blockcfg[i].pred[0];
                edge.statementid = blockcfg[blockcfg[i].pred[0]].blockstatement.size() - 1;
                cfginfo.pred.push_back(edge);
                blockcfg[i].blockstatement.push_back(cfginfo);
            } // exit
            else if (blockcfg[i].succ.size() != 0 && blockcfg[i].pred.size() == 0)
            {
                CFGInfo cfginfo;
                cfginfo.statementid = 0;
                cfginfo.statement = NULL;
                for (int j = 0; j < param.size(); j++)
                    cfginfo.param.push_back(param[j]);
                Edge edge;
                edge.blockid = blockcfg[i].succ[0];
                edge.statementid = 0;
                cfginfo.succ.push_back(edge);
                blockcfg[i].blockstatement.push_back(cfginfo);
            } // entry
            else
            {
                CFGInfo cfginfo;
                cfginfo.statementid = 0;
                cfginfo.statement = NULL;
                Edge edge;
                edge.blockid = blockcfg[i].succ[0];
                edge.statementid = 0;
                cfginfo.succ.push_back(edge);
                Edge edge1;
                edge1.blockid = blockcfg[i].pred[0];
                edge1.statementid = blockcfg[blockcfg[i].pred[0]].blockstatement.size() - 1;
                cfginfo.pred.push_back(edge1);
                blockcfg[i].blockstatement.push_back(cfginfo);
            }
        } // entry or exit
        else
        {
            for (int j = 0; j < blockcfg[i].blockstatement.size(); j++)
            {
                if (j == 0)
                {
                    for (int k = 0; k < blockcfg[i].pred.size(); k++)
                    {
                        Edge edge;
                        edge.blockid = blockcfg[i].pred[k];
                        if (edge.blockid == blockNum - 1)
                            edge.statementid = 0;
                        else
                            edge.statementid = blockcfg[edge.blockid].blockstatement.size() - 1;
                        blockcfg[i].blockstatement[j].pred.push_back(edge);
                    }
                }
                else
                {
                    Edge edge;
                    edge.blockid = i;
                    edge.statementid = j - 1;
                    blockcfg[i].blockstatement[j].pred.push_back(edge);
                }
                if (j == blockcfg[i].blockstatement.size() - 1)
                {
                    for (int k = 0; k < blockcfg[i].succ.size(); k++)
                    {
                        Edge edge;
                        edge.blockid = blockcfg[i].succ[k];
                        edge.statementid = 0;
                        blockcfg[i].blockstatement[j].succ.push_back(edge);
                    }
                }
                else
                {
                    Edge edge;
                    edge.blockid = i;
                    edge.statementid = j + 1;
                    blockcfg[i].blockstatement[j].succ.push_back(edge);
                }
            }
        }
    }
}

void ForwardDominanceTree::dumpStmtCFG()
{
    for (int i = 0; i < blockNum; i++)
    {
        for (int j = 0; j < blockcfg[i].blockstatement.size(); j++)
        {
            std::cout << "Block " << blockcfg[i].blockid << " ";
            std::cout << "Stmt " << blockcfg[i].blockstatement[j].statementid << " \n";
            std::cout << "   pred:";
            if (blockcfg[i].blockstatement[j].pred.size() == 0)
                cout << "NULL\n";
            else
            {
                for (int k = 0; k < blockcfg[i].blockstatement[j].pred.size(); k++)
                {
                    std::cout << "block " << blockcfg[i].blockstatement[j].pred[k].blockid << " "
                              << "stmt " << blockcfg[i].blockstatement[j].pred[k].statementid << " ";
                }
            }
            std::cout << "\n   succ:";
            if (blockcfg[i].blockstatement[j].succ.size() == 0)
                cout << "NULL\n";
            else
            {
                for (int k = 0; k < blockcfg[i].blockstatement[j].succ.size(); k++)
                {
                    std::cout << "block " << blockcfg[i].blockstatement[j].succ[k].blockid << " "
                              << "stmt " << blockcfg[i].blockstatement[j].succ[k].statementid << " ";
                }
            }
            std::cout << endl;
        }
        std::cout << endl;
    }
}

void ForwardDominanceTree::InitTreeNodeVector()
{
    for (int i = 0; i < blockNum; i++)
    {
        // std::cout<<"pred\n";
        if (blockcfg[i].pred.size() != 0)
        {
            for (int j = 0; j < blockcfg[i].pred.size(); j++)
            {
                FDT[i].reverse_succ.push_back(blockcfg[i].pred[j]);
            }
        }
        // std::cout<<"succ\n";
        if (blockcfg[i].succ.size() != 0)
        {
            for (int j = 0; j < blockcfg[i].succ.size(); j++)
            {
                FDT[i].reverse_pred.push_back(blockcfg[i].succ[j]);
            }
        }
        FDT[i].isvisit = false;
        if (FDT[i].blockid == 0)
        {
            FDT[i].Outvector.push_back(1);
            for (int j = 1; j < blockNum; j++)
            {
                FDT[i].Outvector.push_back(0);
            }
        }
        else
        {
            for (int j = 0; j < blockNum; j++)
            {
                FDT[i].Outvector.push_back(1);
            }
        }
        for (int j = 0; j < blockNum; j++)
        {
            FDT[i].Invector.push_back(0);
        }
        // std::cout <<"i=" << i <<std::endl;
    }
}

void ForwardDominanceTree::calculate_block(int num)
{
    if (FDT[num].isvisit == true)
        return;
    queue<TreeNode> q;
    q.push(FDT[num]);
    FDT[num].isvisit = true;
    while (!q.empty())
    {
        TreeNode tempblock = q.front();
        vector<int> temp;
        for (int i = 0; i < blockNum; i++)
            temp.push_back(1);
        if (tempblock.reverse_pred.empty() == false)
        {
            for (int i = 0; i < tempblock.reverse_pred.size(); i++)
            {
                for (int j = 0; j < blockNum; j++)
                {
                    if (FDT[tempblock.reverse_pred[i]].Outvector[j] == 0)
                        temp[j] = 0;
                }
            }
            tempblock.Invector = temp;
        }
        vector<int> res;
        for (int i = 0; i < blockNum; i++)
            res.push_back(tempblock.Invector[i]);
        res[tempblock.blockid] = 1;
        FDT[tempblock.blockid].Outvector = res;
        for (int i = 0; i < tempblock.reverse_succ.size(); i++)
        {
            if (FDT[tempblock.reverse_succ[i]].isvisit == false)
            {
                q.push(FDT[tempblock.reverse_succ[i]]);
                FDT[tempblock.reverse_succ[i]].isvisit = true;
            }
        }
        q.pop();
    }
}

void ForwardDominanceTree::ConstructFDT()
{
    bool ischanging = true;
    std::vector<TreeNode> pre_blockbitvector;
    for (int i = 0; i < blockNum; i++)
    {
        pre_blockbitvector.push_back(FDT[i]);
    }
    int count = 0;
    while (ischanging)
    {
        calculate_block(0);
        // std::cout << count << std::endl;
        // blockvector_output();
        for (int i = 0; i < blockNum; i++)
        {
            if (FDT[i].isvisit == false)
            {
                // std::cout << i <<std::endl;
                std::cout << "wrong bfs" << std::endl;
                for (int j = 0; j < blockNum; j++)
                {
                    FDT[i].Invector[j] = 0;
                    FDT[i].Outvector[j] = 1;
                }
            }
        }
        bool allchange = false;
        for (int i = 0; i < blockNum; i++)
        {
            if (pre_blockbitvector[i].Outvector != FDT[i].Outvector)
            {
                allchange = true;
            }
        }
        if (allchange == true)
        {
            ischanging = true;
            for (int i = 0; i < blockNum; i++)
                FDT[i].isvisit = false;
            for (int i = 0; i < blockNum; i++)
                pre_blockbitvector[i] = FDT[i];
        }
        else
        {
            ischanging = false;
            for (int i = 0; i < blockNum; i++)
                FDT[i].isvisit = false;
        }
        count++;
    }
    // std::cout<< "iteration time: " << count <<std::endl;
}

void ForwardDominanceTree::Getidom()
{
    for (int i = 0; i < blockNum; i++)
    {
        for (int j = 0; j < blockNum; j++)
        {
            if (FDT[i].Outvector[j] == 1)
            {
                if (j != FDT[i].blockid)
                {
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
    for (int i = 0; i < blockNum; i++)
    {
        for (auto iter = FDT[i].postdom.begin(); iter != FDT[i].postdom.end(); iter++)
        {
            int trypost = *iter;
            std::vector<int> temp(FDT[i].postdom.begin(), FDT[i].postdom.end());
            auto newiter = std::remove(std::begin(temp), std::end(temp), trypost);
            temp.erase(newiter, std::end(temp));
            // std::cout <<i <<"temp\n";
            // for(int j=0;j<temp.size();j++)
            //    std::cout<< temp[j];
            // std::cout <<endl;
            // std::cout <<"temp\n";
            if (FDT[trypost].postdom.size() == temp.size())
            {
                // std::cout <<"post = " << trypost << " "<<endl;
                bool issame = true;
                for (int k = 0; k < FDT[trypost].postdom.size(); k++)
                {
                    // std::cout << "trypost " << FDT[trypost].postdom[k] << " " << "cur" << temp[k]<<endl;
                    if (FDT[trypost].postdom[k] == temp[k])
                    {
                    }
                    else
                        issame = false;
                }
                if (issame == true)
                {
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
    for (int i = 0; i < blockNum; i++)
    {
        if (i != 0)
        {
            FDT[FDT[i].idom].real_succ.push_back(FDT[i].blockid);
        }
    }
    for (int i = 0; i < blockNum; i++)
    {
        FDT[i].blockstatement = blockcfg[i].blockstatement;
    }
    for (int i = 0; i < blockNum; i++)
    {
        if (i == 0)
        {
            vector<Edge>().swap(FDT[0].blockstatement[0].pred);
            vector<Edge>().swap(FDT[0].blockstatement[0].succ);
            for (int j = 0; j < FDT[i].real_succ.size(); j++)
            {
                Edge edge;
                edge.blockid = FDT[i].real_succ[j];
                edge.statementid = blockcfg[edge.blockid].blockstatement.size() - 1;
                FDT[0].blockstatement[0].succ.push_back(edge);
            }
        }
        else if (i == blockNum - 1)
        {
            vector<Edge>().swap(FDT[i].blockstatement[0].pred);
            vector<Edge>().swap(FDT[i].blockstatement[0].succ);
            Edge edge;
            edge.blockid = FDT[i].idom;
            edge.statementid = 0;
            FDT[i].blockstatement[0].pred.push_back(edge);
        }
        else
        {
            for (int j = 0; j < FDT[i].blockstatement.size(); j++)
            {
                vector<Edge>().swap(FDT[i].blockstatement[j].pred);
                vector<Edge>().swap(FDT[i].blockstatement[j].succ);
                if (j == 0)
                {
                    for (int k = 0; k < FDT[i].real_succ.size(); k++)
                    {
                        Edge edge;
                        edge.blockid = FDT[i].real_succ[k];
                        edge.statementid = blockcfg[edge.blockid].blockstatement.size() - 1;
                        FDT[i].blockstatement[j].succ.push_back(edge);
                    }
                }
                else
                {
                    Edge edge;
                    edge.blockid = i;
                    edge.statementid = j - 1;
                    FDT[i].blockstatement[j].succ.push_back(edge);
                }
                if (j == FDT[i].blockstatement.size() - 1)
                {
                    Edge edge;
                    edge.blockid = FDT[i].idom;
                    edge.statementid = 0;
                    FDT[i].blockstatement[j].pred.push_back(edge);
                }
                else
                {
                    Edge edge;
                    edge.blockid = i;
                    edge.statementid = j + 1;
                    FDT[i].blockstatement[j].pred.push_back(edge);
                }
            }
        }
    }
    vector<BlockStmt> temp;
    for (int i = 0; i < blockNum; i++)
    {
        BlockStmt blockstmt;
        blockstmt.stmtnum = FDT[i].blockstatement.size();
        for (int j = 0; j < blockstmt.stmtnum; j++)
            blockstmt.fpostdom.push_back(0);
        temp.push_back(blockstmt);
    }
    for (int i = 0; i < blockNum; i++)
    {
        for (int j = 0; j < FDT[i].blockstatement.size(); j++)
        {
            vector<BlockStmt> tempb_stmt = temp;
            // std::cout << "block " << i << " stmt " << j << endl;
            for (int k = 0; k < FDT[i].postdom.size(); k++)
            {
                // std::cout <<"fdt " << FDT[i].postdom[k] << " ";
                for (int t = 0; t < tempb_stmt[FDT[i].postdom[k]].stmtnum; t++)
                {
                    tempb_stmt[FDT[i].postdom[k]].fpostdom[t] = 1;
                }
            }
            int cur_stmtid = FDT[i].blockstatement[j].statementid;
            // cout << "\ncur_stmtid = " << cur_stmtid << " " << FDT[i].blockstatement.size()<< endl;
            for (int k = cur_stmtid; k < FDT[i].blockstatement.size(); k++)
            {
                // std::cout << "stmtid " << k << " ";
                tempb_stmt[FDT[i].blockid].fpostdom[k] = 1;
            }
            FDT[i].blockstatement[j].postdom_stmt = tempb_stmt;
            // std::cout << "good\n";
            // std::cout << endl;
        }
    }
    // TODO: construct stmtcfg edge;
}

void ForwardDominanceTree::dumpFDT()
{
    std::cout << endl;
    for (int i = 0; i < blockNum; i++)
    {
        std::cout << "BLOCK " << FDT[i].blockid << " " << FDT[i].blockstatement.size() << std::endl;
        std::cout << "   succ: ";
        if (FDT[i].real_succ.size() == 0)
        {
            std::cout << "NULL" << std::endl;
        }
        else
        {
            for (int j = 0; j < FDT[i].real_succ.size(); j++)
            {
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

void ForwardDominanceTree::dumpStmtFDT()
{
    std::cout << endl;
    for (int i = 0; i < blockNum; i++)
    {
        for (int j = 0; j < FDT[i].blockstatement.size(); j++)
        {
            std::cout << "Block " << FDT[i].blockid << " ";
            std::cout << "Stmt " << FDT[i].blockstatement[j].statementid << " \n";
            std::cout << "   pred:";
            if (FDT[i].blockstatement[j].pred.size() == 0)
                cout << "NULL";
            else
            {
                for (int k = 0; k < FDT[i].blockstatement[j].pred.size(); k++)
                {
                    std::cout << "block " << FDT[i].blockstatement[j].pred[k].blockid << " "
                              << "stmt " << FDT[i].blockstatement[j].pred[k].statementid << " ";
                }
            }
            std::cout << "\n   succ:";
            if (FDT[i].blockstatement[j].succ.size() == 0)
                cout << "NULL";
            else
            {
                for (int k = 0; k < FDT[i].blockstatement[j].succ.size(); k++)
                {
                    std::cout << "block " << FDT[i].blockstatement[j].succ[k].blockid << " "
                              << "stmt " << FDT[i].blockstatement[j].succ[k].statementid << " ";
                }
            }
            std::cout << endl;
        }
    }
}

ControlDependenceGraph::ControlDependenceGraph(ASTManager *manager, ASTResource *resource, CallGraph *call_graph, ForwardDominanceTree *forwarddominancetree)
{
    this->manager = manager;
    this->resource = resource;
    this->call_graph = call_graph;
    this->forward_dominance_tree = forwarddominancetree;
}

void ControlDependenceGraph::ConstructCDG()
{
    allFunctions = call_graph->getAllFunctions();
    for (int i = 0; i < forward_dominance_tree->blockcfg_forest.size(); i++)
    {
        vector<BlockInfo> tempcfg = forward_dominance_tree->blockcfg_forest[i];
        vector<TreeNode> tempfdt = forward_dominance_tree->FDT_forest[i];
        for (int j = 0; j < tempcfg.size(); j++)
        {
            BlockStmt blockstmt;
            blockstmt.stmtnum = tempcfg[j].blockstatement.size();
            for (int k = 0; k < blockstmt.stmtnum; k++)
                blockstmt.fpostdom.push_back(0);
            Postdom.push_back(blockstmt);
        }
        for (int j = 0; j < tempcfg.size(); j++)
        {
            CDGNode cdgnode;
            cdgnode.blockid = tempcfg[j].blockid;
            cdgnode.blockstatement = tempfdt[j].blockstatement;
            CalculateControlDependent(&cdgnode, i);
            CDG.push_back(cdgnode);
        }
        FillPred();
        // CompleteCFGEdge();
        SM = &manager->getFunctionDecl(allFunctions[i])->getASTContext().getSourceManager();
        GetStmtSourceCode(CDG.size());
        CDG_forest.push_back(CDG);
        vector<CDGNode>().swap(CDG);
    }
}

void ControlDependenceGraph::CalculateControlDependent(CDGNode *cdgnode, int treenum)
{
    // std::cout << cdgnode->blockid << endl;
    for (int i = 0; i < cdgnode->blockstatement.size(); i++)
    {
        vector<BlockStmt> UnionPostDom = CalculateStmtUnionSuccPostdom(cdgnode->blockid, cdgnode->blockstatement[i].statementid, treenum);
        vector<TreeNode> tempfdt = forward_dominance_tree->FDT_forest[treenum];
        vector<BlockInfo> tempcfg = forward_dominance_tree->blockcfg_forest[treenum];
        vector<Edge>().swap(cdgnode->blockstatement[i].succ);
        vector<Edge>().swap(cdgnode->blockstatement[i].pred);
        int blockNum = tempcfg.size();
        for (int j = 0; j < tempcfg[cdgnode->blockid].blockstatement[cdgnode->blockstatement[i].statementid].succ.size(); j++)
        {
            for (int k = 0; k < blockNum; k++)
            {
                Edge edge = tempcfg[cdgnode->blockid].blockstatement[cdgnode->blockstatement[i].statementid].succ[j];
                vector<int> tempfdom = tempfdt[edge.blockid].blockstatement[edge.statementid].postdom_stmt[k].fpostdom;
                for (int t = 0; t < tempfdom.size(); t++)
                {
                    if (tempfdom[t] == 1)
                    {
                        if (UnionPostDom[k].fpostdom[t] == 0)
                        {
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
    vector<int> UnionPostDom = CalculateUnionSuccPostdom(cdgnode->blockid, treenum);
    vector<TreeNode> tempfdt = forward_dominance_tree->FDT_forest[treenum];
    vector<BlockInfo> tempcfg = forward_dominance_tree->blockcfg_forest[treenum];
    int blockNum = tempfdt.size();
    for (int i = 0; i < blockNum; i++)
    {
        cdgnode->controldependent.push_back(0);
    }
    for (int i = 0; i < tempcfg[cdgnode->blockid].succ.size(); i++)
    {
        for (int j = 0; j < blockNum; j++)
        {
            if (tempfdt[tempcfg[cdgnode->blockid].succ[i]].Outvector[j] == 1)
            {
                if (UnionPostDom[j] == 0)
                {
                    cdgnode->controldependent[j] = 1;
                }
            }
        }
    }
    for (int i = 0; i < cdgnode->controldependent.size(); i++)
    {
        if (cdgnode->controldependent[i] == 1)
        {
            cdgnode->succ.push_back(i);
        }
    }
}

vector<BlockStmt> ControlDependenceGraph::CalculateStmtUnionSuccPostdom(int blockid, int stmtid, int treenum)
{
    vector<TreeNode> tempfdt = forward_dominance_tree->FDT_forest[treenum];
    vector<BlockInfo> tempcfg = forward_dominance_tree->blockcfg_forest[treenum];
    int blockNum = tempfdt.size();
    vector<BlockStmt> Unionpost;
    for (int i = 0; i < blockNum; i++)
    {
        BlockStmt blockstmt;
        blockstmt.stmtnum = tempfdt[i].blockstatement.size();
        for (int j = 0; j < blockstmt.stmtnum; j++)
            blockstmt.fpostdom.push_back(1);
        Unionpost.push_back(blockstmt);
    }
    for (int i = 0; i < tempcfg[blockid].blockstatement[stmtid].succ.size(); i++)
    {
        for (int j = 0; j < blockNum; j++)
        {
            vector<int> tempfdom = tempfdt[tempcfg[blockid].blockstatement[stmtid].succ[i].blockid].blockstatement[tempcfg[blockid].blockstatement[stmtid].succ[i].statementid].postdom_stmt[j].fpostdom;
            for (int k = 0; k < tempfdom.size(); k++)
            {
                if (tempfdom[k] == 0)
                {
                    Unionpost[j].fpostdom[k] = 0;
                }
            }
        }
    }
    return Unionpost;
}

vector<int> ControlDependenceGraph::CalculateUnionSuccPostdom(int blockid, int treenum)
{
    vector<TreeNode> tempfdt = forward_dominance_tree->FDT_forest[treenum];
    vector<BlockInfo> tempcfg = forward_dominance_tree->blockcfg_forest[treenum];
    int blockNum = tempfdt.size();
    vector<int> UnionPost;
    for (int i = 0; i < blockNum; i++)
        UnionPost.push_back(1);
    for (int i = 0; i < tempcfg[blockid].succ.size(); i++)
    {
        for (int j = 0; j < blockNum; j++)
        {
            if (tempfdt[tempcfg[blockid].succ[i]].Outvector[j] == 0)
            {
                UnionPost[j] = 0;
            }
        }
    }
    return UnionPost;
}

void ControlDependenceGraph::FillPred()
{
    for (int i = 0; i < CDG.size(); i++)
    {
        for (int j = 0; j < CDG[i].succ.size(); j++)
        {
            CDG[CDG[i].succ[j]].pred.push_back(i);
        }
    }
    for (int i = 0; i < CDG.size(); i++)
    {
        for (int j = 0; j < CDG[i].blockstatement.size(); j++)
        {
            for (int k = 0; k < CDG[i].blockstatement[j].succ.size(); k++)
            {
                Edge edge = CDG[i].blockstatement[j].succ[k];
                Edge predge;
                predge.blockid = i;
                predge.statementid = j;
                CDG[edge.blockid].blockstatement[edge.statementid].pred.push_back(predge);
            }
        }
    }
}

void ControlDependenceGraph::CompleteCFGEdge()
{
    for (int i = 0; i < CDG.size(); i++)
    {
        for (int j = 0; j < CDG[i].blockstatement.size(); j++)
        {
            if (j != 0)
            {
                Edge edge;
                edge.blockid = i;
                edge.statementid = j - 1;
                CDG[i].blockstatement[j].pred.push_back(edge);
            }
            if (j != CDG[i].blockstatement.size() - 1)
            {
                Edge edge;
                edge.blockid = i;
                edge.statementid = j + 1;
                CDG[i].blockstatement[j].succ.push_back(edge);
            }
        }
    }
}

void ControlDependenceGraph::dumpCDG()
{
    for (int i = 0; i < CDG_forest.size(); i++)
    {
        vector<CDGNode> tempcdg = CDG_forest[i];
        for (int j = 0; j < tempcdg.size(); j++)
        {
            std::cout << "BLOCK " << tempcdg[j].blockid << " " << tempcdg[j].blockstatement.size() << std::endl;
            std::cout << "   pred: ";
            if (tempcdg[j].pred.size() == 0)
            {
                std::cout << "NULL" << std::endl;
            }
            else
            {
                for (int k = 0; k < tempcdg[j].pred.size(); k++)
                {
                    std::cout << tempcdg[j].pred[k] << " ";
                }
                std::cout << std::endl;
            }
            std::cout << "   succ: ";
            if (tempcdg[j].succ.size() == 0)
            {
                std::cout << "NULL" << std::endl;
            }
            else
            {
                for (int k = 0; k < tempcdg[j].succ.size(); k++)
                {
                    std::cout << tempcdg[j].succ[k] << " ";
                }
                std::cout << std::endl;
            }
        }
    }
    std::cout << endl;
}

int ControlDependenceGraph::CalculateStmtNo(Edge edge, int treenum)
{
    int res = 0;
    for (int i = 0; i < edge.blockid; i++)
    {
        res = res + CDG_forest[treenum][i].blockstatement.size();
    }
    res = res + edge.statementid + 1 - 1;
    return res;
}

void ControlDependenceGraph::dumpStmtCDG()
{
    std::cout << "dumpstmt\n";
    for (int i = 0; i < CDG_forest.size(); i++)
    {
        vector<CDGNode> tempcdg = CDG_forest[i];
        for (int j = 0; j < tempcdg.size(); j++)
        {
            for (int k = 0; k < tempcdg[j].blockstatement.size(); k++)
            {
                std::cout << "BLOCK " << tempcdg[j].blockid << " Stmt" << tempcdg[j].blockstatement[k].statementid << std::endl;
                std::cout << "   pred: ";
                if (tempcdg[j].blockstatement[k].pred.size() != 0)
                {
                    for (int m = 0; m < tempcdg[j].blockstatement[k].pred.size(); m++)
                    {
                        Edge edge = tempcdg[j].blockstatement[k].pred[m];
                        std::cout << "block " << edge.blockid << " stmt " << edge.statementid << " ";
                    }
                    std::cout << endl;
                }
                else
                {
                    std::cout << "NULL\n";
                }
                std::cout << "   succ: ";
                if (tempcdg[j].blockstatement[k].succ.size() != 0)
                {
                    for (int m = 0; m < tempcdg[j].blockstatement[k].succ.size(); m++)
                    {
                        Edge edge = tempcdg[j].blockstatement[k].succ[m];
                        std::cout << "block " << edge.blockid << " stmt " << edge.statementid << " ";
                    }
                    std::cout << endl;
                }
                else
                {
                    std::cout << "NULL\n";
                }
            }
        }
    }
}

void ControlDependenceGraph::GetStmtSourceCode(int blocknum)
{
    // SM->getsource
    // getSourceLocation
    LangOptions L0;
    L0.CPlusPlus = 1;
    // cout << SM->getBufferData(SM->getFileID(CDG[1].blockstatement[0].statement->getBeginLoc())).size() << endl;
    // CDG[1].blockstatement[0].statement.p
    // common::printLog();
    for (int i = 0; i < blocknum; i++)
    {
        for (int j = 0; j < CDG[i].blockstatement.size(); j++)
        {
            if (CDG[i].blockstatement[j].statement != nullptr)
            {
                std::string buffer1;
                llvm::raw_string_ostream strout1(buffer1);
                CDG[i].blockstatement[j].statement->printPretty(strout1, nullptr, PrintingPolicy(L0));
                string noenter = strout1.str();
                if (noenter[noenter.length() - 1] == '\n')
                {
                    noenter = noenter.substr(0, noenter.length() - 1);
                }
                CDG[i].blockstatement[j].sourcecode = noenter;
                // string statementinfo = CDG[i].blockstatement[j].statement->getSourceRange().printToString(*SM);
                // std::cout << statementinfo << endl;
            }
        }
    }
}

DataDependenceGraph::DataDependenceGraph(ASTManager *manager, ASTResource *resource, CallGraph *call_graph, ForwardDominanceTree *forwarddominancetree)
{
    this->manager = manager;
    this->resource = resource;
    this->call_graph = call_graph;
    this->forward_dominance_tree = forwarddominancetree;
}

void DataDependenceGraph::ConstructDDGForest()
{
    for (int i = 0; i < this->forward_dominance_tree->blockcfg_forest.size(); i++)
    {
        blockcfg = this->forward_dominance_tree->blockcfg_forest[i];
        ConstructDDG();
        vector<BlockInfo>().swap(blockcfg);
        stmtcfg_forest.push_back(stmtcfg);
        vector<StmtBitVector>().swap(stmtcfg);
    }
}

void DataDependenceGraph::ConstructDDG()
{
    this->blocknum = blockcfg.size();
    this->allstmtnum = 0;
    this->bitvectorlength = 0;
    for (int i = 0; i < this->blocknum; i++)
    {
        this->allstmtnum = this->allstmtnum + blockcfg[i].blockstatement.size();
    }
    for (int i = 0; i < this->blocknum; i++)
    {
        if (i == this->blocknum - 1)
        {
            if (this->blockcfg[i].blockstatement[0].param.size() == 0)
                this->bitvectorlength = this->bitvectorlength + blockcfg[i].blockstatement.size();
            else
            {
                this->bitvectorlength = this->bitvectorlength + this->blockcfg[i].blockstatement[0].param.size();
            }
        }
        else
        {
            this->bitvectorlength = this->bitvectorlength + blockcfg[i].blockstatement.size();
        }
    }
    CompleteStmtCFG();
    AnalyzeStmt();
    CalculateGenKill();
    // dumpKillVector();
    DataDependenceCheck();

    // dumpOutVector();
    AddDataDependence();
    // dumpStmtDDG();
}

void DataDependenceGraph::CompleteStmtCFG()
{
    int stmtid = 0;
    for (int i = 0; i < blocknum; i++)
    {
        for (int j = 0; j < blockcfg[i].blockstatement.size(); j++)
        {
            StmtBitVector stmtbitvector;
            stmtbitvector.stmtid = stmtid;
            stmtid++;
            stmtbitvector.edge.blockid = i;
            stmtbitvector.edge.statementid = j;
            stmtbitvector.param = blockcfg[i].blockstatement[j].param;
            stmtbitvector.statement = blockcfg[i].blockstatement[j].statement;
            stmtbitvector.edge_pred = blockcfg[i].blockstatement[j].pred;
            stmtbitvector.edge_succ = blockcfg[i].blockstatement[j].succ;
            for (int k = 0; k < stmtbitvector.edge_pred.size(); k++)
            {
                int add = SwitchEdgeToStmtid(stmtbitvector.edge_pred[k]);
                stmtbitvector.pred.push_back(add);
            }
            for (int k = 0; k < stmtbitvector.edge_succ.size(); k++)
            {
                int add = SwitchEdgeToStmtid(stmtbitvector.edge_succ[k]);
                stmtbitvector.succ.push_back(add);
            }
            stmtbitvector.isvisit = false;
            for (int k = 0; k < bitvectorlength; k++)
            {
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

int DataDependenceGraph::SwitchEdgeToStmtid(Edge edge)
{
    int res = 0;
    for (int i = 0; i < edge.blockid; i++)
    {
        res = res + blockcfg[i].blockstatement.size();
    }
    res = res + edge.statementid;
    return res;
}

void DataDependenceGraph::AnalyzeStmt()
{
    for (int i = 0; i < allstmtnum; i++)
    {
        clang::Stmt *statement = stmtcfg[i].statement;
        if (statement == nullptr && stmtcfg[i].param.size() == 0)
        {
            stmtcfg[i].isusefulstmt = false;
        }
        else if (statement == nullptr && stmtcfg[i].param.size() != 0)
        {
            stmtcfg[i].isusefulstmt = true;
        } // entry
        else
        {
            if (statement->getStmtClass() == clang::Stmt::StmtClass::DeclStmtClass)
            {
                stmtcfg[i].isusefulstmt = true;
                clang::DeclStmt *declstmt = static_cast<clang::DeclStmt *>(statement);
                if (declstmt->isSingleDecl())
                {
                    clang::Decl *oneDecl = declstmt->getSingleDecl();
                    stmtcfg[i].variable = GetStmtLvalue(statement);
                    // std::cout << "good " << i << " " << stmtcfg[i].variable << endl;
                    GetStmtRvalue(statement, &stmtcfg[i].rvalue);
                    if (oneDecl->getKind() == clang::Decl::Kind::Var)
                    {
                        clang::VarDecl *vardecl = static_cast<clang::VarDecl *>(oneDecl);
                    }
                }
                else
                {
                    std::cout << "multiple def\n";
                }
            } // def
            else if (statement->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass)
            {
                clang::BinaryOperator *binaryIter = static_cast<clang::BinaryOperator *>(statement);
                if (binaryIter->isAssignmentOp())
                {
                    stmtcfg[i].isusefulstmt = true;
                    stmtcfg[i].variable = GetStmtLvalue(statement);
                    GetStmtRvalue(statement, &stmtcfg[i].rvalue);
                } // x = 1
                else if (binaryIter->isCompoundAssignmentOp())
                {
                    stmtcfg[i].isusefulstmt = true;
                    stmtcfg[i].variable = GetStmtLvalue(statement);
                    GetStmtRvalue(statement, &stmtcfg[i].rvalue);
                } // ?
                else
                {
                    stmtcfg[i].isusefulstmt = false;
                    GetStmtRvalueU(statement, &stmtcfg[i].rvalue);
                }
            }
            else if (statement->getStmtClass() == clang::Stmt::StmtClass::CompoundAssignOperatorClass)
            {
                stmtcfg[i].isusefulstmt = true;
                stmtcfg[i].variable = GetStmtLvalue(statement);
                GetStmtRvalue(statement, &stmtcfg[i].rvalue);
            } // x += 1
            else if (statement->getStmtClass() == clang::Stmt::StmtClass::UnaryOperatorClass)
            {
                clang::UnaryOperator *unaryIter = static_cast<clang::UnaryOperator *>(statement);
                if (unaryIter->isIncrementDecrementOp())
                {
                    stmtcfg[i].isusefulstmt = true;
                    stmtcfg[i].variable = GetStmtLvalue(statement);
                    GetStmtRvalue(statement, &stmtcfg[i].rvalue);
                }
                else if (unaryIter->isArithmeticOp())
                {
                    stmtcfg[i].isusefulstmt = false;
                    GetStmtRvalueU(statement, &stmtcfg[i].rvalue);
                }
            } // x ++ or x-- or ++x or --x
            else if (statement->getStmtClass() == clang::Stmt::StmtClass::CXXOperatorCallExprClass)
            {
                clang::CXXOperatorCallExpr *cxxopIter = static_cast<clang::CXXOperatorCallExpr *>(statement);
                auto iter = cxxopIter->child_begin();
                if ((*iter) != NULL)
                {
                    clang::ImplicitCastExpr *impliIter = static_cast<clang::ImplicitCastExpr *>((*iter));
                    if (impliIter->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
                    {
                        clang::DeclRefExpr *declIter = static_cast<clang::DeclRefExpr *>(impliIter->getSubExpr());
                        if (declIter->getNameInfo().getAsString() == "operator=")
                        {
                            stmtcfg[i].isusefulstmt = true;
                            stmtcfg[i].variable = GetStmtLvalue(statement);
                            GetStmtRvalue(statement, &stmtcfg[i].rvalue);
                        }
                    }
                }
            }
            else
            {
                // callexpr, returnstmt, cxxconstruct, implicitcast
                stmtcfg[i].isusefulstmt = false;
                if (statement->getStmtClass() != clang::Stmt::StmtClass::CXXConstructExprClass)
                {
                    GetStmtRvalueU(statement, &stmtcfg[i].rvalue);
                }
            }
        }
    }
}

string DataDependenceGraph::GetStmtLvalue(clang::Stmt *statement)
{
    string ret = "nothing";
    if (statement->getStmtClass() == clang::Stmt::StmtClass::DeclStmtClass)
    {
        clang::DeclStmt *declstmt = static_cast<clang::DeclStmt *>(statement);
        if (declstmt->isSingleDecl())
        {
            clang::Decl *oneDecl = declstmt->getSingleDecl();
            if (oneDecl->getKind() == clang::Decl::Kind::Var)
            {
                clang::VarDecl *vardecl = static_cast<clang::VarDecl *>(oneDecl);
                string variableName = vardecl->getNameAsString();
                return variableName;
            }
        }
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass)
    {
        clang::BinaryOperator *binaryIter = static_cast<clang::BinaryOperator *>(statement);
        if (binaryIter->getLHS()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
        {
            clang::DeclRefExpr *declrefiter = static_cast<clang::DeclRefExpr *>(binaryIter->getLHS());
            string variableName = declrefiter->getNameInfo().getAsString();
            return variableName;
        }
        else if (binaryIter->getLHS()->getStmtClass() == clang::Stmt::StmtClass::ArraySubscriptExprClass)
        {
            clang::ArraySubscriptExpr *arrayexpr = static_cast<clang::ArraySubscriptExpr *>(binaryIter->getLHS());
            return analyze_array(arrayexpr);
        }
        else if (binaryIter->getLHS()->getStmtClass() == clang::Stmt::StmtClass::MemberExprClass)
        {
            clang::MemberExpr *memberexpr = static_cast<clang::MemberExpr *>(binaryIter->getLHS());
            return analyze_struct(memberexpr);
        }
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::CompoundAssignOperatorClass)
    {
        clang::CompoundAssignOperator *compoundIter = static_cast<clang::CompoundAssignOperator *>(statement);
        if (compoundIter->getLHS()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
        {
            clang::DeclRefExpr *declrefiter = static_cast<clang::DeclRefExpr *>(compoundIter->getLHS());
            string variableName = declrefiter->getNameInfo().getAsString();
            return variableName;
        }
        else if (compoundIter->getLHS()->getStmtClass() == clang::Stmt::StmtClass::ArraySubscriptExprClass)
        {
            clang::ArraySubscriptExpr *arrayexpr = static_cast<clang::ArraySubscriptExpr *>(compoundIter->getLHS());
            return analyze_array(arrayexpr);
        }
        else if (compoundIter->getLHS()->getStmtClass() == clang::Stmt::StmtClass::MemberExprClass)
        {
            clang::MemberExpr *memberexpr = static_cast<clang::MemberExpr *>(compoundIter->getLHS());
            return analyze_struct(memberexpr);
        }
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::UnaryOperatorClass)
    {
        clang::UnaryOperator *unaryIter = static_cast<clang::UnaryOperator *>(statement);
        if (unaryIter->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
        {
            clang::DeclRefExpr *declrefiter = static_cast<clang::DeclRefExpr *>(unaryIter->getSubExpr());
            string variableName = declrefiter->getNameInfo().getAsString();
            return variableName;
        }
        else if (unaryIter->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::ArraySubscriptExprClass)
        {
            clang::ArraySubscriptExpr *arrayexpr = static_cast<clang::ArraySubscriptExpr *>(unaryIter->getSubExpr());
            return analyze_array(arrayexpr);
        }
        else if (unaryIter->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::MemberExprClass)
        {
            clang::MemberExpr *memberexpr = static_cast<clang::MemberExpr *>(unaryIter->getSubExpr());
            return analyze_struct(memberexpr);
        }
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::CXXOperatorCallExprClass)
    {
        clang::CXXOperatorCallExpr *cxxoperatorIter = static_cast<clang::CXXOperatorCallExpr *>(statement);
        auto iter = cxxoperatorIter->child_begin();
        assert((*iter) != NULL);
        iter++;
        assert((*iter) != NULL);
        if ((*iter)->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
        {
            clang::DeclRefExpr *declrefiter = static_cast<clang::DeclRefExpr *>((*iter));
            return declrefiter->getNameInfo().getAsString();
        }
        else if ((*iter)->getStmtClass() == clang::Stmt::StmtClass::MemberExprClass)
        {
            clang::MemberExpr *memberexpr = static_cast<clang::MemberExpr *>((*iter));
            return analyze_struct(memberexpr);
        }
        else
        {
            std::cout << "missing something\n";
        }
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
    {
        clang::ImplicitCastExpr *impliexpr = static_cast<clang::ImplicitCastExpr *>(statement);
        return GetStmtLvalue(impliexpr->getSubExpr());
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::ParenExprClass)
    {
        clang::ParenExpr *parenexpr = static_cast<clang::ParenExpr *>(statement);
        return GetStmtLvalue(parenexpr->getSubExpr());
    }
    else
    {
        std::cout << "missing some kind of condition\n";
    }
    return ret;
}

string DataDependenceGraph::analyze_array(clang::ArraySubscriptExpr *arrayexpr)
{
    string ret = "nothing";
    if (arrayexpr->getLHS()->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
    {
        clang::ImplicitCastExpr *Implicitexpr = static_cast<clang::ImplicitCastExpr *>(arrayexpr->getLHS());
        if (Implicitexpr->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::ArraySubscriptExprClass)
        {
            clang::ArraySubscriptExpr *Newarrayexpr = static_cast<clang::ArraySubscriptExpr *>(Implicitexpr->getSubExpr());
            return analyze_array(Newarrayexpr);
        }
        else if (Implicitexpr->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
        {
            clang::DeclRefExpr *Declexpr = static_cast<clang::DeclRefExpr *>(Implicitexpr->getSubExpr());
            return Declexpr->getNameInfo().getAsString();
        }
        else if (Implicitexpr->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::MemberExprClass)
        {
            clang::MemberExpr *memberexpr = static_cast<clang::MemberExpr *>(Implicitexpr->getSubExpr());
            return analyze_struct(memberexpr);
        }
    }
    return ret;
}

string DataDependenceGraph::analyze_struct(clang::MemberExpr *structexpr)
{
    string ret = "nothing";
    if (structexpr->getBase()->getStmtClass() == clang::Stmt::StmtClass::MemberExprClass)
    {
        clang::MemberExpr *newstructIter = static_cast<clang::MemberExpr *>(structexpr->getBase());
        return analyze_struct(newstructIter);
    }
    else if (structexpr->getBase()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
    {
        clang::DeclRefExpr *declexpr = static_cast<clang::DeclRefExpr *>(structexpr->getBase());
        return declexpr->getNameInfo().getAsString();
    }
    else if (structexpr->getBase()->getStmtClass() == clang::Stmt::StmtClass::ArraySubscriptExprClass)
    {
        clang::ArraySubscriptExpr *arrayexpr = static_cast<clang::ArraySubscriptExpr *>(structexpr->getBase());
        return analyze_array(arrayexpr);
    }
    return ret;
}

void DataDependenceGraph::GetStmtRvalue(clang::Stmt *statement, vector<string> *rvalue)
{
    if (statement->getStmtClass() == clang::Stmt::StmtClass::DeclStmtClass)
    {
        clang::DeclStmt *declstmt = static_cast<clang::DeclStmt *>(statement);
        if (declstmt->isSingleDecl())
        {
            clang::Decl *oneDecl = declstmt->getSingleDecl();
            if (oneDecl->getKind() == clang::Decl::Kind::Var)
            {
                clang::VarDecl *vardecl = static_cast<clang::VarDecl *>(oneDecl);
                if (vardecl->getInit() != nullptr)
                {
                    clang::Expr *initexpr = vardecl->getInit();
                    if (initexpr->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass)
                    {
                        // rvalue is literal
                    }
                    else if (initexpr->getStmtClass() == clang::Stmt::StmtClass::InitListExprClass)
                    {
                        clang::InitListExpr *initit = static_cast<clang::InitListExpr *>(initexpr);
                        int num = initit->getNumInits();
                        for (int i = 0; i < num; i++)
                        {
                            RecursiveGetOperator(initit->getInit(i), rvalue);
                        }
                    }
                    else if (initexpr->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
                    {
                        clang::ImplicitCastExpr *implicitit = static_cast<clang::ImplicitCastExpr *>(initexpr);
                        RecursiveGetOperator(implicitit->getSubExpr(), rvalue);
                    } // int x = y || int x = !y || int z = 0.5
                    else if (initexpr->getStmtClass() == clang::Stmt::StmtClass::UnaryOperatorClass)
                    {
                        RecursiveGetOperator(initexpr, rvalue);
                    }
                    else if (initexpr->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass)
                    {
                        RecursiveGetOperator(initexpr, rvalue);
                    }
                    else if (initexpr->getStmtClass() == clang::Stmt::StmtClass::CallExprClass)
                    {
                        RecursiveGetOperator(initexpr, rvalue);
                    }
                    else if (initexpr->getStmtClass() == clang::Stmt::StmtClass::CStyleCastExprClass)
                    {
                        RecursiveGetOperator(initexpr, rvalue);
                    }
                    else if (initexpr->getStmtClass() == clang::Stmt::StmtClass::CXXConstructExprClass)
                    {
                        clang::CXXConstructExpr *structIter = static_cast<clang::CXXConstructExpr *>(initexpr);
                        auto iter = structIter->child_begin();
                        for (; iter != structIter->child_end(); iter++)
                        {
                            assert((*iter) != NULL);
                            RecursiveGetOperator((*iter), rvalue);
                        }
                    }
                }
            }
        }
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass)
    {
        clang::BinaryOperator *binaryIter = static_cast<clang::BinaryOperator *>(statement);
        if (binaryIter->getLHS()->getStmtClass() == clang::Stmt::StmtClass::ArraySubscriptExprClass)
        {
            clang::ArraySubscriptExpr *arrayIter = static_cast<clang::ArraySubscriptExpr *>(binaryIter->getLHS());
            string arrayname = analyze_array(arrayIter);
            RecursiveGetOperator(arrayIter, rvalue);
            for (int i = 0; i < rvalue->size(); i++)
            {
                if (arrayname == (*rvalue)[i])
                {
                    auto newiter = std::remove(std::begin((*rvalue)), std::end((*rvalue)), arrayname);
                    rvalue->erase(newiter, std::end((*rvalue)));
                    break;
                }
            }
        }
        else if (binaryIter->getLHS()->getStmtClass() == clang::Stmt::StmtClass::MemberExprClass)
        {
            clang::MemberExpr *memberIter = static_cast<clang::MemberExpr *>(binaryIter->getLHS());
            string structname = analyze_struct(memberIter);
            RecursiveGetOperator(memberIter, rvalue);
            for (int i = 0; i < rvalue->size(); i++)
            {
                if (structname == (*rvalue)[i])
                {
                    auto newiter = std::remove(std::begin((*rvalue)), std::end((*rvalue)), structname);
                    rvalue->erase(newiter, std::end((*rvalue)));
                    break;
                }
            }
        }
        RecursiveGetOperator(binaryIter->getRHS(), rvalue);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::CompoundAssignOperatorClass)
    {
        clang::CompoundAssignOperator *compoundIter = static_cast<clang::CompoundAssignOperator *>(statement);
        if (compoundIter->getLHS()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
        {
            clang::DeclRefExpr *declrefiter = static_cast<clang::DeclRefExpr *>(compoundIter->getLHS());
            RecursiveGetOperator(compoundIter->getLHS(), rvalue);
            RecursiveGetOperator(compoundIter->getRHS(), rvalue);
        }
        else if (compoundIter->getLHS()->getStmtClass() == clang::Stmt::StmtClass::ArraySubscriptExprClass)
        {
            clang::ArraySubscriptExpr *newarrayexpr = static_cast<clang::ArraySubscriptExpr *>(compoundIter->getLHS());
            RecursiveGetOperator(newarrayexpr->getLHS(), rvalue);
            RecursiveGetOperator(newarrayexpr->getRHS(), rvalue);
            RecursiveGetOperator(compoundIter->getRHS(), rvalue);
        }
        else
        {
            RecursiveGetOperator(compoundIter->getLHS(), rvalue);
            RecursiveGetOperator(compoundIter->getRHS(), rvalue);
        }
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::UnaryOperatorClass)
    {
        clang::UnaryOperator *unaryIter = static_cast<clang::UnaryOperator *>(statement);
        if (unaryIter->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
        {
            RecursiveGetOperator(unaryIter->getSubExpr(), rvalue);
        }
        else if (unaryIter->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::ArraySubscriptExprClass)
        {
            clang::ArraySubscriptExpr *arrayiter = static_cast<clang::ArraySubscriptExpr *>(unaryIter->getSubExpr());
            RecursiveGetOperator(arrayiter->getLHS(), rvalue);
            RecursiveGetOperator(arrayiter->getRHS(), rvalue);
        }
        else
        {
            RecursiveGetOperator(unaryIter->getSubExpr(), rvalue);
        }
    } // x++
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::CXXOperatorCallExprClass)
    {
        clang::CXXOperatorCallExpr *cxxoperatorIter = static_cast<clang::CXXOperatorCallExpr *>(statement);
        auto iter = cxxoperatorIter->child_begin();
        assert((*iter) != NULL);
        iter++;
        assert((*iter) != NULL);
        iter++;
        assert((*iter) != NULL);
        return RecursiveGetOperator((*iter), rvalue);
    } // chongzai danshi zhineng chongzai =
}

void DataDependenceGraph::RecursiveGetOperator(clang::Stmt *statement, vector<string> *rvalue)
{
    if (statement == nullptr)
    {
        return;
    }
    if (statement->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
    {
        clang::DeclRefExpr *declrefexp = static_cast<clang::DeclRefExpr *>(statement);
        string name = declrefexp->getNameInfo().getAsString();
        rvalue->push_back(name);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass)
    {
        clang::BinaryOperator *binaryIter = static_cast<clang::BinaryOperator *>(statement);
        RecursiveGetOperator(binaryIter->getLHS(), rvalue);
        RecursiveGetOperator(binaryIter->getRHS(), rvalue);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
    {
        clang::ImplicitCastExpr *implicitit = static_cast<clang::ImplicitCastExpr *>(statement);
        RecursiveGetOperator(implicitit->getSubExpr(), rvalue);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass || statement->getStmtClass() == clang::Stmt::StmtClass::FloatingLiteralClass || statement->getStmtClass() == clang::Stmt::StmtClass::CharacterLiteralClass)
    {
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::ParenExprClass)
    {
        clang::ParenExpr *parenexp = static_cast<clang::ParenExpr *>(statement);
        RecursiveGetOperator(parenexp->getSubExpr(), rvalue);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::UnaryOperatorClass)
    {
        clang::UnaryOperator *unaryIter = static_cast<clang::UnaryOperator *>(statement);
        RecursiveGetOperator(unaryIter->getSubExpr(), rvalue);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::ArraySubscriptExprClass)
    {
        clang::ArraySubscriptExpr *arrayexpr = static_cast<clang::ArraySubscriptExpr *>(statement);
        RecursiveGetOperator(arrayexpr->getLHS(), rvalue);
        RecursiveGetOperator(arrayexpr->getRHS(), rvalue);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::CallExprClass)
    {
        clang::CallExpr *callIter = static_cast<clang::CallExpr *>(statement);
        int argNum = callIter->getNumArgs();
        for (int k = 0; k < argNum; k++)
        {
            RecursiveGetOperator(callIter->getArg(k), rvalue);
        }
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::MemberExprClass)
    {
        clang::MemberExpr *structIter = static_cast<clang::MemberExpr *>(statement);
        RecursiveGetOperator(structIter->getBase(), rvalue);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::CStyleCastExprClass)
    {
        clang::CStyleCastExpr *cstyleIter = static_cast<clang::CStyleCastExpr *>(statement);
        RecursiveGetOperator(cstyleIter->getSubExpr(), rvalue);
    }
    else
    {
        std::cout << "missing sth in rvalue\n";
    }
}

void DataDependenceGraph::GetStmtRvalueU(clang::Stmt *statement, vector<string> *rvalue)
{
    if (statement == nullptr)
    {
        return;
    }
    if (statement->getStmtClass() == clang::Stmt::StmtClass::CallExprClass)
    {
        clang::CallExpr *callIter = static_cast<clang::CallExpr *>(statement);
        int argNum = callIter->getNumArgs();
        for (int k = 0; k < argNum; k++)
        {
            RecursiveGetOperatorU(callIter->getArg(k), rvalue);
        }
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::ReturnStmtClass)
    {
        clang::ReturnStmt *returnIter = static_cast<clang::ReturnStmt *>(statement);
        RecursiveGetOperatorU(returnIter->getRetValue(), rvalue);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::UnaryOperatorClass)
    {
        clang::UnaryOperator *unaryIter = static_cast<clang::UnaryOperator *>(statement);
        assert(unaryIter->isArithmeticOp() == true);
        if (unaryIter->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
        {
            clang::ImplicitCastExpr *implicitit = static_cast<clang::ImplicitCastExpr *>(unaryIter->getSubExpr());
            RecursiveGetOperatorU(implicitit->getSubExpr(), rvalue);
        } //! x (x maybe a boolean or other kind),
        else if (unaryIter->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::ParenExprClass)
        {
            clang::ParenExpr *parenexp = static_cast<clang::ParenExpr *>(unaryIter->getSubExpr());
            RecursiveGetOperatorU(parenexp->getSubExpr(), rvalue);
        }
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass)
    {
        clang::BinaryOperator *binaryIter = static_cast<clang::BinaryOperator *>(statement);
        RecursiveGetOperatorU(binaryIter->getLHS(), rvalue);
        RecursiveGetOperatorU(binaryIter->getRHS(), rvalue);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
    {
        clang::ImplicitCastExpr *impliIter = static_cast<clang::ImplicitCastExpr *>(statement);
        RecursiveGetOperatorU(impliIter->getSubExpr(), rvalue);
    }
    else
    {
        std::cout << "missing sth in useless stmt\n";
    }
}

void DataDependenceGraph::RecursiveGetOperatorU(clang::Stmt *statement, vector<string> *rvalue)
{
    if (statement->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
    {
        clang::DeclRefExpr *declrefexp = static_cast<clang::DeclRefExpr *>(statement);
        string name = declrefexp->getNameInfo().getAsString();
        rvalue->push_back(name);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::ArraySubscriptExprClass)
    {
        clang::ArraySubscriptExpr *arrayexpr = static_cast<clang::ArraySubscriptExpr *>(statement);
        RecursiveGetOperatorU(arrayexpr->getLHS(), rvalue);
        RecursiveGetOperatorU(arrayexpr->getRHS(), rvalue);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
    {
        clang::ImplicitCastExpr *implicitIter = static_cast<clang::ImplicitCastExpr *>(statement);
        RecursiveGetOperatorU(implicitIter->getSubExpr(), rvalue);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::ParenExprClass)
    {
        clang::ParenExpr *parenexp = static_cast<clang::ParenExpr *>(statement);
        RecursiveGetOperatorU(parenexp->getSubExpr(), rvalue);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass)
    {
        clang::BinaryOperator *binaryIter = static_cast<clang::BinaryOperator *>(statement);
        RecursiveGetOperatorU(binaryIter->getLHS(), rvalue);
        RecursiveGetOperatorU(binaryIter->getRHS(), rvalue);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::UnaryOperatorClass)
    {
        clang::UnaryOperator *unaryIter = static_cast<clang::UnaryOperator *>(statement);
        RecursiveGetOperatorU(unaryIter->getSubExpr(), rvalue);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::CallExprClass)
    {
        clang::CallExpr *callIter = static_cast<clang::CallExpr *>(statement);
        int argNum = callIter->getNumArgs();
        for (int k = 0; k < argNum; k++)
        {
            RecursiveGetOperatorU(callIter->getArg(k), rvalue);
        }
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::MemberExprClass)
    {
        clang::MemberExpr *memberIter = static_cast<clang::MemberExpr *>(statement);
        RecursiveGetOperatorU(memberIter->getBase(), rvalue);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::CXXConstructExprClass)
    {
        clang::CXXConstructExpr *cxxIter = static_cast<clang::CXXConstructExpr *>(statement);
        auto iter = cxxIter->child_begin();
        for (; iter != cxxIter->child_end(); iter++)
        {
            assert((*iter) != NULL);
            RecursiveGetOperatorU((*iter), rvalue);
        }
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::CStyleCastExprClass)
    {
        clang::CStyleCastExpr *cstyleIter = static_cast<clang::CStyleCastExpr *>(statement);
        RecursiveGetOperatorU(cstyleIter->getSubExpr(), rvalue);
    }
    else
    {
        // assert(statement->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass);
        // actually this means coder uses char,int,double,long,etc, in a boolean expr,so we ignore it
        return;
    }
}

void DataDependenceGraph::CalculateGenKill()
{
    for (int i = 0; i < allstmtnum; i++)
    {
        if (stmtcfg[i].isusefulstmt == true)
        {
            if (stmtcfg[i].statement != nullptr)
            {
                stmtcfg[i].Genvector[stmtcfg[i].stmtid] = 1;
                vector<int> Kill = KillVariable(stmtcfg[i].variable);
                assert(Kill.size() == bitvectorlength);
                for (int j = 0; j < Kill.size(); j++)
                {
                    stmtcfg[i].Killvector[j] = Kill[j];
                }
            }
            else
            {
                for (int j = 0; j < stmtcfg[i].param.size(); j++)
                {
                    stmtcfg[i].Genvector[stmtcfg[i].stmtid + j] = 1;
                    vector<int> Kill = KillVariable(stmtcfg[i].param[j]);
                    assert(Kill.size() == bitvectorlength);
                    for (int k = 0; k < Kill.size(); k++)
                    {
                        stmtcfg[i].Killvector[k] = Kill[k];
                    }
                }
            }
        }
    }
}

vector<int> DataDependenceGraph::KillVariable(string variable)
{
    vector<int> kill;
    for (int i = 0; i < allstmtnum; i++)
    {
        if (stmtcfg[i].isusefulstmt == true)
        {
            if (stmtcfg[i].param.size() == 0)
            {
                if (variable == stmtcfg[i].variable)
                {
                    kill.push_back(1);
                }
                else
                    kill.push_back(0);
            }
            else
            {
                for (int j = 0; j < stmtcfg[i].param.size(); j++)
                {
                    if (variable == stmtcfg[i].param[j])
                    {
                        kill.push_back(1);
                    }
                    else
                        kill.push_back(0);
                }
            }
        }
        else
            kill.push_back(0);
    }
    return kill;
}

void DataDependenceGraph::DataDependenceCheck()
{
    bool ischanging = true;
    std::vector<StmtBitVector> prestmtcfg;
    for (int i = 0; i < allstmtnum; i++)
    {
        prestmtcfg.push_back(stmtcfg[i]);
    }
    int count = 0;
    while (ischanging)
    {
        CalculateBlock(allstmtnum - 1);
        for (int i = 0; i < allstmtnum; i++)
        {
            if (stmtcfg[i].isvisit == false)
            {
                for (int j = 0; j < bitvectorlength; j++)
                {
                    stmtcfg[i].Invector[j] = 0;
                    stmtcfg[i].Outvector[j] = 0;
                }
            }
        }
        bool allchange = false;
        for (int i = 0; i < allstmtnum; i++)
        {
            if (prestmtcfg[i].Outvector != stmtcfg[i].Outvector)
            {
                allchange = true;
            }
        }
        if (allchange == true)
        {
            ischanging = true;
            for (int i = 0; i < allstmtnum; i++)
                stmtcfg[i].isvisit = false;
            for (int i = 0; i < allstmtnum; i++)
                prestmtcfg[i] = stmtcfg[i];
        }
        else
        {
            ischanging = false;
            for (int i = 0; i < allstmtnum; i++)
                stmtcfg[i].isvisit = false;
        }
        count++;
    }
    // std::cout<< "iteration time: " << count <<std::endl;
}

void DataDependenceGraph::CalculateBlock(int num)
{
    if (stmtcfg[num].isvisit == true)
        return;
    queue<StmtBitVector> q;
    q.push(stmtcfg[num]);
    stmtcfg[num].isvisit = true;
    while (!q.empty())
    {
        StmtBitVector tempblock = q.front();
        vector<int> temp;
        for (int i = 0; i < bitvectorlength; i++)
            temp.push_back(0);
        if (tempblock.pred.empty() == false)
        {
            for (int i = 0; i < tempblock.pred.size(); i++)
            {
                for (int j = 0; j < bitvectorlength; j++)
                {
                    if (stmtcfg[tempblock.pred[i]].Outvector[j] == 1)
                        temp[j] = 1;
                }
            }
            tempblock.Invector = temp;
        }
        vector<int> res;
        for (int i = 0; i < bitvectorlength; i++)
            res.push_back(tempblock.Invector[i]);
        for (int i = 0; i < bitvectorlength; i++)
        {
            if (tempblock.Killvector[i] == 1)
                res[i] = 0;
        }
        for (int i = 0; i < bitvectorlength; i++)
        {
            if (tempblock.Genvector[i] == 1)
                res[i] = 1;
        }
        stmtcfg[tempblock.stmtid].Outvector = res;
        for (int i = 0; i < tempblock.succ.size(); i++)
        {
            if (stmtcfg[tempblock.succ[i]].isvisit == false)
            {
                q.push(stmtcfg[tempblock.succ[i]]);
                stmtcfg[tempblock.succ[i]].isvisit = true;
            }
        }
        q.pop();
    }
}

void DataDependenceGraph::AddDataDependence()
{
    for (int i = 0; i < allstmtnum; i++)
    {
        for (int j = 0; j < stmtcfg[i].rvalue.size(); j++)
        {
            vector<int> preoutvector;
            for (int t = 0; t < bitvectorlength; t++)
            {
                preoutvector.push_back(0);
            }
            for (int t = 0; t < stmtcfg[i].pred.size(); t++)
            {
                int pre = stmtcfg[i].pred[t];
                for (int k = 0; k < bitvectorlength; k++)
                {
                    if (stmtcfg[pre].Outvector[k] == 1)
                        preoutvector[k] = 1;
                }
            }
            for (int k = 0; k < bitvectorlength; k++)
            {
                if (preoutvector[k] == 1)
                {
                    if (k < allstmtnum - 1)
                    {
                        if (stmtcfg[k].variable == stmtcfg[i].rvalue[j])
                        {
                            Edge predge = SwitchStmtidToEdge(stmtcfg[k].stmtid);
                            stmtcfg[i].data_pred.push_back(predge);
                            Edge succedge = SwitchStmtidToEdge(stmtcfg[i].stmtid);
                            stmtcfg[k].data_succ.push_back(succedge);
                            stmtcfg[k].data_dependence.push_back(stmtcfg[k].variable);
                        }
                    }
                    else
                    {
                        for (int m = 0; m < stmtcfg[allstmtnum - 1].param.size(); m++)
                        {
                            if (k - allstmtnum + 1 == m && stmtcfg[allstmtnum - 1].param[m] == stmtcfg[i].rvalue[j])
                            {
                                Edge predge = SwitchStmtidToEdge(allstmtnum - 1);
                                stmtcfg[i].data_pred.push_back(predge);
                                Edge succedge = SwitchStmtidToEdge(stmtcfg[i].stmtid);
                                stmtcfg[allstmtnum - 1].data_succ.push_back(succedge);
                                stmtcfg[allstmtnum - 1].data_dependence.push_back(stmtcfg[allstmtnum - 1].param[m]);
                            }
                        }
                    }
                }
            }
        }
    }
}

Edge DataDependenceGraph::SwitchStmtidToEdge(int stmtid)
{
    int res = 0;
    Edge edge;
    for (int i = 0; i < blocknum; i++)
    {
        res = res + blockcfg[i].blockstatement.size();
        if (stmtid < res)
        {
            edge.blockid = i;
            break;
        }
    }
    res = 0;
    for (int i = 0; i < edge.blockid; i++)
        res = res + blockcfg[i].blockstatement.size();
    edge.statementid = stmtid - res;
    return edge;
}

void DataDependenceGraph::dumpStmtDDG()
{
    for (int i = 0; i < allstmtnum; i++)
    {
        std::cout << "Stmtid: " << stmtcfg[i].stmtid << " "
                  << "Block: " << stmtcfg[i].edge.blockid << " Stmt: " << stmtcfg[i].edge.statementid << endl;
        std::cout << "   Data Dependence pred: ";
        if (stmtcfg[i].data_pred.size() == 0)
        {
            std::cout << "NULL";
        }
        else
        {
            for (int j = 0; j < stmtcfg[i].data_pred.size(); j++)
            {
                std::cout << "block " << stmtcfg[i].data_pred[j].blockid << " stmt " << stmtcfg[i].data_pred[j].statementid << " ";
            }
        }
        std::cout << endl;
        std::cout << "   Data Dependence succ: ";
        if (stmtcfg[i].data_succ.size() == 0)
        {
            std::cout << "NULL";
        }
        else
        {
            for (int j = 0; j < stmtcfg[i].data_succ.size(); j++)
            {
                std::cout << "block " << stmtcfg[i].data_succ[j].blockid << " stmt " << stmtcfg[i].data_succ[j].statementid << " ";
            }
        }
        std::cout << endl;
    }
}

void DataDependenceGraph::dumpOutVector()
{
    for (int i = 0; i < allstmtnum; i++)
    {
        std::cout << "block " << stmtcfg[i].edge.blockid << " stmt " << stmtcfg[i].edge.statementid << " outvector:";
        for (int j = 0; j < bitvectorlength; j++)
        {
            std::cout << stmtcfg[i].Outvector[j];
        }
        std::cout << endl;
    }
}

void DataDependenceGraph::dumpKillVector()
{
    for (int i = 0; i < allstmtnum; i++)
    {
        std::cout << "block " << stmtcfg[i].edge.blockid << " stmt " << stmtcfg[i].edge.statementid << " killvector:";
        for (int j = 0; j < bitvectorlength; j++)
        {
            std::cout << stmtcfg[i].Killvector[j];
        }
        std::cout << endl;
    }
}

ProgramDependencyGraph::ProgramDependencyGraph(ASTManager *manager, ASTResource *resource, CallGraph *call_graph, ControlDependenceGraph *cdg, DataDependenceGraph *ddg)
{
    this->manager = manager;
    this->resource = resource;
    this->call_graph = call_graph;
    this->cdg = cdg;
    this->ddg = ddg;
}

void ProgramDependencyGraph::DrawPdgForest()
{
    blocknum = 0;
    for (int i = 0; i < this->cdg->CDG_forest.size(); i++)
    {
        FunctionDecl *funDecl = manager->getFunctionDecl(this->cdg->allFunctions[i]);
        string funcname = funDecl->getQualifiedNameAsString();
        this->CDG = this->cdg->CDG_forest[i];
        blocknum = this->CDG.size();
        this->stmtcfg = this->ddg->stmtcfg_forest[i];
        WriteDotFile(funcname);
        vector<CDGNode>().swap(CDG);
        vector<StmtBitVector>().swap(stmtcfg);
    }
}

void ProgramDependencyGraph::WriteDotFile(string funcname)
{
    string command = "mkdir -p pdg";
    system(command.c_str());
    string prefix = "pdg/";
    string suffix = ".dot";
    string filename = prefix + funcname + suffix;
    std::fstream out(filename, ios::out);
    std::string head = "digraph \"Program Dependency Graph\" {";
    std::string end = "}";
    std::string label = "    label=\"Program Dependency Graph\"";
    if (out.is_open())
    {
        out << head << std::endl;
        out << label << std::endl
            << std::endl;
    }
    string nodeid = "0xffffffff";
    out << "    Node" << nodeid << " [shape=record,label=\"{Entry}\"];" << endl;
    for (int i = 0; i < this->CDG.size(); i++)
    {
        for (int j = 0; j < this->CDG[i].blockstatement.size(); j++)
        {
            WriteCDGNode(out, i, this->CDG[i].blockstatement[j], nodeid);
        }
    }
    for (int i = 0; i < this->stmtcfg.size(); i++)
    {
        WriteDDGNode(out, stmtcfg[i]);
    }
    /*auto it = blockcfg.begin();
    for (; it != blockcfg.end(); it++) {
        if((*it).succ.size() != 0){
            //std::cout <<(*it).succ[0] << endl;
            WriteNodeDot(out, (*it).blockid,(*it).succ);
        }
    }*/
    out << end << std::endl;
    out.close();
}

void ProgramDependencyGraph::WriteCDGNode(std::ostream &out, int blockid, CFGInfo cdgnode, string startnode)
{
    Edge edge;
    edge.blockid = blockid;
    edge.statementid = cdgnode.statementid;
    // std::cout << "blockid = " << edge.blockid << " stmtid " << edge.statementid << endl;
    int stmtid = this->SwitchEdgeToStmtid(edge);
    // std::cout << "good\n";
    string nodeid = "0x";
    nodeid = nodeid + to_string(stmtid);
    if (cdgnode.statement == nullptr && cdgnode.param.size() != 0)
    {
        out << "    Node" << nodeid << " [shape=record,label=\"{";
        for (int i = 0; i < cdgnode.param.size(); i++)
        {
            out << "Param " << cdgnode.param[i] << " ";
        }
        out << "}\"];" << endl;
    }
    else if (cdgnode.statement == nullptr && cdgnode.param.size() == 0)
    {
    }
    else
    {
        string sourcecode = cdgnode.sourcecode;
        for (int i = 0; i < sourcecode.length(); i++)
        {
            if (sourcecode[i] == '<' || sourcecode[i] == '>' || sourcecode[i] == '"')
            {
                sourcecode.insert(i, "\\");
                i++;
            }
        }
        out << "    Node" << nodeid << " [shape=record,label=\"{";
        out << sourcecode << "}\"];" << endl;
    }
    // std::cout <<"hello\n";
    // std::cout << cdgnode.succ.size() << endl;
    for (int i = 0; i < cdgnode.succ.size(); i++)
    {
        string prefix = "0x";
        // char succid = '0' + succ[i];
        // std::cout <<"hello1\n";
        int succ_stmtid = this->SwitchEdgeToStmtid(cdgnode.succ[i]);
        if (CDG[cdgnode.succ[i].blockid].blockstatement[cdgnode.succ[i].statementid].statement != nullptr)
        {
            // std::cout <<"hello2\n";
            prefix = prefix + to_string(succ_stmtid);
            out << "    Node" << nodeid << " -> "
                << "Node" << prefix;
            out << "[color = red]" << endl;
        }
    }
    if (cdgnode.pred.size() == 0 && cdgnode.statement != nullptr)
    {
        out << "    Node" << startnode << " -> "
            << "Node" << nodeid;
        out << "[style = dotted]" << endl;
    }
}

void ProgramDependencyGraph::WriteDDGNode(std::ostream &out, StmtBitVector ddgnode)
{
    string nodeid = "0x";
    nodeid = nodeid + to_string(ddgnode.stmtid);
    for (int i = 0; i < ddgnode.data_succ.size(); i++)
    {
        string prefix = "0x";
        // char succid = '0' + succ[i];
        int succ_stmtid = this->SwitchEdgeToStmtid(ddgnode.data_succ[i]);
        prefix = prefix + to_string(succ_stmtid);
        out << "    Node" << nodeid << " -> "
            << "Node" << prefix;
        out << "[color = blue,label = \"" << ddgnode.data_dependence[i] << "\"]" << endl;
    }
}

int ProgramDependencyGraph::SwitchEdgeToStmtid(Edge edge)
{
    int res = 0;
    for (int i = 0; i < edge.blockid; i++)
    {
        res = res + this->CDG[i].blockstatement.size();
    }
    res = res + edge.statementid;
    return res;
}

Edge ProgramDependencyGraph::SwitchStmtidToEdge(int stmtid)
{
    int res = 0;
    Edge edge;
    for (int i = 0; i < blocknum; i++)
    {
        res = res + CDG[i].blockstatement.size();
        if (stmtid < res)
        {
            edge.blockid = i;
            break;
        }
    }
    res = 0;
    for (int i = 0; i < edge.blockid; i++)
        res = res + CDG[i].blockstatement.size();
    edge.statementid = stmtid - res;
    return edge;
}