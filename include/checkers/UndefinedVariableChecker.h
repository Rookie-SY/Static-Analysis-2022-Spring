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

#define HASH_SIZE 0x3fff

class UndefinedVariableChecker : public BasicChecker {
private:
  void getEntryFunc();
  void readConfig();
  ASTFunction *entryFunc;
  std::vector<ASTFunction *> allFunctions;
  struct Variable{
    string variable;
    clang::SourceLocation loc;
  };
  struct Initvalue{
    int initkind;
    double value;
    string ref;
  };
  struct StatementInfo{
    int statementno;
    clang::Stmt* stmt;
    bool isdummy;
    vector<Variable> usedVariable;
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
    bool isconstantarray;
    string variable;
    int rvalueKind;
    double rvalueLiteral;
    vector<Variable> rvalueString;
    bool isdummy;
  };
  struct Child{
    string funcname;
    int param_num;
    std::vector<int> parameter_init;
    clang::FunctionDecl* funcDecl;
    ASTFunction* astfunc;
  };
  struct InterNode{
    string funcname;
    bool isvisit;
  };
  bool need_to_print_file;
  int request_fun;
  int maxPathInFun;
  int definitionNum;
  int blockNum;
  int bitVectorLength;
  int nodeNum;
  clang::SourceManager *SM;
  std::vector<InterNode> graph;
  std::vector<std::pair<string,Initvalue>> var_vector;
  std::vector<BlockInfo> block_statement;
  std::vector<BlockInfo> useless_block_statement;
  std::vector<BlockBitVector> blockbitvector;
  std::vector<UsefulStatementInfo> allusefulstatement;
  std::unordered_map<string,vector<string>> locationMap;
public:
  void recursive_get_binaryop(clang::Stmt* statement,UsefulStatementInfo* info);
  void recursive_find_usevariable(clang::Stmt* statement,StatementInfo* info);
  void get_rvalue(clang::Stmt* statement,UsefulStatementInfo* info);
  void report_warning(Variable variable);
/*public:
  unsigned int hash_pjw(string name);
  void insert_Hashtable(string name);
  bool findHashtable(string name);*/
public:
  UndefinedVariableChecker(ASTResource *resource, ASTManager *manager,
                  CallGraph *call_graph, Config *configure)
      : BasicChecker(resource, manager, call_graph, configure){};

  void reset();
  void check();
  void check_child(Child child);

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
  void get_useless_statement_variable();
  string analyze_array(clang::ArraySubscriptExpr* arrayexpr);
  string analyze_struct(clang::MemberExpr* structexpr);

  void check_variable_use_in_block(string name,int blockno,int statementno);
  void check_all_use_in_block(string name,int blockno);
  void calculate_block(int num);
  void blockvector_output();
  void undefined_variable_check();
  void find_dummy_definition(std::vector<Child>* childlist);
  void dump_debug();

  void change_definition_bit(std::vector<int> param_bit);
  void get_call_func_use(string name,int blockno,std::vector<Child>* childlist);
  bool get_arg_name(clang::Stmt* statement,string name);
  void dump_func(std::vector<Child> childlist);
  void is_other_function(clang::FunctionDecl* func);

  void report_file(string filename);
  void init_param(int paramnum);
};