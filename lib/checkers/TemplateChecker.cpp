#include "checkers/TemplateChecker.h"

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

void TemplateChecker::print_stmt_kind(Stmt* statement, int spaceCount)
{
    if(statement == nullptr)
    {
        std::cout << "<<<NULL>>>" << std::endl;
        return;
    }
    std::cout << statement->getStmtClassName() << std::endl;
    
    clang::Stmt::child_iterator iter = statement->child_begin();
    for(; iter != statement->child_end(); iter++)
    {
        for(int i = 0; i < spaceCount; i++)
            std::cout << "  ";
        
        //FIXME:
        /*if((*iter)->getStmtClass() == clang::Stmt::StmtClass::DeclStmtClass)
        {
            clang::DeclStmt myStmt()
            std::cout << "Found Dec at " << myStmt->getBeginLoc().dump();
            if(myStmt->isSingleDecl())
            {
                clang::Decl* myDecl = myStmt->getSingleDecl();
                myDecl->getDeclContext()->dumpDeclContext();
            }
            else
            {
                clang::DeclGroupRef myDeclRef = myStmt.getDeclGroup();
                clang::DeclGroupRef::iterator groupIter = myDeclRef.begin();
                for(; groupIter != myDeclRef.end(); groupIter++)
                {
                    *groupIter->
                }
            }
        }*/

        print_stmt_kind(*iter, spaceCount + 1);
    }
}

void TemplateChecker::check() {
    readConfig();
    getEntryFunc();
    if (entryFunc != nullptr) {
        FunctionDecl *funDecl = manager->getFunctionDecl(entryFunc);
        std::cout << "The entry function is: "
                << funDecl->getQualifiedNameAsString() << std::endl;
        std::cout << "Here is its dump: " << std::endl;
        funDecl->dump();

        std::cout << "Here are related Statements: " << std::endl;
        Stmt* statement =  funDecl->getBody();

        print_stmt_kind(statement, 1);
    }
    LangOptions LangOpts;
    LangOpts.CPlusPlus = true;
    std::unique_ptr<CFG>& cfg = manager->getCFG(entryFunc);
    cfg->dump(LangOpts, true);
    
    /*CFGBlock* cfgBlock = cfg->createBlock();
    std::cout << "CFGBlock is listed: " << std::endl;
    cfgBlock->dump();
    
    unsigned int size = cfgBlock->size();

    using myIterator = clang::CFGBlock::iterator;
    myIterator iter = cfgBlock->begin();
    for(myIterator iter = cfgBlock->begin(); iter != cfgBlock->end(); iter++)
    {
        CFGStmt cfgStmt = *iter->getAs<CFGStmt>();
        const Stmt* myStmt = cfgStmt.getStmt();
        std::cout << "Here are all the statements: " << std::endl;
        myStmt->dump();
    }*/

}

void TemplateChecker::readConfig() {
  std::unordered_map<std::string, std::string> ptrConfig =
      configure->getOptionBlock("TemplateChecker");
  request_fun = stoi(ptrConfig.find("request_fun")->second);
  maxPathInFun = 10;
}

void TemplateChecker::getEntryFunc() {
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
