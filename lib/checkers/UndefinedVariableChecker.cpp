#include "checkers/UndefinedVariableChecker.h"

/*
TODO: lvalue can be struct or array, they need to be considered, do it later
*/

void UndefinedVariableChecker::print_stmt_kind(Stmt *statement, int spaceCount)
{
    if (statement == nullptr)
    {
        for (int i = 0; i < spaceCount; i++)
            std::cout << "  ";
        std::cout << "<<<NULL>>>" << std::endl;
        return;
    }
    for (int i = 0; i < spaceCount; i++)
        std::cout << "  ";
    std::cout << statement->getStmtClassName() << std::endl;
    // clang::Stmt::StmtClass::
    // clang::TranslationUnitDecl::getTranslationUnitDecl();

    if (statement != nullptr && statement->getStmtClass() == clang::Stmt::StmtClass::DeclStmtClass)
    {
        clang::DeclStmt *declstmt = static_cast<clang::DeclStmt *>(statement);
        clang::DeclStmt::decl_iterator it1 = declstmt->decl_begin();
        if (declstmt->isSingleDecl())
        {
            clang::Decl *oneDecl = declstmt->getSingleDecl();
            for (int i = 0; i < spaceCount + 1; i++)
                std::cout << "  ";
            std::cout << oneDecl->getDeclKindName() << std::endl;
            if (oneDecl->getKind() == clang::Decl::Kind::Var)
            {
                clang::VarDecl *vardecl = static_cast<clang::VarDecl *>(oneDecl);
                std::cout << vardecl->getNameAsString() << std::endl;
                // vardecl->dumpColor();
                if (vardecl->getInit() != nullptr)
                {
                    clang::Expr *initexpr = vardecl->getInit();
                    if (initexpr->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass)
                    {
                        clang::IntegerLiteral *integerit = static_cast<clang::IntegerLiteral *>(initexpr);
                        std::cout << integerit->getValue().getZExtValue() << std::endl;
                    }
                }
            }
        }
        else
        {
            clang::DeclGroupRef multDecl = declstmt->getDeclGroup();
            clang::DeclGroupRef::iterator itgr = multDecl.begin();
            for (; itgr != multDecl.end(); itgr++)
            {
                for (int i = 0; i < spaceCount + 1; i++)
                    std::cout << "  ";
                std::cout << (*itgr)->getDeclKindName() << std::endl;
                if ((*itgr)->getKind() == clang::Decl::Kind::Var)
                {
                    clang::VarDecl *vardecl = static_cast<clang::VarDecl *>((*itgr));
                    std::cout << vardecl->getNameAsString() << std::endl;
                    if (vardecl->getInit() != nullptr)
                    {
                        clang::Expr *initexpr = vardecl->getInit();
                        if (initexpr->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass)
                        {
                            clang::IntegerLiteral *integerit = static_cast<clang::IntegerLiteral *>(initexpr);
                            std::cout << integerit->getValue().getZExtValue() << std::endl;
                        }
                    }
                }
            }
        }
    }
    if (statement != nullptr && statement->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
    {
        clang::DeclRefExpr *declrefexp = static_cast<clang::DeclRefExpr *>(statement);
        // declrefexp->getNameInfo().getName().dump();
        // static_cast<clang::VarDecl*>(declrefexp->getDecl())->getInit()->dumpColor();
        // if(static_cast<clang::VarDecl*>(declrefexp->getDecl())->getEvaluatedValue() != nullptr)
        // static_cast<clang::VarDecl*>(declrefexp->getDecl())->getDeclContext()->dumpDeclContext();
        // static_cast<clang::VarDecl*>(declrefexp->getDecl()).imp
        // if(declrefexp->getQualifier())
        //   declrefexp->getQualifier()->dump();
    }
    if (statement != nullptr && statement->getStmtClass() == clang::Stmt::StmtClass::IfStmtClass)
    {
        std::cout << "good\n";
    }
    clang::Stmt::child_iterator iter = statement->child_begin();
    for (; iter != statement->child_end(); iter++)
    {
        print_stmt_kind(*iter, spaceCount + 1);
    }
}

void UndefinedVariableChecker::get_cfg_stmt(unique_ptr<CFG> &cfg)
{
    int i = 0;
    clang::CFG::iterator blockIter;
    for (blockIter = cfg->begin(); blockIter != cfg->end(); blockIter++)
    {
        std::cout << "\033[31m"
                  << "BLOCK" << i << "\033[0m" << std::endl;
        CFGBlock *block = *blockIter;
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
                    print_stmt_kind(statement, 0);
                }
            }
            else if (element.getKind() == clang::CFGElement::Kind::Constructor)
            {
                // may have no use
                std::cout << "Check here.\n"
                          << std::endl;
            }
        }
        i++;
    }
}

/*void UndefinedVariableChecker::insert_Hashtable(string name){
    int hashNum = hash_pjw(name);
    Hashtable[hashNum] = 1;
}

bool UndefinedVariableChecker::findHashtable(string name){
    int hashNum = hash_pjw(name);
    std::cout << name << " " <<hashNum <<std::endl;
    if(Hashtable[hashNum] == 1)
        return true;
    else
        return false;
}

unsigned int UndefinedVariableChecker::hash_pjw(string name)
{
    unsigned int val = 0, i;
    for(int j=0;j<name.length();j++){
        val = (val << 2) + (char)name[j];
        if (i = val & ~HASH_SIZE) val = (val ^ (i >> 12)) & HASH_SIZE;
    }
    return val;
}*/

void UndefinedVariableChecker::reset()
{
    this->blockNum = 0;
    definitionNum = 0;
    std::vector<std::pair<string, Initvalue>>().swap(var_vector);
    std::vector<BlockInfo>().swap(block_statement);
    std::vector<BlockInfo>().swap(useless_block_statement);
    std::vector<BlockBitVector>().swap(blockbitvector);
    std::vector<UsefulStatementInfo>().swap(allusefulstatement);
    std::unordered_map<string, vector<string>>().swap(locationMap);
    assert(var_vector.size() == 0);
    assert(block_statement.size() == 0);
    assert(useless_block_statement.size() == 0);
    assert(blockbitvector.size() == 0);
    assert(allusefulstatement.size() == 0);
    assert(locationMap.size() == 0);
}

void UndefinedVariableChecker::check()
{
    readConfig();
    getEntryFunc();
    for (int i = 0; i < allFunctions.size(); i++)
    {
        InterNode internode;
        internode.isvisit = false;
        internode.funcname = manager->getFunctionDecl(allFunctions[i])->getQualifiedNameAsString();
        graph.push_back(internode);
    }
    nodeNum = allFunctions.size();
    definitionNum = 0;
    this->blockNum = 0;
    need_to_print_file = false;
    std::vector<Child> childlist;
    /*if (entryFunc != nullptr) {
        FunctionDecl *funDecl = manager->getFunctionDecl(entryFunc);
        SM = &manager->getFunctionDecl(entryFunc)->getASTContext().getSourceManager();
        std::cout << "The entry function is: "
                << funDecl->getQualifiedNameAsString() << std::endl;
        std::cout << "Here is its dump: " << std::endl;
        //funDecl->dump();
        std::cout << "Here are related Statements: " << std::endl;
        Stmt* statement =  funDecl->getBody();

        std::vector<ASTFunction*> vec;
        vec = call_graph->getChildren(entryFunc);
        for(int i = 0;i<vec.size();i++){
            Child child;
            child.astfunc = vec[i];
            child.funcDecl = manager->getFunctionDecl(vec[i]);
            child.funcname = child.funcDecl->getQualifiedNameAsString();
            child.param_num = child.funcDecl->getNumParams();
            for(int k=0;k<child.param_num;k++)
                child.parameter_init.push_back(1);
            childlist.push_back(child);
        }*/
    // print_stmt_kind(statement, 0);
    for (int i = 0; i < allFunctions.size(); i++)
    {
        need_to_print_file = false;
        FunctionDecl *funDecl = manager->getFunctionDecl(allFunctions[i]);
        SM = &manager->getFunctionDecl(allFunctions[i])->getASTContext().getSourceManager();
        Stmt *statement = funDecl->getBody();
        LangOptions LangOpts;
        LangOpts.CPlusPlus = true;
        std::unique_ptr<CFG> &cfg = manager->getCFG(allFunctions[i]);
        // cfg->dump(LangOpts, true);
        // get_cfg_stmt(cfg);
        // print_stmt_kind(statement,0);
        is_other_function(funDecl);
        count_definition(cfg);
        get_block_statement(cfg);
        init_blockvector(cfg);
        // std::cout << funDecl->getNumParams() << endl;
        init_param(funDecl->getNumParams());
        get_all_useful_statement();
        get_useless_statement_variable();
        calculate_gen_kill();
        undefined_variable_check();
        // dump_debug();

        // blockvector_output();
        find_dummy_definition(nullptr);
        this->blockNum = 0;
        definitionNum = 0;
        std::vector<std::pair<string, Initvalue>>().swap(var_vector);
        std::vector<BlockInfo>().swap(block_statement);
        std::vector<BlockInfo>().swap(useless_block_statement);
        std::vector<BlockBitVector>().swap(blockbitvector);
        std::vector<UsefulStatementInfo>().swap(allusefulstatement);
        std::unordered_map<string, vector<string>>().swap(locationMap);
    }
    //}
    /*LangOptions LangOpts;
    LangOpts.CPlusPlus = true;
    std::unique_ptr<CFG>& cfg = manager->getCFG(entryFunc);
    //cfg->dump(LangOpts, true);
    //
    //get_cfg_stmt(cfg);
    count_definition(cfg);

    get_block_statement(cfg);
    init_blockvector(cfg);
    get_all_useful_statement();
    get_useless_statement_variable();
    calculate_gen_kill();
    undefined_variable_check();
    //dump_debug();

    //blockvector_output();
    find_dummy_definition(&childlist);
    dump_func(childlist);
    for(int i=0;i<childlist.size();i++){
        reset();
        for(int j=0;j<nodeNum;j++){
            if(graph[j].funcname == childlist[i].funcname){
                if(graph[j].isvisit == false){
                    check_child(childlist[i]);
                }
            }
        }
    }*/
}

