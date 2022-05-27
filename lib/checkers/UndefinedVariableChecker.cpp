#include "checkers/UndefinedVariableChecker.h"

/*
TODO: lvalue can be struct or array, they need to be considered, do it later
*/

void UndefinedVariableChecker::print_stmt_kind(Stmt* statement, int spaceCount)
{
    if(statement == nullptr)
    {
        for(int i = 0; i < spaceCount; i++)
            std::cout << "  ";
        std::cout << "<<<NULL>>>" << std::endl;
        return;
    }
    for(int i = 0; i < spaceCount; i++)
        std::cout << "  ";
    std::cout << statement->getStmtClassName() << std::endl;
    //clang::Stmt::StmtClass::
    //clang::TranslationUnitDecl::getTranslationUnitDecl();
    
    if(statement != nullptr && statement->getStmtClass() == clang::Stmt::StmtClass::DeclStmtClass)
        {
            clang::DeclStmt* declstmt = static_cast<clang::DeclStmt*>(statement);
            clang::DeclStmt::decl_iterator it1 = declstmt->decl_begin();
            if(declstmt->isSingleDecl()){
                clang::Decl* oneDecl = declstmt->getSingleDecl();
                for(int i = 0; i < spaceCount + 1; i++)
                    std::cout << "  ";
                std::cout << oneDecl->getDeclKindName() << std::endl;
                if(oneDecl->getKind() == clang::Decl::Kind::Var){
                    clang::VarDecl* vardecl = static_cast<clang::VarDecl*>(oneDecl);
                    std::cout << vardecl->getNameAsString() << std::endl;
                    //vardecl->dumpColor();
                    if(vardecl->getInit()!= nullptr){
                        clang::Expr* initexpr = vardecl->getInit();
                        if(initexpr->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass){
                            clang::IntegerLiteral* integerit = static_cast<clang::IntegerLiteral*>(initexpr);
                            std::cout << integerit->getValue().getZExtValue() << std::endl;
                        }
                    }
                }   
            }
            else{
                clang::DeclGroupRef multDecl = declstmt->getDeclGroup();
                clang::DeclGroupRef::iterator itgr = multDecl.begin();
                for(; itgr != multDecl.end(); itgr++)
                {
                    for(int i = 0; i < spaceCount + 1; i++)
                        std::cout << "  ";
                    std::cout << (*itgr)->getDeclKindName() << std::endl;
                    if((*itgr)->getKind() == clang::Decl::Kind::Var){
                        clang::VarDecl* vardecl = static_cast<clang::VarDecl*>((*itgr));
                        std::cout << vardecl->getNameAsString() << std::endl;
                        if(vardecl->getInit()!= nullptr){
                            clang::Expr* initexpr = vardecl->getInit();
                            if(initexpr->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass){
                                clang::IntegerLiteral* integerit = static_cast<clang::IntegerLiteral*>(initexpr);
                                std::cout << integerit->getValue().getZExtValue() << std::endl;
                            }
                        }
                    }  
                }
            }
        }
    if(statement != nullptr && statement->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass){
        clang::DeclRefExpr* declrefexp = static_cast<clang::DeclRefExpr*>(statement);
        //declrefexp->getNameInfo().getName().dump();
        //static_cast<clang::VarDecl*>(declrefexp->getDecl())->getInit()->dumpColor();
        //if(static_cast<clang::VarDecl*>(declrefexp->getDecl())->getEvaluatedValue() != nullptr)
        //static_cast<clang::VarDecl*>(declrefexp->getDecl())->getDeclContext()->dumpDeclContext();
            //static_cast<clang::VarDecl*>(declrefexp->getDecl()).imp
        //if(declrefexp->getQualifier())
        //  declrefexp->getQualifier()->dump();
        
    }
    clang::Stmt::child_iterator iter = statement->child_begin();
    for(; iter != statement->child_end(); iter++)
    {
        print_stmt_kind(*iter, spaceCount + 1);
    }
}

void UndefinedVariableChecker::get_cfg_stmt(unique_ptr<CFG>& cfg)
{
    int i = 0;
    clang::CFG::iterator blockIter;
    for(blockIter = cfg->begin(); blockIter != cfg->end(); blockIter++){
        std::cout<< "\033[31m" << "BLOCK" << i <<  "\033[0m" << std::endl;
        CFGBlock* block = *blockIter;
        BumpVector<CFGElement>::reverse_iterator elementIter;
        for(elementIter = block->begin(); elementIter != block->end(); elementIter++){
            CFGElement element = *elementIter;
            
            if(element.getKind() == clang::CFGElement::Kind::Statement){
                llvm::Optional<CFGStmt> stmt = element.getAs<CFGStmt>();
                if(stmt.hasValue() == true){
                    Stmt* statement = const_cast<Stmt* >(stmt.getValue().getStmt());
                    print_stmt_kind(statement,0);
                }
            }
            else if(element.getKind() == clang::CFGElement::Kind::Constructor){
                // may have no use
                std::cout <<"Check here.\n" << std::endl;
            }   
            
        }
        i++;
    }
}

