#include "checkers/TemplateChecker.h"

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

void TemplateChecker::print_stmt_kind(Stmt* statement, int spaceCount)
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
        declrefexp->getNameInfo().getName().dump();
        //static_cast<clang::VarDecl*>(declrefexp->getDecl())->getInit()->dumpColor();
        //if(static_cast<clang::VarDecl*>(declrefexp->getDecl())->getEvaluatedValue() != nullptr)
        static_cast<clang::VarDecl*>(declrefexp->getDecl())->getDeclContext()->dumpDeclContext();
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

        print_stmt_kind(statement, 0);
    }
    LangOptions LangOpts;
    LangOpts.CPlusPlus = true;
    std::unique_ptr<CFG>& cfg = manager->getCFG(entryFunc);
    cfg->dump(LangOpts, true); 

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
