#include "checkers/MainChecker.h"

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

void MainChecker::check()
{
    read_config();
    get_entry_func();
    MemoryLeakChecker MLChecker(resource, manager, call_graph, configure);
    MLChecker.check(entryFunc);
    UndefinedVariableChecker UVChecker(resource, manager, call_graph, configure);
    UVChecker.check(entryFunc);
}

void MainChecker::read_config()
{
    unordered_map<string, string> ptrConfig = 
        configure->getOptionBlock("MainChecker");
    request_fun = std::__cxx11::stoi(ptrConfig.find("request_fun")->second);
    maxPathInFun = 10;
}

void MainChecker::get_entry_func()
{
    vector<ASTFunction* > topLevelFuncs = call_graph->getTopLevelFunctions();
    for(auto fun : topLevelFuncs)
    {
        const FunctionDecl* funDecl = manager->getFunctionDecl(fun);
        if(funDecl->getQualifiedNameAsString() == "main")
        {
            entryFunc = fun;
            return;
        }
    }
    entryFunc = nullptr;
    return;
}