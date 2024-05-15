#include "semant.h"

vector<unordered_map<string, semant_symbolTableInfo>> semant_symbolTable;

void semant_Start(Node node) {
    if (node == nullptr) return;
    semant_symbolTable.push_back(unordered_map<string, semant_symbolTableInfo>());
    semant_CompUnit(node->children[0]);
}

void semant_CompUnit(Node node) {
    if (node == nullptr) return;
    int size = node->children.size();
    if (size == 0) {
        return;
    }
    if (node->children[1]->name == "Decl") {
        semant_CompUnit(node->children[0]);
        semant_Decl(node->children[1]);
    } else {
        semant_CompUnit(node->children[0]);
        semant_FuncDef(node->children[1]);
    }
}

void semant_Decl(Node node) {
    if (node == nullptr) return;
    semant_VarDecl(node->children[0]);
}

semant_DeclType semant_BType(Node node) {
    if (node == nullptr) return semant_ERROR;
    if (node->children[0]->name == "TYPE") {
        return semant_INT;
    }
    return semant_ERROR;
}

void semant_VarDecl(Node node) {
    if (node == nullptr) return;
    semant_DeclType type = semant_BType(node->children[0]);
    semant_VarDef(node->children[1], type);
    semant_VarList(node->children[2], type);

}

void semant_VarList(Node node, semant_DeclType type) {
    if (node == nullptr) return;

    semant_VarDef(node->children[1], type);
    semant_VarList(node->children[2], type);
} 


void semant_VarDef(Node node, semant_DeclType type) {
    if (node == nullptr) return;
    string varName = node->children[0]->data.str;

    if (semant_symbolTable.back().count(varName)){
        cout << "Error: Redefinition of variable " << varName << endl;
        exit(1);
    } else {
        semant_symbolTable.back()[varName] = semant_symbolTableInfo{type, vector<semant_DeclType>()};
    }
    int size = node->children.size();
    if (size == 2) {
        semant_IntConstList(node->children[1], varName, 0);
    } else {
        auto info1 = semant_IntConstList(node->children[1], varName, 0);
        auto arrInfo = semant_FindVar(varName);
        bool isArr = arrInfo.type == semant_ARRAY;
        int level = arrInfo.arraySize.size();
        int totalSize = 1;
        for (int i = 0; i < level; i++) {
            totalSize *= arrInfo.arraySize[i];
        }
        unordered_map<int, int> initMap;
        auto info2 = semant_InitVal(node->children[3], semant_FindVar(varName), level, isArr, initMap);
        if (isArr && initMap[level - 1] > totalSize) {
            cout << "Error: Initialization size mismatch" << endl;
            exit(1);
        }
        if (info1.type != info2.type) {
            cout << "Error: Initialization type mismatch" << endl;
            exit(1);
        }
    }
}

semant_symbolTableInfo semant_IntConstList(Node node, string varName, int arrayDim) {
    if (node == nullptr) return semant_symbolTableInfo{semant_INT, vector<semant_DeclType>()};

    semant_symbolTable.back()[varName].type = semant_ARRAY;
    semant_symbolTable.back()[varName].arraySize.push_back(node->children[1]->data.num);
    semant_IntConstList(node->children[3], varName, arrayDim + 1);
    return semant_symbolTableInfo{semant_ARRAY, vector<semant_DeclType>()};
}

semant_symbolTableInfo semant_InitVal(Node node, const semant_symbolTableInfo& arrInfo, int level, bool isArr, unordered_map<int, int>& initMap) {
    int size = node->children.size();
    if (size == 1) {
        if (isArr) {
            initMap[level] ++;
        }
        return semant_Exp(node->children[0]);
    } else if (size == 2) {
        if (isArr && initMap[level] % arrInfo.arraySize.back() != 0) {
            cout << "Error: Initialization size mismatch" << endl;
            exit(1);
        }

        if (isArr) {
            initMap[level] += arrInfo.arraySize.back();
            if (level != (int)arrInfo.arraySize.size()) {
                initMap[level - 1] = 0;
            }
        }
        return semant_symbolTableInfo{semant_ARRAY, vector<semant_DeclType>()};
    } else {
        if (isArr && initMap[level] % arrInfo.arraySize.back() != 0) {
            cout << "Error: Initialization size mismatch" << endl;
            exit(1);
        }
        semant_InitVal(node->children[1], arrInfo, level - 1, isArr, initMap);
        semant_InitValList(node->children[2], arrInfo, level - 1, isArr, initMap);
        if (isArr) {
            initMap[level] += arrInfo.arraySize.back();
            if (level != (int)arrInfo.arraySize.size()) {
                initMap[level - 1] = 0;
            }
        }
        return semant_symbolTableInfo{semant_ARRAY, vector<semant_DeclType>()};
    }
}