void UndefinedVariableChecker::check_child(Child child)
{
    // std::cout <<"start" << this->blockNum << endl;
    std::vector<Child> childlist;
    std::vector<ASTFunction *> vec;
    vec = call_graph->getChildren(child.astfunc);
    for (int i = 0; i < vec.size(); i++)
    {
        Child newchild;
        newchild.astfunc = vec[i];
        newchild.funcDecl = manager->getFunctionDecl(vec[i]);
        newchild.funcname = newchild.funcDecl->getQualifiedNameAsString();
        newchild.param_num = newchild.funcDecl->getNumParams();
        for (int k = 0; k < newchild.param_num; k++)
            newchild.parameter_init.push_back(1);
        childlist.push_back(newchild);
    }
    need_to_print_file = false;
    SM = &child.funcDecl->getASTContext().getSourceManager();
    std::cout << "The function is: "
              << child.funcDecl->getQualifiedNameAsString() << std::endl;
    // child.funcDecl->dump();
    is_other_function(child.funcDecl);
    LangOptions LangOpts;
    LangOpts.CPlusPlus = true;
    std::unique_ptr<CFG> &cfg = manager->getCFG(child.astfunc);
    // cfg->dump(LangOpts, true);

    // get_cfg_stmt(cfg);
    // std::cout << this->blockNum << endl;
    count_definition(cfg);
    // std::cout << this->blockNum << endl;
    get_block_statement(cfg);

    init_blockvector(cfg);
    change_definition_bit(child.parameter_init);
    get_all_useful_statement();
    get_useless_statement_variable();
    calculate_gen_kill();
    undefined_variable_check();
    // std::cout <<"good\n";
    // dump_debug();
    // std::cout <<"good\n";
    // blockvector_output();
    find_dummy_definition(nullptr);
    // std::cout <<"good\n";
    // std::cout << vec.size() << " " << childlist.size() << endl;
    // std::cout <<"bad\n";
    /*for(int i=0;i < childlist.size();i++){
        std::cout << childlist[i].funcname << " " << childlist[i].param_num << " ";
        childlist[i].funcDecl->dump();
        cout << childlist[i].astfunc->getName() << endl;
        for(int j=0;j<childlist[i].parameter_init.size();j++){
            std::cout << childlist[i].parameter_init[j];
        }
        std::cout << std::endl;
    }*/
    // std::cout <<"good\n";
    // dump_func(childlist);
    // std::cout <<"good\n";
    int curnode = 0;
    // std::cout <<"good\n";
    for (int i = 0; i < nodeNum; i++)
    {
        if (graph[i].funcname == child.funcname)
        {
            curnode = i;
            graph[i].isvisit = true;
            break;
        }
    }
    // std::cout <<"good\n";
    for (int i = 0; i < childlist.size(); i++)
    {
        reset();
        for (int j = 0; j < nodeNum; j++)
        {
            if (graph[j].funcname == childlist[i].funcname)
            {
                if (graph[j].isvisit == false)
                {
                    // std::cout <<"enter" << this->blockNum << endl;
                    if (this->blockNum == 0)
                    {
                        // std::cout <<"good\n";
                        check_child(childlist[i]);
                    }
                }
            }
        }
    }
    graph[curnode].isvisit = false;
}

void UndefinedVariableChecker::change_definition_bit(std::vector<int> param_bit)
{
    for (int i = 0; i < param_bit.size(); i++)
    {
        if (param_bit[i] == 1)
        {
            for (int j = 0; j < this->blockNum; j++)
            {
                blockbitvector[j].Invector[i] = 0;
                blockbitvector[j].Outvector[i] = 0;
            }
        }
    }
}

void UndefinedVariableChecker::init_param(int paramnum)
{
    for (int i = 0; i < paramnum; i++)
    {
        for (int j = 0; j < this->blockNum; j++)
        {
            blockbitvector[j].Invector[i] = 0;
            blockbitvector[j].Outvector[i] = 0;
        }
    }
}

void UndefinedVariableChecker::dump_func(std::vector<Child> childlist)
{
    for (int i = 0; i < childlist.size(); i++)
    {
        std::cout << childlist[i].funcname << " " << childlist[i].param_num << " ";
        for (int j = 0; j < childlist[i].parameter_init.size(); j++)
        {
            std::cout << childlist[i].parameter_init[j];
        }
        std::cout << std::endl;
    }
}

void UndefinedVariableChecker::is_other_function(clang::FunctionDecl *func)
{
    int paramNum = func->getNumParams();
    definitionNum = definitionNum + paramNum;
    for (int i = 0; i < paramNum; i++)
    {
        clang::ParmVarDecl *paramvar = func->getParamDecl(i);
        pair<string, Initvalue> variable;
        Initvalue initvalue;
        initvalue.initkind = NotInitialize;
        variable.second = initvalue;
        variable.first = paramvar->getNameAsString();
        var_vector.push_back(variable);
    }
}

string UndefinedVariableChecker::analyze_array(clang::ArraySubscriptExpr *arrayexpr)
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

string UndefinedVariableChecker::analyze_struct(clang::MemberExpr *structexpr)
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

string UndefinedVariableChecker::get_statement_value(clang::Stmt *statement)
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
        std::cout << "now\n";
        clang::ImplicitCastExpr *impliexpr = static_cast<clang::ImplicitCastExpr *>(statement);
        return get_statement_value(impliexpr->getSubExpr());
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::ParenExprClass)
    {
        clang::ParenExpr *parenexpr = static_cast<clang::ParenExpr *>(statement);
        return get_statement_value(parenexpr->getSubExpr());
    }
    return ret;
}

