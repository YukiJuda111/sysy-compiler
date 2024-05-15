#pragma once
#include "AST.h"
#include <unordered_map>
#include <stack>
#include <vector>


enum semant_DeclType {
    semant_INT,
    semant_ARRAY,
    semant_VOID,
    semant_ERROR
};

typedef struct _semant_symbolTableInfo semant_symbolTableInfo;

struct _semant_symbolTableInfo {
    semant_DeclType type;
    vector<semant_DeclType> params;
    vector<int> arraySize;
};

semant_symbolTableInfo semant_FindVar(string varName);

void semant_Start(Node node);
void semant_CompUnit(Node node);
void semant_Decl(Node node);
semant_DeclType semant_BType(Node node);
void semant_VarDecl(Node node);
void semant_VarList(Node node, semant_DeclType type);
void semant_VarDef(Node node, semant_DeclType type);
semant_symbolTableInfo semant_IntConstList(Node node, string varName, int arrayDim);
semant_symbolTableInfo semant_InitVal(Node node, const semant_symbolTableInfo& info, int level, bool isArr, unordered_map<int, int>& arraySize);
void semant_InitValList(Node node, const semant_symbolTableInfo& info, int level, bool isArr, unordered_map<int, int>& arraySize);
void semant_FuncDef(Node node);
void semant_FuncFParams(Node node, string funcName);
void semant_FuncFParam(Node node, string funcName);
void semant_FuncFParamList(Node node, string funcName);
void semant_Block(Node node, const semant_DeclType& returnType);
void semant_BlockItemList(Node node, const semant_DeclType& returnType);
void semant_BlockItem(Node node, const semant_DeclType& returnType);
void semant_Stmt(Node node, const semant_DeclType& returnType);
semant_symbolTableInfo semant_Exp(Node node);
void semant_Cond(Node node);
semant_symbolTableInfo semant_LVal(Node node);
semant_symbolTableInfo semant_ExpList(Node node, const semant_symbolTableInfo& info, int arrayDim);
semant_symbolTableInfo semant_PrimaryExp(Node node);
void semant_Number(Node node);
semant_symbolTableInfo semant_UnaryExp(Node node);
void semant_UnaryOp(Node node);
void semant_FuncRParams(Node node, const semant_symbolTableInfo& info, bool isRW);
void semant_ExpList2(Node node, int cnt, const semant_symbolTableInfo& info);
semant_symbolTableInfo semant_MulExp(Node node);
semant_symbolTableInfo semant_AddExp(Node node);
semant_symbolTableInfo semant_RelExp(Node node);
semant_symbolTableInfo semant_EqExp(Node node);
semant_symbolTableInfo semant_LAndExp(Node node);
semant_symbolTableInfo semant_LOrExp(Node node);