void semant_InitValList(Node node, const semant_symbolTableInfo& arrayInfo, int level, bool isArr, unordered_map<int, int>& initMap) {
    if (node == nullptr) return;
    semant_InitVal(node->children[1], arrayInfo, level, isArr, initMap);
    semant_InitValList(node->children[2], arrayInfo, level, isArr, initMap);
}

void semant_FuncDef(Node node) {
    if (node == nullptr) return;

    string funcName = node->children[1]->data.str;
    int size = node->children.size();
    if (node->children[0]->isTerminal) {

        if (semant_symbolTable.back().count(funcName)){
            cout << "Error: Redefinition of function " << funcName << endl;
            exit(1);
        } else {
            semant_symbolTable.back()[funcName] = semant_symbolTableInfo{semant_VOID, vector<semant_DeclType>()};
        }

        semant_symbolTable.push_back(unordered_map<string, semant_symbolTableInfo>());

        if (size == 5) {
            semant_Block(node->children[4], semant_VOID);
        } else {
            semant_FuncFParams(node->children[3], funcName);
            semant_Block(node->children[5], semant_VOID);
        }
        semant_symbolTable.pop_back();
    } else {
        semant_BType(node->children[0]);

        if (semant_symbolTable.back().count(funcName)){
            cout << "Error: Redefinition of function " << funcName << endl;
            exit(1);
        } else {
            semant_symbolTable.back()[funcName] = semant_symbolTableInfo{semant_INT, vector<semant_DeclType>()};
        }

        semant_symbolTable.push_back(unordered_map<string, semant_symbolTableInfo>());

        if (size == 5) {
            semant_Block(node->children[4], semant_INT);
        } else {
            semant_FuncFParams(node->children[3], funcName);
            semant_Block(node->children[5], semant_INT);
        }
        semant_symbolTable.pop_back();
    }
}

void semant_FuncFParams(Node node, string funcName) {
    if (node == nullptr) return;
    semant_FuncFParam(node->children[0], funcName);
    semant_FuncFParamList(node->children[1], funcName);
}

void semant_FuncFParamList(Node node, string funcName) {
    if (node == nullptr) return;
    semant_FuncFParam(node->children[1], funcName);
    semant_FuncFParamList(node->children[2], funcName);
}

void semant_FuncFParam(Node node, string funcName) {
    if (node == nullptr) return;
    semant_DeclType type = semant_BType(node->children[0]);
    string varName = node->children[1]->data.str;

    semant_symbolTable.back()[varName] = semant_symbolTableInfo{type, vector<semant_DeclType>()};
    semant_symbolTable[0][funcName].params.push_back(type);

    if (node->children.size() == 5) {
        semant_symbolTable.back()[varName].type = semant_ARRAY;
        semant_symbolTable[0][funcName].params.back() = semant_ARRAY;
        semant_IntConstList(node->children[4], varName, 0);
    }
}

void semant_Block(Node node, const semant_DeclType& returnType) {
    if (node == nullptr) return;
    semant_BlockItemList(node->children[1], returnType);
}

void semant_BlockItemList(Node node, const semant_DeclType& returnType) {
    if (node == nullptr) return;
    semant_BlockItem(node->children[0], returnType);
    semant_BlockItemList(node->children[1], returnType);
}

void semant_BlockItem(Node node, const semant_DeclType& returnType) {
    if (node == nullptr) return;   
    if (node->children[0]->name == "Decl") {
        semant_Decl(node->children[0]);
    } else {
        semant_Stmt(node->children[0], returnType);
    }
}

