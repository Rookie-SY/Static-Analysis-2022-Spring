#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

#include <clang/Tooling/CommonOptionsParser.h>
#include <llvm-c/Target.h>
#include <llvm/Support/CommandLine.h>

#include "checkers/TemplateChecker.h"
#include "checkers/UndefinedVariableChecker.h"
#include "framework/ASTManager.h"
#include "framework/BasicChecker.h"
#include "framework/CallGraph.h"
#include "framework/Config.h"
#include "framework/Logger.h"
#include "framework/ControlFlowGraph.h"
#include "framework/ProgramDependencyGraph.h"

using namespace clang;
using namespace llvm;
using namespace clang::tooling;

int main(int argc, const char *argv[]) {
  
  ofstream process_file("time.txt");
  if (!process_file.is_open()) {
    cerr << "can't open time.txt\n";
    return -1;
  }
  clock_t startCTime, endCTime;
  startCTime = clock();

  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmParser();
  
  std::vector<std::string> ASTs = initialize(argv[1]);

  Config configure(argv[2]);

  ASTResource resource;
  ASTManager manager(ASTs, resource, configure);
  CallGraph call_graph(manager, resource);
  ControlFlowGraph control_flow_graph(&manager, &resource, &call_graph);
  ForwardDominanceTree fdt(&manager, &resource, &call_graph);

  auto enable = configure.getOptionBlock("CheckerEnable");
  
  Logger::configure_UndefinedVariable(configure);
  if (enable.find("UndefinedVariableChecker")->second == "true") {
    process_file << "Starting UndefinedVariableChecker check" << endl;
    clock_t start, end;
    start = clock();

    UndefinedVariableChecker checker(&resource, &manager, &call_graph, &configure);
    checker.check();

    end = clock();
    unsigned sec = unsigned((end - start) / CLOCKS_PER_SEC);
    unsigned min = sec / 60;
    process_file << "Time: " << min << "min" << sec % 60 << "sec" << endl;
    process_file
        << "End of UndefinedVariableChecker "
           "check\n-----------------------------------------------------------"
        << endl;
  }

  if (enable.find("CallGraphChecker")->second == "true") {
    process_file << "Starting CallGraphChecker check" << endl;
    clock_t start, end;
    start = clock();

    call_graph.printCallGraph(std::cout);
    std::fstream out("outTest.dot", ios::out);
    std::fstream Nodeout("NodeTest.dot",ios::out);
    if(Nodeout.is_open()){
      call_graph.printCallGraph(Nodeout);
    }
    Nodeout.close();
      if (out.is_open()) {
      call_graph.writeDotFile(out);
    }
    out.close();

    end = clock();
    unsigned sec = unsigned((end - start) / CLOCKS_PER_SEC);
    unsigned min = sec / 60;
    process_file << "Time: " << min << "min" << sec % 60 << "sec" << endl;
    process_file
        << "End of CallGraphChecker "
           "check\n-----------------------------------------------------------"
        << endl;
  }
  control_flow_graph.drawCfg();
  fdt.ConstructFDTFromCfg();
  ControlDependenceGraph cdg(&manager, &resource, &call_graph,&fdt);
  cdg.ConstructCDG();
  //cdg.dumpCDG();
  cdg.dumpStmtCDG();
  DataDependenceGraph ddg(&manager, &resource, &call_graph,&fdt);
  ddg.ConstructDDGForest();
  ProgramDependencyGraph pdg(&manager, &resource, &call_graph,&cdg, &ddg);
  pdg.DrawPdgForest();
  endCTime = clock();
  unsigned sec = unsigned((endCTime - startCTime) / CLOCKS_PER_SEC);
  unsigned min = sec / 60;
  process_file << "-----------------------------------------------------------"
                  "\nTotal time: "
               << min << "min" << sec % 60 << "sec" << endl;
  return 0;
}