void UndefinedVariableChecker::check() {
    readConfig();
    getEntryFunc();
    definitionNum = 0;
    blockNum = 0;
    
    if (entryFunc != nullptr) {
        FunctionDecl *funDecl = manager->getFunctionDecl(entryFunc);
        SM = &manager->getFunctionDecl(entryFunc)->getASTContext().getSourceManager();
        std::cout << "The entry function is: "
                << funDecl->getQualifiedNameAsString() << std::endl;
        std::cout << "Here is its dump: " << std::endl;
        funDecl->dump();
        std::cout << "Here are related Statements: " << std::endl;
        Stmt* statement =  funDecl->getBody();

        //print_stmt_kind(statement, 0);
    }
    LangOptions LangOpts;
    LangOpts.CPlusPlus = true;
    std::unique_ptr<CFG>& cfg = manager->getCFG(entryFunc);
    cfg->dump(LangOpts, true); 
    //
    count_definition(cfg);
    get_block_statement(cfg);
    init_blockvector(cfg);
    get_all_useful_statement();
    calculate_gen_kill();
    undefined_variable_check();
    dump_debug();
    
    blockvector_output();
    find_dummy_definition();
    //get_cfg_stmt(cfg);
}

string UndefinedVariableChecker::get_statement_value(clang::Stmt* statement){
    string ret = "nothing";
    if(statement->getStmtClass() == clang::Stmt::StmtClass::DeclStmtClass){
        clang::DeclStmt* declstmt = static_cast<clang::DeclStmt*>(statement);
        if(declstmt->isSingleDecl()){
            clang::Decl* oneDecl = declstmt->getSingleDecl();   
            if(oneDecl->getKind() == clang::Decl::Kind::Var){
                clang::VarDecl* vardecl = static_cast<clang::VarDecl*>(oneDecl);
                string variableName = vardecl->getNameAsString();
                return variableName;
            }
        }
    }
    else if(statement->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass){
        clang::BinaryOperator* binaryIter = static_cast<clang::BinaryOperator*>(statement);
        if(binaryIter->getLHS()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass){
            clang::DeclRefExpr* declrefiter = static_cast<clang::DeclRefExpr*>(binaryIter->getLHS());
            string variableName = declrefiter->getNameInfo().getAsString();
            return variableName;
        }
    }
    else if(statement->getStmtClass() == clang::Stmt::StmtClass::CompoundAssignOperatorClass){
        clang::CompoundAssignOperator* compoundIter = static_cast<clang::CompoundAssignOperator*>(statement);
        if(compoundIter->getLHS()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass){
            clang::DeclRefExpr* declrefiter = static_cast<clang::DeclRefExpr*>(compoundIter->getLHS());
            string variableName = declrefiter->getNameInfo().getAsString();
            return variableName;
        }
    }
    else if(statement->getStmtClass() == clang::Stmt::StmtClass::UnaryOperatorClass){
        clang::UnaryOperator* unaryIter = static_cast<clang::UnaryOperator*>(statement);
        if(unaryIter->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass){
            clang::DeclRefExpr* declrefiter = static_cast<clang::DeclRefExpr*>(unaryIter->getSubExpr());
            string variableName = declrefiter->getNameInfo().getAsString();
            return variableName;
        }
    }
    return ret;
}

void UndefinedVariableChecker::recursive_get_binaryop(clang::Stmt* statement,UsefulStatementInfo* info){
    if(statement->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass){
        clang::BinaryOperator* binaryIter = static_cast<clang::BinaryOperator*>(statement);
        recursive_get_binaryop(binaryIter->getLHS(),info);
        recursive_get_binaryop(binaryIter->getRHS(),info);
    }
    else if(statement->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass){
        if(info->rvalueKind == 0 || info->rvalueKind == Literal)
            info->rvalueKind = Ref;
        clang::ImplicitCastExpr* implicitit = static_cast<clang::ImplicitCastExpr*>(statement);
        if(implicitit->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass){
            clang::DeclRefExpr* declrefexp = static_cast<clang::DeclRefExpr*>(implicitit->getSubExpr());
            string variable = declrefexp->getNameInfo().getAsString();
            info->rvalueString.push_back(variable);
        }
        else if(implicitit->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::ParenExprClass){
            clang::ParenExpr* parenexp = static_cast<clang::ParenExpr*>(implicitit->getSubExpr());
            clang::DeclRefExpr* declrefexp = static_cast<clang::DeclRefExpr*>(parenexp->getSubExpr());
            string variable = declrefexp->getNameInfo().getAsString();
            info->rvalueString.push_back(variable);
        }
    }
    else if(statement->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass){
        if(info->rvalueKind == 0)
            info->rvalueKind = Literal;
        clang::IntegerLiteral* integerit = static_cast<clang::IntegerLiteral*>(statement);
        info->rvalueLiteral = info->rvalueLiteral + integerit->getValue().getZExtValue();
    }
    else if(statement->getStmtClass() == clang::Stmt::StmtClass::ParenExprClass){
        clang::ParenExpr* parenexp = static_cast<clang::ParenExpr*>(statement);
        recursive_get_binaryop(parenexp->getSubExpr(),info);
    }
    else if(statement->getStmtClass() == clang::Stmt::StmtClass::UnaryOperatorClass){
        clang::UnaryOperator* unaryIter = static_cast<clang::UnaryOperator*>(statement);
        if(unaryIter->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass){
            clang::ImplicitCastExpr* implicitit = static_cast<clang::ImplicitCastExpr*>(unaryIter->getSubExpr());
            if(implicitit->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass){
                if(info->rvalueKind == 0 || info->rvalueKind == Literal)
                    info->rvalueKind = Ref;
                clang::DeclRefExpr* declrefiter = static_cast<clang::DeclRefExpr*>(implicitit->getSubExpr());
                string variableName = declrefiter->getNameInfo().getAsString();
                info->rvalueString.push_back(variableName);
            }
            else if(implicitit->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::ParenExprClass){
                clang::ParenExpr* parenexp = static_cast<clang::ParenExpr*>(implicitit->getSubExpr());
                clang::DeclRefExpr* declrefexp = static_cast<clang::DeclRefExpr*>(parenexp->getSubExpr());
                string variable = declrefexp->getNameInfo().getAsString();
                info->rvalueString.push_back(variable);
                //recursive_get_binaryop(parenexp->getSubExpr(),info);
            }
        }
        else if(unaryIter->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::ParenExprClass){
            clang::ParenExpr* parenexp = static_cast<clang::ParenExpr*>(unaryIter->getSubExpr());
            recursive_get_binaryop(parenexp->getSubExpr(),info);
        }
    }
}