void semant_Stmt(Node node, const semant_DeclType& returnType) {
    if (node == nullptr) return;

    if (node->children[0]->name == "LVal") {
        auto type1 = semant_LVal(node->children[0]);
    
        auto type2 = semant_Exp(node->children[2]);

        if (type1.type != type2.type) {
            cout << "Error: Assignment type mismatch" << endl;
            exit(1);
        }
        return;
    } 

    if (node->children[0]->name == "SEMICOLON") {
        return;
    }

    if (node->children[0]->name == "Exp") {
        semant_Exp(node->children[0]);
        return;
    }

    if (node->children[0]->name == "Block") {
        semant_symbolTable.push_back(unordered_map<string, semant_symbolTableInfo>());
        semant_Block(node->children[0], returnType);
        semant_symbolTable.pop_back();
        return;
    }

    int size = node->children.size();
    if (node->children[0]->name == "IF" && size == 5) {
        semant_Cond(node->children[2]);

        semant_Stmt(node->children[4], returnType);
        return;
    }

    if (node->children[0]->name == "IF" && size == 7) {

        semant_Cond(node->children[2]);

        semant_Stmt(node->children[4], returnType);

        semant_Stmt(node->children[6], returnType);
        return;
    }

    if (node->children[0]->name == "WHILE") {
        semant_Cond(node->children[2]);

        semant_Stmt(node->children[4], returnType);
        return;
    }

    if (node->children[0]->name == "RETURN" && size == 2) {
        if (returnType != semant_VOID) {
            cout << "Error: Return type mismatch" << endl;
            exit(1);
        }

        return;
    }

    if (node->children[0]->name == "RETURN" && size == 3) {
        if (returnType != semant_INT) {
            cout << "Error: Return type mismatch" << endl;
            exit(1);
        }

        semant_Exp(node->children[1]);

        return;
    }
}

semant_symbolTableInfo semant_Exp(Node node) {
    return semant_AddExp(node->children[0]);
}

void semant_Cond(Node node) {
    semant_LOrExp(node->children[0]);
}

semant_symbolTableInfo semant_LVal(Node node) {
    string varName = node->children[0]->data.str;
    
    semant_symbolTableInfo info = semant_FindVar(varName);
    if(info.type == semant_ERROR) {
        cout << "Error: Undefined variable " << varName << endl;
        exit(1);
    }
    if (info.type == semant_VOID) {
        cout << "Error: Variable " << varName << " is void" << endl;
        exit(1);
    }

    auto info2 = semant_ExpList(node->children[1], info, 0);
    return info2;
}

semant_symbolTableInfo semant_FindVar(string varName) {
    for (int i = semant_symbolTable.size() - 1; i >= 0; i--) {
        if (semant_symbolTable[i].count(varName)) {
            return semant_symbolTable[i][varName];
        }
    }
    return semant_symbolTableInfo{semant_ERROR, vector<semant_DeclType>()};
}

semant_symbolTableInfo semant_ExpList(Node node, const semant_symbolTableInfo& info, int arrayDim) {
    if (node == nullptr) {
        return semant_symbolTableInfo{semant_INT, vector<semant_DeclType>()};
    }

    if (info.type != semant_ARRAY) {
        cout << "Error: Variable is not an array" << endl;
        exit(1);
    }

    auto info2 = semant_Exp(node->children[1]);

    semant_ExpList(node->children[3], info, arrayDim + 1);
    return semant_symbolTableInfo{semant_INT, vector<semant_DeclType>(), info.arraySize};
}

semant_symbolTableInfo semant_PrimaryExp(Node node) {
    if (node->children[0]->name == "LPAREN") {
        auto type = semant_Exp(node->children[1]);

        return type;
    } 

    if (node->children[0]->name == "LVal") {
        auto type = semant_LVal(node->children[0]);
        return type;
    }

    semant_Number(node->children[0]);
    return semant_symbolTableInfo{semant_INT, vector<semant_DeclType>()};
}

void semant_Number(Node node) {
    return;
}

semant_symbolTableInfo semant_UnaryExp(Node node) {
    int sz = node->children.size();
    if (sz == 1) {
        auto type = semant_PrimaryExp(node->children[0]);
        return type;
    } 

    if (sz == 3) {
        string varName = node->children[0]->data.str;
        auto info = semant_FindVar(varName);
        if (varName != "write" && varName != "read" && info.type == semant_ERROR) {
            cout << "Error: Undefined function " << varName << endl;
            exit(1);
        }

        if (varName != "write" && varName != "read" && info.params.size() != 0) {
            cout << "Error: Function " << varName << " params mismatch" << endl;
            exit(1);
        }

        return semant_symbolTableInfo{semant_INT, vector<semant_DeclType>()};
    }

    if (sz == 4) {
        string varName = node->children[0]->data.str;
        auto info = semant_FindVar(varName);
        if (varName != "write" && varName != "read" && info.type == semant_ERROR) {
            cout << "Error: Undefined function " << varName << endl;
            exit(1);
        }
        
        if (varName != "write" && varName != "read") {
            semant_FuncRParams(node->children[2], info, false);
        } else {
            semant_FuncRParams(node->children[2], info, true);
        }

        return semant_symbolTableInfo{semant_INT, vector<semant_DeclType>()};
    }

    semant_UnaryOp(node->children[0]);
    auto type = semant_UnaryExp(node->children[1]);
    return type;
}

