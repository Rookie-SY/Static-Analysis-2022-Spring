#include "checkers/MemoryLeakChecker.h"

void MemoryLeakChecker::check(ASTFunction* _entryFunc)
{
    entryFunc = _entryFunc;
    currentEntryFunc = entryFunc;
    allFunctions = call_graph->getAllFunctions();
    for(auto fun : allFunctions)
    {
        FunctionDecl* funcDecl = manager->getFunctionDecl(fun);
        if(funcDecl->getNameAsString() == "ascii_escape_unicode" ||
            funcDecl->getNameAsString() == "profile_int")
            continue;
        SM = &manager->getFunctionDecl(fun)->getASTContext().getSourceManager();
        Pointers.SM = SM;
        // cout << funcDecl->getNameAsString() << endl;
        LangOptions LangOpts;
        LangOpts.CPlusPlus = true;
        unique_ptr<CFG>& cfg = manager->getCFG(fun);
        // if(funcDecl->getNameAsString() == "RotatingTree_Get")
        // {
            // funcDecl->dump();
            // cfg->dump(LangOpts, true);
        // }
        TraceRecord traceRecord;
        traceRecord.calleeLocation.push(funcDecl->getLocation());
        traceRecord.calleeName.push(funcDecl->getNameAsString());
        handle_cfg(cfg, traceRecord, true);

        report_memory_leak();
        report_double_free();
        report_memory_safety();

        vector<ReportPointer>().swap(leakResult);
        vector<Pointer>().swap(SafetyResult);
        vector<DFreePointer>().swap(DFreeResult);
        vector<string>().swap(DeleteError);
        for(unsigned i = 0; i < allLeakPointers.size(); i++)
            vector<ReportPointer>().swap(allLeakPointers[i]);
        vector< vector<ReportPointer> >().swap(allLeakPointers);
        for(unsigned i = 0; i < allDFreePointers.size(); i++)
            vector<DFreePointer>().swap(allDFreePointers[i]);
        vector< vector<DFreePointer> >().swap(allDFreePointers);
    }
    // if(entryFunc != nullptr)
    // {
    //     FunctionDecl* funcDecl = manager->getFunctionDecl(entryFunc);
    //     // cout << funcDecl->getLocation().printToString(*SM) << endl;
    //     // funcDecl->dump();
    //     SM = &manager->getFunctionDecl(entryFunc)->getASTContext().getSourceManager();
    //     Pointers.SM = SM;
    //     LangOptions LangOpts;
    //     LangOpts.CPlusPlus = true;
    //     unique_ptr<CFG>& cfg = manager->getCFG(entryFunc);
    //     // cfg->dump(LangOpts, true);      
    //     TraceRecord traceRecord;
    //     traceRecord.calleeLocation.push(funcDecl->getLocation());
    //     traceRecord.calleeName.push(funcDecl->getNameAsString());

    //     // handle_cfg(cfg, traceRecord, true);
    // }
    // else
    // {
    //     cout << "\033[31m" << "No entry function detected, process exit.\n" << "\033[0m";
    //     exit(1);
    // }
}

void ProgramPaths::get_paths(vector<int> pathVector, CFGBlock block)
{
    // vector<int>::iterator iter = find(pathVector.begin(), pathVector.end(), block.getBlockID());
    // if(iter != pathVector.end())
    //     return;
    const clang::Stmt *blockStmt = block.getTerminatorStmt();
    if (blockStmt != nullptr)
    {
        clang::Stmt::StmtClass stmtClass = blockStmt->getStmtClass();
        if (stmtClass == clang::Stmt::StmtClass::WhileStmtClass || stmtClass == clang::Stmt::StmtClass::DoStmtClass || stmtClass == clang::Stmt::StmtClass::ForStmtClass)
        {
            int cnt = count(pathVector.begin(), pathVector.end(), block.getBlockID());
            if(cnt == 0)
                pathVector.push_back(block.getBlockID());
            else if(cnt == 1)
            {
                pathVector.push_back(block.getBlockID());
            }
            else if(cnt > 1)
            {
                clang::CFGBlock::succ_iterator blockIter = block.succ_begin();
                blockIter++;
                clang::CFGBlock::AdjacentBlock nextBlock = *blockIter;
                if(!nextBlock.isReachable())
                {
                    blockIter++;
                    return;
                }
                get_paths(pathVector, *nextBlock.getReachableBlock());
                return;
            }
        }
        else
            pathVector.push_back(block.getBlockID());
    }
    else
        pathVector.push_back(block.getBlockID());
    // cout << block.getBlockID() << endl;
    if (block.getBlockID() == 0)
        pathContainer.push_back(pathVector);
    else
    {
        clang::CFGBlock::succ_iterator blockIter = block.succ_begin();
        while (blockIter != block.succ_end())
        {
            clang::CFGBlock::AdjacentBlock nextBlock = *blockIter;
            if(!nextBlock.isReachable())
            {
                blockIter++;
                continue;
            }
            get_paths(pathVector, *nextBlock.getReachableBlock());
            blockIter++;
        }
    }
}

bool ProgramPaths::travel_block(CFGBlock block, bool *isBlockVisited)
{
    if (isBlockVisited[block.getBlockID()])
        return true;

    isBlockVisited[block.getBlockID()] = true;

    blockContainer[block.getBlockID()] = block;

    clang::CFGBlock::succ_iterator blockIter = block.succ_begin();

    while (blockIter != block.succ_end())
    {
        clang::CFGBlock::AdjacentBlock nextBlock = *blockIter;
        if(!nextBlock.isReachable())
            return false;
        bool reachableFlag = travel_block(*nextBlock.getReachableBlock(), isBlockVisited);

        if(!reachableFlag)
            return false;
        blockIter++;
    }
    return true;
}

void MemoryLeakChecker::handle_cfg(unique_ptr<CFG>& CFG, TraceRecord traceRecord, bool isEntryFunc)
{
    ProgramPaths allPaths;
    get_cfg_paths(CFG, allPaths);
    handle_cfg_paths(allPaths, traceRecord, isEntryFunc);
    vector<CFGBlock>().swap(allPaths.blockContainer);
    for(unsigned i = 0; i < allPaths.pathContainer.size(); i++)
        vector<int>().swap(allPaths.pathContainer[i]);
    vector< vector<int> >().swap(allPaths.pathContainer);
}