void UndefinedVariableChecker::get_rvalue(clang::Stmt* statement,UsefulStatementInfo* info){
    if(statement->getStmtClass() == clang::Stmt::StmtClass::DeclStmtClass){
        clang::DeclStmt* declstmt = static_cast<clang::DeclStmt*>(statement);
        if(declstmt->isSingleDecl()){
            clang::Decl* oneDecl = declstmt->getSingleDecl();   
            if(oneDecl->getKind() == clang::Decl::Kind::Var){
                clang::VarDecl* vardecl = static_cast<clang::VarDecl*>(oneDecl);
                if(vardecl->getInit()!= nullptr){
                    clang::Expr* initexpr = vardecl->getInit();
                    if(initexpr->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass){
                        clang::IntegerLiteral* integerit = static_cast<clang::IntegerLiteral*>(initexpr);
                        info->rvalueKind = Literal;
                        info->rvalueLiteral = integerit->getValue().getZExtValue() + info->rvalueLiteral;
                    }
                    else {
                        if(initexpr->getStmtClass() == clang::Stmt::StmtClass::InitListExprClass){

                        }
                        else if(initexpr->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass){
                            clang::ImplicitCastExpr* implicitit = static_cast<clang::ImplicitCastExpr*>(initexpr);
                            clang::DeclRefExpr* declrefexp = static_cast<clang::DeclRefExpr*>(implicitit->getSubExpr());
                            string variable = declrefexp->getNameInfo().getAsString();
                            info->rvalueKind = Ref;
                            info->rvalueString.push_back(variable);
                        }

                    }
                }
            }
        }
    }
    else if(statement->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass){
        clang::BinaryOperator* binaryIter = static_cast<clang::BinaryOperator*>(statement);
        recursive_get_binaryop(binaryIter->getRHS(),info);
    }
    else if(statement->getStmtClass() == clang::Stmt::StmtClass::CompoundAssignOperatorClass){
        clang::CompoundAssignOperator* compoundIter = static_cast<clang::CompoundAssignOperator*>(statement);
        if(compoundIter->getLHS()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass){
            clang::DeclRefExpr* declrefiter = static_cast<clang::DeclRefExpr*>(compoundIter->getLHS());
            info->rvalueKind = Ref;
            string variableName = declrefiter->getNameInfo().getAsString();
            info->rvalueString.push_back(variableName);
            if(compoundIter->getRHS()->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass){
                clang::ImplicitCastExpr* implicitit = static_cast<clang::ImplicitCastExpr*>(compoundIter->getRHS());
                clang::DeclRefExpr* declrefexp = static_cast<clang::DeclRefExpr*>(implicitit->getSubExpr());
                info->rvalueString.push_back(declrefexp->getNameInfo().getAsString());
            }
        }
    }
    else if(statement->getStmtClass() == clang::Stmt::StmtClass::UnaryOperatorClass){
        clang::UnaryOperator* unaryIter = static_cast<clang::UnaryOperator*>(statement);
        if(unaryIter->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass){
            clang::DeclRefExpr* declrefiter = static_cast<clang::DeclRefExpr*>(unaryIter->getSubExpr());
            info->rvalueKind = Ref;
            string variableName = declrefiter->getNameInfo().getAsString();
            info->rvalueString.push_back(variableName);
        }
    }

}

vector<int> UndefinedVariableChecker::kill_variable(int statementno,string variable){
    vector<int> kill;
    for(int i=0;i<allusefulstatement.size();i++){
        if(statementno != i){
            if(variable == allusefulstatement[i].variable){
                kill.push_back(i);
            }
        }
    }
    return kill;
}

