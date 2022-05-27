#include <fstream>
#include <iostream>
#include <list>
#include <queue>
#include <sstream>
#include <stack>
#include <string>

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/Attr.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Analysis/CFG.h>
#include <clang/Frontend/ASTConsumers.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Lex/Lexer.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/raw_ostream.h>

#include "framework/BasicChecker.h"

using namespace clang;
using namespace llvm;
using namespace clang::driver;
using namespace clang::tooling;
using namespace std;

#define IsInitialize 1
#define NotInitialize 2
#define Reference 3

#define Literal 4
#define Ref 5

class UndefinedVariableChecker : public BasicChecker {
private:
  void getEntryFunc();
  void readConfig();
  ASTFunction *entryFunc;
  
  struct Initvalue{
    int initkind;
    double value;
    string ref;
  };
  struct StatementInfo{
    int statementno;
    clang::Stmt* stmt;
    bool isdummy;
  };
  struct BlockInfo{
    int blockno;
    vector<StatementInfo> usefulBlockStatement;
  };
  struct BlockBitVector{
    int blockid;
    vector<int> Invector;
    vector<int> Outvector;
    vector<int> Genvector;
    vector<int> Killvector;
    vector<int> pred;
    vector<int> succ;
    bool isvisit;
  };
  struct UsefulStatementInfo{
    clang::Stmt* statement;
    string variable;
    int rvalueKind;
    double rvalueLiteral;
    vector<string> rvalueString;
    bool isdummy;
  };
  int request_fun;
  int maxPathInFun;
  int definitionNum;
  int blockNum;
  int bitVectorLength;
  clang::SourceManager *SM;
  std::vector<std::pair<string,Initvalue>> var_vector;
  std::vector<BlockInfo> block_statement;
  std::vector<BlockBitVector> blockbitvector;
  std::vector<UsefulStatementInfo> allusefulstatement;
public:
  void recursive_get_binaryop(clang::Stmt* statement,UsefulStatementInfo* info);
  void get_rvalue(clang::Stmt* statement,UsefulStatementInfo* info);
  void report_warning(string name,clang::Stmt* statement);
public:
  UndefinedVariableChecker(ASTResource *resource, ASTManager *manager,
                  CallGraph *call_graph, Config *configure)
      : BasicChecker(resource, manager, call_graph, configure){};
  void check();
  string get_statement_value(clang::Stmt* statement);
  vector<int> kill_variable(int statementno,string variable);
  void print_stmt_kind(Stmt* statement, int spaceCount);
  void get_cfg_stmt(unique_ptr<CFG>& cfg);
  void get_bit_vector_length();
  void count_definition(unique_ptr<CFG>& cfg);
  void get_block_statement(unique_ptr<CFG>& cfg);
  void init_blockvector(unique_ptr<CFG>& cfg);

  void get_all_useful_statement();
  void calculate_gen_kill();

  void check_variable_use_in_block(string name,int blockno,int statementno);
  void calculate_block(int num);
  void blockvector_output();
  void undefined_variable_check();
  void find_dummy_definition();
  void dump_debug();
};