void MemoryLeakChecker::get_cfg_paths(unique_ptr<CFG> &CFG, ProgramPaths& allPaths)
{
//     if(CFG->getEntry().getBlockID() > 80)
//         return;
    bool *isBlockVisited = new bool[CFG->getEntry().getBlockID() + 1];
    for (int i = 0; i < CFG->getEntry().getBlockID() + 1; i++)
    {
        isBlockVisited[i] = false;
        allPaths.blockContainer.push_back(CFG->getEntry());
    }
    allPaths.travel_block(CFG->getEntry(), isBlockVisited);

    bool hasMalloc = false;
    for(int i = 0; i < CFG->getEntry().getBlockID(); i++)
    {
        CFGBlock currentBlock = allPaths.blockContainer[i];
        BumpVector<CFGElement>::reverse_iterator elemIter;
        for(elemIter = currentBlock.begin(); elemIter != currentBlock.end(); elemIter++)
        {
            CFGElement element = *elemIter;
            assert(element.getKind() == clang::CFGElement::Kind::Statement);
            if (element.getKind() == clang::CFGElement::Kind::Statement)
            {
                llvm::Optional<CFGStmt> stmt = element.getAs<CFGStmt>();
                if (stmt.hasValue() == true)
                {
                    Stmt *statement = const_cast<Stmt *>(stmt.getValue().getStmt());
                    /// if CallExpr(s) exit in DeclStmt or BinaryOperator, need special handle
                    // cout << statement->getStmtClassName() << endl;
                    if(statement->getStmtClass() == Stmt::StmtClass::CallExprClass)
                    {
                        clang::CallExpr* callExpr = static_cast<clang::CallExpr* >(statement);
                        Stmt::child_iterator it = callExpr->child_begin();
                        assert((*it) != nullptr);
                        if((*it)->getStmtClass() == Stmt::StmtClass::ImplicitCastExprClass)
                        {
                            if((*(*it)->child_begin())->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
                            {
                                clang::DeclRefExpr* declRefExpr = static_cast<clang::DeclRefExpr*>(*(*it)->child_begin());
                                if(declRefExpr->getFoundDecl()->getNameAsString() == "malloc")
                                {
                                    hasMalloc = true;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if(!hasMalloc)
        return;
    vector<int> pathVector;
    allPaths.get_paths(pathVector, CFG->getEntry());
}

void MemoryLeakChecker::handle_cfg_paths(ProgramPaths allPaths, TraceRecord traceRecord, bool isEntryFunc)
{
    // cout << allPaths.pathContainer.size() << endl;
    for(unsigned int i = 0; i < allPaths.pathContainer.size(); i++)
    {
        handle_single_path(allPaths, allPaths.pathContainer[i], traceRecord);
        if(isEntryFunc)
            prepare_next_path(i);
    }
    // handle_single_path(allPaths, allPaths.pathContainer[0]);
}

void MemoryLeakChecker::handle_single_path(ProgramPaths allPaths, vector<int> path, TraceRecord traceRecord)
{
    // for(unsigned i = 0; i < path.size() - 1; i++)
    //     std::cout << path[i] << " -> ";
    // std::cout << path[path.size() - 1] << std::endl;
    for (unsigned currentID = 0; currentID < path.size(); currentID++)
    {
        CFGBlock currentBlock = allPaths.blockContainer[path[currentID]];

        BumpVector<CFGElement>::reverse_iterator elemIter;
        for (elemIter = currentBlock.begin(); elemIter != currentBlock.end(); elemIter++)
        {
            CFGElement element = *elemIter;
            assert(element.getKind() == clang::CFGElement::Kind::Statement);

            if (element.getKind() == clang::CFGElement::Kind::Statement)
            {
                llvm::Optional<CFGStmt> stmt = element.getAs<CFGStmt>();
                if (stmt.hasValue() == true)
                {
                    Stmt *statement = const_cast<Stmt *>(stmt.getValue().getStmt());
                    /// if CallExpr(s) exit in DeclStmt or BinaryOperator, need special handle
                    // cout << statement->getStmtClassName() << endl;
                    handle_stmt(statement, elemIter, traceRecord);
                    if(special_case(statement, elemIter))
                        elemIter++;
                }
            }
        }
    }
}

bool MemoryLeakChecker::special_case(Stmt *statement, BumpVector<CFGElement>::reverse_iterator elemIter)
{
    CFGElement nextElement = *(++elemIter);

    llvm::Optional<CFGStmt> nextStmt = nextElement.getAs<CFGStmt>();
    if (nextStmt.hasValue() == true)
    {
        Stmt *nextStatement = const_cast<Stmt *>(nextStmt.getValue().getStmt());
        if (statement->getStmtClass() == clang::Stmt::StmtClass::CallExprClass && 
            (nextStatement->getStmtClass() == clang::Stmt::StmtClass::DeclStmtClass ||
             nextStatement->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass ||
             nextStatement->getStmtClass() == clang::Stmt::StmtClass::ReturnStmtClass))
        {
            string preFuncName, curFuncName;

            clang::CallExpr* preCallExpr = static_cast<clang::CallExpr* >(statement);
            Stmt::child_iterator iter = preCallExpr->child_begin();
            assert((*iter) != nullptr);
            if((*iter)->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
            {
                assert((*(*iter)->child_begin()) != nullptr);
                if((*(*iter)->child_begin())->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
                {
                    clang::DeclRefExpr* preDeclRef = static_cast<clang::DeclRefExpr* >(*(*iter)->child_begin());
                    preFuncName = preDeclRef->getFoundDecl()->getNameAsString();
                }
            }
            
            for(iter = nextStatement->child_begin(); iter != nextStatement->child_end(); iter++)
            {
                if((*iter)->getStmtClass() == clang::Stmt::StmtClass::CStyleCastExprClass)
                {
                    if((*(*iter)->child_begin())->getStmtClass() == clang::Stmt::StmtClass::CallExprClass)
                    {
                        clang::CallExpr* curCallExpr = static_cast<clang::CallExpr* >(*(*iter)->child_begin());
                        Stmt::child_iterator it = curCallExpr->child_begin();
                        assert((*it) != nullptr);
                        if((*it)->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
                        {
                            if((*(*it)->child_begin())->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
                            {
                                clang::DeclRefExpr* curDeclRef = static_cast<clang::DeclRefExpr* >(*(*it)->child_begin());
                                curFuncName = curDeclRef->getFoundDecl()->getNameAsString();
                            }
                        }
                    }
                }
                else if((*iter)->getStmtClass() == clang::Stmt::StmtClass::CallExprClass)
                {
                    clang::CallExpr* curCallExpr = static_cast<clang::CallExpr* >(*iter);
                    Stmt::child_iterator it = curCallExpr->child_begin();
                    assert((*it) != nullptr);
                    if((*it)->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
                    {
                        if((*(*it)->child_begin())->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
                        {
                            clang::DeclRefExpr* curDeclRef = static_cast<clang::DeclRefExpr* >(*(*it)->child_begin());
                            curFuncName = curDeclRef->getFoundDecl()->getNameAsString();
                        }
                    }
                }
                else if((*iter)->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass)
                {
                    clang::BinaryOperator* binaryOp = static_cast<clang::BinaryOperator* >(*iter);
                    if(binaryOp->getLHS()->getStmtClass() == Stmt::StmtClass::CStyleCastExprClass ||
                        binaryOp->getRHS()->getStmtClass() == Stmt::StmtClass::CStyleCastExprClass)
                    {
                        clang::CStyleCastExpr* castExpr;
                        if(binaryOp->getLHS()->getStmtClass() == Stmt::StmtClass::CStyleCastExprClass)
                            castExpr = static_cast<clang::CStyleCastExpr* >(binaryOp->getLHS());
                        else
                            castExpr = static_cast<clang::CStyleCastExpr* >(binaryOp->getRHS());
                        if(castExpr != nullptr && ((*castExpr->child_begin())->getStmtClass() == Stmt::StmtClass::CallExprClass))
                        {
                            clang::CallExpr* curCallExpr = static_cast<clang::CallExpr* >(*(castExpr->child_begin()));
                            Stmt::child_iterator it = curCallExpr->child_begin();
                            assert((*it) != nullptr);
                            if((*it)->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
                            {
                                if((*(*it)->child_begin())->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
                                {
                                    clang::DeclRefExpr* curDeclRef = static_cast<clang::DeclRefExpr* >(*(*it)->child_begin());
                                    curFuncName = curDeclRef->getFoundDecl()->getNameAsString();
                                }
                            }
                        }
                    }
                }
            }
            // if(preFuncName == "_PyArgv_AsWstrList")
            //     cout << preFuncName << "-" << curFuncName << endl;
            if(curFuncName != "" && preFuncName == curFuncName)
                return true;
        }
        else if(statement->getStmtClass() == clang::Stmt::StmtClass::CXXNewExprClass && 
            (nextStatement->getStmtClass() == clang::Stmt::StmtClass::DeclStmtClass ||
             nextStatement->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass))
        {
            return true;
        }
    }
    return false;
}

void MemoryLeakChecker::handle_stmt(Stmt *statement, BumpVector<CFGElement>::reverse_iterator elemIter, TraceRecord traceRecord)
{
    if (statement == nullptr)
        return;
    if(statement->getStmtClass() == clang::Stmt::StmtClass::CallExprClass) {
        handle_callExpr(statement, elemIter, traceRecord);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::DeclStmtClass) {
        handle_declStmt(statement);
    }
    else if(statement->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass) { 
        handle_binaryOperator(statement, traceRecord);
    }
    else if(statement->getStmtClass() == clang::Stmt::StmtClass::CXXNewExprClass) {
        handle_newExpr(statement, elemIter, traceRecord);
    }
    else if(statement->getStmtClass() == clang::Stmt::StmtClass::CXXDeleteExprClass) {
        handle_deleteExpr(statement);
    }
    else if(statement->getStmtClass() == clang::Stmt::StmtClass::CompoundAssignOperatorClass) {
        handle_compoundOp(statement);
    }
    else if(statement->getStmtClass() == clang::Stmt::StmtClass::UnaryOperatorClass) {
        handle_unaryOp(statement);
    }
}

void MemoryLeakChecker::handle_callExpr(Stmt* statement, BumpVector<CFGElement>::reverse_iterator elemIter, TraceRecord traceRecord)
{
    bool isRightCall = false;
    CFGElement nextElement = *(++elemIter);
    llvm::Optional<CFGStmt> nextStmt = nextElement.getAs<CFGStmt>();
    if (nextStmt.hasValue() == true)
    {
        Stmt *nextStatement = const_cast<Stmt *>(nextStmt.getValue().getStmt());
        if (nextStatement->getStmtClass() == clang::Stmt::StmtClass::DeclStmtClass ||
            nextStatement->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass)
        {
            string preFuncName, curFuncName;

            clang::CallExpr* preCallExpr = static_cast<clang::CallExpr* >(statement);
            Stmt::child_iterator iter = preCallExpr->child_begin();
            assert((*iter) != nullptr);
            if((*iter)->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
            {
                assert((*(*iter)->child_begin()) != nullptr);
                if((*(*iter)->child_begin())->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
                {
                    clang::DeclRefExpr* preDeclRef = static_cast<clang::DeclRefExpr* >(*(*iter)->child_begin());
                    preFuncName = preDeclRef->getFoundDecl()->getNameAsString();
                }
            }
            for(iter = nextStatement->child_begin(); iter != nextStatement->child_end(); iter++)
            {
                if((*iter)->getStmtClass() == clang::Stmt::StmtClass::CStyleCastExprClass)
                {
                    if((*(*iter)->child_begin())->getStmtClass() == clang::Stmt::StmtClass::CallExprClass)
                    {
                        clang::CallExpr* curCallExpr = static_cast<clang::CallExpr* >(*(*iter)->child_begin());
                        Stmt::child_iterator it = curCallExpr->child_begin();
                        assert((*it) != nullptr);
                        if((*it)->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
                        {
                            if((*(*it)->child_begin())->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
                            {
                                clang::DeclRefExpr* curDeclRef = static_cast<clang::DeclRefExpr* >(*(*it)->child_begin());
                                curFuncName = curDeclRef->getFoundDecl()->getNameAsString();
                            }
                        }
                    }
                    else if((*(*iter)->child_begin())->getStmtClass() == Stmt::StmtClass::ParenExprClass)
                    {
                        clang::ParenExpr* paren = static_cast<clang::ParenExpr* >(*(*iter)->child_begin());
                        if((*paren->child_begin())->getStmtClass() == Stmt::StmtClass::BinaryOperatorClass)
                        {
                            clang::BinaryOperator* binaryOp = static_cast<clang::BinaryOperator* >(*paren->child_begin());
                            if(binaryOp->getLHS()->getStmtClass() == Stmt::StmtClass::CallExprClass)
                            {
                                clang::CallExpr* curCallExpr = static_cast<clang::CallExpr* >(binaryOp->getLHS());
                                Stmt::child_iterator it = curCallExpr->child_begin();
                                assert((*it) != nullptr);
                                if((*it)->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
                                {
                                    if((*(*it)->child_begin())->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
                                    {
                                        clang::DeclRefExpr* curDeclRef = static_cast<clang::DeclRefExpr* >(*(*it)->child_begin());
                                        curFuncName = curDeclRef->getFoundDecl()->getNameAsString();
                                    }
                                }
                            }
                            else if(binaryOp->getRHS()->getStmtClass() == Stmt::StmtClass::CallExprClass)
                            {
                                clang::CallExpr* curCallExpr = static_cast<clang::CallExpr* >(binaryOp->getRHS());
                                Stmt::child_iterator it = curCallExpr->child_begin();
                                assert((*it) != nullptr);
                                if((*it)->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
                                {
                                    if((*(*it)->child_begin())->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
                                    {
                                        clang::DeclRefExpr* curDeclRef = static_cast<clang::DeclRefExpr* >(*(*it)->child_begin());
                                        curFuncName = curDeclRef->getFoundDecl()->getNameAsString();
                                    }
                                }
                            }
                        }
                    }
                }
                else if((*iter)->getStmtClass() == clang::Stmt::StmtClass::CallExprClass)
                {
                    clang::CallExpr* curCallExpr = static_cast<clang::CallExpr* >(*iter);
                    Stmt::child_iterator it = curCallExpr->child_begin();
                    assert((*it) != nullptr);
                    if((*it)->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
                    {
                        if((*(*it)->child_begin())->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
                        {
                            clang::DeclRefExpr* curDeclRef = static_cast<clang::DeclRefExpr* >(*(*it)->child_begin());
                            curFuncName = curDeclRef->getFoundDecl()->getNameAsString();
                        }
                    }
                }
                else if((*iter)->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass)
                {
                    clang::BinaryOperator* binaryOp = static_cast<clang::BinaryOperator* >(*iter);
                    if(binaryOp->getLHS()->getStmtClass() == Stmt::StmtClass::CallExprClass || binaryOp->getRHS()->getStmtClass() == Stmt::StmtClass::CallExprClass)
                    {
                        clang::CallExpr* curCallExpr;
                        if(binaryOp->getLHS()->getStmtClass() == Stmt::StmtClass::CallExprClass)
                            curCallExpr = static_cast<clang::CallExpr* >(binaryOp->getLHS());
                        else
                            curCallExpr = static_cast<clang::CallExpr* >(binaryOp->getRHS());
                        Stmt::child_iterator it = curCallExpr->child_begin();
                        assert((*it) != nullptr);   
                        if((*it)->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
                        {
                            if((*(*it)->child_begin())->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
                            {
                                clang::DeclRefExpr* curDeclRef = static_cast<clang::DeclRefExpr* >(*(*it)->child_begin());
                                curFuncName = curDeclRef->getFoundDecl()->getNameAsString();
                            }
                        }
                    }
                    else if(binaryOp->getLHS()->getStmtClass() == Stmt::StmtClass::CStyleCastExprClass || binaryOp->getRHS()->getStmtClass() == Stmt::StmtClass::CStyleCastExprClass)
                    {
                        clang::CStyleCastExpr* castExpr;
                        if(binaryOp->getLHS()->getStmtClass() == Stmt::StmtClass::CStyleCastExprClass)
                            castExpr = static_cast<clang::CStyleCastExpr* >(binaryOp->getLHS());
                        else
                            castExpr = static_cast<clang::CStyleCastExpr* >(binaryOp->getRHS());
                        if(castExpr != nullptr && (*castExpr->child_begin())->getStmtClass() == Stmt::StmtClass::CallExprClass)
                        {
                            clang::CallExpr* curCallExpr = static_cast<clang::CallExpr* >(*castExpr->child_begin());
                            Stmt::child_iterator it = curCallExpr->child_begin();
                            assert((*it) != nullptr);
                            if((*it)->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
                            {
                                if((*(*it)->child_begin())->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
                                {
                                    clang::DeclRefExpr* curDeclRef = static_cast<clang::DeclRefExpr* >(*(*it)->child_begin());
                                    curFuncName = curDeclRef->getFoundDecl()->getNameAsString();
                                }
                            }
                        }
                    }
                }
            }
            // if(preFuncName == "_Py_GetEnv")
                // cout << "isRight: " << preFuncName << "-" << curFuncName << endl;
            if(curFuncName != "" && preFuncName == curFuncName)
                isRightCall = true;
        }
    }

    clang::CallExpr* callExprStmt = static_cast<clang::CallExpr* >(statement);

    Stmt::child_iterator iter = callExprStmt->child_begin();
    assert((*iter) != nullptr);
    if((*iter)->getStmtClass() == Stmt::StmtClass::ImplicitCastExprClass)
    {
        assert((*(*iter)->child_begin()) != nullptr);
        if((*(*iter)->child_begin())->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
        {
            clang::DeclRefExpr* declRefExpr = static_cast<clang::DeclRefExpr*>(*(*iter)->child_begin());
            if(declRefExpr->getFoundDecl()->getNameAsString() == "malloc" && isRightCall)
            {
                bool computable = true;
                int localSize = 0;
                Stmt* sizeStmt = (*(++iter));
                if(sizeStmt->getStmtClass() == Stmt::StmtClass::ImplicitCastExprClass ||
                    sizeStmt->getStmtClass() == Stmt::StmtClass::BinaryOperatorClass  ||
                    sizeStmt->getStmtClass() == Stmt::StmtClass::UnaryExprOrTypeTraitExprClass)
                    computable = handle_malloc_size(sizeStmt, localSize);
                else 
                    computable = false;

                /// ignore the second CallExpr
                Stmt *nextStatement = const_cast<Stmt *>(nextStmt.getValue().getStmt());
                if(nextStatement->getStmtClass() == clang::Stmt::StmtClass::DeclStmtClass)
                {
                    clang::DeclStmt* declStmt = static_cast<clang::DeclStmt* >(nextStatement);
                    clang::DeclGroupRef groupDecl = declStmt->getDeclGroup();
                    clang::DeclGroupRef::iterator DGIter = groupDecl.begin();

                    for( ; DGIter != groupDecl.end(); DGIter++)
                    {
                        if((*DGIter)->getKind() == clang::Decl::Kind::Var)
                        {
                            clang::VarDecl* varDecl = static_cast<clang::VarDecl *>(*DGIter);
                            Pointers.new_pointer(varDecl->getNameAsString(), varDecl->getLocation(), traceRecord, 
                                varDecl->getType().getAsString(), false, computable, localSize);
                        }
                    }
                }
                else if(nextStatement->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass)
                {
                    clang::BinaryOperator* binaryOpStmt = static_cast<clang::BinaryOperator* >(nextStatement);
                    if(binaryOpStmt->isAssignmentOp())
                    {
                        assert(binaryOpStmt->getLHS()->getType().getTypePtrOrNull()->isAnyPointerType());
                        assert(binaryOpStmt->getLHS()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass);
                        
                        clang::DeclRefExpr* leftDeclRef = static_cast<clang::DeclRefExpr* >(binaryOpStmt->getLHS());
                        
                        Pointers.special_new_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), leftDeclRef->getFoundDecl()->getLocation(), 
                            declRefExpr->getLocation(), traceRecord, false, leftDeclRef->getType().getAsString(), computable, localSize);
                    }
                }
            }
            else if(declRefExpr->getFoundDecl()->getNameAsString() == "free")
            {
                clang::Stmt* tmpStmt = static_cast<clang::Stmt* >(callExprStmt->getArg(0));

                while(tmpStmt->getStmtClass() != clang::Stmt::StmtClass::DeclRefExprClass)
                {
                    auto children = tmpStmt->children().begin();
                    tmpStmt = *children;
                }
                clang::DeclRefExpr* deletePointer = static_cast<clang::DeclRefExpr* >(tmpStmt); 
                // cout << deletePointer->getFoundDecl()->getLocation().printToString(*SM) << endl;
                Pointers.free_pointer(deletePointer->getFoundDecl()->getNameAsString(), deletePointer->getFoundDecl()->getLocation(), declRefExpr->getExprLoc());
            }
            else
            {
                return;
                if(declRefExpr->getType().getTypePtrOrNull()->isFunctionType())
                {
                    string funcName = declRefExpr->getFoundDecl()->getNameAsString();
                
                    ASTFunction* fun = get_callee_func(funcName);
                    if(fun == nullptr)
                    {
                        cout << declRefExpr->getLocation().printToString(*SM) << endl;
                        return;
                    }
                    FunctionDecl* funcDecl = manager->getFunctionDecl(fun);
                    ASTFunction* tmpEntryFunc = currentEntryFunc;
                    handle_callee(callExprStmt, fun, funcDecl, traceRecord);
                    currentEntryFunc = tmpEntryFunc;
                    SM = &manager->getFunctionDecl(currentEntryFunc)->getASTContext().getSourceManager();
                    Pointers.SM = SM;
                    if(isRightCall)
                    {
                        Stmt *nextStatement = const_cast<Stmt *>(nextStmt.getValue().getStmt());
                        if(nextStatement->getStmtClass() == Stmt::StmtClass::DeclStmtClass)
                        {
                            clang::DeclStmt* declStmt = static_cast<clang::DeclStmt* >(nextStatement);
                            clang::DeclGroupRef groupDecl = declStmt->getDeclGroup();
                            clang::DeclGroupRef::iterator DGIter = groupDecl.begin();
                            for( ; DGIter != groupDecl.end(); DGIter++)
                            {
                                if((*DGIter)->getKind() == clang::Decl::Kind::Var)
                                {
                                    clang::VarDecl* varDecl = static_cast<clang::VarDecl *>(*DGIter);
                                    if(!funcDecl->getReturnType().getTypePtrOrNull()->isAnyPointerType())
                                        return;
                                    assert(funcDecl->hasBody());

                                    Stmt* funcBody = funcDecl->getBody();
                                    clang::Stmt::child_iterator iter = funcBody->child_begin();
                                    for( ; iter != funcBody->child_end(); iter++)
                                        if((*iter)->getStmtClass() == clang::Stmt::StmtClass::ReturnStmtClass)
                                            break;
                                    if((*iter)->getStmtClass() != Stmt::StmtClass::ReturnStmtClass)
                                        return;
                                    clang::ReturnStmt* retStmt = static_cast<clang::ReturnStmt* >(*iter);
                                    if((*(*(*iter)->child_begin())->child_begin())->getStmtClass() != Stmt::StmtClass::DeclRefExprClass)
                                        return;
                                    clang::DeclRefExpr* declRefExpr = static_cast<clang::DeclRefExpr* >(*(*(*iter)->child_begin())->child_begin());
                                    
                                    if((*declStmt->child_begin())->getStmtClass() == Stmt::StmtClass::CallExprClass)
                                    {
                                        Pointers.modify_pointer(varDecl->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(), 
                                            varDecl->getLocation(), declRefExpr->getFoundDecl()->getLocation(), varDecl->getType().getAsString());
                                    }
                                    else if((*declStmt->child_begin())->getStmtClass() == Stmt::StmtClass::BinaryOperatorClass)
                                    {
                                        clang::BinaryOperator* binaryOp = static_cast<clang::BinaryOperator* >(*declStmt->child_begin());
                                        if(binaryOp->getLHS()->getStmtClass() == Stmt::StmtClass::CallExprClass || binaryOp->getRHS()->getStmtClass() == Stmt::StmtClass::CallExprClass)
                                            handle_binary_callee_in_declStmt(varDecl, declRefExpr, binaryOp);
                                        else if(binaryOp->getLHS()->getStmtClass() == Stmt::StmtClass::CStyleCastExprClass || binaryOp->getRHS()->getStmtClass() == Stmt::StmtClass::CStyleCastExprClass)
                                        {
                                            Expr* subLeft = binaryOp->getLHS();
                                            Expr* subRight = binaryOp->getRHS();
                                            if(subLeft->getStmtClass() == Stmt::StmtClass::CStyleCastExprClass)
                                            {
                                                clang::ImplicitCastExpr* subCastExpr = static_cast<clang::ImplicitCastExpr* >(*(*subLeft->child_begin())->child_begin());
                                                Expr* tmp = subCastExpr->getSubExpr();
                                                if(tmp->getStmtClass() == Stmt::StmtClass::DeclRefExprClass)
                                                {
                                                    int base = 0;
                                                    bool op = false;
                                                    if(string(binaryOp->getOpcodeStr()) == "+")
                                                        op = true;
                                                    else if(string(binaryOp->getOpcodeStr()) == "-")
                                                        op = false;
                                                    else{
                                                        cout << string(binaryOp->getOpcodeStr()) << "should not appear as pointer operator.\n";
                                                        return;
                                                    } 
                                                    if(subRight->getStmtClass() == Stmt::StmtClass::IntegerLiteralClass)
                                                    {
                                                        clang::IntegerLiteral* integer = static_cast<clang::IntegerLiteral* >(subRight);
                                                        base = integer->getValue().getZExtValue();
                                                        Pointers.modify_pointer(varDecl->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(), 
                                                            varDecl->getLocation(), declRefExpr->getFoundDecl()->getLocation(), varDecl->getType().getAsString());
                                                        Pointers.handle_one_pointer(varDecl->getNameAsString(), varDecl->getLocation(), base, op);
                                                    }
                                                    else if(subRight->getStmtClass() == Stmt::StmtClass::UnaryExprOrTypeTraitExprClass)
                                                    {
                                                        clang::UnaryExprOrTypeTraitExpr* unaryOp = static_cast<clang::UnaryExprOrTypeTraitExpr* >(subRight);
                                                        base = Pointers.get_unit_length(unaryOp->getArgumentType().getAsString());

                                                        if(base == 0){
                                                            Pointers.handle_computable(varDecl->getNameAsString(), varDecl->getLocation());
                                                            return;
                                                        }
                                                        Pointers.modify_pointer(varDecl->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(), 
                                                            varDecl->getLocation(), declRefExpr->getFoundDecl()->getLocation(), varDecl->getType().getAsString());
                                                        Pointers.handle_one_pointer(varDecl->getNameAsString(), varDecl->getLocation(), base, op);
                                                    }
                                                    else
                                                    {
                                                        Pointers.handle_computable(varDecl->getNameAsString(), varDecl->getLocation());
                                                        return;
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                clang::ImplicitCastExpr* subCastExpr = static_cast<clang::ImplicitCastExpr* >(*(*subRight->child_begin())->child_begin());
                                                Expr* tmp = subCastExpr->getSubExpr();
                                                if(tmp->getStmtClass() == Stmt::StmtClass::DeclRefExprClass)
                                                {
                                                    int base = 0;
                                                    bool op = false;
                                                    if(string(binaryOp->getOpcodeStr()) == "+")
                                                        op = true;
                                                    else if(string(binaryOp->getOpcodeStr()) == "-")
                                                        op = false;
                                                    else{
                                                        cout << string(binaryOp->getOpcodeStr()) << "should not appear as pointer operator.\n";
                                                        return;
                                                    } 
                                                    if(subLeft->getStmtClass() == Stmt::StmtClass::IntegerLiteralClass)
                                                    {
                                                        clang::IntegerLiteral* integer = static_cast<clang::IntegerLiteral* >(subLeft);
                                                        base = integer->getValue().getZExtValue();
                                                        Pointers.modify_pointer(varDecl->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(), 
                                                            varDecl->getLocation(), declRefExpr->getFoundDecl()->getLocation(), varDecl->getType().getAsString());
                                                        Pointers.handle_one_pointer(varDecl->getNameAsString(), varDecl->getLocation(), base, op);
                                                    }
                                                    else if(subLeft->getStmtClass() == Stmt::StmtClass::UnaryExprOrTypeTraitExprClass)
                                                    {
                                                        clang::UnaryExprOrTypeTraitExpr* unaryOp = static_cast<clang::UnaryExprOrTypeTraitExpr* >(subLeft);
                                                        base = Pointers.get_unit_length(unaryOp->getArgumentType().getAsString());

                                                        if(base == 0){
                                                            Pointers.handle_computable(varDecl->getNameAsString(), varDecl->getLocation());
                                                        return;
                                                        }
                                                        Pointers.modify_pointer(varDecl->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(), 
                                                            varDecl->getLocation(), declRefExpr->getFoundDecl()->getLocation(), varDecl->getType().getAsString());
                                                        Pointers.handle_one_pointer(varDecl->getNameAsString(), varDecl->getLocation(), base, op);
                                                    }
                                                    else
                                                    {
                                                        Pointers.handle_computable(varDecl->getNameAsString(), varDecl->getLocation());
                                                        return;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    else if((*declStmt->child_begin())->getStmtClass() == Stmt::StmtClass::CStyleCastExprClass)
                                    {
                                        if((*(*declStmt->child_begin())->child_begin())->getStmtClass() == Stmt::StmtClass::CallExprClass)
                                        {
                                            Pointers.modify_pointer(varDecl->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(), 
                                                varDecl->getLocation(), declRefExpr->getFoundDecl()->getLocation(), varDecl->getType().getAsString());
                                        }
                                        else if((*(*declStmt->child_begin())->child_begin())->getStmtClass() == clang::Stmt::StmtClass::ParenExprClass)
                                        {
                                            clang::ParenExpr* paren = static_cast<clang::ParenExpr* >(*(*declStmt->child_begin())->child_begin());
                                            if((*paren->child_begin())->getStmtClass() == Stmt::StmtClass::BinaryOperatorClass)
                                            {
                                                clang::BinaryOperator* binaryOp = static_cast<clang::BinaryOperator* >(*paren->child_begin());
                                                handle_binary_callee_in_declStmt(varDecl, declRefExpr, binaryOp);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else if(nextStatement->getStmtClass() == Stmt::StmtClass::BinaryOperatorClass)
                        {
                            clang::BinaryOperator* binaryOpStmt = static_cast<clang::BinaryOperator* >(nextStatement);
                            if(!funcDecl->getReturnType().getTypePtrOrNull()->isAnyPointerType())
                                return;
                            assert(funcDecl->hasBody());

                            Stmt* funcBody = funcDecl->getBody();
                            clang::Stmt::child_iterator iter = funcBody->child_begin();
                            for( ; iter != funcBody->child_end(); iter++)
                                if((*iter)->getStmtClass() == clang::Stmt::StmtClass::ReturnStmtClass)
                                    break;
                            if((*iter)->getStmtClass() != Stmt::StmtClass::ReturnStmtClass)
                                return;
                            clang::ReturnStmt* retStmt = static_cast<clang::ReturnStmt* >(*iter);
                            if((*(*(*iter)->child_begin())->child_begin())->getStmtClass() != Stmt::StmtClass::DeclRefExprClass)
                                return;
                            clang::DeclRefExpr* declRefExpr = static_cast<clang::DeclRefExpr* >(*(*(*iter)->child_begin())->child_begin());

                            Expr* leftExpr = binaryOpStmt->getLHS();
                            Expr* rightExpr = binaryOpStmt->getRHS();
                            assert(leftExpr->getType().getTypePtrOrNull()->isAnyPointerType());
                            clang::DeclRefExpr* leftDeclRef = static_cast<clang::DeclRefExpr*>(leftExpr);
                            if(rightExpr->getStmtClass() == clang::Stmt::StmtClass::CallExprClass)
                            {
                                Pointers.modify_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(),
                                    leftDeclRef->getFoundDecl()->getLocation(), declRefExpr->getFoundDecl()->getLocation(), leftDeclRef->getType().getAsString()); 
                            }
                            else if(rightExpr->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass)
                            {
                                clang::BinaryOperator* subBinary = static_cast<clang::BinaryOperator* >(rightExpr);
                                if(subBinary->getLHS()->getStmtClass() == Stmt::StmtClass::CallExprClass || subBinary->getRHS()->getStmtClass() == Stmt::StmtClass::CallExprClass)
                                    handle_binary_callee_in_binaryOp(leftDeclRef, declRefExpr, subBinary);
                                else if(subBinary->getLHS()->getStmtClass() == Stmt::StmtClass::CStyleCastExprClass || subBinary->getRHS()->getStmtClass() == Stmt::StmtClass::CStyleCastExprClass)
                                {
                                    Expr* subLeft = subBinary->getLHS();
                                    Expr* subRight = subBinary->getRHS();
                                    if(subLeft->getStmtClass() == Stmt::StmtClass::CStyleCastExprClass)
                                    {
                                        clang::ImplicitCastExpr* subCastExpr = static_cast<clang::ImplicitCastExpr* >(*(*subLeft->child_begin())->child_begin());
                                        Expr* tmp = subCastExpr->getSubExpr();
                                        if(tmp->getStmtClass() == Stmt::StmtClass::DeclRefExprClass)
                                        {
                                            int base = 0;
                                            bool op = false;
                                            if(string(subBinary->getOpcodeStr()) == "+")
                                                op = true;
                                            else if(string(subBinary->getOpcodeStr()) == "-")
                                                op = false;
                                            else{
                                                cout << string(subBinary->getOpcodeStr()) << "should not appear as pointer operator.\n";
                                                return;
                                            } 
                                            if(subRight->getStmtClass() == Stmt::StmtClass::IntegerLiteralClass)
                                            {
                                                clang::IntegerLiteral* integer = static_cast<clang::IntegerLiteral* >(subRight);
                                                base = integer->getValue().getZExtValue();
                                                Pointers.modify_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(), 
                                                    leftDeclRef->getFoundDecl()->getLocation(), declRefExpr->getFoundDecl()->getLocation(), leftDeclRef->getType().getAsString());
                                                Pointers.handle_one_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), leftDeclRef->getFoundDecl()->getLocation(), base, op);
                                            }
                                            else if(subRight->getStmtClass() == Stmt::StmtClass::UnaryExprOrTypeTraitExprClass)
                                            {
                                                clang::UnaryExprOrTypeTraitExpr* unaryOp = static_cast<clang::UnaryExprOrTypeTraitExpr* >(subRight);
                                                base = Pointers.get_unit_length(unaryOp->getArgumentType().getAsString());

                                                if(base == 0){
                                                    Pointers.handle_computable(leftDeclRef->getFoundDecl()->getNameAsString(), 
                                                        leftDeclRef->getFoundDecl()->getLocation());
                                                return;
                                                }
                                                Pointers.modify_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(), 
                                                    leftDeclRef->getFoundDecl()->getLocation(), declRefExpr->getFoundDecl()->getLocation(), leftDeclRef->getType().getAsString());
                                                Pointers.handle_one_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), leftDeclRef->getFoundDecl()->getLocation(), base, op);
                                            }
                                            else
                                            {
                                                Pointers.handle_computable(leftDeclRef->getFoundDecl()->getNameAsString(), 
                                                                                leftDeclRef->getFoundDecl()->getLocation());
                                                return;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        clang::ImplicitCastExpr* subCastExpr = static_cast<clang::ImplicitCastExpr* >(*(*subRight->child_begin())->child_begin());
                                        Expr* tmp = subCastExpr->getSubExpr();
                                        if(tmp->getStmtClass() == Stmt::StmtClass::DeclRefExprClass)
                                        {
                                            int base = 0;
                                            bool op = false;
                                            if(string(subBinary->getOpcodeStr()) == "+")
                                                op = true;
                                            else if(string(subBinary->getOpcodeStr()) == "-")
                                                op = false;
                                            else{
                                                cout << string(subBinary->getOpcodeStr()) << "should not appear as pointer operator.\n";
                                                return;
                                            } 
                                            if(subLeft->getStmtClass() == Stmt::StmtClass::IntegerLiteralClass)
                                            {
                                                clang::IntegerLiteral* integer = static_cast<clang::IntegerLiteral* >(subLeft);
                                                base = integer->getValue().getZExtValue();
                                                Pointers.modify_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(), 
                                                    leftDeclRef->getFoundDecl()->getLocation(), declRefExpr->getFoundDecl()->getLocation(), leftDeclRef->getType().getAsString());
                                                Pointers.handle_one_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), leftDeclRef->getFoundDecl()->getLocation(), base, op);
                                            }
                                            else if(subLeft->getStmtClass() == Stmt::StmtClass::UnaryExprOrTypeTraitExprClass)
                                            {
                                                clang::UnaryExprOrTypeTraitExpr* unaryOp = static_cast<clang::UnaryExprOrTypeTraitExpr* >(subLeft);
                                                base = Pointers.get_unit_length(unaryOp->getArgumentType().getAsString());

                                                if(base == 0){
                                                    Pointers.handle_computable(leftDeclRef->getFoundDecl()->getNameAsString(), 
                                                        leftDeclRef->getFoundDecl()->getLocation());
                                                return;
                                                }
                                                Pointers.modify_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(), 
                                                    leftDeclRef->getFoundDecl()->getLocation(), declRefExpr->getFoundDecl()->getLocation(), leftDeclRef->getType().getAsString());
                                                Pointers.handle_one_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), leftDeclRef->getFoundDecl()->getLocation(), base, op);
                                            }
                                            else
                                            {
                                                Pointers.handle_computable(leftDeclRef->getFoundDecl()->getNameAsString(), 
                                                                                leftDeclRef->getFoundDecl()->getLocation());
                                                return;
                                            }
                                        }
                                    }
                                }
                            }
                            else if(rightExpr->getStmtClass() == clang::Stmt::StmtClass::CStyleCastExprClass)
                            {
                                if((*rightExpr->child_begin())->getStmtClass() == Stmt::StmtClass::CallExprClass)
                                {
                                    Pointers.modify_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(), 
                                        leftDeclRef->getFoundDecl()->getLocation(), declRefExpr->getFoundDecl()->getLocation(), leftDeclRef->getType().getAsString());
                                }
                                else if((*rightExpr->child_begin())->getStmtClass() == clang::Stmt::StmtClass::ParenExprClass)
                                {
                                    clang::ParenExpr* paren = static_cast<clang::ParenExpr* >(*rightExpr->child_begin());
                                    if((*paren->child_begin())->getStmtClass() == Stmt::StmtClass::BinaryOperatorClass)
                                    {
                                        clang::BinaryOperator* binaryOp = static_cast<clang::BinaryOperator* >(*paren->child_begin());
                                        handle_binary_callee_in_binaryOp(leftDeclRef, declRefExpr, binaryOp);
                                    }
                                }
                            }
                        }
                    }
                }
                
            }
        }
    }
}

ASTFunction* MemoryLeakChecker::get_callee_func(string funcName)
{
    vector<ASTFunction *> childVec = call_graph->getChildren(currentEntryFunc);
    // cout << endl << funcName << endl;
    for(auto fun : childVec)
    {
        FunctionDecl* funDecl = manager->getFunctionDecl(fun);
        string tmpName = funDecl->getQualifiedNameAsString();
        if(funcName == "_Py_INCREF")
            cout << tmpName << endl;
        // cout << "1" << tmpName << endl;
        if(tmpName == funcName)
        {
            // cout << funcName << endl;
            return fun;
        }
    }
    cout << "Not found: " << funcName << endl;
    return nullptr;
}

bool MemoryLeakChecker::handle_malloc_size(Stmt* sizeStmt, int& size)
{
    /// only stand for: malloc(integer literal) or malloc(integer literal * sizeof(type)) or malloc(sizeof(type))
    if(sizeStmt->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
    {
        Stmt::child_iterator iter = sizeStmt->child_begin();
        if((*iter)->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass)
        {
            clang::IntegerLiteral* integer = static_cast<clang::IntegerLiteral* >(*iter);
            size = integer->getValue().getZExtValue();
            // cout << size << endl;
            return true;
        }
        else
            return false;
    }
    else if(sizeStmt->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass)
    {
        Stmt::child_iterator iter = sizeStmt->child_begin();
        if((*iter)->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
        {
            if((*(*iter)->child_begin())->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass)
            {
                clang::IntegerLiteral* integer = static_cast<clang::IntegerLiteral* >(*(*iter)->child_begin());
                size = integer->getValue().getZExtValue();
            }
            else
                return false;
            iter++;
            if((*iter)->getStmtClass() == clang::Stmt::StmtClass::UnaryExprOrTypeTraitExprClass)
            {
                clang::UnaryExprOrTypeTraitExpr* unaryExpr = static_cast<clang::UnaryExprOrTypeTraitExpr* >(*iter);
                size *= Pointers.get_unit_length(unaryExpr->getArgumentType().getAsString());
                return true;
            }
            else
                return false;
        }
        else if((*iter)->getStmtClass() == clang::Stmt::StmtClass::UnaryExprOrTypeTraitExprClass)
        {
            clang::UnaryExprOrTypeTraitExpr* unaryExpr = static_cast<clang::UnaryExprOrTypeTraitExpr* >(*iter);
            iter++;
            if((*iter)->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
            {
                if((*(*iter)->child_begin())->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass)
                {
                    clang::IntegerLiteral* integer = static_cast<clang::IntegerLiteral* >(*(*iter)->child_begin());
                    size = integer->getValue().getZExtValue();
                }
                else
                    return false;
                
                size *= Pointers.get_unit_length(unaryExpr->getArgumentType().getAsString());
                return true;
            }
            return false;
        }   
        else
            return false;
        
    }
    else if(sizeStmt->getStmtClass() == clang::Stmt::StmtClass::UnaryExprOrTypeTraitExprClass)
    {
        clang::UnaryExprOrTypeTraitExpr* unaryExpr = static_cast<clang::UnaryExprOrTypeTraitExpr* >(sizeStmt);
        size = Pointers.get_unit_length(unaryExpr->getArgumentType().getAsString());
        return true;
    }
    return false;
}

bool MemoryLeakChecker::handle_new_size(CXXNewExpr* newExpr, int& size)
{
    if(newExpr->isArray())
    {
        size = Pointers.get_unit_length(newExpr->getType().getAsString());
        Stmt::child_iterator iter = newExpr->child_begin();
        if((*iter)->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
        {
            if((*(*iter)->child_begin())->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass)
            {
                clang::IntegerLiteral* integer = static_cast<clang::IntegerLiteral* >(*(*iter)->child_begin());
                size *= integer->getValue().getZExtValue();
            }
            else
                return false;
        }
        else   
            return false;
        return true;
    }
    else
    {
        size = Pointers.get_unit_length(newExpr->getType().getAsString());
        return true;
    }
    return false;
}

void MemoryLeakChecker::handle_callee(CallExpr* callExprStmt, ASTFunction* fun, const FunctionDecl* funcDecl, TraceRecord traceRecord)
{
    // if(funcDecl->getNameAsString() == "PyMem_RawMalloc")
    //     cout << "yes\n";
    unsigned paramCount = funcDecl->getNumParams();
    unsigned argNum = callExprStmt->getNumArgs();

    assert(paramCount == argNum);
    traceRecord.calleeName.push(funcDecl->getNameAsString());
    traceRecord.calleeLocation.push(callExprStmt->getExprLoc());
    
    for(unsigned i = 0; i < paramCount; i++)
    {
        const clang::ParmVarDecl* param = funcDecl->getParamDecl(i);
        if(param->getType().getTypePtrOrNull()->isAnyPointerType())
        {
            Expr* arg = callExprStmt->getArg(i);
            if(arg->getStmtClass() == clang::Stmt::StmtClass::CStyleCastExprClass
                || arg->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
            {
                clang::Stmt::child_iterator iter = arg->child_begin();
                arg = static_cast<clang::Expr* >(*iter);
            }
            
            if(arg->getStmtClass() == clang::Stmt::StmtClass::CallExprClass)
            {
                clang::CallExpr* tmpCallExpr = static_cast<clang::CallExpr* >(arg);
                clang::Stmt::child_iterator iter = tmpCallExpr->child_begin();
                for(; iter != tmpCallExpr->child_end(); iter++)
                    if((*iter)->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
                        break;
                
                clang::DeclRefExpr* declRefExpr = static_cast<clang::DeclRefExpr*>(*(*iter)->child_begin());
                if(declRefExpr->getFoundDecl()->getNameAsString() == "malloc")
                {
                    bool computable = true;
                    int localSize = 0;
                    Stmt* sizeStmt = static_cast<clang::Stmt* >(*(++iter));
                    computable = handle_malloc_size(sizeStmt, localSize);
                    Pointers.new_pointer(param->getNameAsString(), param->getLocation(), traceRecord, 
                        param->getType().getAsString(), false, computable, localSize);
                }
                else
                {
                    for(auto fun : allFunctions)
                    {
                        FunctionDecl* calleeFunc = manager->getFunctionDecl(fun);
                        if(calleeFunc->getNameAsString() == declRefExpr->getFoundDecl()->getNameAsString())
                        {
                            handle_return_pointer(calleeFunc, param);
                            break;
                        }
                    }
                }
            }
            else if(arg->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
            {       
                clang::DeclRefExpr* declRefExpr = static_cast<clang::DeclRefExpr* >(arg);
                Pointers.modify_pointer(param->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(), param->getLocation(), declRefExpr->getFoundDecl()->getLocation(), param->getType().getAsString()); 
                Pointers.handle_callee_pointer(declRefExpr->getFoundDecl()->getNameAsString(), declRefExpr->getFoundDecl()->getLocation(), param->getLocation());
            }
        }   
    }
    // if(funcDecl->getNameAsString() == "unicode_decode_utf8")
    //     funcDecl->dump();
    // cout << funcDecl->getNameAsString() << endl;
    // Pointers.handle_non_callee_pointer();
    currentEntryFunc = fun;
    unique_ptr<CFG>& cfg = manager->getCFG(fun);
    SM = &manager->getFunctionDecl(fun)->getASTContext().getSourceManager();
    Pointers.SM = SM;
    handle_cfg(cfg, traceRecord, false);
    // Pointers.handle_after_callee();
}

void MemoryLeakChecker::handle_return_pointer(FunctionDecl* calleeFunc, const ParmVarDecl* param)
{
    assert(calleeFunc->getReturnType().getTypePtrOrNull()->isAnyPointerType());
    assert(calleeFunc->hasBody());

    Stmt* funcBody = calleeFunc->getBody();
    clang::Stmt::child_iterator iter = funcBody->child_begin();
    for(; iter != funcBody->child_end(); iter++)
        if((*iter)->getStmtClass() == clang::Stmt::StmtClass::ReturnStmtClass)
            break;
    clang::ReturnStmt* retStmt = static_cast<clang::ReturnStmt* >(*iter);
    clang::DeclRefExpr* declRefExpr = static_cast<clang::DeclRefExpr* >(*(*(*iter)->child_begin())->child_begin());

    Pointers.modify_pointer(param->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(), 
                            param->getLocation(), declRefExpr->getFoundDecl()->getLocation(), param->getType().getAsString());
}

void MemoryLeakChecker::handle_binaryOperator(Stmt* statement, TraceRecord traceRecord)
{
    clang::BinaryOperator* binaryOpStmt = static_cast<clang::BinaryOperator* >(statement);
    if(binaryOpStmt->isAssignmentOp())
    {
        Expr* leftExpr = binaryOpStmt->getLHS();
        Expr* rightExpr = binaryOpStmt->getRHS();
        if(leftExpr->getType().getTypePtrOrNull()->isAnyPointerType())
        {
            Stmt::child_iterator iter = rightExpr->child_begin();

            if(leftExpr->getStmtClass() != Stmt::StmtClass::DeclRefExprClass)
                return;
            clang::DeclRefExpr* leftDeclRef = static_cast<clang::DeclRefExpr*>(leftExpr);

            // if(rightExpr->getStmtClass() == Stmt::StmtClass::CallExprClass)
            // {
            //     clang::CallExpr* callExprStmt = static_cast<clang::CallExpr* >(rightExpr);
            //     clang::Expr* calleeExpr = callExprStmt->getCallee();
            //     clang::Stmt::child_iterator it = calleeExpr->child_begin();
            //     for( ; it != calleeExpr->child_end(); it++)
            //     {
            //         if((*it)->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
            //         {
            //             clang::DeclRefExpr* rightDeclRef = static_cast<clang::DeclRefExpr* >(*it);
            //             string funcName = rightDeclRef->getFoundDecl()->getNameAsString();
            //             for(auto fun : allFunctions)
            //             {
            //                 const FunctionDecl* funcDecl = manager->getFunctionDecl(fun);

            //                 if(funcDecl->getNameAsString() == funcName)
            //                 {
            //                     handle_callee(callExprStmt, fun, funcDecl, traceRecord);
            //                 }
            //             }
            //         }
            //     }
            // }
            if(rightExpr->getStmtClass() == Stmt::StmtClass::ImplicitCastExprClass)
            {
                if((*iter)->getStmtClass() == Stmt::StmtClass::DeclRefExprClass)
                {
                    clang::DeclRefExpr* rightDeclRef = static_cast<clang::DeclRefExpr*>(*iter);
                    Pointers.modify_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), rightDeclRef->getFoundDecl()->getNameAsString(), 
                        leftDeclRef->getFoundDecl()->getLocation(), rightDeclRef->getFoundDecl()->getLocation(), leftDeclRef->getType().getAsString());
                }
                else if((*iter)->getStmtClass() == Stmt::StmtClass::UnaryOperatorClass)
                {
                    clang::UnaryOperator* unaryOp = static_cast<clang::UnaryOperator* >(*iter);
                    handle_unaryOp_in_binaryStmt(leftDeclRef, unaryOp);
                }
            }
            else if(rightExpr->getStmtClass() == Stmt::StmtClass::CStyleCastExprClass)
            {
                assert(*(rightExpr->child_begin()) != nullptr);
                if((*(rightExpr->child_begin()))->getStmtClass() == Stmt::StmtClass::ImplicitCastExprClass)
                {
                    clang::ImplicitCastExpr* castExpr = static_cast<clang::ImplicitCastExpr* >(*(rightExpr->child_begin()));
                    assert(*(castExpr->child_begin()));
                    if((*(castExpr->child_begin()))->getStmtClass() == Stmt::StmtClass::DeclRefExprClass)
                    {
                        clang::DeclRefExpr* rightDeclRef = static_cast<clang::DeclRefExpr* >(*(castExpr->child_begin()));
                        Pointers.modify_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), rightDeclRef->getFoundDecl()->getNameAsString(), 
                            leftDeclRef->getFoundDecl()->getLocation(), rightDeclRef->getFoundDecl()->getLocation(), leftDeclRef->getType().getAsString());
                    }
                    else if((*(castExpr->child_begin()))->getStmtClass() == Stmt::StmtClass::UnaryOperatorClass)
                    {
                        clang::UnaryOperator* unaryOp = static_cast<clang::UnaryOperator* >(*(castExpr->child_begin()));
                        handle_unaryOp_in_binaryStmt(leftDeclRef, unaryOp);
                    }
                }
                else if((*(rightExpr->child_begin()))->getStmtClass() == Stmt::StmtClass::ParenExprClass)
                {
                    clang::ParenExpr* parenExpr = static_cast<clang::ParenExpr* >(*(rightExpr->child_begin()));
                    if((*parenExpr->child_begin())->getStmtClass() == Stmt::StmtClass::BinaryOperatorClass)
                    {
                        clang::BinaryOperator* binaryOp = static_cast<clang::BinaryOperator* >((*parenExpr->child_begin()));
                        Expr* leftExpr = binaryOp->getLHS();
                        Expr* rightExpr = binaryOp->getRHS();
                        if(leftExpr->getStmtClass() == Stmt::StmtClass::ImplicitCastExprClass)
                        {
                            clang::ImplicitCastExpr* castExpr = static_cast<clang::ImplicitCastExpr* >(leftExpr);
                            Expr* tmp = castExpr->getSubExpr();
                            if(tmp->getStmtClass() == Stmt::StmtClass::DeclRefExprClass)
                            {
                                clang::DeclRefExpr* rightDeclRef = static_cast<clang::DeclRefExpr* >(tmp);
                                int base = 0;
                                bool op = false;
                                if(string(binaryOp->getOpcodeStr()) == "+")
                                    op = true;
                                else if(string(binaryOp->getOpcodeStr()) == "-")
                                    op = false;
                                else {
                                    cout << string(binaryOp->getOpcodeStr()) << "should not appear as pointer operator.\n";
                                    return;
                                }

                                if(rightExpr->getStmtClass() == Stmt::StmtClass::IntegerLiteralClass) {
                                    clang::IntegerLiteral* integer = static_cast<clang::IntegerLiteral* >(rightExpr);
                                    base = integer->getValue().getZExtValue();
                                }
                                else if(rightExpr->getStmtClass() == Stmt::StmtClass::UnaryExprOrTypeTraitExprClass) {
                                    clang::UnaryExprOrTypeTraitExpr* unaryOp = static_cast<clang::UnaryExprOrTypeTraitExpr* >(rightExpr);
                                    base = Pointers.get_unit_length(unaryOp->getArgumentType().getAsString());
                                    if(base == 0){
                                        Pointers.handle_computable(leftDeclRef->getFoundDecl()->getNameAsString(), leftDeclRef->getFoundDecl()->getLocation());
                                        return;
                                    }
                                }
                                else {
                                    Pointers.handle_computable(leftDeclRef->getFoundDecl()->getNameAsString(), leftDeclRef->getFoundDecl()->getLocation());
                                    return;
                                }
                                Pointers.modify_pointer_after_change(leftDeclRef->getFoundDecl()->getNameAsString(), rightDeclRef->getFoundDecl()->getNameAsString(), 
                                    leftDeclRef->getFoundDecl()->getLocation(), rightDeclRef->getFoundDecl()->getLocation(), leftDeclRef->getType().getAsString(), base, op);
                            }
                        }
                    }
                }
                else if((*(rightExpr->child_begin()))->getStmtClass() == Stmt::StmtClass::UnaryOperatorClass)
                {
                    clang::UnaryOperator* unaryOp = static_cast<clang::UnaryOperator* >(*(rightExpr->child_begin()));
                    handle_unaryOp_in_binaryStmt(leftDeclRef, unaryOp);
                }
            }
            else if(rightExpr->getStmtClass() == Stmt::StmtClass::BinaryOperatorClass)
            {
                clang::BinaryOperator* subBinaryOp = static_cast<clang::BinaryOperator* >(rightExpr);
                handle_binary_pointer_in_binaryOp(leftDeclRef, subBinaryOp);
            }
            else if(rightExpr->getStmtClass() == Stmt::StmtClass::UnaryOperatorClass)
            {
                clang::UnaryOperator* unaryOp = static_cast<clang::UnaryOperator* >(rightExpr);
                handle_unaryOp_in_binaryStmt(leftDeclRef, unaryOp);
            }
        }
    }
}

void MemoryLeakChecker::handle_declStmt(Stmt* statement)
{
    clang::DeclStmt* declStmt = static_cast<clang::DeclStmt* >(statement);
    clang::DeclGroupRef groupDecl = declStmt->getDeclGroup();
    clang::DeclGroupRef::iterator DGIter = groupDecl.begin();
    for (; DGIter != groupDecl.end(); DGIter++)
    {
        if((*DGIter)->getKind() == clang::Decl::Kind::Var)
        {
            clang::VarDecl *varDecl = static_cast<clang::VarDecl *>((*DGIter));
            if(varDecl->getType().getTypePtrOrNull()->isAnyPointerType())
            {
                // cout << varDecl->getNameAsString() << endl;
                Stmt::child_iterator iter = declStmt->child_begin();
                for(; iter != declStmt->child_end(); iter++)
                {
                    if((*iter)->getStmtClass() == Stmt::StmtClass::CStyleCastExprClass)
                    {
                        if((*(*iter)->child_begin())->getStmtClass() == Stmt::StmtClass::ImplicitCastExprClass)
                        {
                            clang::ImplicitCastExpr* impExpr = static_cast<clang::ImplicitCastExpr*>(*(*iter)->child_begin());
                            Stmt::child_iterator secIter = impExpr->child_begin();
                            for(; secIter != impExpr->child_end(); secIter++)
                            {
                                if((*secIter)->getStmtClass() == Stmt::StmtClass::DeclRefExprClass)
                                {
                                    clang::DeclRefExpr* declRefExpr = static_cast<clang::DeclRefExpr*>(*secIter);
                                    Pointers.modify_pointer(varDecl->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(), 
                                    varDecl->getLocation(), declRefExpr->getFoundDecl()->getLocation(), varDecl->getType().getAsString());
                                }
                                else if((*secIter)->getStmtClass() == Stmt::StmtClass::UnaryOperatorClass)
                                {
                                    clang::UnaryOperator* unaryOp = static_cast<clang::UnaryOperator* >(*secIter);
                                    handle_unaryOp_in_declStmt(varDecl, unaryOp);
                                }
                            }
                        }
                        else if((*(*iter)->child_begin())->getStmtClass() == Stmt::StmtClass::ParenExprClass)
                        {
                            clang::ParenExpr* parenExpr = static_cast<clang::ParenExpr* >(*(*iter)->child_begin());
                            if((*parenExpr->child_begin())->getStmtClass() == Stmt::StmtClass::BinaryOperatorClass)
                            {
                                clang::BinaryOperator* binaryOp = static_cast<clang::BinaryOperator* >((*parenExpr->child_begin()));
                                Expr* leftExpr = binaryOp->getLHS();
                                Expr* rightExpr = binaryOp->getRHS();
                                if(leftExpr->getStmtClass() == Stmt::StmtClass::ImplicitCastExprClass)
                                {
                                    clang::ImplicitCastExpr* castExpr = static_cast<clang::ImplicitCastExpr* >(leftExpr);
                                    Expr* tmp = castExpr->getSubExpr();
                                    if(tmp->getStmtClass() == Stmt::StmtClass::DeclRefExprClass)
                                    {
                                        clang::DeclRefExpr* rightDeclRef = static_cast<clang::DeclRefExpr* >(tmp);
                                        int base = 0;
                                        bool op = false;
                                        if(string(binaryOp->getOpcodeStr()) == "+")
                                            op = true;
                                        else if(string(binaryOp->getOpcodeStr()) == "-")
                                            op = false;
                                        else {
                                            cout << string(binaryOp->getOpcodeStr()) << "should not appear as pointer operator.\n";
                                            return;
                                        }

                                        if(rightExpr->getStmtClass() == Stmt::StmtClass::IntegerLiteralClass) {
                                            clang::IntegerLiteral* integer = static_cast<clang::IntegerLiteral* >(rightExpr);
                                            base = integer->getValue().getZExtValue();
                                        }
                                        else if(rightExpr->getStmtClass() == Stmt::StmtClass::UnaryExprOrTypeTraitExprClass) {
                                            clang::UnaryExprOrTypeTraitExpr* unaryOp = static_cast<clang::UnaryExprOrTypeTraitExpr* >(rightExpr);
                                            base = Pointers.get_unit_length(unaryOp->getArgumentType().getAsString());
                                            if(base == 0){
                                                Pointers.handle_computable(varDecl->getNameAsString(), varDecl->getLocation());
                                                return;
                                            }
                                        }
                                        else {
                                            Pointers.handle_computable(varDecl->getNameAsString(), varDecl->getLocation());
                                            return;
                                        }
                                        Pointers.modify_pointer_after_change(varDecl->getNameAsString(), rightDeclRef->getFoundDecl()->getNameAsString(), varDecl->getLocation(),
                                            rightDeclRef->getFoundDecl()->getLocation(), varDecl->getType().getAsString(), base, op);
                                    }
                                }
                            }
                        }
                        else if((*(*iter)->child_begin())->getStmtClass() == Stmt::StmtClass::UnaryOperatorClass)
                        {
                            clang::UnaryOperator* unaryOp = static_cast<clang::UnaryOperator* >(*(*iter)->child_begin());
                            handle_unaryOp_in_declStmt(varDecl, unaryOp);
                        }
                    }   
                    else if((*iter)->getStmtClass() == Stmt::StmtClass::ImplicitCastExprClass)
                    {
                        clang::ImplicitCastExpr* impExpr = static_cast<clang::ImplicitCastExpr*>(*iter);
                        Stmt::child_iterator secIter = impExpr->child_begin();
                        for(; secIter != impExpr->child_end(); secIter++)
                        {
                            if((*secIter)->getStmtClass() == Stmt::StmtClass::DeclRefExprClass)
                            {
                                clang::DeclRefExpr* declRefExpr = static_cast<clang::DeclRefExpr*>(*secIter);
                                Pointers.modify_pointer(varDecl->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(), 
                                varDecl->getLocation(), declRefExpr->getFoundDecl()->getLocation(), varDecl->getType().getAsString());
                            }
                            else if((*secIter)->getStmtClass() == Stmt::StmtClass::UnaryOperatorClass)
                            {
                                clang::UnaryOperator* unaryOp = static_cast<clang::UnaryOperator* >(*secIter);
                                handle_unaryOp_in_declStmt(varDecl, unaryOp);
                            } 
                        }
                    }
                    else if((*iter)->getStmtClass() == Stmt::StmtClass::BinaryOperatorClass)
                    {
                        clang::BinaryOperator* binaryOp = static_cast<clang::BinaryOperator* >(*iter);
                        handle_binary_pointer_in_declStmt(varDecl, binaryOp);
                    }
                    else if((*iter)->getStmtClass() == Stmt::StmtClass::UnaryOperatorClass)
                    {
                        clang::UnaryOperator* unaryOp = static_cast<clang::UnaryOperator* >(*iter);
                            handle_unaryOp_in_declStmt(varDecl, unaryOp);
                    }
                }
            }
        }
    }
}

void MemoryLeakChecker::handle_newExpr(Stmt* statement, BumpVector<CFGElement>::reverse_iterator elemIter, TraceRecord traceRecord)
{
    CFGElement nextElement = *(++elemIter);
    llvm::Optional<CFGStmt> nextStmt = nextElement.getAs<CFGStmt>();
    
    clang::CXXNewExpr* newExpr = static_cast<clang::CXXNewExpr* >(statement);
    assert(nextStmt.hasValue());
    Stmt *nextStatement = const_cast<Stmt *>(nextStmt.getValue().getStmt());

    if(nextStatement->getStmtClass() == clang::Stmt::StmtClass::DeclStmtClass)
    {
        clang::DeclStmt* declStmt = static_cast<clang::DeclStmt* >(nextStatement);
        clang::DeclGroupRef groupDecl = declStmt->getDeclGroup();
        clang::DeclGroupRef::iterator DGIter = groupDecl.begin();
        for( ; DGIter != groupDecl.end(); DGIter++)
        {
            if((*DGIter)->getKind() == clang::Decl::Kind::Var)
            {
                clang::VarDecl* varDecl = static_cast<clang::VarDecl *>(*DGIter);
                bool computable = true;
                int localSize = 0;
                computable = handle_new_size(newExpr, localSize);
                Pointers.new_pointer(varDecl->getNameAsString(), varDecl->getLocation(), traceRecord, 
                    varDecl->getType().getAsString(), newExpr->isArray(), computable, localSize);
            }
        }
    } 
    else if(nextStatement->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass)
    {
        clang::BinaryOperator* binaryOpStmt = static_cast<clang::BinaryOperator* >(nextStatement);
        if(binaryOpStmt->isAssignmentOp())
        {
            assert(binaryOpStmt->getLHS()->getType().getTypePtrOrNull()->isAnyPointerType());
            assert(binaryOpStmt->getLHS()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass);
            
            clang::DeclRefExpr* leftDeclRef = static_cast<clang::DeclRefExpr* >(binaryOpStmt->getLHS());

            bool computable = true;
            int localSize = 0;
            computable = handle_new_size(newExpr, localSize);
            Pointers.special_new_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), leftDeclRef->getFoundDecl()->getLocation(), newExpr->getExprLoc(),
                traceRecord, newExpr->isArray(), leftDeclRef->getType().getAsString(), computable, localSize);
        }
    }
}

void MemoryLeakChecker::handle_deleteExpr(Stmt* statement)
{
    clang::CXXDeleteExpr* deleteExpr = static_cast<clang::CXXDeleteExpr* >(statement);

    for(clang::Stmt::child_iterator iter = deleteExpr->child_begin(); iter != deleteExpr->child_end(); iter++)
    {
        if((*iter)->getStmtClass() == Stmt::StmtClass::ImplicitCastExprClass)
        {
            assert((*iter)->child_begin()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass);
            clang::DeclRefExpr* declRefExpr = static_cast<clang::DeclRefExpr* >(*((*iter)->child_begin()));

            Pointers.delete_pointer(declRefExpr->getFoundDecl()->getNameAsString(), declRefExpr->getFoundDecl()->getLocation(), 
                deleteExpr);
        }
    }
}

void MemoryLeakChecker::handle_compoundOp(Stmt* statement)
{
    /// only stand for +=/-= integer
    CompoundAssignOperator* comOp = static_cast<CompoundAssignOperator* >(statement);

    Expr* leftExpr = comOp->getLHS();
    if(leftExpr->getType().getTypePtrOrNull()->isAnyPointerType())
    {
        if(leftExpr->getStmtClass() == Stmt::StmtClass::DeclRefExprClass)
        {
            DeclRefExpr* leftDeclRef = static_cast<DeclRefExpr* >(leftExpr);
            Expr* rightExpr = comOp->getRHS();
            if(rightExpr->getStmtClass() != Stmt::StmtClass::IntegerLiteralClass &&
                rightExpr->getStmtClass() != Stmt::StmtClass::UnaryExprOrTypeTraitExprClass)
            {
                /// all same memory pointer not computable
                Pointers.handle_computable(leftDeclRef->getFoundDecl()->getNameAsString(), 
                    leftDeclRef->getFoundDecl()->getLocation());
                return;
            }
            int base = 0;
            if(rightExpr->getStmtClass() == Stmt::StmtClass::IntegerLiteralClass)
            {
                clang::IntegerLiteral* rightInt = static_cast<clang::IntegerLiteral* >(rightExpr);
                base = rightInt->getValue().getZExtValue();
            }
            else
            {
                clang::UnaryExprOrTypeTraitExpr* unaryOp = static_cast<clang::UnaryExprOrTypeTraitExpr* >(rightExpr);
                base = Pointers.get_unit_length(unaryOp->getArgumentType().getAsString());

                if(base == 0){
                    Pointers.handle_computable(leftDeclRef->getFoundDecl()->getNameAsString(), 
                        leftDeclRef->getFoundDecl()->getLocation());
                return;
                }
            }

            bool op = false;
            if(string(comOp->getOpcodeStr()) == "+=")
                op = true;
            else if(string(comOp->getOpcodeStr()) == "-=")
                op = false;
            else{
                cout << string(comOp->getOpcodeStr()) << "should not appear as pointer operator.\n";
                return;
            }
            Pointers.handle_one_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), leftDeclRef->getFoundDecl()->getLocation(), 
                base, op);
            /// same pointer + or -
        }
    }
}

void MemoryLeakChecker::handle_unaryOp(Stmt* statement)
{
    UnaryOperator* unaryOp = static_cast<UnaryOperator* >(statement);

    Expr* leftExpr = unaryOp->getSubExpr();
    if(leftExpr->getType().getTypePtrOrNull()->isAnyPointerType())
    {
        if(leftExpr->getStmtClass() == Stmt::StmtClass::DeclRefExprClass)
        {
            clang::DeclRefExpr* leftDeclRef = static_cast<clang::DeclRefExpr* >(leftExpr);

            bool op = false;
            if(unaryOp->isIncrementOp(unaryOp->getOpcode()))
                op = true;
            else if(unaryOp->isDecrementOp(unaryOp->getOpcode()))
                op = false;
            else{
                cout << "Should not appear as pointer operator at line: " << unaryOp->getExprLoc().printToString(*SM).substr(64, 2) << ".\n";
                return;
            }
            Pointers.handle_one_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), leftDeclRef->getFoundDecl()->getLocation(),
                1, op);
        }
    }
}

void MemoryLeakChecker::handle_binary_pointer_in_binaryOp(DeclRefExpr* leftDeclRef, BinaryOperator* binaryOp)
{
    Expr* subLeft = binaryOp->getLHS();
    Expr* subRight = binaryOp->getRHS();
    if(subLeft->getStmtClass() == Stmt::StmtClass::ImplicitCastExprClass ||
        subLeft->getStmtClass() == Stmt::StmtClass::CStyleCastExprClass)
    {
        clang::ImplicitCastExpr* subCastExpr;
        if(subLeft->getStmtClass() == Stmt::StmtClass::ImplicitCastExprClass)
            subCastExpr = static_cast<clang::ImplicitCastExpr* >(subLeft);
        else
            subCastExpr = static_cast<clang::ImplicitCastExpr* >(*(subLeft->child_begin()));
        
        Expr* tmp = subCastExpr->getSubExpr();
        if(tmp->getStmtClass() == Stmt::StmtClass::DeclRefExprClass)
        {
            clang::DeclRefExpr* rightDeclRef = static_cast<clang::DeclRefExpr* >(tmp);
            if(leftDeclRef->getFoundDecl()->getNameAsString() != rightDeclRef->getFoundDecl()->getNameAsString() ||
                leftDeclRef->getFoundDecl()->getLocation() != rightDeclRef->getFoundDecl()->getLocation())
            {
                Pointers.modify_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), rightDeclRef->getFoundDecl()->getNameAsString(), 
                    leftDeclRef->getFoundDecl()->getLocation(), rightDeclRef->getFoundDecl()->getLocation(), leftDeclRef->getType().getAsString());
                
            }
            int base = 0;
            bool op = false;
            if(string(binaryOp->getOpcodeStr()) == "+")
                op = true;
            else if(string(binaryOp->getOpcodeStr()) == "-")
                op = false;
            else{
                cout << string(binaryOp->getOpcodeStr()) << "should not appear as pointer operator.\n";
                return;
            }

            if(subRight->getStmtClass() == Stmt::StmtClass::IntegerLiteralClass)
            {
                clang::IntegerLiteral* integer = static_cast<clang::IntegerLiteral* >(subRight);
                base = integer->getValue().getZExtValue();
                Pointers.handle_one_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), leftDeclRef->getFoundDecl()->getLocation(), 
                    base, op);
            }
            else if(subRight->getStmtClass() == Stmt::StmtClass::UnaryExprOrTypeTraitExprClass)
            {
                clang::UnaryExprOrTypeTraitExpr* unaryOp = static_cast<clang::UnaryExprOrTypeTraitExpr* >(subRight);
                base = Pointers.get_unit_length(unaryOp->getArgumentType().getAsString());

                if(base == 0){
                    Pointers.handle_computable(leftDeclRef->getFoundDecl()->getNameAsString(), 
                        leftDeclRef->getFoundDecl()->getLocation());
                return;
                }
                Pointers.handle_one_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), leftDeclRef->getFoundDecl()->getLocation(), 
                    base, op);
            }
            else
            {
                Pointers.handle_computable(leftDeclRef->getFoundDecl()->getNameAsString(), 
                                                leftDeclRef->getFoundDecl()->getLocation());
                return;
            }
        }
    }
    else if(subRight->getStmtClass() == Stmt::StmtClass::ImplicitCastExprClass ||
        subRight->getStmtClass() == Stmt::StmtClass::CStyleCastExprClass)
    {
        clang::ImplicitCastExpr* subCastExpr;
        if(subRight->getStmtClass() == Stmt::StmtClass::ImplicitCastExprClass)
            subCastExpr = static_cast<clang::ImplicitCastExpr* >(subRight);
        else
            subCastExpr = static_cast<clang::ImplicitCastExpr* >(*(subRight->child_begin()));
        
        Expr* tmp = subCastExpr->getSubExpr();
        if(tmp->getStmtClass() == Stmt::StmtClass::DeclRefExprClass)
        {
            clang::DeclRefExpr* rightDeclRef = static_cast<clang::DeclRefExpr* >(tmp);
            if(leftDeclRef->getFoundDecl()->getNameAsString() != rightDeclRef->getFoundDecl()->getNameAsString() ||
                leftDeclRef->getFoundDecl()->getLocation() != rightDeclRef->getFoundDecl()->getLocation())
            {
                Pointers.modify_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), rightDeclRef->getFoundDecl()->getNameAsString(), 
                    leftDeclRef->getFoundDecl()->getLocation(), rightDeclRef->getFoundDecl()->getLocation(), leftDeclRef->getType().getAsString());
                
            }
            int base = 0;
            bool op = false;
            if(string(binaryOp->getOpcodeStr()) == "+")
                op = true;
            else if(string(binaryOp->getOpcodeStr()) == "-")
                op = false;
            else{
                cout << string(binaryOp->getOpcodeStr()) << "should not appear as pointer operator.\n";
                return;
            }

            if(subLeft->getStmtClass() == Stmt::StmtClass::IntegerLiteralClass)
            {
                clang::IntegerLiteral* integer = static_cast<clang::IntegerLiteral* >(subLeft);
                base = integer->getValue().getZExtValue();
                Pointers.handle_one_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), leftDeclRef->getFoundDecl()->getLocation(), 
                    base, op);
            }
            else if(subLeft->getStmtClass() == Stmt::StmtClass::UnaryExprOrTypeTraitExprClass)
            {
                clang::UnaryExprOrTypeTraitExpr* unaryOp = static_cast<clang::UnaryExprOrTypeTraitExpr* >(subLeft);
                base = Pointers.get_unit_length(unaryOp->getArgumentType().getAsString());

                if(base == 0){
                    Pointers.handle_computable(leftDeclRef->getFoundDecl()->getNameAsString(), 
                        leftDeclRef->getFoundDecl()->getLocation());
                return;
                }
                Pointers.handle_one_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), leftDeclRef->getFoundDecl()->getLocation(), 
                    base, op);
            }
            else
            {
                Pointers.handle_computable(leftDeclRef->getFoundDecl()->getNameAsString(), 
                                                leftDeclRef->getFoundDecl()->getLocation());
                return;
            }
        }
    }
}

void MemoryLeakChecker::handle_binary_pointer_in_declStmt(VarDecl* varDecl, BinaryOperator* binaryOp)
{
    Expr* leftExpr = binaryOp->getLHS();
    Expr* rightExpr = binaryOp->getRHS();
    if(leftExpr->getStmtClass() == Stmt::StmtClass::ImplicitCastExprClass ||
        leftExpr->getStmtClass() == Stmt::StmtClass::CStyleCastExprClass)
    {
        clang::ImplicitCastExpr* castExpr;
        if(leftExpr->getStmtClass() == Stmt::StmtClass::ImplicitCastExprClass)
            castExpr = static_cast<clang::ImplicitCastExpr* >(leftExpr);
        else
            castExpr = static_cast<clang::ImplicitCastExpr* >(*(leftExpr->child_begin()));
        
        Expr* tmp = castExpr->getSubExpr();
        if(tmp->getStmtClass() == Stmt::StmtClass::DeclRefExprClass)
        {
            clang::DeclRefExpr* rightDeclRef = static_cast<clang::DeclRefExpr* >(tmp);
            Pointers.modify_pointer(varDecl->getNameAsString(), rightDeclRef->getFoundDecl()->getNameAsString(), 
                varDecl->getLocation(), rightDeclRef->getFoundDecl()->getLocation(), varDecl->getType().getAsString());
                
            int base = 0;
            bool op = false;
            if(string(binaryOp->getOpcodeStr()) == "+")
                op = true;
            else if(string(binaryOp->getOpcodeStr()) == "-")
                op = false;
            else
            {
                cout << string(binaryOp->getOpcodeStr()) << "should not appear as pointer operator.\n";
                return;
            }

            if(rightExpr->getStmtClass() == Stmt::StmtClass::IntegerLiteralClass)
            {
                clang::IntegerLiteral* integer = static_cast<clang::IntegerLiteral* >(rightExpr);
                base = integer->getValue().getZExtValue();
                Pointers.handle_one_pointer(varDecl->getNameAsString(), varDecl->getLocation(), base, op);
            }
            else if(rightExpr->getStmtClass() == Stmt::StmtClass::UnaryExprOrTypeTraitExprClass)
            {
                clang::UnaryExprOrTypeTraitExpr* unaryOp = static_cast<clang::UnaryExprOrTypeTraitExpr* >(rightExpr);
                base = Pointers.get_unit_length(unaryOp->getArgumentType().getAsString());

                if(base == 0){
                    Pointers.handle_computable(varDecl->getNameAsString(), varDecl->getLocation());
                    return;
                }
                Pointers.handle_one_pointer(varDecl->getNameAsString(), varDecl->getLocation(), base, op);
            }
            else
            {
                Pointers.handle_computable(varDecl->getNameAsString(), varDecl->getLocation());
                return;
            }
        }
    }
    else if(rightExpr->getStmtClass() == Stmt::StmtClass::ImplicitCastExprClass ||
        rightExpr->getStmtClass() == Stmt::StmtClass::CStyleCastExprClass)
    {
        clang::ImplicitCastExpr* castExpr;
        if(rightExpr->getStmtClass() == Stmt::StmtClass::ImplicitCastExprClass)
            castExpr = static_cast<clang::ImplicitCastExpr* >(rightExpr);
        else
            castExpr = static_cast<clang::ImplicitCastExpr* >(*(rightExpr->child_begin()));
        
        Expr* tmp = castExpr->getSubExpr();
        if(tmp->getStmtClass() == Stmt::StmtClass::DeclRefExprClass)
        {
            clang::DeclRefExpr* rightDeclRef = static_cast<clang::DeclRefExpr* >(tmp);
            Pointers.modify_pointer(varDecl->getNameAsString(), rightDeclRef->getFoundDecl()->getNameAsString(), 
                varDecl->getLocation(), rightDeclRef->getFoundDecl()->getLocation(), varDecl->getType().getAsString());
                
            int base = 0;
            bool op = false;
            if(string(binaryOp->getOpcodeStr()) == "+")
                op = true;
            else if(string(binaryOp->getOpcodeStr()) == "-")
                op = false;
            else
            {
                cout << string(binaryOp->getOpcodeStr()) << "should not appear as pointer operator.\n";
                return;
            }

            if(leftExpr->getStmtClass() == Stmt::StmtClass::IntegerLiteralClass)
            {
                clang::IntegerLiteral* integer = static_cast<clang::IntegerLiteral* >(leftExpr);
                base = integer->getValue().getZExtValue();
                Pointers.handle_one_pointer(varDecl->getNameAsString(), varDecl->getLocation(), base, op);
            }
            else if(leftExpr->getStmtClass() == Stmt::StmtClass::UnaryExprOrTypeTraitExprClass)
            {
                clang::UnaryExprOrTypeTraitExpr* unaryOp = static_cast<clang::UnaryExprOrTypeTraitExpr* >(leftExpr);
                base = Pointers.get_unit_length(unaryOp->getArgumentType().getAsString());

                if(base == 0){
                    Pointers.handle_computable(varDecl->getNameAsString(), varDecl->getLocation());
                    return;
                }
                Pointers.handle_one_pointer(varDecl->getNameAsString(), varDecl->getLocation(), base, op);
            }
            else
            {
                Pointers.handle_computable(varDecl->getNameAsString(), varDecl->getLocation());
                return;
            }
        }
    }
}

void MemoryLeakChecker::handle_binary_callee_in_binaryOp(DeclRefExpr* leftDeclRef, DeclRefExpr* declRefExpr, BinaryOperator* subBinary)
{
    Expr* subLeft = subBinary->getLHS();
    Expr* subRight = subBinary->getRHS();

    if(subLeft->getStmtClass() == clang::Stmt::StmtClass::CallExprClass)
    {
        int base = 0;
        if(subRight->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass)
        {
            clang::IntegerLiteral* integer = static_cast<clang::IntegerLiteral* >(subRight);
            base = integer->getValue().getZExtValue();
        }
        else if(subRight->getStmtClass() == clang::Stmt::StmtClass::UnaryExprOrTypeTraitExprClass)
        {
            clang::UnaryExprOrTypeTraitExpr* unaryOp = static_cast<clang::UnaryExprOrTypeTraitExpr* >(subRight);
            base = Pointers.get_unit_length(unaryOp->getArgumentType().getAsString());
            if(base == 0){
                Pointers.modify_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(),
                    leftDeclRef->getFoundDecl()->getLocation(), declRefExpr->getFoundDecl()->getLocation(), leftDeclRef->getType().getAsString());
                Pointers.handle_computable(leftDeclRef->getFoundDecl()->getNameAsString(), leftDeclRef->getFoundDecl()->getLocation());
                return;
            }
        }
        else
        {
            Pointers.modify_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(),
                leftDeclRef->getFoundDecl()->getLocation(), declRefExpr->getFoundDecl()->getLocation(), leftDeclRef->getType().getAsString());
            Pointers.handle_computable(leftDeclRef->getFoundDecl()->getNameAsString(), leftDeclRef->getFoundDecl()->getLocation());
            return;
        }   
        bool op = false;
        if(string(subBinary->getOpcodeStr()) == "+")
            op = true;
        else if(string(subBinary->getOpcodeStr()) == "-")
            op = false;
        else {
            cout << string(subBinary->getOpcodeStr()) << "should not appear as pointer operator.\n";
            return;
        }
        Pointers.modify_pointer_after_change(leftDeclRef->getFoundDecl()->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(), 
            leftDeclRef->getFoundDecl()->getLocation(), declRefExpr->getFoundDecl()->getLocation(), leftDeclRef->getType().getAsString(), base, op);
    }
    else if(subRight->getStmtClass() == clang::Stmt::StmtClass::CallExprClass)
    {
        int base = 0;
        if(subLeft->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass)
        {
            clang::IntegerLiteral* integer = static_cast<clang::IntegerLiteral* >(subLeft);
            base = integer->getValue().getZExtValue();
        }
        else if(subLeft->getStmtClass() == clang::Stmt::StmtClass::UnaryExprOrTypeTraitExprClass)
        {
            clang::UnaryExprOrTypeTraitExpr* unaryOp = static_cast<clang::UnaryExprOrTypeTraitExpr* >(subLeft);
            base = Pointers.get_unit_length(unaryOp->getArgumentType().getAsString());
            if(base == 0){
                Pointers.handle_computable(leftDeclRef->getFoundDecl()->getNameAsString(), leftDeclRef->getFoundDecl()->getLocation());
                return;
            }
        }
        else
        {
            Pointers.modify_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(),
                leftDeclRef->getFoundDecl()->getLocation(), declRefExpr->getFoundDecl()->getLocation(), leftDeclRef->getType().getAsString());
            Pointers.handle_computable(leftDeclRef->getFoundDecl()->getNameAsString(), leftDeclRef->getFoundDecl()->getLocation());
            return;
        }   
        bool op = false;
        if(string(subBinary->getOpcodeStr()) == "+")
            op = true;
        else if(string(subBinary->getOpcodeStr()) == "-")
            op = false;
        else {
            cout << string(subBinary->getOpcodeStr()) << "should not appear as pointer operator.\n";
            return;
        }
        Pointers.modify_pointer_after_change(leftDeclRef->getFoundDecl()->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(), 
            leftDeclRef->getFoundDecl()->getLocation(), declRefExpr->getFoundDecl()->getLocation(), leftDeclRef->getType().getAsString(), base, op);
    }
}

void MemoryLeakChecker::handle_binary_callee_in_declStmt(VarDecl* varDecl, DeclRefExpr* declRefExpr, BinaryOperator* binaryOp)
{
    Expr* subLeft = binaryOp->getLHS();
    Expr* subRight = binaryOp->getRHS();

    if(subLeft->getStmtClass() == clang::Stmt::StmtClass::CallExprClass)
    {
        int base = 0;
        if(subRight->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass)
        {
            clang::IntegerLiteral* integer = static_cast<clang::IntegerLiteral* >(subRight);
            base = integer->getValue().getZExtValue();
        }
        else if(subRight->getStmtClass() == clang::Stmt::StmtClass::UnaryExprOrTypeTraitExprClass)
        {
            clang::UnaryExprOrTypeTraitExpr* unaryOp = static_cast<clang::UnaryExprOrTypeTraitExpr* >(subRight);
            base = Pointers.get_unit_length(unaryOp->getArgumentType().getAsString());
            if(base == 0){
                Pointers.modify_pointer(varDecl->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(),
                    varDecl->getLocation(), declRefExpr->getFoundDecl()->getLocation(), varDecl->getType().getAsString());
                Pointers.handle_computable(varDecl->getNameAsString(), varDecl->getLocation());
                return;
            }
        }
        else
        {
            Pointers.modify_pointer(varDecl->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(),
                varDecl->getLocation(), declRefExpr->getFoundDecl()->getLocation(), varDecl->getType().getAsString());
            Pointers.handle_computable(varDecl->getNameAsString(), varDecl->getLocation());
            return;
        }   
        bool op = false;
        if(string(binaryOp->getOpcodeStr()) == "+")
            op = true;
        else if(string(binaryOp->getOpcodeStr()) == "-")
            op = false;
        else {
            cout << string(binaryOp->getOpcodeStr()) << "should not appear as pointer operator.\n";
            return;
        }
        Pointers.modify_pointer_after_change(varDecl->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(), 
            varDecl->getLocation(), declRefExpr->getFoundDecl()->getLocation(), varDecl->getType().getAsString(), base, op);
    }
    else if(subRight->getStmtClass() == clang::Stmt::StmtClass::CallExprClass)
    {
        int base = 0;
        if(subLeft->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass)
        {
            clang::IntegerLiteral* integer = static_cast<clang::IntegerLiteral* >(subLeft);
            base = integer->getValue().getZExtValue();
        }
        else if(subLeft->getStmtClass() == clang::Stmt::StmtClass::UnaryExprOrTypeTraitExprClass)
        {
            clang::UnaryExprOrTypeTraitExpr* unaryOp = static_cast<clang::UnaryExprOrTypeTraitExpr* >(subLeft);
            base = Pointers.get_unit_length(unaryOp->getArgumentType().getAsString());
            if(base == 0){
                Pointers.modify_pointer(varDecl->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(),
                    varDecl->getLocation(), declRefExpr->getFoundDecl()->getLocation(), varDecl->getType().getAsString());
                Pointers.handle_computable(varDecl->getNameAsString(), varDecl->getLocation());
                return;
            }
        }
        else
        {
            Pointers.modify_pointer(varDecl->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(),
                varDecl->getLocation(), declRefExpr->getFoundDecl()->getLocation(), varDecl->getType().getAsString());
            Pointers.handle_computable(varDecl->getNameAsString(), varDecl->getLocation());
            return;
        }   
        bool op = false;
        if(string(binaryOp->getOpcodeStr()) == "+")
            op = true;
        else if(string(binaryOp->getOpcodeStr()) == "-")
            op = false;
        else {
            cout << string(binaryOp->getOpcodeStr()) << "should not appear as pointer operator.\n";
            return;
        }
        Pointers.modify_pointer_after_change(varDecl->getNameAsString(), declRefExpr->getFoundDecl()->getNameAsString(), 
            varDecl->getLocation(), declRefExpr->getFoundDecl()->getLocation(), varDecl->getType().getAsString(), base, op);
    }
}

void MemoryLeakChecker::handle_unaryOp_in_declStmt(VarDecl* varDecl, UnaryOperator* unaryOp)
{
    Expr* rightExpr = unaryOp->getSubExpr();
    if(rightExpr->getType().getTypePtrOrNull()->isAnyPointerType())
    {
        if(rightExpr->getStmtClass() == Stmt::StmtClass::DeclRefExprClass)
        {
            clang::DeclRefExpr* rightDeclRef = static_cast<clang::DeclRefExpr* >(rightExpr);

            bool op = false;
            if(unaryOp->isIncrementOp(unaryOp->getOpcode()))
                op = true;
            else if(unaryOp->isDecrementOp(unaryOp->getOpcode()))
                op = false;
            else{
                cout << "Should not appear as pointer operator at line: " << unaryOp->getExprLoc().printToString(*SM).substr(64, 2) << ".\n";
                return;
            }
            if(unaryOp->isPostfix())
            {
                Pointers.modify_pointer(varDecl->getNameAsString(), rightDeclRef->getFoundDecl()->getNameAsString(),
                    varDecl->getLocation(), rightDeclRef->getFoundDecl()->getLocation(), varDecl->getType().getAsString());
                Pointers.handle_one_pointer(rightDeclRef->getFoundDecl()->getNameAsString(), rightDeclRef->getFoundDecl()->getLocation(), 
                    1, op);
            }
            else if(unaryOp->isPrefix())
            {
                Pointers.handle_one_pointer(rightDeclRef->getFoundDecl()->getNameAsString(), rightDeclRef->getFoundDecl()->getLocation(),
                    1, op);
                Pointers.modify_pointer(varDecl->getNameAsString(), rightDeclRef->getFoundDecl()->getNameAsString(),
                    varDecl->getLocation(), rightDeclRef->getFoundDecl()->getLocation(), varDecl->getType().getAsString());
            }
        }
    }
}

void MemoryLeakChecker::handle_unaryOp_in_binaryStmt(DeclRefExpr* leftDeclRef, UnaryOperator* unaryOp)
{
    Expr* rightExpr = unaryOp->getSubExpr();
    if(rightExpr->getType().getTypePtrOrNull()->isAnyPointerType())
    {
        if(rightExpr->getStmtClass() == Stmt::StmtClass::DeclRefExprClass)
        {
            clang::DeclRefExpr* rightDeclRef = static_cast<clang::DeclRefExpr* >(rightExpr);

            bool op = false;
            if(unaryOp->isIncrementOp(unaryOp->getOpcode()))
                op = true;
            else if(unaryOp->isDecrementOp(unaryOp->getOpcode()))
                op = false;
            else{
                cout << "Should not appear as pointer operator at line: " << unaryOp->getExprLoc().printToString(*SM).substr(64, 2) << ".\n";
                return;
            }
            if(unaryOp->isPostfix())
            {
                Pointers.modify_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), rightDeclRef->getFoundDecl()->getNameAsString(),
                    leftDeclRef->getFoundDecl()->getLocation(), rightDeclRef->getFoundDecl()->getLocation(), leftDeclRef->getType().getAsString());
                Pointers.handle_one_pointer(rightDeclRef->getFoundDecl()->getNameAsString(), rightDeclRef->getFoundDecl()->getLocation(), 
                    1, op);
            }
            else if(unaryOp->isPrefix())
            {
                Pointers.handle_one_pointer(rightDeclRef->getFoundDecl()->getNameAsString(), rightDeclRef->getFoundDecl()->getLocation(),
                    1, op);
                Pointers.modify_pointer(leftDeclRef->getFoundDecl()->getNameAsString(), rightDeclRef->getFoundDecl()->getNameAsString(),
                    leftDeclRef->getFoundDecl()->getLocation(), rightDeclRef->getFoundDecl()->getLocation(), leftDeclRef->getType().getAsString());
            }
        }
    }
}

int PointerSet::get_memoryID(string name, SourceLocation location, int& pos, int mode)
{
    int pointerID = 0;
    for(unsigned i = 0; i < pointerVec.size(); i++) {
        if(pointerVec[i].location.top() == location) {
            if(pointerVec[i].pointerID > 0) {
                pointerID = pointerVec[i].memoryID;
                pos = i;
            }
            else
                pointerID = pointerVec[i].pointerID;
        }
    }
    if(pointerID < 0){
        for(unsigned i = 0; i < pointerVec.size(); i++) {
            if(pointerVec[i].pointerID == -pointerID) {
                pointerID = pointerVec[i].memoryID;
                pos = i;
                break;
            }
        }
    }
    // cout << name << endl;
    // if(pointerID <= 0)
    // {
    //     if(mode == 1)
    //     {
    //         cout << "Pointer " << name << " doesn't exit or the memory it directs has been freed or it doesn't direct a memory at line ";
    //         cout << location.printToString(*SM).substr(64, 2) << endl;
    //     }
    //     else
    //         cout << "Free the same memory twice.\n";
    // }
    return pointerID;
}

int PointerSet::get_unit_length(string type)
{
    if(type == "char *" || type == "char")
        return 1;
    else if(type == "unsigned char *" || type == "unsigned char")
        return 1;
    else if(type == "int *" || type == "int")
        return 4;
    else if(type == "unsigned int *" || type == "unsigned int")
        return 4;
    else if(type == "float *" || type == "float")
        return 4;
    else if(type == "double *" || type == "double")
        return 8;
    return 0;
}

void PointerSet::handle_callee_pointer(string name, SourceLocation location, SourceLocation newLocation)
{
    for(unsigned i = 0; i < pointerVec.size(); i++)
        if(pointerVec[i].pointerName == name && pointerVec[i].location.top() == location)
            pointerVec[i].location.push(newLocation);
}

void PointerSet::handle_non_callee_pointer()
{
    int minDepth = 0xff;
    for(unsigned i = 0; i < pointerVec.size(); i++)
        minDepth = pointerVec[i].location.size() < minDepth ? pointerVec[i].location.size() : minDepth;
    
    for(unsigned i = 0; i < pointerVec.size(); i++)
        if(pointerVec[i].location.size() == minDepth)
            pointerVec[i].location.push(pointerVec[i].location.top());
}

void PointerSet::handle_after_callee()
{
    for(unsigned i = 0; i < pointerVec.size(); i++)
        if(pointerVec[i].location.size() != 1)
            pointerVec[i].location.pop();
}

void PointerSet::new_pointer(string name, SourceLocation _location, TraceRecord traceRecord, string type, bool isArray, bool computable, int localSize)
{
    Pointer newPointer;
    newPointer.pointerName = name;
    newPointer.initialName = name;
    newPointer.memoryID = get_new_memory_count();
    newPointer.location.push(_location);
    newPointer.pointerID = get_new_point_count();
    newPointer.isArray = isArray;
    newPointer.record = traceRecord;

    newPointer.isComputable = computable;
    newPointer.compute.unitLength = get_unit_length(type);
    newPointer.compute.local.begin = 0;
    newPointer.compute.local.end = localSize == 0 ? 100 : localSize;
    newPointer.compute.syn.begin = 0;
    newPointer.compute.syn.end = localSize;

    pointerVec.push_back(newPointer);
}

void PointerSet::delete_pointer(string name, SourceLocation location, clang::CXXDeleteExpr* deleteExpr)
{
    int pos = -1;
    int deleteMemoryID = get_memoryID(name, location, pos, 1);
    // cout << location.printToString(*SM) << endl;
    bool isArray = deleteExpr->isArrayForm();
    for(auto iter = pointerVec.begin(); iter != pointerVec.end(); iter++)
    {
        if((*iter).memoryID == deleteMemoryID)
        {
            if((*iter).location.top() == location)
            {
                if((*iter).isArray == true)
                {
                    if(isArray)
                    {
                        free_pointer(name, location, deleteExpr->getExprLoc());
                    }
                    else{
                        string warning = "WARNING: ";
                        warning += "At line ";
                        string line, col;
                        bool isLoc = true;
                        string tmp = deleteExpr->getExprLoc().printToString(*SM);
                        for(unsigned i = tmp.size() - 1; i >= 0; i--)
                        {
                            if(tmp[i] == ':'){
                                isLoc = false;
                                continue;
                            }
                            if(!(tmp[i] >= '0' && tmp[i] <= '9'))
                                break;
                            if(isLoc)
                                col += tmp[i];
                            else
                                line += tmp[i];
                        }
                        reverse(line.begin(), line.end());
                        warning += line;
                        warning += " 'delete' applied to a pointer that was allocated with 'new[]'; did you mean 'delete[]'?";
                        DeleteVec.push_back(warning);
                    }
                }
                else
                {
                    if(isArray){
                        string warning = "WARNING: ";
                        warning += "At line ";
                        string line, col;
                        bool isLoc = true;
                        string tmp = deleteExpr->getExprLoc().printToString(*SM);
                        for(unsigned i = tmp.size() - 1; i >= 0; i--)
                        {
                            if(tmp[i] == ':'){
                                isLoc = false;
                                continue;
                            }
                            if(!(tmp[i] >= '0' && tmp[i] <= '9'))
                                break;
                            if(isLoc)
                                col += tmp[i];
                            else
                                line += tmp[i];
                        }
                        reverse(line.begin(), line.end());
                        warning += line;
                        warning += " 'delete[]' applied to a pointer that was allocated with 'new'; did you mean 'delete'";
                        DeleteVec.push_back(warning);
                    }
                    else
                    {
                        free_pointer(name, location, deleteExpr->getExprLoc());
                    }
                }
            }
            else
                (*iter).pointerName = "nullptr";
        }
    }
}

void PointerSet::free_pointer(string name, SourceLocation location, SourceLocation exprLocation)
{
    int pos = -1;
    int deleteMemoryID = get_memoryID(name, location, pos, 1);
    // cout << location.printToString(*SM) << endl;
    if(pos != -1)
    {
        if(pointerVec[pos].isComputable)
        {
            if(pointerVec[pos].compute.local.begin != 0)
            {
                SafetyVec.push_back(pointerVec[pos]);
            }
            else
            {
                if(pointerVec[pos].compute.local.end == 0)
                {
                    DFreePointer tmp;
                    tmp.exprLocation = exprLocation;
                    tmp.location = pointerVec[pos].location.top();
                    tmp.memoryID = pointerVec[pos].memoryID;
                    tmp.pointerName = pointerVec[pos].initialName;
                    tmp.record = pointerVec[pos].record;
                    DFreeVec.push_back(tmp);
                }
                else
                {
                    pointerVec[pos].compute.local.end = 0;
                    for(auto iter = pointerVec.begin(); iter != pointerVec.end(); iter++)
                    {
                        if((*iter).memoryID == deleteMemoryID)
                        {
                            (*iter).pointerName = "nullptr";
                            (*iter).compute.local.end = 0;
                        }
                    }
                }
            }
        }
        else 
        {
            for(auto iter = pointerVec.begin(); iter != pointerVec.end(); iter++)
            {
                if((*iter).memoryID == deleteMemoryID)
                    (*iter).pointerName = "nullptr";
            }
        }
    }
}

void PointerSet::modify_pointer(string leftName, string rightName, SourceLocation leftLocation, SourceLocation rightLocation, string type)
{
    int pos = -1;
    int memoryID = get_memoryID(rightName, rightLocation, pos, 1);
    
    bool isExistPointer = false;
    int pointerID = 0;
    for(unsigned i = 0; i < pointerVec.size(); i++) 
    {
        if(pointerVec[i].location.top() == leftLocation)
        {
            if(pointerVec[i].pointerID > 0) 
            {
                if(pointerVec[i].pointerName != "nullptr") 
                { 
                    if(pointerVec[i].pointerName == leftName)
                        isExistPointer = true;
                    pointerVec[i].pointerName = "null_" + pointerVec[i].pointerName;
                }
                pointerVec[i].pointerID = -pointerVec[i].pointerID;
                pointerID = pointerVec[i].pointerID;
            }
            else
            {
                pointerID = pointerVec[i].pointerID;
            }
        }
    }
    if(pointerID < 0)
    {
        for(unsigned i = 0; i < pointerVec.size(); i++) 
        {
            if(pointerVec[i].pointerID == -pointerID) 
            {
                if(pointerVec[i].pointerName != "nullptr") 
                {
                    if(pointerVec[i].pointerName == leftName)
                        isExistPointer = true;
                    pointerVec[i].pointerName = "null_" + pointerVec[i].pointerName;
                    pointerVec[i].pointerID = -pointerVec[i].pointerID;
                }
            }
        }
    }
    if(pos != -1)
    {
        Pointer newPointer;
        newPointer.pointerName = leftName;
        newPointer.initialName = leftName;
        newPointer.memoryID = memoryID;
        newPointer.pointerID = isExistPointer == true ? (-pointerID) : get_new_point_count();
        newPointer.location.push(leftLocation);
        newPointer.record = pointerVec[pos].record;
        newPointer.isArray = pointerVec[pos].isArray;
        newPointer.isComputable = pointerVec[pos].isComputable;
        newPointer.compute.unitLength = get_unit_length(type);
        newPointer.compute.local = pointerVec[pos].compute.local;
        newPointer.compute.syn = pointerVec[pos].compute.syn;
        pointerVec.push_back(newPointer);
    }
}

void PointerSet::modify_pointer_after_change(string leftName, string rightName, SourceLocation leftLocation, SourceLocation rightLocation, string type, int base, bool op)
{
    int pos = -1;
    int memoryID = get_memoryID(rightName, rightLocation, pos, 1);
    
    bool isExistPointer = false;
    int pointerID = 0;
    TraceRecord record;
    for(unsigned i = 0; i < pointerVec.size(); i++) 
    {
        if(pointerVec[i].location.top() == leftLocation)
        {
            if(pointerVec[i].pointerID > 0) 
            {
                if(pointerVec[i].pointerName != "nullptr") 
                { 
                    if(pointerVec[i].pointerName == leftName)
                        isExistPointer = true;
                    pointerVec[i].pointerName = "null_" + pointerVec[i].pointerName;
                }
                pointerVec[i].pointerID = -pointerVec[i].pointerID;
                pointerID = pointerVec[i].pointerID;
                record = pointerVec[i].record;
            }
            else
            {
                pointerID = pointerVec[i].pointerID;
                record = pointerVec[i].record;
            }
        }
    }
    if(pointerID < 0)
    {
        for(unsigned i = 0; i < pointerVec.size(); i++) 
        {
            if(pointerVec[i].pointerID == -pointerID) 
            {
                if(pointerVec[i].pointerName != "nullptr") 
                {
                    if(pointerVec[i].pointerName == leftName)
                        isExistPointer = true;
                    pointerVec[i].pointerName = "null_" + pointerVec[i].pointerName;
                    pointerVec[i].pointerID = -pointerVec[i].pointerID;
                }
            }
        }
    }
    if(pos != -1)
    {
        Pointer newPointer;
        newPointer.pointerName = leftName;
        newPointer.initialName = leftName;
        newPointer.memoryID = memoryID;
        newPointer.pointerID = isExistPointer == true ? (-pointerID) : get_new_point_count();
        newPointer.location.push(leftLocation);
        newPointer.record = record;
        newPointer.isArray = pointerVec[pos].isArray;
        newPointer.isComputable = pointerVec[pos].isComputable;
        newPointer.compute.unitLength = get_unit_length(type);
        newPointer.compute.syn = pointerVec[pos].compute.syn;
        if(op)
            newPointer.compute.local.begin = pointerVec[pos].compute.local.begin + base * pointerVec[pos].compute.unitLength;
        else
            newPointer.compute.local.begin = pointerVec[pos].compute.local.begin - base * pointerVec[pos].compute.unitLength;
        newPointer.compute.local.end = pointerVec[pos].compute.local.end;
        pointerVec.push_back(newPointer);
    }
}

void PointerSet::special_new_pointer(string leftName, SourceLocation leftLocation, SourceLocation rightLocation, TraceRecord traceRecord, bool isArray, string type, bool computable, int localSize)
{
    bool isPrimary = false;
    int pointerID = 0;
    TraceRecord record;
    for(unsigned i = 0; i < pointerVec.size(); i++)
    {
        if(pointerVec[i].location.top() == leftLocation)
        {
            if(pointerVec[i].pointerID > 0)
            {
                if(pointerVec[i].pointerName != "nullptr")
                    pointerVec[i].pointerName = "null_" + pointerVec[i].pointerName;
                pointerVec[i].pointerID = -pointerVec[i].pointerID;
                isPrimary = true;
            }
            pointerID = -pointerVec[i].pointerID;
            record = pointerVec[i].record;
        }
    }
    /// case: just declare, not been initialed
    if(pointerID == 0)
    {
        new_pointer(leftName, leftLocation, traceRecord, type, isArray, computable, localSize);
        return;
    }
    if(!isPrimary)
    {
        for(unsigned i = 0; i < pointerVec.size(); i++)
        {
            if(pointerVec[i].pointerID == pointerID)
            {
                if(pointerVec[i].pointerName != "nullptr")
                    pointerVec[i].pointerName = "null_" + pointerVec[i].pointerName;
                pointerVec[i].pointerID = -pointerVec[i].pointerID;
            }
        }
    }
    Pointer newPointer;
    newPointer.pointerName = leftName;
    newPointer.initialName = leftName;
    newPointer.memoryID = get_new_memory_count();
    newPointer.pointerID = pointerID;
    newPointer.location.push(rightLocation);
    newPointer.record = record;
    newPointer.isArray = isArray;

    newPointer.isComputable = computable;
    newPointer.compute.local.begin = 0;
    newPointer.compute.local.end = localSize == 0 ? 100 : localSize;
    newPointer.compute.syn.begin = 0;
    newPointer.compute.syn.end = localSize;
    newPointer.compute.unitLength = get_unit_length(type);

    pointerVec.push_back(newPointer);
}

void PointerSet::handle_computable(string name, SourceLocation location)
{
    int pos = -1;
    int memoryID = get_memoryID(name, location, pos, 1);

    for(unsigned i = 0; i < pointerVec.size(); i++) {
        if(pointerVec[i].memoryID == memoryID) {
            pointerVec[i].isComputable = false;
        }
    }
}

void PointerSet::handle_one_pointer(string name, SourceLocation location, int base, bool op)
{
    int pos = -1;
    int memoryID = get_memoryID(name, location, pos, 1);

    if(pos != -1)
    {
        if(!pointerVec[pos].isComputable)
            return;
        if(op)
            pointerVec[pos].compute.local.begin += pointerVec[pos].compute.unitLength * base;
        else
            pointerVec[pos].compute.local.begin -= pointerVec[pos].compute.unitLength * base;
    }
}

void PointerSet::print_set()
{
    cout << "Pointer set: \n";
    for(unsigned i = 0; i < pointerVec.size(); i++)
    {
        cout << pointerVec[i].pointerName << "\t" << " - memoryID:" << pointerVec[i].memoryID << " - locationSize:" 
            << pointerVec[i].location.size() << " - pointerID:" << pointerVec[i].pointerID 
            << " - location: " << pointerVec[i].location.top().printToString(*SM).substr(64, 5) << " - recordSize:" << pointerVec[i].record.calleeLocation.size()
            << " - unitSize: " << pointerVec[i].compute.unitLength;
        if(pointerVec[i].isComputable)
            cout << " - local: [" << pointerVec[i].compute.local.begin << ", " << pointerVec[i].compute.local.end << "]" 
            << " - syn: [" << pointerVec[i].compute.syn.begin << ", " << pointerVec[i].compute.syn.end << "]" << endl;
        else
            cout << " - not computable." << endl;
    }
 
}

void MemoryLeakChecker::handle_location_string(string& line, string& loc, string& file, SourceLocation location)
{
    string tmp = location.printToString(*SM);
    bool isLoc = true;
    for(unsigned i = tmp.size() - 1; i >= 0; i--)
    {
        if(tmp[i] == ':'){
            isLoc = false;
            continue;
        }
        if(!(tmp[i] >= '0' && tmp[i] <= '9'))
            break;
        if(isLoc)
            loc += tmp[i];
        else
            line += tmp[i];
    }
    for(unsigned i = 0; i < tmp.size(); i++)
    {
        file += tmp[i];
        if(tmp[i] == ':')
            break;
    }
    reverse(line.begin(), line.end());
    reverse(loc.begin(), loc.end());
}

void MemoryLeakChecker::report_memory_leak()
{
    check_leak_same_pointer();

    bool isLeaked = !leakResult.empty();

    // if(!isLeaked)
    //     cout << "\033[32mNo memory leak detected.\n" << "\033[0m";
    if(isLeaked)
    {
        string file;
        for(unsigned i = 0; i < leakResult[0].location.printToString(*SM).size(); i++)
        {
            file += leakResult[0].location.printToString(*SM)[i];
            if(leakResult[0].location.printToString(*SM)[i] == ':')
                break;
        }
        std::cout << file << std::endl;
        for(unsigned i = 0; i < leakResult.size(); i++)
        {
            string tmpFile, line, col;
            handle_location_string(line, col, tmpFile, leakResult[i].location);
            if(tmpFile != file)
            {
                file = tmpFile;
                std::cout << file << endl;
            }
            std::cout << "WARNING: MEMORY LEAK POINTER ";
            std::cout << leakResult[i].pointerName << ": Line: " << line << " Column: " << col << endl;
            // cout << "The source of pointer: ";
            // while(!leakResult[i].record.calleeLocation.empty())
            // {
            //     assert(leakResult[i].record.calleeLocation.size() == leakResult[i].record.calleeName.size());
            //     line = "";
            //     loc = "";
            //     handle_location_string(line, loc, leakResult[i].record.calleeLocation.front());
            //     cout << "line: " << line << " & loc: " << loc << ", function name: " 
            //         << leakResult[i].record.calleeName.front();
            //     if(leakResult[i].record.calleeLocation.size() != 1) 
            //         cout << " -> ";
            //     leakResult[i].record.calleeLocation.pop();
            //     leakResult[i].record.calleeName.pop();
            // }
            // cout << endl;
        }
    }
}

void MemoryLeakChecker::report_double_free()
{
    check_Dfree_same_pointer();

    bool isDFreed = !DFreeResult.empty();
    // if(!isDFreed)
    //     cout << "\033[32mNo double free detected.\n" << "\033[0m";
    if(isDFreed)
    {
        string file;
        for(unsigned i = 0; i < DFreeResult[0].location.printToString(*SM).size(); i++)
        {
            file += DFreeResult[0].location.printToString(*SM)[i];
            if(DFreeResult[0].location.printToString(*SM)[i] == ':')
                break;
        }
        std::cout << file << endl;

        for(unsigned i = 0; i < DFreeResult.size(); i++)
        {
            string tmpFile, line, col;
            handle_location_string(line, col, tmpFile, DFreeResult[i].location);
            if(tmpFile != file)
            {
                file = tmpFile;
                std::cout << file << endl;
            }
            std::cout << "WARNING: DOUBLE FREE POINTER ";
            std::cout << DFreeResult[i].pointerName << ": Line: " << line << " Column: " << col << endl;
            // cout << "The source of pointer: ";
            // while(!DFreeResult[i].record.calleeLocation.empty())
            // {
            //     assert(DFreeResult[i].record.calleeLocation.size() == DFreeResult[i].record.calleeName.size());
            //     line = "";
            //     loc = "";
            //     handle_location_string(line, loc, DFreeResult[i].record.calleeLocation.front());
            //     cout << "line: " << line << " & loc: " << loc << ", function name: " 
            //         << DFreeResult[i].record.calleeName.front();
            //     if(DFreeResult[i].record.calleeLocation.size() != 1) 
            //         cout << " -> ";
            //     DFreeResult[i].record.calleeLocation.pop();
            //     DFreeResult[i].record.calleeName.pop();
            // }
            // cout << endl;
        }
    }
}

void MemoryLeakChecker::report_memory_safety()
{
    for(unsigned i = 0; i < DeleteError.size(); i++)
        cout << DeleteError[i] << endl;
    if(SafetyResult.size() == 0)
        return;
    string file;
    for(unsigned i = 0; i < SafetyResult[0].location.top().printToString(*SM).size(); i++)
    {
        file += SafetyResult[0].location.top().printToString(*SM)[i];
        if(SafetyResult[0].location.top().printToString(*SM)[i] == ':')
            break;
    }
    for(unsigned pos = 0; pos < SafetyResult.size(); pos++)
    {
        string line, col;
        string tmp = SafetyResult[pos].location.top().printToString(*SM);
        bool isLoc = true;
        for(unsigned i = tmp.size() - 1; i >= 0; i--)
        {
            if(tmp[i] == ':'){
                isLoc = false;
                continue;
            }
            if(!(tmp[i] >= '0' && tmp[i] <= '9'))
                break;
            if(isLoc)
                col += tmp[i];
            else
                line += tmp[i];
        }
        reverse(line.begin(), line.end());
        reverse(col.begin(), col.end());

        std::cout << file << std::endl;
        std::cout << "WARNING: Pointer " << SafetyResult[pos].initialName << " can't be freed or deleted."  << " Line: " << line << " Column: " << col << endl;
    }  
}

void MemoryLeakChecker::prepare_next_path(unsigned count)
{
    // Pointers.print_set();
    vector<ReportPointer> tmpVec;
    allLeakPointers.push_back(tmpVec);
    for(unsigned i = 0; i < Pointers.pointerVec.size(); i++)
    {
        if(Pointers.pointerVec[i].pointerName != "nullptr")
        {
            ReportPointer tmp;
            tmp.location = Pointers.pointerVec[i].location.top();
            tmp.memoryID = Pointers.pointerVec[i].memoryID;
            tmp.pointerName = Pointers.pointerVec[i].initialName;
            tmp.record = Pointers.pointerVec[i].record;

            allLeakPointers[count].push_back(tmp);
        }
    }
    allDFreePointers.push_back(Pointers.DFreeVec);
    for(unsigned i = 0; i < Pointers.SafetyVec.size(); i++)
    {
        bool isSame = false;
        for(unsigned j = 0; j < SafetyResult.size(); j++)
        {
            if(Pointers.SafetyVec[i].pointerID == SafetyResult[j].pointerID && 
                Pointers.SafetyVec[i].location.top() == SafetyResult[j].location.top())
                isSame = true;
        }
        if(!isSame)
            SafetyResult.push_back(Pointers.SafetyVec[i]);
    }
    for(unsigned i = 0; i < Pointers.DeleteVec.size(); i++)
    {
        bool isSame = false;
        for(unsigned j = 0; j < DeleteError.size(); j++)
        {
            if(DeleteError[j] == Pointers.DeleteVec[i])
                isSame = true;
        }
        if(!isSame)
            DeleteError.push_back(Pointers.DeleteVec[i]);
    }
    Pointers.pointCount = 1;
    Pointers.memoryCount = 1;
    vector<Pointer>().swap(Pointers.pointerVec);
    vector<Pointer>().swap(Pointers.SafetyVec);
    vector<string>().swap(Pointers.DeleteVec);
    vector<DFreePointer>().swap(Pointers.DFreeVec);
}

void MemoryLeakChecker::check_leak_same_pointer()
{
    /// same main paths & same pointers
    for(unsigned i = 0; i < allLeakPointers.size(); i++)
    {
        vector<ReportPointer> tmpVec = allLeakPointers[i];
        int maxMemoryCount = 0;
        for(unsigned j = 0; j < tmpVec.size(); j++)
            maxMemoryCount = maxMemoryCount > tmpVec[j].memoryID ? maxMemoryCount : tmpVec[j].memoryID;
        
        bool* isVisited = new bool[maxMemoryCount];
        for(int j = 0; j < maxMemoryCount; j++)
            isVisited[j] = false;
        
        for(vector<ReportPointer>::iterator iter = tmpVec.begin(); iter != tmpVec.end(); )
        {
            if(isVisited[iter->memoryID - 1] == false)
            {
                isVisited[iter->memoryID - 1] = true;
                iter++;
            }
            else
                tmpVec.erase(iter);
        }

        vector<ReportPointer> finalVec;
        for(unsigned j = 0; j < tmpVec.size(); j++)
        {
            bool isSamePointer = false;
            for(unsigned k = 0; k < finalVec.size(); k++)
            {
                if(is_same_pointer(tmpVec[j], finalVec[k]))
                {
                    isSamePointer = true;
                    break;
                }
            }
            if(!isSamePointer)
                finalVec.push_back(tmpVec[j]);
        }

        allLeakPointers[i] = finalVec;
    }
    /// different main paths & same pointers
    for(unsigned i = 0; i < allLeakPointers.size(); i++)
    {
        for(unsigned j = 0; j < allLeakPointers[i].size(); j++)
        {
            bool isSamePointer = false;
            for(unsigned k = 0; k < leakResult.size(); k++)
            {
                if(is_same_pointer(allLeakPointers[i][j], leakResult[k]))
                {
                    isSamePointer = true;
                    break;
                }
            }
            if(!isSamePointer)
                leakResult.push_back(allLeakPointers[i][j]);
        }
    }
}

void MemoryLeakChecker::check_Dfree_same_pointer()
{
    for(unsigned i = 0; i < allDFreePointers.size() ;i++)
    {
        vector<DFreePointer> tmpVec = allDFreePointers[i];
        int maxMemoryCount = 0;
        for(unsigned j = 0; j < tmpVec.size(); j++)
            maxMemoryCount = maxMemoryCount > tmpVec[j].memoryID ? maxMemoryCount : tmpVec[j].memoryID;
        
        bool* isVisited = new bool[maxMemoryCount];
        for(int j = 0; j < maxMemoryCount; j++)
            isVisited[j] = false;
        
        for(vector<DFreePointer>::iterator iter = tmpVec.begin(); iter != tmpVec.end(); )
        {
            if(isVisited[iter->memoryID - 1] == false)
            {
                isVisited[iter->memoryID - 1] = true;
                iter++;
            }
            else
                tmpVec.erase(iter);
        }

        vector<DFreePointer> finalVec;
        for(unsigned j = 0; j < tmpVec.size(); j++)
        {
            bool isSamePointer = false;
            for(unsigned k = 0; k < finalVec.size(); k++)
            {
                if(is_same_pointer(convert_pointer(tmpVec[j]), convert_pointer(finalVec[k])))
                {
                    isSamePointer = true;
                    break;
                }
            }
            if(!isSamePointer)
                finalVec.push_back(tmpVec[j]);
        }

        allDFreePointers[i] = finalVec;
    }
    for(unsigned i = 0; i < allDFreePointers.size(); i++)
    {
        for(unsigned j = 0; j < allDFreePointers[i].size(); j++)
        {
            bool isSamePointer = false;
            for(unsigned k = 0; k < DFreeResult.size(); k++)
            {
                if(is_same_pointer(convert_pointer(allDFreePointers[i][j]), convert_pointer(DFreeResult[k])))
                {
                    isSamePointer = true;
                    break;
                }
            }
            if(!isSamePointer)
                DFreeResult.push_back(allDFreePointers[i][j]);
        }
    }
}

ReportPointer MemoryLeakChecker::convert_pointer(DFreePointer tmp)
{
    ReportPointer pointer;
    pointer.location = tmp.location;
    pointer.memoryID = tmp.memoryID;
    pointer.pointerName = tmp.pointerName;
    pointer.record = tmp.record;
    return pointer;
}

bool MemoryLeakChecker::is_same_pointer(ReportPointer left, ReportPointer right)
{
    if(left.location != right.location)
        return false;
    
    queue<string> leftNameQueue, rightNameQueue; 
    leftNameQueue = left.record.calleeName;
    rightNameQueue = right.record.calleeName;
    while(!leftNameQueue.empty() || !rightNameQueue.empty())
    {
        string leftName = leftNameQueue.front();
        string rightName = rightNameQueue.front();

        if(leftName != rightName)
            return false;
        leftNameQueue.pop();
        rightNameQueue.pop();
    }
    if(leftNameQueue.empty() && !rightNameQueue.empty())
        return false;
    else if(!leftNameQueue.empty() && rightNameQueue.empty())
        return false;

    queue<SourceLocation> leftLocQueue, rightLocQueue;
    leftLocQueue = left.record.calleeLocation;
    rightLocQueue = right.record.calleeLocation;
    while(!leftLocQueue.empty() || !rightLocQueue.empty())
    {
        SourceLocation leftLoc = leftLocQueue.front();
        SourceLocation rightLoc = rightLocQueue.front();

        if(leftLoc != rightLoc)
            return false;
        leftLocQueue.pop();
        rightLocQueue.pop();
    }
    if(leftLocQueue.empty() && !rightLocQueue.empty())
        return false;
    else if(!leftLocQueue.empty() && rightLocQueue.empty())
        return false;
    
    return true;
}
