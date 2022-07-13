#pragma once
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

struct TraceRecord
{
    queue<SourceLocation> calleeLocation;
    queue<string> calleeName;
};

struct MemoryBlock
{
    int begin;
    int end;
};

struct ComputeRecord
{
    int unitLength;
    MemoryBlock local;
    MemoryBlock syn;
};

struct Pointer
{
    string pointerName;
    string initialName;
    int memoryID;
    int pointerID;
    stack<SourceLocation> location;
    TraceRecord record;
    bool isArray;

    bool isComputable;
    ComputeRecord compute;
};

struct DFreePointer
{
    string pointerName;
    TraceRecord record;
    int memoryID;
    SourceLocation location;
    SourceLocation exprLocation;
};

struct ReportPointer
{
    string pointerName;
    TraceRecord record;
    int memoryID;
    SourceLocation location;
};

class ProgramPaths
{
private:
    vector<vector<int> > pathContainer; /// 程序执行路径保存block序号
    vector<CFGBlock> blockContainer;    /// 将block按序存在一个vector内
public:
    ProgramPaths(){
    }
    void get_paths(vector<int> pathVector, CFGBlock block);
    bool travel_block(CFGBlock block, bool* isBlockVisited);

friend class MemoryLeakChecker;
};

class PointerSet
{
private:
    vector<Pointer> pointerVec;
    vector<DFreePointer> DFreeVec;
    int pointCount;
    int memoryCount;
public:
    PointerSet(){
        pointCount = 1;
        memoryCount = 1;
    }
    clang::SourceManager* SM;

    int get_memoryID(string name, SourceLocation location, int& pos, int mode);
    int get_new_point_count(){
        return pointCount++;
    }
    int get_new_memory_count(){
        return memoryCount++;
    }
    int get_unit_length(string type);

    void new_pointer(string name, SourceLocation _location, TraceRecord traceRecord, string type, bool isArray, bool computable, int localSize);
    void modify_pointer(string leftName, string rightName, SourceLocation leftLocation, SourceLocation rightLocation, string type);
    void modify_pointer_after_change(string leftName, string rightName, SourceLocation leftLocation, SourceLocation rightLocation, string type, int base, bool op);
    void special_new_pointer(string leftName, SourceLocation leftLocation, SourceLocation rightLocation, TraceRecord traceRecord, bool isArray, string type, bool computable, int localSize);
    void delete_pointer(string name, SourceLocation location, clang::CXXDeleteExpr* deleteExpr);
    void free_pointer(string name, SourceLocation location, SourceLocation exprLocation);
    void handle_computable(string name, SourceLocation location);
    void handle_one_pointer(string name, SourceLocation location, int base, bool op);

    void handle_callee_pointer(string name, SourceLocation location, SourceLocation newLocation);
    void handle_non_callee_pointer();
    void handle_after_callee();
    
    void print_set();
friend class MemoryLeakChecker;
};

class MemoryLeakChecker :   public BasicChecker
{
public:
    MemoryLeakChecker(ASTResource *resource, ASTManager *manager,
                        CallGraph *call_graph, Config *configure)
        :   BasicChecker(resource, manager, call_graph, configure){};
    
    void check(ASTFunction* _entryFunc);

private:
    vector<ASTFunction *> allFunctions;
    ASTFunction* entryFunc;
    ASTFunction* currentEntryFunc;
    PointerSet Pointers;
    vector<vector<ReportPointer> >allLeakPointers;
    vector<vector<DFreePointer> >allDFreePointers;
    vector<ReportPointer> leakResult;
    vector<DFreePointer> DFreeResult;
    clang::SourceManager* SM;

    void read_config();
    void get_entry_func();
    void handle_cfg(unique_ptr<CFG>& CFG, TraceRecord traceRecord, bool isEntryFunc);

    void get_cfg_paths(unique_ptr<CFG>& CFG, ProgramPaths& allPaths);
    void handle_cfg_paths(ProgramPaths allPaths, TraceRecord traceRecord, bool isEntryFunc);
    void handle_single_path(ProgramPaths allPaths, vector<int> path, TraceRecord traceRecord);
    
    ASTFunction* get_callee_func(string funcName);
    bool special_case(Stmt* statement, BumpVector<CFGElement>::reverse_iterator elemIter);
    void handle_callExpr(Stmt* statement, BumpVector<CFGElement>::reverse_iterator elemIter, TraceRecord traceRecord);
    bool handle_malloc_size(Stmt* sizeStmt, int& size);
    bool handle_new_size(CXXNewExpr* newExpr, int& size);
    void handle_callee(CallExpr* callExprStmt, ASTFunction* fun, const FunctionDecl* funcDecl, TraceRecord traceRecord);
    void handle_return_pointer(FunctionDecl* calleeFunc, const ParmVarDecl* param);
    void handle_binaryOperator(Stmt* statement, TraceRecord traceRecord);
    void handle_declStmt(Stmt* statement);
    void handle_stmt(Stmt* statement, BumpVector<CFGElement>::reverse_iterator elemIter, TraceRecord traceRecord);
    void handle_newExpr(Stmt* statement, BumpVector<CFGElement>::reverse_iterator elemIter, TraceRecord traceRecord);
    void handle_deleteExpr(Stmt* statement);
    void handle_compoundOp(Stmt* statement);
    void handle_unaryOp(Stmt* statement);
    void handle_binary_pointer_in_binaryOp(DeclRefExpr* leftDeclRef, BinaryOperator* binaryOp);
    void handle_binary_pointer_in_declStmt(VarDecl* VarDecl, BinaryOperator* binaryOp);
    void handle_binary_callee_in_binaryOp(DeclRefExpr* leftDeclRef, DeclRefExpr* declRefExpr, BinaryOperator* subBinary);
    void handle_binary_callee_in_declStmt(VarDecl* varDecl, DeclRefExpr* declRefExpr, BinaryOperator* binaryOp);
    void handle_unaryOp_in_declStmt(VarDecl* varDecl, UnaryOperator* unaryOp);
    void handle_unaryOp_in_binaryStmt(DeclRefExpr* leftDeclRef, UnaryOperator* unaryOp);
    
    void handle_location_string(string& line, string& loc, SourceLocation location);
    ReportPointer convert_pointer(DFreePointer tmp);
    void prepare_next_path(unsigned count);
    void check_leak_same_pointer();
    void check_Dfree_same_pointer();
    bool is_same_pointer(ReportPointer left, ReportPointer right);
    void report_memory_leak();
    void report_double_free();
};