void UndefinedVariableChecker::count_definition(unique_ptr<CFG>& cfg){
    clang::CFG::iterator blockIter;
    for(blockIter = cfg->begin(); blockIter != cfg->end(); blockIter++){
        blockNum++;
        CFGBlock* block = *blockIter;
        BumpVector<CFGElement>::reverse_iterator elementIter;
        for(elementIter = block->begin(); elementIter != block->end(); elementIter++){
            CFGElement element = *elementIter;           
            if(element.getKind() == clang::CFGElement::Kind::Statement){
                llvm::Optional<CFGStmt> stmt = element.getAs<CFGStmt>();
                if(stmt.hasValue() == true){
                    Stmt* statement = const_cast<Stmt* >(stmt.getValue().getStmt());
                    if(statement == nullptr){
                        std::cout<< "\033[31m" << "error here" <<  "\033[0m" <<std::endl;
                    }
                    else{
                        if(statement != nullptr && statement->getStmtClass() == clang::Stmt::StmtClass::DeclStmtClass){
                            clang::DeclStmt* declstmt = static_cast<clang::DeclStmt*>(statement);
                            if(declstmt->isSingleDecl()){
                                clang::Decl* oneDecl = declstmt->getSingleDecl();
                                if(oneDecl->getKind() == clang::Decl::Kind::Var){
                                    clang::VarDecl* vardecl = static_cast<clang::VarDecl*>(oneDecl);
                                    pair<string,Initvalue> variable;
                                    string name = vardecl->getNameAsString();
                                    definitionNum++;
                                    variable.first = name;
                                    if(vardecl->getInit()!= nullptr){
                                        Initvalue initvalue;
                                        clang::Expr* initexpr = vardecl->getInit();
                                        if(initexpr->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass){
                                            initvalue.initkind = IsInitialize;
                                            clang::IntegerLiteral* integerit = static_cast<clang::IntegerLiteral*>(initexpr);
                                            //std::cout << integerit->getValue().getZExtValue() << std::endl;
                                            initvalue.value = integerit->getValue().getZExtValue();
                                        }
                                        else{
                                            if(initexpr->getStmtClass() == clang::Stmt::StmtClass::InitListExprClass){
                                                initvalue.initkind = IsInitialize;
                                                //array struct?
                                                //std::cout<<"here\n";
                                            }
                                            else if(initexpr->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass){
                                                //std::cout<<"there\n";
                                                initvalue.initkind = Reference;
                                                clang::ImplicitCastExpr* implicitit = static_cast<clang::ImplicitCastExpr*>(initexpr);
                                                clang::DeclRefExpr* declrefexp = static_cast<clang::DeclRefExpr*>(implicitit->getSubExpr());
                                                //declrefexp->dump();
                                                initvalue.ref = declrefexp->getNameInfo().getAsString();
                                            }
                                            //std::cout<<name<<std::endl;
                                        }
                                        variable.second = initvalue;
                                        var_vector.push_back(variable);
                                    }
                                    else{
                                        //std::cout<< "no initialize"<<std::endl;
                                        Initvalue initvalue;
                                        initvalue.initkind = NotInitialize;
                                        variable.second = initvalue;
                                        var_vector.push_back(variable);
                                    }
                                }   
                            }
                            else{
                                clang::DeclGroupRef multDecl = declstmt->getDeclGroup();
                                clang::DeclGroupRef::iterator itgr = multDecl.begin();
                                for(; itgr != multDecl.end(); itgr++)
                                {
                                    std::cout << (*itgr)->getDeclKindName() << std::endl;
                                    if((*itgr)->getKind() == clang::Decl::Kind::Var){
                                        clang::VarDecl* vardecl = static_cast<clang::VarDecl*>((*itgr));
                                        pair<string,Initvalue> variable;
                                        string name = vardecl->getNameAsString();
                                        definitionNum++;
                                        variable.first = name;
                                        if(vardecl->getInit()!= nullptr){
                                            Initvalue initvalue;
                                            clang::Expr* initexpr = vardecl->getInit();
                                            if(initexpr->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass){
                                                initvalue.initkind = IsInitialize;
                                                clang::IntegerLiteral* integerit = static_cast<clang::IntegerLiteral*>(initexpr);
                                                //std::cout << integerit->getValue().getZExtValue() << std::endl;
                                                initvalue.value = integerit->getValue().getZExtValue();
                                            }
                                            else{
                                                if(initexpr->getStmtClass() == clang::Stmt::StmtClass::InitListExprClass){
                                                    initvalue.initkind = IsInitialize;
                                                //array struct?
                                                }
                                                else if(initexpr->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass){
                                                    initvalue.initkind = Reference;
                                                    clang::ImplicitCastExpr* implicitit = static_cast<clang::ImplicitCastExpr*>(initexpr);
                                                    clang::DeclRefExpr* declrefexp = static_cast<clang::DeclRefExpr*>(implicitit->getSubExpr());
                                                    //declrefexp->dump();
                                                    initvalue.ref = declrefexp->getNameInfo().getAsString();
                                                }
                                            }
                                            variable.second = initvalue;
                                            var_vector.push_back(variable);
                                        }
                                        else{
                                            //std::cout<< "no initialize"<<std::endl;
                                            Initvalue initvalue;
                                            initvalue.initkind = NotInitialize;
                                            variable.second = initvalue;
                                            var_vector.push_back(variable);
                                        }
                                    }  
                                }
                            }
                        }
                    }
                }
            }
            else if(element.getKind() == clang::CFGElement::Kind::Constructor){
                // may have no use
                std::cout << "Check here.\n" << std::endl;
            }   
            
        }
    }
}

void UndefinedVariableChecker::readConfig() {
  std::unordered_map<std::string, std::string> ptrConfig =
      configure->getOptionBlock("UndefinedVariableChecker");
  request_fun = stoi(ptrConfig.find("request_fun")->second);
  maxPathInFun = 10;
}

void UndefinedVariableChecker::getEntryFunc() {
  std::vector<ASTFunction *> topLevelFuncs = call_graph->getTopLevelFunctions();
  for (auto fun : topLevelFuncs) {
    const FunctionDecl *funDecl = manager->getFunctionDecl(fun);
    if (funDecl->getQualifiedNameAsString() == "main") {
      entryFunc = fun;
      return;
    }
  }
  entryFunc = nullptr;
  return;
}

void UndefinedVariableChecker::blockvector_output(){
    for(int i=0;i<blockNum;i++){
        std::cout<< "\033[31m" << "Block Number: " << blockbitvector[i].blockid;
        std::cout << "   Out vector: " << "\033[0m";
        for(int j=0;j<bitVectorLength;j++){
            std::cout<< blockbitvector[i].Outvector[j];
        }
        std::cout << std::endl;
    }
}