void semant_UnaryOp(Node node) {
    return;
}

void semant_FuncRParams(Node node, const semant_symbolTableInfo& info, bool isRW) {

    if(isRW) {
        auto type = semant_Exp(node->children[0]);
        if (type.type !=  semant_INT && type.type != semant_ARRAY) {
            cout << "Error: Function params mismatch" << endl;
            exit(1);
        }
        return;
    }

    int cnt = 0;
    auto type = semant_Exp(node->children[0]);
    
    cnt ++;
    semant_ExpList2(node->children[1], cnt, info);
}

void semant_ExpList2(Node node, int cnt, const semant_symbolTableInfo& info) {
    if (node == nullptr) {
        if (cnt != (int)info.params.size()) {
            cout << "Error: Function params mismatch" << endl;
            exit(1);
        }
        return;
    }

    auto type = semant_Exp(node->children[1]);
    if (type.type != info.params[cnt]) {
        cout << "Error: Function params mismatch" << endl;
        exit(1);
    }
    semant_ExpList2(node->children[2], cnt + 1, info);
    return;
}

semant_symbolTableInfo semant_MulExp(Node node) {

    if (node->children[0]->name == "UnaryExp") {
        auto type = semant_UnaryExp(node->children[0]);
        return type;
    }
    
    auto type1 = semant_MulExp(node->children[0]);

    auto type2 = semant_UnaryExp(node->children[2]);
    if (type1.type != type2.type) {
        cout << "Error: Type mismatch" << endl;
        exit(1);
    }
    return type1;
}

semant_symbolTableInfo semant_AddExp(Node node) {

    if (node->children[0]->name == "MulExp") {
        auto type = semant_MulExp(node->children[0]);
        return type;
    }
    
    auto type1 = semant_AddExp(node->children[0]);

    auto type2 = semant_MulExp(node->children[2]);
    if (type1.type != type2.type) {
        cout << "Error: Type mismatch" << endl;
        exit(1);
    }
    return type1;
}

semant_symbolTableInfo semant_RelExp(Node node) {

    if (node->children[0]->name == "AddExp") {
        auto type = semant_AddExp(node->children[0]);
        return type;
    }
    
    auto type1 = semant_RelExp(node->children[0]);

    auto type2 = semant_AddExp(node->children[2]);
    if (type1.type != type2.type) {
        cout << "Error: Type mismatch" << endl;
        exit(1);
    }
    return type1;
}

semant_symbolTableInfo semant_EqExp(Node node) {

    if (node->children[0]->name == "RelExp") {
        auto type = semant_RelExp(node->children[0]);
        return type;
    }
    
    auto type1 = semant_EqExp(node->children[0]);

    auto type2 = semant_RelExp(node->children[2]);
    if (type1.type != type2.type) {
        cout << "Error: Type mismatch" << endl;
        exit(1);
    }
    return type1;
}

semant_symbolTableInfo semant_LAndExp(Node node) {

    if (node->children[0]->name == "EqExp") {
        auto type = semant_EqExp(node->children[0]);
        return type;
    }
    
    auto type1 = semant_LAndExp(node->children[0]);

    auto type2 = semant_EqExp(node->children[2]);
    if (type1.type != type2.type) {
        cout << "Error: Type mismatch" << endl;
        exit(1);
    }
    return type1;
}

semant_symbolTableInfo semant_LOrExp(Node node) {

    if (node->children[0]->name == "LAndExp") {
        auto type = semant_LAndExp(node->children[0]);
        return type;
    }
    
    auto type1 = semant_LOrExp(node->children[0]);

    auto type2 = semant_LAndExp(node->children[2]);
    if (type1.type != type2.type) {
        cout << "Error: Type mismatch" << endl;
        exit(1);
    }
    return type1;
}