void UndefinedVariableChecker::recursive_get_binaryop(clang::Stmt *statement, UsefulStatementInfo *info)
{
    if (statement == nullptr)
    {
        return;
    }
    if (statement->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass)
    {
        clang::BinaryOperator *binaryIter = static_cast<clang::BinaryOperator *>(statement);
        recursive_get_binaryop(binaryIter->getLHS(), info);
        recursive_get_binaryop(binaryIter->getRHS(), info);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
    {
        if (info->rvalueKind == 0 || info->rvalueKind == Literal)
            info->rvalueKind = Ref;
        clang::DeclRefExpr *declrefexp = static_cast<clang::DeclRefExpr *>(statement);
        Variable variable;
        variable.variable = declrefexp->getNameInfo().getAsString();
        variable.loc = declrefexp->getLocation();
        info->rvalueString.push_back(variable);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
    {
        clang::ImplicitCastExpr *implicitit = static_cast<clang::ImplicitCastExpr *>(statement);
        if (implicitit->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
        {
            if (info->rvalueKind == 0 || info->rvalueKind == Literal)
                info->rvalueKind = Ref;
            clang::DeclRefExpr *declrefexp = static_cast<clang::DeclRefExpr *>(implicitit->getSubExpr());
            Variable variable;
            variable.variable = declrefexp->getNameInfo().getAsString();
            variable.loc = declrefexp->getLocation();
            info->rvalueString.push_back(variable);
        }
        else if (implicitit->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::ParenExprClass)
        {
            clang::ParenExpr *parenexp = static_cast<clang::ParenExpr *>(implicitit->getSubExpr());
            recursive_get_binaryop(parenexp->getSubExpr(), info);
        }
        else if (implicitit->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::ArraySubscriptExprClass)
        {
            if (info->rvalueKind == 0 || info->rvalueKind == Literal)
                info->rvalueKind = Ref;
            clang::ArraySubscriptExpr *arrayexpr = static_cast<clang::ArraySubscriptExpr *>(implicitit->getSubExpr());
            /*string arrayname = analyze_array(arrayexpr);
            Variable variable;
            variable.variable = arrayname;
            variable.loc = arrayexpr->getBeginLoc();
            info->rvalueString.push_back(variable);*/
            recursive_get_binaryop(arrayexpr->getLHS(), info);
            recursive_get_binaryop(arrayexpr->getRHS(), info);
        }
        else
        {
            recursive_get_binaryop(implicitit->getSubExpr(), info);
        }
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass)
    {
        if (info->rvalueKind == 0)
            info->rvalueKind = Literal;
        clang::IntegerLiteral *integerit = static_cast<clang::IntegerLiteral *>(statement);
        info->rvalueLiteral = info->rvalueLiteral + integerit->getValue().getZExtValue();
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::FloatingLiteralClass)
    {
        if (info->rvalueKind == 0)
            info->rvalueKind = Literal;
        clang::FloatingLiteral *floatingit = static_cast<clang::FloatingLiteral *>(statement);
        info->rvalueLiteral = info->rvalueLiteral + floatingit->getValue().convertToDouble();
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::CharacterLiteralClass)
    {
        if (info->rvalueKind == 0)
            info->rvalueKind = Literal;
        clang::CharacterLiteral *characterit = static_cast<clang::CharacterLiteral *>(statement);
        info->rvalueLiteral = info->rvalueLiteral + characterit->getValue();
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::ParenExprClass)
    {
        clang::ParenExpr *parenexp = static_cast<clang::ParenExpr *>(statement);
        recursive_get_binaryop(parenexp->getSubExpr(), info);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::UnaryOperatorClass)
    {
        clang::UnaryOperator *unaryIter = static_cast<clang::UnaryOperator *>(statement);
        if (unaryIter->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
        {
            clang::ImplicitCastExpr *implicitit = static_cast<clang::ImplicitCastExpr *>(unaryIter->getSubExpr());
            if (implicitit->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
            {
                if (info->rvalueKind == 0 || info->rvalueKind == Literal)
                    info->rvalueKind = Ref;
                clang::DeclRefExpr *declrefiter = static_cast<clang::DeclRefExpr *>(implicitit->getSubExpr());
                Variable variable;
                string variableName = declrefiter->getNameInfo().getAsString();
                variable.variable = variableName;
                variable.loc = declrefiter->getLocation();
                info->rvalueString.push_back(variable);
            }
            else if (implicitit->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::ParenExprClass)
            {
                clang::ParenExpr *parenexp = static_cast<clang::ParenExpr *>(implicitit->getSubExpr());
                recursive_get_binaryop(parenexp->getSubExpr(), info);
            }
            else
            {
                recursive_get_binaryop(implicitit->getSubExpr(), info);
            }
        }
        else if (unaryIter->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::ParenExprClass)
        {
            clang::ParenExpr *parenexp = static_cast<clang::ParenExpr *>(unaryIter->getSubExpr());
            recursive_get_binaryop(parenexp->getSubExpr(), info);
        }
        else
        {
            recursive_get_binaryop(unaryIter->getSubExpr(), info);
        }
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::ArraySubscriptExprClass)
    {
        if (info->rvalueKind == 0 || info->rvalueKind == Literal)
            info->rvalueKind = Ref;
        clang::ArraySubscriptExpr *arrayexpr = static_cast<clang::ArraySubscriptExpr *>(statement);
        /*string arrayname = analyze_array(arrayexpr);
        Variable variable;
        variable.variable = arrayname;
        variable.loc = arrayexpr->getBeginLoc();
        info->rvalueString.push_back(variable);*/
        recursive_get_binaryop(arrayexpr->getLHS(), info);
        recursive_get_binaryop(arrayexpr->getRHS(), info);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::CallExprClass)
    {
        clang::CallExpr *callIter = static_cast<clang::CallExpr *>(statement);
        int argNum = callIter->getNumArgs();
        for (int k = 0; k < argNum; k++)
        {
            recursive_get_binaryop(callIter->getArg(k), info);
        }
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::MemberExprClass)
    {
        clang::MemberExpr *structIter = static_cast<clang::MemberExpr *>(statement);
        /*string structname = analyze_struct(structIter);
        if(info->rvalueKind == 0 || info->rvalueKind == Literal)
            info->rvalueKind = Ref;
        Variable variable;
        variable.variable = structname;
        variable.loc = structIter->getBeginLoc();
        info->rvalueString.push_back(variable);*/
        recursive_get_binaryop(structIter->getBase(), info);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::CStyleCastExprClass)
    {
        clang::CStyleCastExpr *cstyleIter = static_cast<clang::CStyleCastExpr *>(statement);
        recursive_get_binaryop(cstyleIter->getSubExpr(), info);
    }
}

void UndefinedVariableChecker::get_rvalue(clang::Stmt *statement, UsefulStatementInfo *info)
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
                        clang::IntegerLiteral *integerit = static_cast<clang::IntegerLiteral *>(initexpr);
                        info->rvalueKind = Literal;
                        info->rvalueLiteral = integerit->getValue().getZExtValue() + info->rvalueLiteral;
                    }
                    else
                    {
                        if (initexpr->getStmtClass() == clang::Stmt::StmtClass::InitListExprClass)
                        {
                            clang::InitListExpr *initit = static_cast<clang::InitListExpr *>(initexpr);
                            // std::cout << "-------------******-------------------\n";
                            int num = initit->getNumInits();
                            for (int i = 0; i < num; i++)
                            {
                                recursive_get_binaryop(initit->getInit(i), info);
                            }
                        }
                        else if (initexpr->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
                        {
                            clang::ImplicitCastExpr *implicitit = static_cast<clang::ImplicitCastExpr *>(initexpr);
                            Variable variable;
                            if (implicitit->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
                            {
                                clang::DeclRefExpr *declrefexp = static_cast<clang::DeclRefExpr *>(implicitit->getSubExpr());
                                variable.variable = declrefexp->getNameInfo().getAsString();
                                variable.loc = declrefexp->getLocation();
                                info->rvalueKind = Ref;
                                info->rvalueString.push_back(variable);
                            }
                            else if (implicitit->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::ArraySubscriptExprClass)
                            {
                                clang::ArraySubscriptExpr *arrayexpr = static_cast<clang::ArraySubscriptExpr *>(implicitit->getSubExpr());
                                /*variable.variable = analyze_array(arrayexpr);
                                variable.loc = arrayexpr->getBeginLoc();
                                info->rvalueKind = Ref;
                                info->rvalueString.push_back(variable);*/
                                recursive_get_binaryop(arrayexpr->getLHS(), info);
                                recursive_get_binaryop(arrayexpr->getRHS(), info);
                            }
                            else if (implicitit->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::UnaryOperatorClass)
                            {
                                clang::UnaryOperator *unaryexp = static_cast<clang::UnaryOperator *>(implicitit->getSubExpr());
                                recursive_get_binaryop(implicitit->getSubExpr(), info);
                            }
                            else if (implicitit->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass)
                            {
                                clang::BinaryOperator *binaryexp = static_cast<clang::BinaryOperator *>(implicitit->getSubExpr());
                                recursive_get_binaryop(implicitit->getSubExpr(), info);
                            }
                            else if (implicitit->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass)
                            {
                                clang::IntegerLiteral *integerit = static_cast<clang::IntegerLiteral *>(implicitit->getSubExpr());
                                info->rvalueKind = Literal;
                                info->rvalueLiteral = integerit->getValue().getZExtValue();
                            }
                            else if (implicitit->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::FloatingLiteralClass)
                            {
                                clang::FloatingLiteral *integerit = static_cast<clang::FloatingLiteral *>(implicitit->getSubExpr());
                                // std::cout << integerit->getValue().getSizeInBits();
                                info->rvalueKind = Literal;
                                info->rvalueLiteral = integerit->getValue().convertToDouble();
                            }
                            else if (implicitit->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::CharacterLiteralClass)
                            {
                                clang::CharacterLiteral *charit = static_cast<clang::CharacterLiteral *>(implicitit->getSubExpr());
                                info->rvalueKind = Literal;
                                info->rvalueLiteral = charit->getValue();
                            }
                            else
                            {
                                recursive_get_binaryop(implicitit->getSubExpr(), info);
                            }

                        } // int x = y || int x = !y || int z = 0.5
                        else if (initexpr->getStmtClass() == clang::Stmt::StmtClass::UnaryOperatorClass)
                        {
                            recursive_get_binaryop(initexpr, info);
                        }
                        else if (initexpr->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass)
                        {
                            recursive_get_binaryop(initexpr, info);
                        }
                        else if (initexpr->getStmtClass() == clang::Stmt::StmtClass::CallExprClass)
                        {
                            recursive_get_binaryop(initexpr, info);
                        }
                        else if (initexpr->getStmtClass() == clang::Stmt::StmtClass::CXXConstructExprClass)
                        {
                            clang::CXXConstructExpr *structIter = static_cast<clang::CXXConstructExpr *>(initexpr);
                            // structIter->getBestDynamicClassTypeExpr()->dump();
                            auto iter = structIter->child_begin();
                            for (; iter != structIter->child_end(); iter++)
                            {
                                assert((*iter) != NULL);
                                /*if((*iter)->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass){
                                    clang::ImplicitCastExpr* implicitit = static_cast<clang::ImplicitCastExpr*>((*iter));
                                    assert(implicitit->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass);
                                    clang::DeclRefExpr* declrefIter = static_cast<clang::DeclRefExpr*>(implicitit->getSubExpr());
                                    info->rvalueKind = Ref;
                                    Variable variable;
                                    variable.variable = declrefIter->getNameInfo().getAsString();
                                    variable.loc = declrefIter->getLocation();
                                    info->rvalueString.push_back(variable);
                                }*/
                                recursive_get_binaryop((*iter), info);
                            }
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
            recursive_get_binaryop(arrayIter, info);
            if (info->rvalueKind == Ref)
            {
                for (auto iter = info->rvalueString.begin(); iter != info->rvalueString.end(); iter++)
                {
                    if (arrayname == iter->variable)
                    {
                        info->rvalueString.erase(iter);
                        break;
                    }
                }
            }
            if (info->rvalueString.size() == 0)
            {
                info->rvalueKind = 0;
                info->rvalueLiteral = 0;
            }
        }
        else if (binaryIter->getLHS()->getStmtClass() == clang::Stmt::StmtClass::MemberExprClass)
        {
            clang::MemberExpr *memberIter = static_cast<clang::MemberExpr *>(binaryIter->getLHS());
            string structname = analyze_struct(memberIter);
            recursive_get_binaryop(memberIter, info);
            if (info->rvalueKind == Ref)
            {
                for (auto iter = info->rvalueString.begin(); iter != info->rvalueString.end(); iter++)
                {
                    if (structname == iter->variable)
                    {
                        info->rvalueString.erase(iter);
                        break;
                    }
                }
            }
            if (info->rvalueString.size() == 0)
            {
                info->rvalueKind = 0;
                info->rvalueLiteral = 0;
            }
        }
        recursive_get_binaryop(binaryIter->getRHS(), info);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::CompoundAssignOperatorClass)
    {
        clang::CompoundAssignOperator *compoundIter = static_cast<clang::CompoundAssignOperator *>(statement);
        if (compoundIter->getLHS()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
        {
            clang::DeclRefExpr *declrefiter = static_cast<clang::DeclRefExpr *>(compoundIter->getLHS());
            info->rvalueKind = Ref;
            string variableName = declrefiter->getNameInfo().getAsString();
            Variable variable;
            variable.variable = variableName;
            variable.loc = declrefiter->getLocation();
            info->rvalueString.push_back(variable);
            if (compoundIter->getRHS()->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
            {
                clang::ImplicitCastExpr *implicitit = static_cast<clang::ImplicitCastExpr *>(compoundIter->getRHS());
                Variable tempvariable;
                if (implicitit->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
                {
                    clang::DeclRefExpr *declrefexp = static_cast<clang::DeclRefExpr *>(implicitit->getSubExpr());
                    tempvariable.variable = declrefexp->getNameInfo().getAsString();
                    tempvariable.loc = declrefexp->getLocation();
                    info->rvalueString.push_back(tempvariable);
                }
                else if (implicitit->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::ArraySubscriptExprClass)
                {
                    clang::ArraySubscriptExpr *arrayexpr = static_cast<clang::ArraySubscriptExpr *>(implicitit->getSubExpr());
                    /*string arrayname = analyze_array(arrayexpr);
                    tempvariable.variable = arrayname;
                    tempvariable.loc = arrayexpr->getBeginLoc();
                    info->rvalueString.push_back(tempvariable);*/
                    recursive_get_binaryop(arrayexpr->getLHS(), info);
                    recursive_get_binaryop(arrayexpr->getRHS(), info);
                }
            }
            else
            {
                recursive_get_binaryop(compoundIter->getRHS(), info);
            }
        }
        else if (compoundIter->getLHS()->getStmtClass() == clang::Stmt::StmtClass::ArraySubscriptExprClass)
        {
            clang::ArraySubscriptExpr *newarrayexpr = static_cast<clang::ArraySubscriptExpr *>(compoundIter->getLHS());
            /*string temparray = analyze_array(newarrayexpr);
            Variable variable;
            variable.variable = temparray;
            variable.loc = newarrayexpr->getBeginLoc();
            info->rvalueKind = Ref;
            info->rvalueString.push_back(variable);*/
            recursive_get_binaryop(newarrayexpr->getLHS(), info);
            recursive_get_binaryop(newarrayexpr->getRHS(), info);
            recursive_get_binaryop(compoundIter->getRHS(), info);
            /*if(compoundIter->getRHS()->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass){
                clang::ImplicitCastExpr* implicitit = static_cast<clang::ImplicitCastExpr*>(compoundIter->getRHS());
                Variable tempvariable;
                if(implicitit->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass){
                    clang::DeclRefExpr* declrefexp = static_cast<clang::DeclRefExpr*>(implicitit->getSubExpr());
                    tempvariable.variable = declrefexp->getNameInfo().getAsString();
                    tempvariable.loc = declrefexp->getLocation();
                    info->rvalueString.push_back(tempvariable);
                }
                else if(implicitit->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::ArraySubscriptExprClass){
                    clang::ArraySubscriptExpr* arrayexpr = static_cast<clang::ArraySubscriptExpr*>(implicitit->getSubExpr());
                    string arrayname = analyze_array(arrayexpr);
                    tempvariable.variable = arrayname;
                    tempvariable.loc = arrayexpr->getBeginLoc();
                    info->rvalueString.push_back(tempvariable);
                    recursive_get_binaryop(arrayexpr->getRHS(),info);
                }
                else{
                    recursive_get_binaryop(implicitit->getSubExpr(),info);
                }
            }*/
        }
        else
        {
            recursive_get_binaryop(compoundIter->getLHS(), info);
            recursive_get_binaryop(compoundIter->getRHS(), info);
        }
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::UnaryOperatorClass)
    {
        clang::UnaryOperator *unaryIter = static_cast<clang::UnaryOperator *>(statement);
        if (unaryIter->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
        {
            clang::DeclRefExpr *declrefiter = static_cast<clang::DeclRefExpr *>(unaryIter->getSubExpr());
            info->rvalueKind = Ref;
            string variableName = declrefiter->getNameInfo().getAsString();
            Variable variable;
            variable.variable = variableName;
            variable.loc = declrefiter->getLocation();
            info->rvalueString.push_back(variable);
        }
        else if (unaryIter->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::ArraySubscriptExprClass)
        {
            clang::ArraySubscriptExpr *arrayiter = static_cast<clang::ArraySubscriptExpr *>(unaryIter->getSubExpr());
            info->rvalueKind = Ref;
            /*string variableName = analyze_array(arrayiter);
            Variable variable;
            variable.variable = variableName;
            variable.loc = arrayiter->getBeginLoc();
            info->rvalueString.push_back(variable);*/
            recursive_get_binaryop(arrayiter->getLHS(), info);
            recursive_get_binaryop(arrayiter->getRHS(), info);
        }
        else
        {
            recursive_get_binaryop(unaryIter->getSubExpr(), info);
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
        return recursive_get_binaryop((*iter), info);
    }
}

vector<int> UndefinedVariableChecker::kill_variable(int statementno, string variable)
{
    vector<int> kill;
    for (int i = 0; i < allusefulstatement.size(); i++)
    {
        if (statementno != i)
        {
            if (variable == allusefulstatement[i].variable)
            {
                kill.push_back(i);
            }
        }
    }
    return kill;
}

void UndefinedVariableChecker::count_definition(unique_ptr<CFG> &cfg)
{
    clang::CFG::iterator blockIter;
    for (blockIter = cfg->begin(); blockIter != cfg->end(); blockIter++)
    {
        this->blockNum++;
        CFGBlock *block = *blockIter;
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
                    if (statement == nullptr)
                    {
                        std::cout << "\033[31m"
                                  << "error here"
                                  << "\033[0m" << std::endl;
                    }
                    else
                    {
                        if (statement != nullptr && statement->getStmtClass() == clang::Stmt::StmtClass::DeclStmtClass)
                        {
                            clang::DeclStmt *declstmt = static_cast<clang::DeclStmt *>(statement);
                            if (declstmt->isSingleDecl())
                            {
                                clang::Decl *oneDecl = declstmt->getSingleDecl();
                                if (oneDecl->getKind() == clang::Decl::Kind::Var)
                                {
                                    clang::VarDecl *vardecl = static_cast<clang::VarDecl *>(oneDecl);
                                    pair<string, Initvalue> variable;
                                    string name = vardecl->getNameAsString();
                                    definitionNum++;
                                    variable.first = name;
                                    if (vardecl->getInit() != nullptr)
                                    {
                                        Initvalue initvalue;
                                        clang::Expr *initexpr = vardecl->getInit();
                                        if (initexpr->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass)
                                        {
                                            initvalue.initkind = IsInitialize;
                                            clang::IntegerLiteral *integerit = static_cast<clang::IntegerLiteral *>(initexpr);
                                            // std::cout << integerit->getValue().getZExtValue() << std::endl;
                                            initvalue.value = integerit->getValue().getZExtValue();
                                        }
                                        else
                                        {
                                            if (initexpr->getStmtClass() == clang::Stmt::StmtClass::InitListExprClass)
                                            {
                                                initvalue.initkind = IsInitialize;
                                                // array struct?
                                            }
                                            else if (initexpr->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
                                            {
                                                // std::cout<<"there\n";
                                                initvalue.initkind = Reference;
                                                clang::ImplicitCastExpr *implicitit = static_cast<clang::ImplicitCastExpr *>(initexpr);
                                                if (implicitit->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
                                                {
                                                    clang::DeclRefExpr *declrefexp = static_cast<clang::DeclRefExpr *>(implicitit->getSubExpr());
                                                    // declrefexp->dump();
                                                    initvalue.ref = declrefexp->getNameInfo().getAsString();
                                                }
                                                else if (implicitit->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::ArraySubscriptExprClass)
                                                {
                                                    clang::ArraySubscriptExpr *arrayexpr = static_cast<clang::ArraySubscriptExpr *>(implicitit->getSubExpr());
                                                    string variablename = analyze_array(arrayexpr);
                                                    initvalue.ref = variablename;
                                                }
                                            }
                                            // std::cout<<name<<std::endl;
                                        }
                                        variable.second = initvalue;
                                        var_vector.push_back(variable);
                                    }
                                    else
                                    {
                                        // std::cout<< "no initialize"<<std::endl;
                                        Initvalue initvalue;
                                        initvalue.initkind = NotInitialize;
                                        variable.second = initvalue;
                                        var_vector.push_back(variable);
                                    }
                                }
                            }
                            else
                            {
                                clang::DeclGroupRef multDecl = declstmt->getDeclGroup();
                                clang::DeclGroupRef::iterator itgr = multDecl.begin();
                                for (; itgr != multDecl.end(); itgr++)
                                {
                                    std::cout << (*itgr)->getDeclKindName() << std::endl;
                                    if ((*itgr)->getKind() == clang::Decl::Kind::Var)
                                    {
                                        clang::VarDecl *vardecl = static_cast<clang::VarDecl *>((*itgr));
                                        pair<string, Initvalue> variable;
                                        string name = vardecl->getNameAsString();
                                        definitionNum++;
                                        variable.first = name;
                                        if (vardecl->getInit() != nullptr)
                                        {
                                            Initvalue initvalue;
                                            clang::Expr *initexpr = vardecl->getInit();
                                            if (initexpr->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass)
                                            {
                                                initvalue.initkind = IsInitialize;
                                                clang::IntegerLiteral *integerit = static_cast<clang::IntegerLiteral *>(initexpr);
                                                // std::cout << integerit->getValue().getZExtValue() << std::endl;
                                                initvalue.value = integerit->getValue().getZExtValue();
                                            }
                                            else
                                            {
                                                if (initexpr->getStmtClass() == clang::Stmt::StmtClass::InitListExprClass)
                                                {
                                                    initvalue.initkind = IsInitialize;
                                                    // array struct?
                                                }
                                                else if (initexpr->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
                                                {
                                                    initvalue.initkind = Reference;
                                                    clang::ImplicitCastExpr *implicitit = static_cast<clang::ImplicitCastExpr *>(initexpr);
                                                    clang::DeclRefExpr *declrefexp = static_cast<clang::DeclRefExpr *>(implicitit->getSubExpr());
                                                    // declrefexp->dump();
                                                    initvalue.ref = declrefexp->getNameInfo().getAsString();
                                                }
                                                else
                                                {
                                                    initvalue.initkind = Reference;
                                                }
                                            }
                                            variable.second = initvalue;
                                            var_vector.push_back(variable);
                                        }
                                        else
                                        {
                                            // std::cout<< "no initialize"<<std::endl;
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
            else if (element.getKind() == clang::CFGElement::Kind::Constructor)
            {
                // may have no use
                std::cout << "Check here.\n"
                          << std::endl;
            }
        }
    }
}

void UndefinedVariableChecker::readConfig()
{
    std::unordered_map<std::string, std::string> ptrConfig =
        configure->getOptionBlock("UndefinedVariableChecker");
    request_fun = stoi(ptrConfig.find("request_fun")->second);
    maxPathInFun = 10;
}

void UndefinedVariableChecker::getEntryFunc()
{
    std::vector<ASTFunction *> topLevelFuncs = call_graph->getTopLevelFunctions();
    allFunctions = call_graph->getAllFunctions();
    for (auto fun : topLevelFuncs)
    {
        const FunctionDecl *funDecl = manager->getFunctionDecl(fun);
        if (funDecl->getQualifiedNameAsString() == "main")
        {
            entryFunc = fun;
            return;
        }
    }
    entryFunc = nullptr;
    return;
}

void UndefinedVariableChecker::blockvector_output()
{
    for (int i = 0; i < this->blockNum; i++)
    {
        std::cout << "\033[31m"
                  << "Block Number: " << blockbitvector[i].blockid;
        std::cout << "   Out vector: "
                  << "\033[0m";
        for (int j = 0; j < bitVectorLength; j++)
        {
            std::cout << blockbitvector[i].Outvector[j];
        }
        std::cout << std::endl;
    }
}

void UndefinedVariableChecker::dump_debug()
{
    assert(definitionNum == var_vector.size());
    std::cout << "Definition Number: " << definitionNum << std::endl;
    std::cout << "Block Number: " << this->blockNum << std::endl;
    for (int i = 0; i < var_vector.size(); i++)
    {
        std::cout << "Variable Name:" << var_vector[i].first;
        if (var_vector[i].second.initkind == IsInitialize)
        {
            std::cout << "     Is initialized " << var_vector[i].second.value << std::endl;
        }
        else if (var_vector[i].second.initkind == Reference)
        {
            std::cout << "     Is referenced " << var_vector[i].second.ref << std::endl;
        }
        else
        {
            std::cout << "     Not initialized" << std::endl;
        }
    }
    /*for(int i = 0;i<block_statement.size();i++){
        std::cout << "Block" << block_statement[i].blockno << ":   "<<std::endl;
        for(int j=0;j<block_statement[i].usefulBlockStatement.size();j++){
            block_statement[i].usefulBlockStatement[j].stmt->dump();
        }
    }*/
    std::cout << "bit vector length:  " << bitVectorLength << std::endl;
    /*for(int i=0;i<blockbitvector.size();i++){
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
    }*/
    /*for(int i=0;i<this->blockNum;i++){
        std::cout << "\033[31m" << "BLOCK" << blockbitvector[i].blockid << ": gen   " << "\033[0m";
        for(int j=0;j<bitVectorLength;j++){
            std::cout<< blockbitvector[i].Genvector[j];
        }
        std::cout << "\033[31m" << "   kill:   " << "\033[0m";
        for(int j=0;j<bitVectorLength;j++){
            std::cout<< blockbitvector[i].Killvector[j];
        }
        std::cout << std::endl;
    }*/
    std::cout << "******************************" << std::endl;
    for (int i = 0; i < allusefulstatement.size(); i++)
    {
        // allusefulstatement[i].statement->getBeginLoc().dump(*SM);
        std::cout << "left: " << allusefulstatement[i].variable << " ";
        if (allusefulstatement[i].rvalueKind == Literal)
            std::cout << "right literal:" << allusefulstatement[i].rvalueLiteral << std::endl;
        else if (allusefulstatement[i].rvalueKind == Ref)
        {
            std::cout << "right ref: ";
            for (int j = 0; j < allusefulstatement[i].rvalueString.size(); j++)
            {
                std::cout << allusefulstatement[i].rvalueString[j].variable << " ";
                // allusefulstatement[i].rvalueString[j].loc.dump(*SM);
            }
            std::cout << std::endl;
        }
    }
    /*std::cout<< "******************************" <<std::endl;
    for(int i=0;i<this->blockNum;i++){
        std::cout << "Block" << i << std::endl;
        for(int j=0;j<useless_block_statement[i].usefulBlockStatement.size();j++){
            useless_block_statement[i].usefulBlockStatement[j].stmt->dump();
            for(int k=0;k<useless_block_statement[i].usefulBlockStatement[j].usedVariable.size();k++){
                std::cout << useless_block_statement[i].usefulBlockStatement[j].usedVariable[k] << " ";
            }
            std::cout << std::endl;
        }
    }*/
}

void UndefinedVariableChecker::get_bit_vector_length()
{
    bitVectorLength = definitionNum;
    for (int i = 0; i < block_statement.size(); i++)
    {
        bitVectorLength = block_statement[i].usefulBlockStatement.size() + bitVectorLength;
    }
}

void UndefinedVariableChecker::get_block_statement(unique_ptr<CFG> &cfg)
{
    int curBlockNum = 0;
    int uselessStatementNum = 0;
    int statementNum = definitionNum;
    clang::CFG::iterator blockIter;
    for (blockIter = cfg->begin(); blockIter != cfg->end(); blockIter++)
    {
        BlockInfo blockInfo;
        BlockInfo uselessBlockInfo;
        CFGBlock *block = *blockIter;
        blockInfo.blockno = block->getBlockID();
        uselessBlockInfo.blockno = block->getBlockID();
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
                    if (statement == nullptr)
                    {
                        std::cout << "\033[31m"
                                  << "error here"
                                  << "\033[0m" << std::endl;
                    }
                    else
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
                                    // std::cout << vardecl->getNameAsString() << std::endl;
                                    if (vardecl->getInit() != nullptr)
                                    {
                                        clang::Expr *initexpr = vardecl->getInit();
                                        if (initexpr->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass)
                                        {
                                            StatementInfo statementPair;
                                            statementPair.statementno = statementNum;
                                            statementNum++;
                                            statementPair.stmt = statement;
                                            statementPair.isdummy = false;
                                            blockInfo.usefulBlockStatement.push_back(statementPair);
                                        }
                                        else
                                        {
                                            if (initexpr->getStmtClass() == clang::Stmt::StmtClass::InitListExprClass)
                                            {
                                                // array struct?
                                                StatementInfo statementPair;
                                                statementPair.statementno = statementNum;
                                                statementNum++;
                                                statementPair.stmt = statement;
                                                statementPair.isdummy = false;
                                                blockInfo.usefulBlockStatement.push_back(statementPair);
                                            }
                                            else if (initexpr->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
                                            {
                                                StatementInfo statementPair;
                                                statementPair.statementno = statementNum;
                                                statementNum++;
                                                statementPair.stmt = statement;
                                                statementPair.isdummy = false;
                                                blockInfo.usefulBlockStatement.push_back(statementPair);
                                            }
                                            else
                                            {
                                                if (initexpr->getStmtClass() != clang::Stmt::StmtClass::CXXConstructExprClass)
                                                {
                                                    StatementInfo statementPair;
                                                    statementPair.statementno = statementNum;
                                                    statementNum++;
                                                    statementPair.stmt = statement;
                                                    statementPair.isdummy = false;
                                                    blockInfo.usefulBlockStatement.push_back(statementPair);
                                                }
                                                else
                                                {
                                                    clang::CXXConstructExpr *structIter = static_cast<clang::CXXConstructExpr *>(initexpr);
                                                    // structIter->getBestDynamicClassTypeExpr()->dump();
                                                    auto iter = structIter->child_begin();
                                                    for (; iter != structIter->child_end(); iter++)
                                                    {
                                                        StatementInfo statementPair;
                                                        statementPair.statementno = statementNum;
                                                        statementNum++;
                                                        statementPair.stmt = statement;
                                                        statementPair.isdummy = false;
                                                        blockInfo.usefulBlockStatement.push_back(statementPair);
                                                        break;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        // std::cout << "definition doesn't have initialize = operator\n";
                                    }
                                }
                            }
                            else
                            {
                                clang::DeclGroupRef multDecl = declstmt->getDeclGroup();
                                clang::DeclGroupRef::iterator itgr = multDecl.begin();
                                for (; itgr != multDecl.end(); itgr++)
                                {
                                    if ((*itgr)->getKind() == clang::Decl::Kind::Var)
                                    {
                                        clang::VarDecl *vardecl = static_cast<clang::VarDecl *>((*itgr));
                                        if (vardecl->getInit() != nullptr)
                                        {
                                            clang::Expr *initexpr = vardecl->getInit();
                                            if (initexpr->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass)
                                            {
                                                StatementInfo statementPair;
                                                statementPair.statementno = statementNum;
                                                statementNum++;
                                                statementPair.stmt = statement;
                                                statementPair.isdummy = false;
                                                blockInfo.usefulBlockStatement.push_back(statementPair);
                                            }
                                            else
                                            {
                                                if (initexpr->getStmtClass() == clang::Stmt::StmtClass::InitListExprClass)
                                                {
                                                    // array struct?
                                                    StatementInfo statementPair;
                                                    statementPair.statementno = statementNum;
                                                    statementNum++;
                                                    statementPair.stmt = statement;
                                                    statementPair.isdummy = false;
                                                    blockInfo.usefulBlockStatement.push_back(statementPair);
                                                }
                                                else if (initexpr->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
                                                {
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
                        else if (statement->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass)
                        {
                            clang::BinaryOperator *binaryIter = static_cast<clang::BinaryOperator *>(statement);
                            if (binaryIter->isAssignmentOp())
                            {
                                StatementInfo statementPair;
                                statementPair.statementno = statementNum;
                                statementNum++;
                                statementPair.stmt = statement;
                                statementPair.isdummy = false;
                                blockInfo.usefulBlockStatement.push_back(statementPair);
                            } // x = 1
                            else if (binaryIter->isCompoundAssignmentOp())
                            {
                                StatementInfo statementPair;
                                statementPair.statementno = statementNum;
                                statementNum++;
                                statementPair.stmt = statement;
                                statementPair.isdummy = false;
                                blockInfo.usefulBlockStatement.push_back(statementPair);
                            } // ?
                            else
                            {
                                StatementInfo statementPair;
                                statementPair.statementno = uselessStatementNum;
                                uselessStatementNum++;
                                statementPair.stmt = statement;
                                statementPair.isdummy = false;
                                uselessBlockInfo.usefulBlockStatement.push_back(statementPair);
                            }
                        }
                        else if (statement->getStmtClass() == clang::Stmt::StmtClass::CompoundAssignOperatorClass)
                        {
                            StatementInfo statementPair;
                            statementPair.statementno = statementNum;
                            statementNum++;
                            statementPair.stmt = statement;
                            statementPair.isdummy = false;
                            blockInfo.usefulBlockStatement.push_back(statementPair);
                        } // x += 1
                        else if (statement->getStmtClass() == clang::Stmt::StmtClass::UnaryOperatorClass)
                        {
                            clang::UnaryOperator *unaryIter = static_cast<clang::UnaryOperator *>(statement);
                            if (unaryIter->isIncrementDecrementOp())
                            {
                                StatementInfo statementPair;
                                statementPair.statementno = statementNum;
                                statementNum++;
                                statementPair.stmt = statement;
                                statementPair.isdummy = false;
                                blockInfo.usefulBlockStatement.push_back(statementPair);
                            }
                            else if (unaryIter->isArithmeticOp())
                            {
                                StatementInfo statementPair;
                                statementPair.statementno = uselessStatementNum;
                                uselessStatementNum++;
                                statementPair.stmt = statement;
                                statementPair.isdummy = false;
                                uselessBlockInfo.usefulBlockStatement.push_back(statementPair);
                            }
                        } // x ++ or x-- or ++x or --x
                        else if (statement->getStmtClass() == clang::Stmt::StmtClass::CXXOperatorCallExprClass)
                        {
                            clang::CXXOperatorCallExpr *cxxopIter = static_cast<clang::CXXOperatorCallExpr *>(statement);
                            auto iter = cxxopIter->child_begin();
                            if ((*iter) != NULL)
                            {
                                clang::ImplicitCastExpr *impliIter = static_cast<clang::ImplicitCastExpr *>((*iter));
                                assert(impliIter->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass);
                                clang::DeclRefExpr *declIter = static_cast<clang::DeclRefExpr *>(impliIter->getSubExpr());
                                if (declIter->getNameInfo().getAsString() == "operator=")
                                {
                                    StatementInfo statementPair;
                                    statementPair.statementno = statementNum;
                                    statementNum++;
                                    statementPair.stmt = statement;
                                    statementPair.isdummy = false;
                                    blockInfo.usefulBlockStatement.push_back(statementPair);
                                }
                            }
                        }
                        else
                        {
                            // assert(statement->getStmtClass() == clang::Stmt::StmtClass::CXXConstructExprClass || statement->getStmtClass() == clang::Stmt::StmtClass::CallExprClass || statement->getStmtClass() == clang::Stmt::StmtClass::ReturnStmtClass || statement->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass);
                            // yinshi zhuanhuan
                            if (statement->getStmtClass() != clang::Stmt::StmtClass::CXXConstructExprClass)
                            {
                                StatementInfo statementPair;
                                statementPair.statementno = uselessStatementNum;
                                uselessStatementNum++;
                                statementPair.stmt = statement;
                                statementPair.isdummy = false;
                                uselessBlockInfo.usefulBlockStatement.push_back(statementPair);
                            }
                        } // call function
                        // statement->dump();
                        // std::cout <<"statement" << statementNum << std::endl;
                    }
                }
            }
            else if (element.getKind() == clang::CFGElement::Kind::Constructor)
            {
                // may have no use
                std::cout << "Check here.\n"
                          << std::endl;
            }
        }
        block_statement.push_back(blockInfo);
        useless_block_statement.push_back(uselessBlockInfo);
        curBlockNum++;
    }
    // std::cout << curBlockNum <<" " << this->blockNum << endl;
    assert(curBlockNum == this->blockNum);

    // std::cout<< statementNum<< std::endl;
    get_bit_vector_length();
}

void UndefinedVariableChecker::init_blockvector(unique_ptr<CFG> &cfg)
{
    clang::CFG::iterator blockIter;
    for (blockIter = cfg->begin(); blockIter != cfg->end(); blockIter++)
    {
        CFGBlock *block = *blockIter;
        BlockBitVector blockelement;
        blockelement.blockid = block->getBlockID();
        if (block->succ_empty() == false)
        {
            // int t = 0;
            /*if(block->getBlockID() == 6)
                std::cout << block->succ_size() <<endl;*/
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
        for (int i = 0; i < bitVectorLength; i++)
        {
            if (i < definitionNum)
            {
                blockelement.Invector.push_back(1);
                blockelement.Outvector.push_back(1);
            }
            else
            {
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

void UndefinedVariableChecker::get_all_useful_statement()
{
    for (int i = 0; i < this->blockNum; i++)
    {
        for (int j = 0; j < block_statement[i].usefulBlockStatement.size(); j++)
        {
            string name = get_statement_value(block_statement[i].usefulBlockStatement[j].stmt);
            UsefulStatementInfo info;
            info.isdummy = false;
            info.variable = name;
            info.statement = block_statement[i].usefulBlockStatement[j].stmt;
            info.rvalueLiteral = 0;
            info.rvalueKind = 0;
            get_rvalue(block_statement[i].usefulBlockStatement[j].stmt, &info);
            allusefulstatement.push_back(info);
        }
    }
}

void UndefinedVariableChecker::recursive_find_usevariable(clang::Stmt *statement, StatementInfo *info)
{
    if (statement == nullptr)
    {
        return;
    }
    if (statement->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
    {
        clang::DeclRefExpr *declrefexp = static_cast<clang::DeclRefExpr *>(statement);
        Variable variable;
        variable.variable = declrefexp->getNameInfo().getAsString();
        variable.loc = declrefexp->getLocation();
        info->usedVariable.push_back(variable);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::ArraySubscriptExprClass)
    {
        clang::ArraySubscriptExpr *arrayexpr = static_cast<clang::ArraySubscriptExpr *>(statement);
        /*string arrayname = analyze_array(arrayexpr);
        Variable variable;
        variable.variable = arrayname;
        variable.loc = arrayexpr->getBeginLoc();
        info->usedVariable.push_back(variable);*/
        recursive_find_usevariable(arrayexpr->getLHS(), info);
        recursive_find_usevariable(arrayexpr->getRHS(), info);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
    {
        clang::ImplicitCastExpr *implicitIter = static_cast<clang::ImplicitCastExpr *>(statement);
        recursive_find_usevariable(implicitIter->getSubExpr(), info);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::ParenExprClass)
    {
        clang::ParenExpr *parenexp = static_cast<clang::ParenExpr *>(statement);
        recursive_find_usevariable(parenexp->getSubExpr(), info);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass)
    {
        clang::BinaryOperator *binaryIter = static_cast<clang::BinaryOperator *>(statement);
        recursive_find_usevariable(binaryIter->getLHS(), info);
        recursive_find_usevariable(binaryIter->getRHS(), info);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::UnaryOperatorClass)
    {
        clang::UnaryOperator *unaryIter = static_cast<clang::UnaryOperator *>(statement);
        recursive_find_usevariable(unaryIter->getSubExpr(), info);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::CallExprClass)
    {
        clang::CallExpr *callIter = static_cast<clang::CallExpr *>(statement);
        int argNum = callIter->getNumArgs();
        for (int k = 0; k < argNum; k++)
        {
            recursive_find_usevariable(callIter->getArg(k), info);
        }
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::MemberExprClass)
    {
        clang::MemberExpr *memberIter = static_cast<clang::MemberExpr *>(statement);
        recursive_find_usevariable(memberIter->getBase(), info);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::CXXConstructExprClass)
    {
        clang::CXXConstructExpr *cxxIter = static_cast<clang::CXXConstructExpr *>(statement);
        auto iter = cxxIter->child_begin();
        for (; iter != cxxIter->child_end(); iter++)
        {
            assert((*iter) != NULL);
            recursive_find_usevariable((*iter), info);
        }
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::CStyleCastExprClass)
    {
        clang::CStyleCastExpr *cstyleIter = static_cast<clang::CStyleCastExpr *>(statement);
        recursive_find_usevariable(cstyleIter->getSubExpr(), info);
    }
    else
    {
        // assert(statement->getStmtClass() == clang::Stmt::StmtClass::IntegerLiteralClass);
        // actually this means coder uses char,int,double,long,etc, in a boolean expr,so we ignore it
        return;
    }
}

void UndefinedVariableChecker::get_useless_statement_variable()
{
    for (int i = 0; i < this->blockNum; i++)
    {
        for (int j = 0; j < useless_block_statement[i].usefulBlockStatement.size(); j++)
        {
            if (useless_block_statement[i].usefulBlockStatement[j].stmt->getStmtClass() == clang::Stmt::StmtClass::CallExprClass)
            {
                // std::cout << "callexpr\n";
                clang::CallExpr *callIter = static_cast<clang::CallExpr *>(useless_block_statement[i].usefulBlockStatement[j].stmt);
                int argNum = callIter->getNumArgs();
                for (int k = 0; k < argNum; k++)
                {
                    recursive_find_usevariable(callIter->getArg(k), &useless_block_statement[i].usefulBlockStatement[j]);
                }
            }
            else if (useless_block_statement[i].usefulBlockStatement[j].stmt->getStmtClass() == clang::Stmt::StmtClass::ReturnStmtClass)
            {
                // std::cout << "return\n";
                clang::ReturnStmt *returnIter = static_cast<clang::ReturnStmt *>(useless_block_statement[i].usefulBlockStatement[j].stmt);
                recursive_find_usevariable(returnIter->getRetValue(), &useless_block_statement[i].usefulBlockStatement[j]);
            }
            else if (useless_block_statement[i].usefulBlockStatement[j].stmt->getStmtClass() == clang::Stmt::StmtClass::UnaryOperatorClass)
            {
                // std::cout << "unaryop\n";
                clang::UnaryOperator *unaryIter = static_cast<clang::UnaryOperator *>(useless_block_statement[i].usefulBlockStatement[j].stmt);
                assert(unaryIter->isArithmeticOp() == true);
                if (unaryIter->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
                {
                    clang::ImplicitCastExpr *implicitit = static_cast<clang::ImplicitCastExpr *>(unaryIter->getSubExpr());
                    recursive_find_usevariable(implicitit->getSubExpr(), &useless_block_statement[i].usefulBlockStatement[j]);
                } //! x (x maybe a boolean or other kind),
                else if (unaryIter->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::ParenExprClass)
                {
                    clang::ParenExpr *parenexp = static_cast<clang::ParenExpr *>(unaryIter->getSubExpr());
                    recursive_find_usevariable(parenexp->getSubExpr(), &useless_block_statement[i].usefulBlockStatement[j]);
                }
            }
            else if (useless_block_statement[i].usefulBlockStatement[j].stmt->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass)
            {
                // std::cout << "binaryop\n";
                clang::BinaryOperator *binaryIter = static_cast<clang::BinaryOperator *>(useless_block_statement[i].usefulBlockStatement[j].stmt);
                recursive_find_usevariable(binaryIter->getLHS(), &useless_block_statement[i].usefulBlockStatement[j]);
                recursive_find_usevariable(binaryIter->getRHS(), &useless_block_statement[i].usefulBlockStatement[j]);
            }
            else if (useless_block_statement[i].usefulBlockStatement[j].stmt->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
            {
                // std::cout << "impli\n";
                clang::ImplicitCastExpr *impliIter = static_cast<clang::ImplicitCastExpr *>(useless_block_statement[i].usefulBlockStatement[j].stmt);
                recursive_find_usevariable(impliIter->getSubExpr(), &useless_block_statement[i].usefulBlockStatement[j]);
            }
            else if (useless_block_statement[i].usefulBlockStatement[j].stmt->getStmtClass() == clang::Stmt::StmtClass::CStyleCastExprClass)
            {
                // std::cout << "cstyle\n";
                clang::CStyleCastExpr *cstyleIter = static_cast<clang::CStyleCastExpr *>(useless_block_statement[i].usefulBlockStatement[j].stmt);
                recursive_find_usevariable(cstyleIter->getSubExpr(), &useless_block_statement[i].usefulBlockStatement[j]);
            }
        }
    }
}

void UndefinedVariableChecker::calculate_gen_kill()
{
    for (int i = 0; i < this->blockNum; i++)
    {
        vector<string> alreadyCalulateVariable;
        for (int j = block_statement[i].usefulBlockStatement.size() - 1; j >= 0; j--)
        {
            if (block_statement[i].usefulBlockStatement[j].stmt->getStmtClass() == clang::Stmt::StmtClass::DeclStmtClass)
            {
                clang::DeclStmt *declstmt = static_cast<clang::DeclStmt *>(block_statement[i].usefulBlockStatement[j].stmt);
                if (declstmt->isSingleDecl())
                {
                    clang::Decl *oneDecl = declstmt->getSingleDecl();
                    if (oneDecl->getKind() == clang::Decl::Kind::Var)
                    {
                        clang::VarDecl *vardecl = static_cast<clang::VarDecl *>(oneDecl);
                        // std::cout << vardecl->getNameAsString() << std::endl;
                        string variableName = vardecl->getNameAsString();
                        bool isexist = false;
                        for (int k = 0; k < alreadyCalulateVariable.size(); k++)
                        {
                            if (alreadyCalulateVariable[k] == variableName)
                            {
                                isexist = true;
                                break;
                            }
                        }
                        if (isexist == false)
                        {
                            alreadyCalulateVariable.push_back(variableName);
                            assert(i == blockbitvector[i].blockid);
                            blockbitvector[i].Genvector[block_statement[i].usefulBlockStatement[j].statementno] = 1;
                            vector<int> kill;
                            kill = kill_variable(block_statement[i].usefulBlockStatement[j].statementno - definitionNum, variableName);
                            for (int t = 0; t < kill.size(); t++)
                            {
                                blockbitvector[i].Killvector[kill[t] + definitionNum] = 1;
                            }
                        }
                    }
                }
            }
            else if (block_statement[i].usefulBlockStatement[j].stmt->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass)
            {
                clang::BinaryOperator *binaryIter = static_cast<clang::BinaryOperator *>(block_statement[i].usefulBlockStatement[j].stmt);
                if (binaryIter->getLHS()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
                {
                    clang::DeclRefExpr *declrefiter = static_cast<clang::DeclRefExpr *>(binaryIter->getLHS());
                    string variableName = declrefiter->getNameInfo().getAsString();
                    bool isexist = false;
                    for (int k = 0; k < alreadyCalulateVariable.size(); k++)
                    {
                        if (alreadyCalulateVariable[k] == variableName)
                        {
                            isexist = true;
                            break;
                        }
                    }
                    if (isexist == false)
                    {
                        alreadyCalulateVariable.push_back(variableName);
                        assert(i == blockbitvector[i].blockid);
                        blockbitvector[i].Genvector[block_statement[i].usefulBlockStatement[j].statementno] = 1;
                        vector<int> kill;
                        kill = kill_variable(block_statement[i].usefulBlockStatement[j].statementno - definitionNum, variableName);
                        for (int t = 0; t < kill.size(); t++)
                        {
                            blockbitvector[i].Killvector[kill[t] + definitionNum] = 1;
                        }
                    }
                }
                else if (binaryIter->getLHS()->getStmtClass() == clang::Stmt::StmtClass::ArraySubscriptExprClass)
                {
                    clang::ArraySubscriptExpr *arrayexpr = static_cast<clang::ArraySubscriptExpr *>(binaryIter->getLHS());
                    string variableName = analyze_array(arrayexpr);
                    bool isexist = false;
                    for (int k = 0; k < alreadyCalulateVariable.size(); k++)
                    {
                        if (alreadyCalulateVariable[k] == variableName)
                        {
                            isexist = true;
                            break;
                        }
                    }
                    if (isexist == false)
                    {
                        alreadyCalulateVariable.push_back(variableName);
                        assert(i == blockbitvector[i].blockid);
                        blockbitvector[i].Genvector[block_statement[i].usefulBlockStatement[j].statementno] = 1;
                        vector<int> kill;
                        kill = kill_variable(block_statement[i].usefulBlockStatement[j].statementno - definitionNum, variableName);
                        for (int t = 0; t < kill.size(); t++)
                        {
                            blockbitvector[i].Killvector[kill[t] + definitionNum] = 1;
                        }
                    }
                }
                else if (binaryIter->getLHS()->getStmtClass() == clang::Stmt::StmtClass::MemberExprClass)
                {
                    clang::MemberExpr *memberexpr = static_cast<clang::MemberExpr *>(binaryIter->getLHS());
                    string variableName = analyze_struct(memberexpr);
                    bool isexist = false;
                    for (int k = 0; k < alreadyCalulateVariable.size(); k++)
                    {
                        if (alreadyCalulateVariable[k] == variableName)
                        {
                            isexist = true;
                            break;
                        }
                    }
                    if (isexist == false)
                    {
                        alreadyCalulateVariable.push_back(variableName);
                        assert(i == blockbitvector[i].blockid);
                        blockbitvector[i].Genvector[block_statement[i].usefulBlockStatement[j].statementno] = 1;
                        vector<int> kill;
                        kill = kill_variable(block_statement[i].usefulBlockStatement[j].statementno - definitionNum, variableName);
                        for (int t = 0; t < kill.size(); t++)
                        {
                            blockbitvector[i].Killvector[kill[t] + definitionNum] = 1;
                        }
                    }
                }
            }
            else if (block_statement[i].usefulBlockStatement[j].stmt->getStmtClass() == clang::Stmt::StmtClass::CompoundAssignOperatorClass)
            {
                clang::CompoundAssignOperator *compoundIter = static_cast<clang::CompoundAssignOperator *>(block_statement[i].usefulBlockStatement[j].stmt);
                if (compoundIter->getLHS()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
                {
                    clang::DeclRefExpr *declrefiter = static_cast<clang::DeclRefExpr *>(compoundIter->getLHS());
                    string variableName = declrefiter->getNameInfo().getAsString();
                    bool isexist = false;
                    for (int k = 0; k < alreadyCalulateVariable.size(); k++)
                    {
                        if (alreadyCalulateVariable[k] == variableName)
                        {
                            isexist = true;
                            break;
                        }
                    }
                    if (isexist == false)
                    {
                        alreadyCalulateVariable.push_back(variableName);
                        assert(i == blockbitvector[i].blockid);
                        blockbitvector[i].Genvector[block_statement[i].usefulBlockStatement[j].statementno] = 1;
                        vector<int> kill;
                        kill = kill_variable(block_statement[i].usefulBlockStatement[j].statementno - definitionNum, variableName);
                        for (int t = 0; t < kill.size(); t++)
                        {
                            blockbitvector[i].Killvector[kill[t] + definitionNum] = 1;
                        }
                    }
                }
                else if (compoundIter->getLHS()->getStmtClass() == clang::Stmt::StmtClass::ArraySubscriptExprClass)
                {
                    clang::ArraySubscriptExpr *arrayexpr = static_cast<clang::ArraySubscriptExpr *>(compoundIter->getLHS());
                    string variableName = analyze_array(arrayexpr);
                    bool isexist = false;
                    for (int k = 0; k < alreadyCalulateVariable.size(); k++)
                    {
                        if (alreadyCalulateVariable[k] == variableName)
                        {
                            isexist = true;
                            break;
                        }
                    }
                    if (isexist == false)
                    {
                        alreadyCalulateVariable.push_back(variableName);
                        assert(i == blockbitvector[i].blockid);
                        blockbitvector[i].Genvector[block_statement[i].usefulBlockStatement[j].statementno] = 1;
                        vector<int> kill;
                        kill = kill_variable(block_statement[i].usefulBlockStatement[j].statementno - definitionNum, variableName);
                        for (int t = 0; t < kill.size(); t++)
                        {
                            blockbitvector[i].Killvector[kill[t] + definitionNum] = 1;
                        }
                    }
                }
                else if (compoundIter->getLHS()->getStmtClass() == clang::Stmt::StmtClass::MemberExprClass)
                {
                    clang::MemberExpr *memberexpr = static_cast<clang::MemberExpr *>(compoundIter->getLHS());
                    string variableName = analyze_struct(memberexpr);
                    bool isexist = false;
                    for (int k = 0; k < alreadyCalulateVariable.size(); k++)
                    {
                        if (alreadyCalulateVariable[k] == variableName)
                        {
                            isexist = true;
                            break;
                        }
                    }
                    if (isexist == false)
                    {
                        alreadyCalulateVariable.push_back(variableName);
                        assert(i == blockbitvector[i].blockid);
                        blockbitvector[i].Genvector[block_statement[i].usefulBlockStatement[j].statementno] = 1;
                        vector<int> kill;
                        kill = kill_variable(block_statement[i].usefulBlockStatement[j].statementno - definitionNum, variableName);
                        for (int t = 0; t < kill.size(); t++)
                        {
                            blockbitvector[i].Killvector[kill[t] + definitionNum] = 1;
                        }
                    }
                }
            }
            else if (block_statement[i].usefulBlockStatement[j].stmt->getStmtClass() == clang::Stmt::StmtClass::UnaryOperatorClass)
            {
                clang::UnaryOperator *unaryIter = static_cast<clang::UnaryOperator *>(block_statement[i].usefulBlockStatement[j].stmt);
                if (unaryIter->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
                {
                    clang::DeclRefExpr *declrefiter = static_cast<clang::DeclRefExpr *>(unaryIter->getSubExpr());
                    string variableName = declrefiter->getNameInfo().getAsString();
                    bool isexist = false;
                    for (int k = 0; k < alreadyCalulateVariable.size(); k++)
                    {
                        if (alreadyCalulateVariable[k] == variableName)
                        {
                            isexist = true;
                            break;
                        }
                    }
                    if (isexist == false)
                    {
                        alreadyCalulateVariable.push_back(variableName);
                        assert(i == blockbitvector[i].blockid);
                        blockbitvector[i].Genvector[block_statement[i].usefulBlockStatement[j].statementno] = 1;
                        vector<int> kill;
                        kill = kill_variable(block_statement[i].usefulBlockStatement[j].statementno - definitionNum, variableName);
                        for (int t = 0; t < kill.size(); t++)
                        {
                            blockbitvector[i].Killvector[kill[t] + definitionNum] = 1;
                        }
                    }
                }
                else if (unaryIter->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::ArraySubscriptExprClass)
                {
                    clang::ArraySubscriptExpr *arrayexpr = static_cast<clang::ArraySubscriptExpr *>(unaryIter->getSubExpr());
                    string variableName = analyze_array(arrayexpr);
                    bool isexist = false;
                    for (int k = 0; k < alreadyCalulateVariable.size(); k++)
                    {
                        if (alreadyCalulateVariable[k] == variableName)
                        {
                            isexist = true;
                            break;
                        }
                    }
                    if (isexist == false)
                    {
                        alreadyCalulateVariable.push_back(variableName);
                        assert(i == blockbitvector[i].blockid);
                        blockbitvector[i].Genvector[block_statement[i].usefulBlockStatement[j].statementno] = 1;
                        vector<int> kill;
                        kill = kill_variable(block_statement[i].usefulBlockStatement[j].statementno - definitionNum, variableName);
                        for (int t = 0; t < kill.size(); t++)
                        {
                            blockbitvector[i].Killvector[kill[t] + definitionNum] = 1;
                        }
                    }
                }
                else if (unaryIter->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::MemberExprClass)
                {
                    clang::MemberExpr *memberexpr = static_cast<clang::MemberExpr *>(unaryIter->getSubExpr());
                    string variableName = analyze_struct(memberexpr);
                    bool isexist = false;
                    for (int k = 0; k < alreadyCalulateVariable.size(); k++)
                    {
                        if (alreadyCalulateVariable[k] == variableName)
                        {
                            isexist = true;
                            break;
                        }
                    }
                    if (isexist == false)
                    {
                        alreadyCalulateVariable.push_back(variableName);
                        assert(i == blockbitvector[i].blockid);
                        blockbitvector[i].Genvector[block_statement[i].usefulBlockStatement[j].statementno] = 1;
                        vector<int> kill;
                        kill = kill_variable(block_statement[i].usefulBlockStatement[j].statementno - definitionNum, variableName);
                        for (int t = 0; t < kill.size(); t++)
                        {
                            blockbitvector[i].Killvector[kill[t] + definitionNum] = 1;
                        }
                    }
                }
            }
            else if (block_statement[i].usefulBlockStatement[j].stmt->getStmtClass() == clang::Stmt::StmtClass::CXXOperatorCallExprClass)
            {
                clang::CXXOperatorCallExpr *cxxoperatorIter = static_cast<clang::CXXOperatorCallExpr *>(block_statement[i].usefulBlockStatement[j].stmt);
                auto iter = cxxoperatorIter->child_begin();
                assert((*iter) != NULL);
                iter++;
                assert((*iter) != NULL);
                if ((*iter)->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
                {
                    clang::DeclRefExpr *declrefiter = static_cast<clang::DeclRefExpr *>((*iter));
                    string variableName = declrefiter->getNameInfo().getAsString();
                    bool isexist = false;
                    for (int k = 0; k < alreadyCalulateVariable.size(); k++)
                    {
                        if (alreadyCalulateVariable[k] == variableName)
                        {
                            isexist = true;
                            break;
                        }
                    }
                    if (isexist == false)
                    {
                        alreadyCalulateVariable.push_back(variableName);
                        assert(i == blockbitvector[i].blockid);
                        blockbitvector[i].Genvector[block_statement[i].usefulBlockStatement[j].statementno] = 1;
                        vector<int> kill;
                        kill = kill_variable(block_statement[i].usefulBlockStatement[j].statementno - definitionNum, variableName);
                        for (int t = 0; t < kill.size(); t++)
                        {
                            blockbitvector[i].Killvector[kill[t] + definitionNum] = 1;
                        }
                    }
                }
                else if ((*iter)->getStmtClass() == clang::Stmt::StmtClass::MemberExprClass)
                {
                    clang::MemberExpr *memberexpr = static_cast<clang::MemberExpr *>((*iter));
                    string variableName = analyze_struct(memberexpr);
                    bool isexist = false;
                    for (int k = 0; k < alreadyCalulateVariable.size(); k++)
                    {
                        if (alreadyCalulateVariable[k] == variableName)
                        {
                            isexist = true;
                            break;
                        }
                    }
                    if (isexist == false)
                    {
                        alreadyCalulateVariable.push_back(variableName);
                        assert(i == blockbitvector[i].blockid);
                        blockbitvector[i].Genvector[block_statement[i].usefulBlockStatement[j].statementno] = 1;
                        vector<int> kill;
                        kill = kill_variable(block_statement[i].usefulBlockStatement[j].statementno - definitionNum, variableName);
                        for (int t = 0; t < kill.size(); t++)
                        {
                            blockbitvector[i].Killvector[kill[t] + definitionNum] = 1;
                        }
                    }
                }
            }
        }
        if (alreadyCalulateVariable.empty() == false)
        {
            for (int k = 0; k < alreadyCalulateVariable.size(); k++)
            {
                for (int t = 0; t < definitionNum; t++)
                {
                    if (var_vector[t].first == alreadyCalulateVariable[k])
                    {
                        blockbitvector[i].Killvector[t] = 1;
                    }
                }
            }
        }
    }
}

void UndefinedVariableChecker::calculate_block(int num)
{
    if (blockbitvector[num].isvisit == true)
        return;
    queue<BlockBitVector> q;
    q.push(blockbitvector[num]);
    blockbitvector[num].isvisit = true;
    while (!q.empty())
    {
        BlockBitVector tempblock = q.front();
        vector<int> temp;
        for (int i = 0; i < bitVectorLength; i++)
            temp.push_back(0);
        if (tempblock.pred.empty() == false)
        {
            for (int i = 0; i < tempblock.pred.size(); i++)
            {
                for (int j = 0; j < bitVectorLength; j++)
                {
                    if (blockbitvector[tempblock.pred[i]].Outvector[j] == 1)
                        temp[j] = 1;
                }
            }
            tempblock.Invector = temp;
        }
        vector<int> res;
        for (int i = 0; i < bitVectorLength; i++)
            res.push_back(tempblock.Invector[i]);
        for (int i = 0; i < bitVectorLength; i++)
        {
            if (tempblock.Killvector[i] == 1)
                res[i] = 0;
        }
        for (int i = 0; i < bitVectorLength; i++)
        {
            if (tempblock.Genvector[i] == 1)
                res[i] = 1;
        }
        blockbitvector[tempblock.blockid].Outvector = res;
        for (int i = 0; i < tempblock.succ.size(); i++)
        {
            if (blockbitvector[tempblock.succ[i]].isvisit == false)
            {
                q.push(blockbitvector[tempblock.succ[i]]);
                blockbitvector[tempblock.succ[i]].isvisit = true;
            }
        }
        q.pop();
    }
}

void UndefinedVariableChecker::undefined_variable_check()
{
    bool ischanging = true;
    std::vector<BlockBitVector> pre_blockbitvector;
    for (int i = 0; i < this->blockNum; i++)
    {
        pre_blockbitvector.push_back(blockbitvector[i]);
    }
    int count = 0;
    while (ischanging)
    {
        calculate_block(this->blockNum - 1);
        // std::cout << count << std::endl;
        // blockvector_output();
        for (int i = 0; i < this->blockNum; i++)
        {
            if (blockbitvector[i].isvisit == false)
            {
                // std::cout << i <<std::endl;
                // std::cout<< "wrong bfs" <<std::endl;
                for (int j = 0; j < bitVectorLength; j++)
                {
                    blockbitvector[i].Invector[j] = 0;
                    blockbitvector[i].Outvector[j] = 0;
                }
            }
        }
        bool allchange = false;
        for (int i = 0; i < this->blockNum; i++)
        {
            if (pre_blockbitvector[i].Outvector != blockbitvector[i].Outvector)
            {
                allchange = true;
            }
        }
        if (allchange == true)
        {
            ischanging = true;
            for (int i = 0; i < this->blockNum; i++)
                blockbitvector[i].isvisit = false;
            for (int i = 0; i < this->blockNum; i++)
                pre_blockbitvector[i] = blockbitvector[i];
        }
        else
        {
            ischanging = false;
            for (int i = 0; i < this->blockNum; i++)
                blockbitvector[i].isvisit = false;
        }
        count++;
    }
    // std::cout<< "iteration time: " << count <<std::endl;
}

void UndefinedVariableChecker::report_warning(Variable variable)
{
    string statementinfo = variable.loc.printToString(*SM);
    int divideline1 = 0, divideline2 = 0;
    for (int i = 0; i < statementinfo.length(); i++)
    {
        if (statementinfo[i] == ':' && divideline1 == 0)
        {
            divideline1 = i;
        }
        else if (statementinfo[i] == ':' && divideline1 != 0)
        {
            divideline2 = i;
            break;
        }
    }
    string file = statementinfo.substr(0, divideline1);
    if (need_to_print_file == false)
        report_file(file);
    string line = statementinfo.substr(divideline1 + 1, divideline2 - divideline1 - 1);
    string column = statementinfo.substr(divideline2 + 1, statementinfo.length() - divideline2 - 1);
    string hashstring = line + "0" + column + "0";
    bool isexist = false;
    for (int i = 0; i < locationMap[variable.variable].size(); i++)
    {
        if (locationMap[variable.variable][i] == hashstring)
        {
            isexist = true;
            break;
        }
    }
    if (isexist == false)
    {
        locationMap[variable.variable].push_back(hashstring);
        std::cout << "WARNING:USE UNINITIALIZED VARIABLE " << variable.variable << ": "
                  << "Line: " << line << " Column: " << column << std::endl;
    }
}

void UndefinedVariableChecker::check_all_use_in_block(string name, int blockno)
{
    for (int i = 0; i < useless_block_statement[blockno].usefulBlockStatement.size(); i++)
    {
        for (int j = 0; j < useless_block_statement[blockno].usefulBlockStatement[i].usedVariable.size(); j++)
        {
            if (name == useless_block_statement[blockno].usefulBlockStatement[i].usedVariable[j].variable)
            {
                report_warning(useless_block_statement[blockno].usefulBlockStatement[i].usedVariable[j]);
            }
        }
    }
}

void UndefinedVariableChecker::check_variable_use_in_block(string name, int blockno, int statementno)
{
    bool isknown = false;
    int preStatementnum = 0;
    // if(name == "a")
    //   std::cout << blockno << " " << statementno <<std::endl;
    for (int i = 0; i < blockno; i++)
    {
        preStatementnum = preStatementnum + block_statement[i].usefulBlockStatement.size();
    }
    preStatementnum = preStatementnum + definitionNum;
    for (int i = preStatementnum - definitionNum; i <= statementno - definitionNum; i++)
    {
        if (allusefulstatement[i].variable == name)
        {
            if (allusefulstatement[i].rvalueKind == Literal)
            {
                isknown = true;
            }
            else if (allusefulstatement[i].rvalueKind == Ref)
            {
                bool hasmyself = false;
                for (int j = 0; j < allusefulstatement[i].rvalueString.size(); j++)
                {
                    if (name == allusefulstatement[i].rvalueString[j].variable)
                        hasmyself = true;
                }
                if (hasmyself == true)
                {
                    if (isknown == false)
                    {
                        // definition use myself warning should be reported
                        for (int j = 0; j < allusefulstatement[i].rvalueString.size(); j++)
                        {
                            if (name == allusefulstatement[i].rvalueString[j].variable)
                                report_warning(allusefulstatement[i].rvalueString[j]);
                        }
                    }
                    else
                    {
                        // has inited do nothing
                    }
                }
                else
                    isknown = true;
            }
            else
            {
                // std::cout << "stupid\n";
            }
        }
        else
        {
            if (allusefulstatement[i].rvalueKind == Literal)
            {
            }
            else
            {
                for (int j = 0; j < allusefulstatement[i].rvalueString.size(); j++)
                {
                    if (name == allusefulstatement[i].rvalueString[j].variable)
                    {
                        if (isknown == false)
                        {
                            // use uninited variable warning should be reported
                            report_warning(allusefulstatement[i].rvalueString[j]);
                        }
                    }
                }
            }
        }
    }
}

bool UndefinedVariableChecker::get_arg_name(clang::Stmt *statement, string name)
{
    bool ret = false;
    if (statement->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
    {
        clang::ImplicitCastExpr *impliexpr = static_cast<clang::ImplicitCastExpr *>(statement);
        return get_arg_name(impliexpr->getSubExpr(), name);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
    {
        clang::DeclRefExpr *declexpr = static_cast<clang::DeclRefExpr *>(statement);
        if (declexpr->getNameInfo().getAsString() == name)
            return true;
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::BinaryOperatorClass)
    {
        clang::BinaryOperator *binaryIter = static_cast<clang::BinaryOperator *>(statement);
        bool left = get_arg_name(binaryIter->getLHS(), name);
        bool right = get_arg_name(binaryIter->getRHS(), name);
        if (left == false && right == false)
            return false;
        else
            return true;
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::UnaryOperatorClass)
    {
        clang::UnaryOperator *unaryIter = static_cast<clang::UnaryOperator *>(statement);
        return get_arg_name(unaryIter->getSubExpr(), name);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::ParenExprClass)
    {
        clang::ParenExpr *parenIter = static_cast<clang::ParenExpr *>(statement);
        return get_arg_name(parenIter->getSubExpr(), name);
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::ArraySubscriptExprClass)
    {
        clang::ArraySubscriptExpr *arrayIter = static_cast<clang::ArraySubscriptExpr *>(statement);
        bool left = get_arg_name(arrayIter->getLHS(), name);
        bool right = get_arg_name(arrayIter->getRHS(), name);
        if (left == false && right == false)
            return false;
        else
            return true;
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::MemberExprClass)
    {
        clang::MemberExpr *structIter = static_cast<clang::MemberExpr *>(statement);
        string struct_name = analyze_struct(structIter);
        if (name == struct_name)
            return true;
        else
            return false;
    }
    else if (statement->getStmtClass() == clang::Stmt::StmtClass::CXXConstructExprClass)
    {
        clang::CXXConstructExpr *cxxIter = static_cast<clang::CXXConstructExpr *>(statement);
        auto iter = cxxIter->child_begin();
        for (; iter != cxxIter->child_end(); iter++)
        {
            assert((*iter) != NULL);
            return get_arg_name((*iter), name);
        }
    }
    return ret;
}

void UndefinedVariableChecker::get_call_func_use(string name, int blockno, std::vector<Child> *childlist)
{
    for (int i = 0; i < useless_block_statement[blockno].usefulBlockStatement.size(); i++)
    {
        for (int j = 0; j < useless_block_statement[blockno].usefulBlockStatement[i].usedVariable.size(); j++)
        {
            if (name == useless_block_statement[blockno].usefulBlockStatement[i].usedVariable[j].variable)
            {
                if (useless_block_statement[blockno].usefulBlockStatement[i].stmt->getStmtClass() == clang::Stmt::StmtClass::CallExprClass)
                {
                    clang::CallExpr *callexpr = static_cast<clang::CallExpr *>(useless_block_statement[blockno].usefulBlockStatement[i].stmt);
                    clang::Expr *expr = callexpr->getCallee();
                    if (expr->getStmtClass() == clang::Stmt::StmtClass::ImplicitCastExprClass)
                    {
                        clang::ImplicitCastExpr *impliexpr = static_cast<clang::ImplicitCastExpr *>(expr);
                        if (impliexpr->getSubExpr()->getStmtClass() == clang::Stmt::StmtClass::DeclRefExprClass)
                        {
                            // sth wrong
                            clang::DeclRefExpr *declexpr = static_cast<clang::DeclRefExpr *>(impliexpr->getSubExpr());
                            string func_name = declexpr->getNameInfo().getAsString();
                            int arg_num = callexpr->getNumArgs();
                            for (int k = 0; k < arg_num; k++)
                            {
                                if (get_arg_name(callexpr->getArg(k), name) == true)
                                {
                                    for (int t = 0; t < childlist->size(); t++)
                                    {
                                        if ((*childlist)[t].funcname == func_name)
                                        {
                                            (*childlist)[t].parameter_init[k] = 0;
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
}

void UndefinedVariableChecker::find_dummy_definition(std::vector<Child> *childlist)
{
    // std::cout << "dummy\n";
    for (int i = 0; i < definitionNum; i++)
    {
        int target_statement_no = i;
        string name = var_vector[i].first;
        assert(i < var_vector.size());
        queue<BlockBitVector> q;
        q.push(blockbitvector[this->blockNum - 1]);
        blockbitvector[this->blockNum - 1].isvisit = true;
        while (!q.empty())
        {
            bool stop = false;
            BlockBitVector tempblock = q.front();
            if (tempblock.blockid != this->blockNum - 1 && tempblock.blockid != 0)
            {
                // std::cout <<"***********" << name <<std::endl;
                if (tempblock.Outvector[target_statement_no] == 1)
                {
                    // std::cout <<"***********" << name << " " << tempblock.blockid<<std::endl;
                    int statenum = definitionNum;
                    for (int j = 0; j <= tempblock.blockid; j++)
                        statenum = statenum + block_statement[j].usefulBlockStatement.size();
                    check_variable_use_in_block(name, tempblock.blockid, statenum - 1);
                    check_all_use_in_block(name, tempblock.blockid);
                    if (childlist != nullptr)
                    {
                        get_call_func_use(name, tempblock.blockid, childlist);
                        // std::cout << "childlist " << childlist->size() << endl;
                    }
                }
                else
                { // kill unreachable
                    // std::cout <<"kill***" << name << std::endl;
                    bool isreplace = false;
                    for (int j = 0; j < bitVectorLength; j++)
                    {
                        if (j >= definitionNum && tempblock.Outvector[j] == 1 && allusefulstatement[j - definitionNum].variable == name)
                        { // gen variable
                            // std::cout << name <<std::endl;
                            check_variable_use_in_block(name, tempblock.blockid, j);
                            isreplace = true;
                            stop = true;
                        }
                    }
                    if (isreplace == false)
                        stop = true;
                }
            }
            else if (tempblock.blockid == this->blockNum - 1)
            {
                if (tempblock.Outvector[target_statement_no] == 0)
                {
                    q.pop();
                    continue;
                }
            }
            if (stop == true)
            {
                q.pop();
                continue;
            }
            // std::cout <<"1***********" << name << std::endl;
            for (int i = 0; i < tempblock.succ.size(); i++)
            {
                if (blockbitvector[tempblock.succ[i]].isvisit == false)
                {
                    // std::cout <<"add***********" << name << std::endl;
                    q.push(blockbitvector[tempblock.succ[i]]);
                    blockbitvector[tempblock.succ[i]].isvisit = true;
                }
            }
            q.pop();
        }
        for (int j = 0; j < this->blockNum; j++)
        {
            blockbitvector[j].isvisit = false;
        }
        // std::cout << "variable " << name << "finished" <<std::endl;
    }
    // std::cout << "out dummy\n";
}

void UndefinedVariableChecker::report_file(string filename)
{
    need_to_print_file = true;
    std::cout << filename << std::endl;
}