void UndefinedVariableChecker::dump_debug(){
    assert(definitionNum == var_vector.size());
    std::cout<< "Definition Number: "<< definitionNum <<std::endl;
    std::cout<< "Block Number: "<< blockNum <<std::endl;
    for(int i=0;i<var_vector.size();i++){
        std::cout<< "Variable Name:" << var_vector[i].first;
        if(var_vector[i].second.initkind == IsInitialize){
            std::cout<< "     Is initialized " << var_vector[i].second.value << std::endl; 
        }   
        else if (var_vector[i].second.initkind == Reference)
        {
            std::cout<< "     Is referenced " << var_vector[i].second.ref << std::endl; 
        }
        else{
            std::cout<< "     Not initialized"<< std::endl; 
        }
    }
    /*for(int i = 0;i<block_statement.size();i++){
        std::cout << "Block" << block_statement[i].blockno << ":   "<<std::endl;
        for(int j=0;j<block_statement[i].usefulBlockStatement.size();j++){
            block_statement[i].usefulBlockStatement[j].stmt->dump();
        }
    }*/
    std::cout<< "bit vector length:  "<< bitVectorLength <<std::endl;
    for(int i=0;i<blockbitvector.size();i++){
        std::cout << "Block" << blockbitvector[i].blockid << ":   "<<std::endl;
        std::cout << "Succ:";
        for(int j=0;j<blockbitvector[i].succ.size();j++){
            std::cout<< blockbitvector[i].succ[j] << " ";
        }
        std::cout<<std::endl;
        std::cout << "Pred:";
        for(int j=0;j<blockbitvector[i].pred.size();j++){
            std::cout<< blockbitvector[i].pred[j] << " ";
        }
        std::cout<<std::endl;
    }
    for(int i=0;i<blockNum;i++){
        std::cout << "\033[31m" << "BLOCK" << blockbitvector[i].blockid << ": gen   " << "\033[0m";
        for(int j=0;j<bitVectorLength;j++){
            std::cout<< blockbitvector[i].Genvector[j];
        }
        std::cout << "\033[31m" << "   kill:   " << "\033[0m";
        for(int j=0;j<bitVectorLength;j++){
            std::cout<< blockbitvector[i].Killvector[j];
        }
        std::cout << std::endl;
    }
    std::cout<< "******************************" <<std::endl;
    for(int i=0;i<allusefulstatement.size();i++){
        
        //allusefulstatement[i].statement->getBeginLoc().dump(*SM);
        std::cout << "left: " << allusefulstatement[i].variable << " ";
        if(allusefulstatement[i].rvalueKind == Literal)
            std::cout << "right literal:" << allusefulstatement[i].rvalueLiteral <<std::endl;
        else if(allusefulstatement[i].rvalueKind == Ref){
            std::cout << "right ref: ";
            for(int j=0;j<allusefulstatement[i].rvalueString.size();j++){
                std::cout << allusefulstatement[i].rvalueString[j] << " ";
            }
            std:: cout << std::endl;
        }
    }
}

void UndefinedVariableChecker::get_bit_vector_length(){
    bitVectorLength = definitionNum;
    for(int i = 0;i<block_statement.size();i++){
        bitVectorLength = block_statement[i].usefulBlockStatement.size() + bitVectorLength;
    }
}

