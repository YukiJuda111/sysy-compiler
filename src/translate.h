#pragma once
#include "AST.h"
#include <unordered_map>
#include <stack>
#include <vector>


enum DeclType {
    INT,
    ARRAY,
    VOID,
    ERROR
};

typedef struct _symbolTableInfo symbolTableInfo;

struct _symbolTableInfo {
    DeclType type;
    vector<DeclType> params;
    vector<int> arraySize;
    int tempIndex;
    bool isConst;
    int constValue;
    bool isGlobal;
    bool isNot;
    bool isAddr;
    bool isArrParam;
};

void dumpNode(Node node);
void printFileIr(FILE* output);
void Start(Node node);
void CompUnit(Node node);
void Decl(Node node, bool isGlobal);
DeclType BType(Node node);
void VarDecl(Node node, bool isGlobal);
void VarList(Node node, DeclType type, bool isGlobal);
void VarDef(Node node, DeclType type, bool isGlobal);
symbolTableInfo IntConstList(Node node, string varName);
symbolTableInfo InitVal(Node node, const symbolTableInfo& info, int& curPtr, vector<pair<int, bool> >& initValues, int level);
void InitValList(Node node, const symbolTableInfo& info, int& curPtr, vector<pair<int, bool> >& initValues, int level);
void FuncDef(Node node);
void FuncFParams(Node node, string funcName);
void FuncFParam(Node node, string funcName);
void FuncFParamList(Node node, string funcName);
void Block(Node node, const DeclType& returnType);
void BlockItemList(Node node, const DeclType& returnType);
void BlockItem(Node node, const DeclType& returnType);
void Stmt(Node node, const DeclType& returnType);
symbolTableInfo Exp(Node node);
void Cond(Node node, const int& trueLabel, const int& falseLabel);
symbolTableInfo LVal(Node node);
symbolTableInfo ExpList(Node node, const symbolTableInfo& info, const int level);
symbolTableInfo PrimaryExp(Node node);
void Number(Node node);
symbolTableInfo UnaryExp(Node node);
void UnaryOp(Node node);
void FuncRParams(Node node, const symbolTableInfo& info);
void ExpList2(Node node, const symbolTableInfo& info);
symbolTableInfo MulExp(Node node);
symbolTableInfo AddExp(Node node);
symbolTableInfo RelExp(Node node, const int& trueLabel, const int& falseLabel);
symbolTableInfo EqExp(Node node, const int& trueLabel, const int& falseLabel);
symbolTableInfo LAndExp(Node node, const int& trueLabel, const int& falseLabel);
symbolTableInfo LOrExp(Node node, const int& trueLabel, const int& falseLabel);
void printSymbolTable();
symbolTableInfo FindVar(string varName);