void UndefinedVariableChecker::get_block_statement(unique_ptr<CFG>& cfg){
    int curBlockNum = 0;
    int statementNum = definitionNum;
    clang::CFG::iterator blockIter;
    for(blockIter = cfg->begin(); blockIter != cfg->end(); blockIter++){
        BlockInfo blockInfo;
        CFGBlock* block = *blockIter;
        blockInfo.blockno = block->getBlockID();
        BumpVector<CFGElement>::reverse_iterator elementIter;
        for(elementIter = block->begin(); elementIter != block->end(); elementIter++){
            CFGElement element = *elementIter;
            
            if(element.getKind() == clang::CFGElement::Kind::Statement){
                llvm::Optional<CFGStmt> stmt = element.getAs<CFGStmt>();
                if(stmt.hasValue() == true){
                    Stmt* statement = const_cast<Stmt* >(stmt.getValue().getStmt());
                    if(statement == nullptr){
                        std::cout<< "\033[31m" << "error here" <<  "\033[0m" <<std::endl;
                    }
                    else{
                        //statement->dump();
                        if(statement->getStmtClass() == clang::Stmt::StmtClass::DeclStmtClass){
                            clang::DeclStmt* declstmt = static_cast<clang::DeclStmt*>(statement);
                            if(declstmt->isSingleDecl()){
                                clang::Decl* oneDecl = declstmt->getSingleDecl();   
                                if(oneDecl->getKind() == clang::Decl::Kind::Var){
                                    clang::VarDecl* vardecl = static_cast<clang::VarDecl*>(oneDecl);
                                    //std::cout << vardecl->getNameAsString() << std::endl;
                                    if(vardecl->getInit()!= nullptr){
                                        clang::Expr* initexpr = vardecl->getInit();
                                        if(initexpr->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass){
                                            StatementInfo statementPair;
                                            statementPair.statementno = statementNum;
                                            statementNum++;
                                            statementPair.stmt = statement;
                                            statementPair.isdummy = false;
                                            blockInfo.usefulBlockStatement.push_back(statementPair);
                                        }
                                        else{
                                            if(initexpr->getStmtClass() == clang::Stmt::StmtClass::InitListExprClass){
                                                //array struct?



                                            }
                                            else if(initexpr->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass){
                                                StatementInfo statementPair;
                                                statementPair.statementno = statementNum;
                                                statementNum++;
                                                statementPair.stmt = statement;
                                                statementPair.isdummy = false;
                                                blockInfo.usefulBlockStatement.push_back(statementPair);
                                            }
                                        }
                                    }
                                    else{
                                        //std::cout << "definition doesn't have initialize = operator\n";
                                    }
                                }   
                            }
                            else{
                                clang::DeclGroupRef multDecl = declstmt->getDeclGroup();
                                clang::DeclGroupRef::iterator itgr = multDecl.begin();
                                for(; itgr != multDecl.end(); itgr++)
                                {
                                    if((*itgr)->getKind() == clang::Decl::Kind::Var){
                                        clang::VarDecl* vardecl = static_cast<clang::VarDecl*>((*itgr));
                                        if(vardecl->getInit()!= nullptr){
                                            clang::Expr* initexpr = vardecl->getInit();
                                            if(initexpr->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass){
                                                StatementInfo statementPair;
                                                statementPair.statementno = statementNum;
                                                statementNum++;
                                                statementPair.stmt = statement;
                                                statementPair.isdummy = false;
                                                blockInfo.usefulBlockStatement.push_back(statementPair);
                                            }
                                            else{
                                                if(initexpr->getStmtClass() == clang::Stmt::StmtClass::InitListExprClass){
                                                //array struct?



                                                }
                                                else if(initexpr->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass){
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
                        }
                        else if(statement->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass){
                            clang::BinaryOperator* binaryIter = static_cast<clang::BinaryOperator*>(statement);
                            
                            if(binaryIter->isAssignmentOp()){
                                StatementInfo statementPair;
                                statementPair.statementno = statementNum;
                                statementNum++;
                                statementPair.stmt = statement;
                                statementPair.isdummy = false;
                                blockInfo.usefulBlockStatement.push_back(statementPair);
                            }// x = 1
                            else if(binaryIter->isCompoundAssignmentOp()){
                                StatementInfo statementPair;
                                statementPair.statementno = statementNum;
                                statementNum++;
                                statementPair.stmt = statement;
                                statementPair.isdummy = false;
                                blockInfo.usefulBlockStatement.push_back(statementPair);
                            }// ?
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
                        }//x ++ or x-- or ++x or --x
                    }
                }
            }
            else if(element.getKind() == clang::CFGElement::Kind::Constructor){
                // may have no use
                std::cout <<"Check here.\n" << std::endl;
            }   
            
        }
        block_statement.push_back(blockInfo);
        curBlockNum++;
    }
    assert(curBlockNum == blockNum);
    std::cout<< statementNum<< std::endl;
    get_bit_vector_length();
}

void UndefinedVariableChecker::init_blockvector(unique_ptr<CFG>& cfg){
    clang::CFG::iterator blockIter;
    for(blockIter = cfg->begin(); blockIter != cfg->end(); blockIter++){
        CFGBlock* block = *blockIter;
        BlockBitVector blockelement;
        blockelement.blockid = block->getBlockID();
        if(block->succ_empty() == false){
            //int t = 0;
            /*if(block->getBlockID() == 6)
                std::cout << block->succ_size() <<endl;*/
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
        for(int i=0;i<bitVectorLength;i++){
            if(i < definitionNum){
                blockelement.Invector.push_back(1);
                blockelement.Outvector.push_back(1);
            }
            else{
                blockelement.Invector.push_back(0);
                blockelement.Outvector.push_back(0);
            }
            blockelement.Genvector.push_back(0);
            blockelement.Killvector.push_back(0);
        }
        blockelement.isvisit = false;
        blockbitvector.push_back(blockelement);
    }
}

void UndefinedVariableChecker::get_all_useful_statement(){
    for(int i=0;i<blockNum;i++){
        for(int j=0;j<block_statement[i].usefulBlockStatement.size();j++){
            string name = get_statement_value(block_statement[i].usefulBlockStatement[j].stmt);
            UsefulStatementInfo info;
            info.isdummy = false;
            info.variable = name;
            info.statement = block_statement[i].usefulBlockStatement[j].stmt;
            info.rvalueLiteral = 0;
            info.rvalueKind = 0;
            get_rvalue(block_statement[i].usefulBlockStatement[j].stmt,&info);
            allusefulstatement.push_back(info);
        }
    }
}

void UndefinedVariableChecker::calculate_gen_kill(){
    for(int i = 0;i < blockNum; i++){
        vector<string> alreadyCalulateVariable;
        for(int j=block_statement[i].usefulBlockStatement.size()-1;j >= 0;j--){
            if(block_statement[i].usefulBlockStatement[j].stmt->getStmtClass() == clang::Stmt::StmtClass::DeclStmtClass){
                clang::DeclStmt* declstmt = static_cast<clang::DeclStmt*>(block_statement[i].usefulBlockStatement[j].stmt);
                if(declstmt->isSingleDecl()){
                    clang::Decl* oneDecl = declstmt->getSingleDecl();   
                    if(oneDecl->getKind() == clang::Decl::Kind::Var){
                        clang::VarDecl* vardecl = static_cast<clang::VarDecl*>(oneDecl);
                        //std::cout << vardecl->getNameAsString() << std::endl;
                        string variableName = vardecl->getNameAsString();
                        bool isexist = false;
                        for(int k=0;k<alreadyCalulateVariable.size();k++){
                            if(alreadyCalulateVariable[k] == variableName){
                                isexist = true;
                                break;
                            }
                        }
                        if(isexist == false){
                            alreadyCalulateVariable.push_back(variableName);
                            assert(i == blockbitvector[i].blockid);
                            blockbitvector[i].Genvector[block_statement[i].usefulBlockStatement[j].statementno] = 1;
                            vector<int> kill;
                            kill = kill_variable(block_statement[i].usefulBlockStatement[j].statementno-definitionNum,variableName);
                            for(int t=0;t<kill.size();t++){
                                blockbitvector[i].Killvector[kill[t]+definitionNum] = 1;
                            }
                        }
                    }
                }
            }
            else if(block_statement[i].usefulBlockStatement[j].stmt->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass){
                clang::BinaryOperator* binaryIter = static_cast<clang::BinaryOperator*>(block_statement[i].usefulBlockStatement[j].stmt);
                if(binaryIter->getLHS()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass){
                    clang::DeclRefExpr* declrefiter = static_cast<clang::DeclRefExpr*>(binaryIter->getLHS());
                    string variableName = declrefiter->getNameInfo().getAsString();
                    bool isexist = false;
                    for(int k=0;k<alreadyCalulateVariable.size();k++){
                        if(alreadyCalulateVariable[k] == variableName){
                            isexist = true;
                            break;
                        }
                    }
                    if(isexist == false){
                        alreadyCalulateVariable.push_back(variableName);
                        assert(i == blockbitvector[i].blockid);
                        blockbitvector[i].Genvector[block_statement[i].usefulBlockStatement[j].statementno] = 1;
                        vector<int> kill;
                        kill = kill_variable(block_statement[i].usefulBlockStatement[j].statementno-definitionNum,variableName);
                        for(int t=0;t<kill.size();t++){
                            blockbitvector[i].Killvector[kill[t]+definitionNum] = 1;
                        }
                    }
                }
            }
            else if(block_statement[i].usefulBlockStatement[j].stmt->getStmtClass() == clang::Stmt::StmtClass::CompoundAssignOperatorClass){
                clang::CompoundAssignOperator* compoundIter = static_cast<clang::CompoundAssignOperator*>(block_statement[i].usefulBlockStatement[j].stmt);
                if(compoundIter->getLHS()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass){
                    clang::DeclRefExpr* declrefiter = static_cast<clang::DeclRefExpr*>(compoundIter->getLHS());
                    string variableName = declrefiter->getNameInfo().getAsString();
                    bool isexist = false;
                    for(int k=0;k<alreadyCalulateVariable.size();k++){
                        if(alreadyCalulateVariable[k] == variableName){
                            isexist = true;
                            break;
                        }
                    }
                    if(isexist == false){
                        alreadyCalulateVariable.push_back(variableName);
                        assert(i == blockbitvector[i].blockid);
                        blockbitvector[i].Genvector[block_statement[i].usefulBlockStatement[j].statementno] = 1;
                        vector<int> kill;
                        kill = kill_variable(block_statement[i].usefulBlockStatement[j].statementno-definitionNum,variableName);
                        for(int t=0;t<kill.size();t++){
                            blockbitvector[i].Killvector[kill[t]+definitionNum] = 1;
                        }
                    }
                }
            }
            else if(block_statement[i].usefulBlockStatement[j].stmt->getStmtClass() == clang::Stmt::StmtClass::UnaryOperatorClass){
                clang::UnaryOperator* unaryIter = static_cast<clang::UnaryOperator*>(block_statement[i].usefulBlockStatement[j].stmt);
                if(unaryIter->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass){
                    clang::DeclRefExpr* declrefiter = static_cast<clang::DeclRefExpr*>(unaryIter->getSubExpr());
                    string variableName = declrefiter->getNameInfo().getAsString();
                    bool isexist = false;
                    for(int k=0;k<alreadyCalulateVariable.size();k++){
                        if(alreadyCalulateVariable[k] == variableName){
                            isexist = true;
                            break;
                        }
                    }
                    if(isexist == false){
                        alreadyCalulateVariable.push_back(variableName);
                        assert(i == blockbitvector[i].blockid);
                        blockbitvector[i].Genvector[block_statement[i].usefulBlockStatement[j].statementno] = 1;
                        vector<int> kill;
                        kill = kill_variable(block_statement[i].usefulBlockStatement[j].statementno-definitionNum,variableName);
                        for(int t=0;t<kill.size();t++){
                            blockbitvector[i].Killvector[kill[t]+definitionNum] = 1;
                        }
                    }
                }
            }
        }
        if(alreadyCalulateVariable.empty() == false){
            for(int k = 0;k<alreadyCalulateVariable.size();k++){
                for(int t=0;t<definitionNum;t++){
                    if(var_vector[t].first == alreadyCalulateVariable[k]){
                        blockbitvector[i].Killvector[t] = 1;
                    }
                }
            }
        }
    }
}

void UndefinedVariableChecker::calculate_block(int num){
    if(blockbitvector[num].isvisit == true) return;
    queue<BlockBitVector> q;
    q.push(blockbitvector[num]);
    blockbitvector[num].isvisit = true;
    while(!q.empty()){
        BlockBitVector tempblock = q.front();
        vector<int> temp;
        for(int i=0;i<bitVectorLength;i++)
            temp.push_back(0);
        if(tempblock.pred.empty() == false){
            for(int i=0;i<tempblock.pred.size();i++){
                for(int j=0;j<bitVectorLength;j++){
                    if(blockbitvector[tempblock.pred[i]].Outvector[j] == 1)
                        temp[j] = 1;
                }
            }
            tempblock.Invector = temp;
        }
        vector<int> res;
        for(int i=0;i<bitVectorLength;i++)
            res.push_back(tempblock.Invector[i]);
        for(int i=0;i<bitVectorLength;i++){
            if(tempblock.Killvector[i] == 1)
                res[i] = 0;
        }
        for(int i=0;i<bitVectorLength;i++)
        {
            if(tempblock.Genvector[i] == 1)
                res[i] = 1;
        }
        blockbitvector[tempblock.blockid].Outvector = res;
        for(int i=0;i<tempblock.succ.size();i++){
            if(blockbitvector[tempblock.succ[i]].isvisit == false){
                q.push(blockbitvector[tempblock.succ[i]]);
                blockbitvector[tempblock.succ[i]].isvisit = true;
            }
        }
        q.pop();
    }
}

void UndefinedVariableChecker::undefined_variable_check(){
    bool ischanging = true;
    std::vector<BlockBitVector> pre_blockbitvector;
    for(int i=0;i<blockNum;i++){
        pre_blockbitvector.push_back(blockbitvector[i]);
    }
    int count = 0;
    while(ischanging){
        calculate_block(blockNum-1);
        std::cout << count << std::endl;
        //blockvector_output();
        for(int i=0;i<blockNum;i++){
            if(blockbitvector[i].isvisit == false){
                //std::cout << i <<std::endl;
                std::cout<< "wrong bfs" <<std::endl;
                for(int j=0;j<bitVectorLength;j++){
                    blockbitvector[i].Invector[j] = 0;
                    blockbitvector[i].Outvector[j] = 0;
                }
            }
        }
        bool allchange = false;
        for(int i=0;i<blockNum;i++){
            if(pre_blockbitvector[i].Outvector != blockbitvector[i].Outvector){
                allchange = true;
            }
        }
        if(allchange == true){
            ischanging = true;
            for(int i=0;i<blockNum;i++)
                blockbitvector[i].isvisit = false;
            for(int i=0;i<blockNum;i++)
                pre_blockbitvector[i] = blockbitvector[i];
        }
        else{
            ischanging = false;
            for(int i=0;i<blockNum;i++)
                blockbitvector[i].isvisit = false;
        }
        count++;
    }
    std::cout<< "iteration time: " << count <<std::endl;
}

void UndefinedVariableChecker::report_warning(string name,clang::Stmt* statement){
    string statementinfo = statement->getBeginLoc().printToString(*SM);
    int divideline1 = 0,divideline2 = 0;
    for(int i=0;i<statementinfo.length();i++){
        if(statementinfo[i] == ':' && divideline1 == 0){
            divideline1 = i;
        }
        else if(statementinfo[i] == ':' && divideline1 != 0){
            divideline2 = i;
            break;
        }
    }
    string line = statementinfo.substr(divideline1 + 1,divideline2 - divideline1 - 1);
    string column = statementinfo.substr(divideline2 + 1,statementinfo.length()-divideline2 - 1);
    std::cout << "\033[31m" << "WARNING:USE UNINITIALIZED VARIABLE " << name << ": " << "Line: " << line << " Column: " << column << "\033[0m" << std::endl;
}

void UndefinedVariableChecker::check_variable_use_in_block(string name,int blockno,int statementno){
    bool isknown = false;
    int preStatementnum = 0;
    //if(name == "a")
      //  std::cout << blockno << " " << statementno <<std::endl;
    for(int i=0;i<blockno;i++){
        preStatementnum = preStatementnum + block_statement[i].usefulBlockStatement.size();
    }
    preStatementnum = preStatementnum + definitionNum;
    for(int i=preStatementnum - definitionNum;i<=statementno - definitionNum;i++){
        if(allusefulstatement[i].variable == name){
            if(allusefulstatement[i].rvalueKind == Literal){
                isknown = true;
            }
            else if(allusefulstatement[i].rvalueKind == Ref){
                bool hasmyself = false;
                for(int j=0;j<allusefulstatement[i].rvalueString.size();j++){
                    if(name == allusefulstatement[i].rvalueString[j])
                        hasmyself = true;
                }
                if(hasmyself == true){
                    if(isknown == false){
                        //definition use myself warning should be reported
                        report_warning(name,allusefulstatement[i].statement);
                    }
                    else{
                        // has inited do nothing
                    }
                }
                else   
                    isknown = true;
            }
        }
        else{
            if(allusefulstatement[i].rvalueKind == Literal){
            }
            else{
                for(int j=0;j<allusefulstatement[i].rvalueString.size();j++){
                    if(name == allusefulstatement[i].rvalueString[j]){
                        if(isknown == false)
                        {
                            // use uninited variable warning should be reported
                            report_warning(name,allusefulstatement[i].statement);
                        }
                    }
                }
            }
        }
    }
}

void UndefinedVariableChecker::find_dummy_definition(){
    for(int i=0;i<definitionNum;i++){
        int target_statement_no = i;
        string name = var_vector[i].first;
        queue<BlockBitVector> q;
        q.push(blockbitvector[blockNum-1]);
        blockbitvector[blockNum-1].isvisit = true;
        while(!q.empty()){
            bool stop = false;
            BlockBitVector tempblock = q.front();
            if(tempblock.blockid != blockNum-1 && tempblock.blockid != 0){
                if(tempblock.Outvector[target_statement_no] == 1){
                    int statenum = definitionNum;
                    for(int j=0;j<=tempblock.blockid;j++)
                        statenum = statenum + block_statement[j].usefulBlockStatement.size();
                    check_variable_use_in_block(name,tempblock.blockid, statenum - 1);
                }
                else{//kill unreachable
                    for(int j=0;j<bitVectorLength;j++){
                        if(tempblock.Outvector[j] == 1 && allusefulstatement[j-definitionNum].variable == name){ // gen variable                        
                            check_variable_use_in_block(name,tempblock.blockid, j);
                            stop = true;
                        }
                    }
                }
            }
            if(stop == true) break;
            for(int i=0;i<tempblock.succ.size();i++){
                if(blockbitvector[tempblock.succ[i]].isvisit == false){
                    q.push(blockbitvector[tempblock.succ[i]]);
                    blockbitvector[tempblock.succ[i]].isvisit = true;
                }
            }
            q.pop();
        }
        for(int j=0;j<blockNum;j++){
            blockbitvector[j].isvisit = false;
        }
        //std::cout << "variable " << name << "finished" <<std::endl;
    }
}