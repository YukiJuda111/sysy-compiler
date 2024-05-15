#include "translate.h"
#include "ir.h"
vector<unordered_map<string, symbolTableInfo>> symbolTable;
int tempVarCnt = 1;
int labelCnt = 1;

symbolTableInfo FindVar(string varName) {
    for (int i = symbolTable.size() - 1; i >= 0; i--) {
        if (symbolTable[i].count(varName)) {
            return symbolTable[i][varName];
        }
    }
    return symbolTableInfo{ERROR, vector<DeclType>()};
}

void printSymbolTable() {
    for (auto &table : symbolTable) {
        cout << "---Symbol Table---" << endl;
        for (auto &entry : table) {
            cout << entry.first << " : ";
            if (entry.second.type == INT) {
                cout << "int";
            } else if (entry.second.type == ARRAY) {
                cout << "array";
                for (auto &size : entry.second.arraySize) {
                    cout << "[" << size << "]";
                }
            } else if (entry.second.type == VOID) {
                cout << "void";
            } else {
                cout << "error";
            }
            cout << " " << entry.second.tempIndex;
            cout << " " << entry.second.isGlobal;
            // if (entry.second.params.size() > 0) {
            //     cout << " (";
            //     for (auto &param : entry.second.params) {
            //         if (param == INT) {
            //             cout << "int, ";
            //         } else if (param == ARRAY) {
            //             cout << "array, ";
            //         } else if (param == VOID) {
            //             cout << "void, ";
            //         } else {
            //             cout << "error, ";
            //         }
            //     }
            //     cout << ")";
            // }
            cout << endl;
        }
    }
}

void printIr () {

    cout << "-------------Global IR--------------" << endl;
    for (auto &line : globalIr) {
        cout << line << endl;
    }
    cout << "------------------------------------" << endl;

    cout << "-----------------IR-----------------" << endl;
    for (auto &line : ir) {
        cout << line << endl;
    }
    cout << "------------------------------------" << endl;
}

void printFileIr(FILE* output) {
    for (auto &line : globalIr) {
        fprintf(output, "%s\n", line.c_str());
    }
    for (auto &line : ir) {
        fprintf(output, "%s\n", line.c_str());
    }
}


void dumpNode(Node node) {
    // if (!node->isTerminal)
    //     cout << "[" << node->name << "] number of children " << node->children.size() << endl;
    // else
    //     node->dumpTree();
}

void Start(Node node) {
    if (node == nullptr) return;
    symbolTable.push_back(unordered_map<string, symbolTableInfo>());
    dumpNode(node);
    CompUnit(node->children[0]);
}

void CompUnit(Node node) {
    if (node == nullptr) return;
    dumpNode(node);
    int size = node->children.size();
    if (size == 0) {
        return;
    }
    if (node->children[1]->name == "Decl") {
        CompUnit(node->children[0]);
        Decl(node->children[1], true);
    } else {
        CompUnit(node->children[0]);
        FuncDef(node->children[1]);
    }
}

void Decl(Node node, bool isGlobal) {
    if (node == nullptr) return;
    dumpNode(node);
    VarDecl(node->children[0], isGlobal);
}

DeclType BType(Node node) {
    if (node == nullptr) return ERROR;
    dumpNode(node);
    if (node->children[0]->name == "TYPE") {
        return INT;
    }
    return ERROR;
}

void VarDecl(Node node, bool isGlobal) {
    if (node == nullptr) return;
    dumpNode(node);
    DeclType type = BType(node->children[0]);
    VarDef(node->children[1], type, isGlobal);
    VarList(node->children[2], type, isGlobal);
}

void VarList(Node node, DeclType type, bool isGlobal) {
    if (node == nullptr) return;
    dumpNode(node);
    VarDef(node->children[1], type, isGlobal);
    VarList(node->children[2], type, isGlobal);
} 


void VarDef(Node node, DeclType type, bool isGlobal) {
    if (node == nullptr) return;
    dumpNode(node);
    string varName = node->children[0]->data.str;
    string tempName = "t" + to_string(tempVarCnt);
    symbolTable.back()[varName] = symbolTableInfo{type, vector<DeclType>(),vector<int>(), tempVarCnt, false};
    tempVarCnt++;
    symbolTable.back()[varName].isGlobal = isGlobal;
    int size = node->children.size();
    if (size == 2) {    // VarDef -> IDENT IntConstList

        auto info = IntConstList(node->children[1], varName);
        if ( isGlobal && info.type == INT ) { // 没有初始值的全局变量
            globalIr.push_back("GLOBAL " + tempName + ":");
            globalIr.push_back(".WORD #0"); 
            return;
        } 
        
        if (isGlobal && info.type == ARRAY ) { // 没有初始值的全局数组
            globalIr.push_back("GLOBAL " + tempName + ":");
            int totalSize = 1;
            for (auto &size : info.arraySize) {
                totalSize *= size;
            }
            for (int i = 0; i < totalSize; i++) {
                globalIr.push_back(".WORD #0");
            }
            return;
        }

        if (info.type == ARRAY) {   // 没有初始值的局部数组
            int totalSize = 1;
            for (auto &size : info.arraySize) {
                totalSize *= size;
            }
            ir.push_back("DEC " + tempName + " #" + to_string(totalSize * 4));
            return;
        }


    } else {    // VarDef -> IDENT IntConstList = InitVal
        auto info1 = IntConstList(node->children[1], varName);

        int totalSize = 0;
        // initValues<value/addr, notConst>
        vector<pair<int, bool> > initValues;   
        if (info1.type == ARRAY) {
            totalSize = 1;
            for (auto &size : info1.arraySize) {
                totalSize *= size;
            }
            for (int i = 0; i < totalSize; i++) {
                initValues.push_back(make_pair(0, false));
            }
        }
        int curPtr = 0;
        auto info2 = InitVal(node->children[3], FindVar(varName), curPtr, initValues, 0);

        if (isGlobal && info1.type == INT) { // 有初始值的全局变量
            if (info2.isConst) {
                globalIr.push_back("GLOBAL " + tempName + ":");
                globalIr.push_back(".WORD #" + to_string(info2.constValue));
            } else {
                // TODO: int a = b; a,b都是全局变量 
                // 在这个lab中,我们不考虑这种情况
            } 
            return;
        } 

        if (isGlobal && info1.type == ARRAY) {  // 有初始值的全局数组
            info1.isGlobal = true;
            globalIr.push_back("GLOBAL " + tempName + ":");
            for (auto &val : initValues) {
                globalIr.push_back(".WORD #" + to_string(val.first));
            }
            return;
        }

        if (info1.type == ARRAY) {  // 有初始值的局部数组
            // DEC t1 #20
            ir.push_back("DEC " + tempName + " #" + to_string(totalSize * 4));
            string newTemp = "t" + to_string(tempVarCnt++);
            ir.push_back(newTemp + " = " + tempName);
            string oldTemp = "t" + to_string(tempVarCnt++);
            for (auto &val : initValues) {
                if (val.second) {   // val是变量
                    // t2 = t_val
                    ir.push_back(oldTemp + " = t" + to_string(val.first));
                } else {    // val是常数
                    // t2 = #val
                    ir.push_back(oldTemp + " = #" + to_string(val.first));
                }
                // *t1 = t2
                ir.push_back("*" + newTemp + " = " + oldTemp);
                // t2 = t1
                ir.push_back(oldTemp + " = " + newTemp);
                // t1 = t2 + #4
                ir.push_back(newTemp + " = " + oldTemp + " + #4");
            }
            return;
        }

        // 有初始值的局部变量
        if (info2.isConst) {    
            ir.push_back(tempName + " = #" + to_string(info2.constValue));
        } else if (info2.isAddr) {  // int a = b[1]这种形式
            int newTemp = tempVarCnt++;
            ir.push_back("t" + to_string(newTemp) + " = *t" + to_string(info2.tempIndex));
            ir.push_back(tempName + " = t" + to_string(newTemp));
        } else {
            string tempName2 = "t" + to_string(info2.tempIndex);
            ir.push_back(tempName + " = " + tempName2);
        }
    }
}

symbolTableInfo IntConstList(Node node, string varName) {
    if (node == nullptr) {
        return symbolTableInfo{INT};
    }
    dumpNode(node);
    // IntConstList -> [ IntConst ] IntConstList
    auto& info = symbolTable.back()[varName];
    info.type = ARRAY;
    int arrSize = node->children[1]->data.num;
    info.arraySize.push_back(arrSize);
    IntConstList(node->children[3], varName);
    return info;
}

symbolTableInfo InitVal(Node node, const symbolTableInfo& arrInfo, int& curPtr, vector<pair<int, bool> >& initValues, int level) {
    // a[2][3][4]
    // level0 : 对齐2*3*4 level1: 对齐3*4 level2: 对齐4
    dumpNode(node);
    int size = node->children.size();
    if (size == 1) {    // InitVal -> Exp
        if (arrInfo.type == ARRAY) {
            auto info = Exp(node->children[0]);
            if (info.isConst) {
                initValues[curPtr].first = info.constValue;
                initValues[curPtr].second = false;
            } else if (info.isAddr) {   // info是数组/全局变量
                int newTemp = tempVarCnt++;
                ir.push_back("t" + to_string(newTemp) + " = *t" + to_string(info.tempIndex));
                initValues[curPtr].first = newTemp;
                initValues[curPtr].second = true;
            } else {    // info是单个变量
                initValues[curPtr].first = info.tempIndex;
                initValues[curPtr].second = true;
            }
            curPtr ++;
            return symbolTableInfo{ARRAY};
        }
        return Exp(node->children[0]);
    } else if (size == 2) { // InitVal -> { }
        int arrDim = arrInfo.arraySize.size();
        int totalSize = 1;
        for (int i = 0; i < arrDim - level; i++) {
            totalSize *= arrInfo.arraySize[arrDim - i - 1];
        }
        // 对齐
        curPtr += totalSize;
        return symbolTableInfo{ARRAY};
    } else {    // InitVal -> { InitValList }
        InitVal(node->children[1], arrInfo, curPtr, initValues, level + 1);
        InitValList(node->children[2], arrInfo, curPtr, initValues, level + 1);
        int arrDim = arrInfo.arraySize.size();
        int totalSize = 1;
        for (int i = 0; i < arrDim - level; i++) {
            totalSize *= arrInfo.arraySize[arrDim - i - 1];
        }
        // 对齐
        if (curPtr % totalSize != 0) {
            curPtr += totalSize - curPtr % totalSize;
        }
        return symbolTableInfo{ARRAY};
    }
}

void InitValList(Node node, const symbolTableInfo& arrayInfo, int& curPtr, vector<pair<int, bool> >& initValues, int level) {
    if (node == nullptr) return;
    dumpNode(node);
    // InitValList -> InitVal , InitValList
    InitVal(node->children[1], arrayInfo, curPtr, initValues, level);
    InitValList(node->children[2], arrayInfo, curPtr, initValues, level);
}

void FuncDef(Node node) {
    if (node == nullptr) return;
    dumpNode(node);

    string funcName = node->children[1]->data.str;
    int size = node->children.size();
    if (node->children[0]->isTerminal) {    // FuncDef -> VOID IDENT ( FuncFParams ) Block

        symbolTable.back()[funcName] = symbolTableInfo{VOID, vector<DeclType>()};
        symbolTable.push_back(unordered_map<string, symbolTableInfo>());

        ir.push_back("FUNCTION " + funcName + ":");

        if (size == 5) {
            Block(node->children[4], VOID);
        } else {
            FuncFParams(node->children[3], funcName);
            Block(node->children[5], VOID);
        }
        symbolTable.pop_back();
    } else {    // FuncDef -> BType IDENT ( FuncFParams ) Block
        BType(node->children[0]);

        symbolTable.back()[funcName] = symbolTableInfo{INT, vector<DeclType>()};
        symbolTable.push_back(unordered_map<string, symbolTableInfo>());

        ir.push_back("FUNCTION " + funcName + ":");

        if (size == 5) {
            Block(node->children[4], INT);
        } else {
            FuncFParams(node->children[3], funcName);
            Block(node->children[5], INT);
        }

        symbolTable.pop_back();
    }
}

void FuncFParams(Node node, string funcName) {
    if (node == nullptr) return;
    dumpNode(node);
    // FuncFParams -> FuncFParam FuncFParamList
    FuncFParam(node->children[0], funcName);
    FuncFParamList(node->children[1], funcName);
}

void FuncFParamList(Node node, string funcName) {
    if (node == nullptr) return;
    dumpNode(node);
    // FuncFParamList -> , FuncFParam FuncFParamList
    FuncFParam(node->children[1], funcName);
    FuncFParamList(node->children[2], funcName);
}

void FuncFParam(Node node, string funcName) {
    if (node == nullptr) return;
    dumpNode(node);
    // FuncFParam -> BType IDENT
    DeclType type = BType(node->children[0]);
    string varName = node->children[1]->data.str;

    symbolTable.back()[varName] = symbolTableInfo{type, vector<DeclType>(), vector<int>(), tempVarCnt, false, 0};
    tempVarCnt ++;
    symbolTable[0][funcName].params.push_back(type);

    string tempName = "t" + to_string(symbolTable.back()[varName].tempIndex);
    ir.push_back("PARAM " + tempName);

    // FuncFParam -> BType IDENT [] IntConstList
    if (node->children.size() == 5) {
        symbolTable.back()[varName].type = ARRAY;
        symbolTable[0][funcName].params.back() = ARRAY;
        symbolTable.back()[varName].arraySize.push_back(0);
        IntConstList(node->children[4], varName);
    }
}

void Block(Node node, const DeclType& returnType) {
    if (node == nullptr) return;
    dumpNode(node);
    BlockItemList(node->children[1], returnType);
}

void BlockItemList(Node node, const DeclType& returnType) {
    if (node == nullptr) return;
    dumpNode(node);
    BlockItem(node->children[0], returnType);
    BlockItemList(node->children[1], returnType);
}

void BlockItem(Node node, const DeclType& returnType) {
    if (node == nullptr) return;
    dumpNode(node);    
    if (node->children[0]->name == "Decl") {
        Decl(node->children[0], false);
    } else {
        Stmt(node->children[0], returnType);
    }
}

void Stmt(Node node, const DeclType& returnType) {
    if (node == nullptr) return;
    dumpNode(node);

    if (node->children[0]->name == "LVal") {    // Stmt -> LVal = Exp ;
        auto type1 = LVal(node->children[0]);
        auto type2 = Exp(node->children[2]);

        if (type1.isAddr) { // 数组/全局变量的赋值
            if (type2.isConst) {
                int tNew = tempVarCnt++;
                ir.push_back("t" + to_string(tNew) + " = #" + to_string(type2.constValue));
                ir.push_back("*t" + to_string(type1.tempIndex) + " = t" + to_string(tNew));
            } else if (type2.isAddr) {
                int tNew = tempVarCnt++;
                ir.push_back("t" + to_string(tNew) + " = *t" + to_string(type2.tempIndex));
                ir.push_back("*t" + to_string(type1.tempIndex) + " = t" + to_string(tNew));
            } else {
                ir.push_back("*t" + to_string(type1.tempIndex) + " = t" + to_string(type2.tempIndex));
            }
            return;
        }


        if (type2.isConst) {
            ir.push_back("t" + to_string(type1.tempIndex) + " = #" + to_string(type2.constValue));
        } else if (type2.isAddr) {
            int tNew = tempVarCnt++;
            ir.push_back("t" + to_string(tNew) + " = *t" + to_string(type2.tempIndex));
            ir.push_back("t" + to_string(type1.tempIndex) + " = t" + to_string(tNew));
        } else {
            ir.push_back("t" + to_string(type1.tempIndex) + " = t" + to_string(type2.tempIndex));
        }
        return;
    } 

    if (node->children[0]->name == "SEMICOLON") {   // Stmt -> ;
        return;
    }

    if (node->children[0]->name == "Exp") { // Stmt -> Exp ;
        Exp(node->children[0]);
        return;
    }

    if (node->children[0]->name == "Block") {  // Stmt -> Block
        symbolTable.push_back(unordered_map<string, symbolTableInfo>());
        Block(node->children[0], returnType);
        symbolTable.pop_back();
        return;
    }

    int size = node->children.size();
    if (node->children[0]->name == "IF" && size == 5) {   // Stmt -> IF ( Cond ) Stmt
        int labelFirst = labelCnt;
        labelCnt ++;
        int labelSecond = labelCnt;
        labelCnt ++;
        Cond(node->children[2], labelFirst, labelSecond);
        ir.push_back("LABEL label" + to_string(labelFirst) + ":");
        Stmt(node->children[4], returnType);
        ir.push_back("LABEL label" + to_string(labelSecond) + ":");
        return;
    }

    if (node->children[0]->name == "IF" && size == 7) {  // Stmt -> IF ( Cond ) Stmt ELSE Stmt
        int labelFirst = labelCnt;
        labelCnt ++;
        int labelSecond = labelCnt;
        labelCnt ++;
        int labelThird = labelCnt;
        labelCnt ++;
        Cond(node->children[2],labelFirst, labelSecond);
        ir.push_back("LABEL label" + to_string(labelFirst) + ":");
        Stmt(node->children[4], returnType);
        ir.push_back("GOTO label" + to_string(labelThird));
        ir.push_back("LABEL label" + to_string(labelSecond) + ":");
        Stmt(node->children[6], returnType);
        ir.push_back("LABEL label" + to_string(labelThird) + ":");
        return;
    }

    if (node->children[0]->name == "WHILE") {   // Stmt -> WHILE ( Cond ) Stmt
        int firstLabel = labelCnt;
        labelCnt ++;
        int secondLabel = labelCnt;
        labelCnt ++;
        int thirdLabel = labelCnt;
        labelCnt ++;
        ir.push_back("LABEL label" + to_string(firstLabel) + ":");
        Cond(node->children[2], secondLabel, thirdLabel);
        ir.push_back("LABEL label" + to_string(secondLabel) + ":");
        Stmt(node->children[4], returnType);
        ir.push_back("GOTO label" + to_string(firstLabel));
        ir.push_back("LABEL label" + to_string(thirdLabel) + ":");
        return;
    }

    if (node->children[0]->name == "RETURN" && size == 2) { // Stmt -> RETURN ;
        ir.push_back("RETURN");
        return;
    }

    if (node->children[0]->name == "RETURN" && size == 3) { // Stmt -> RETURN Exp ;
        auto info = Exp(node->children[1]);
        if (info.isConst) {
            ir.push_back("t" + to_string(tempVarCnt) + " = #" + to_string(info.constValue));
            ir.push_back("RETURN t" + to_string(tempVarCnt));
            tempVarCnt ++;
        } else {
            ir.push_back("RETURN t" + to_string(info.tempIndex));
        }
        return;
    }
}

symbolTableInfo Exp(Node node) {
    dumpNode(node);
    return AddExp(node->children[0]);
}

void Cond(Node node, const int& trueLabel, const int& falseLabel) {
    dumpNode(node);
    LOrExp(node->children[0], trueLabel, falseLabel);
}

symbolTableInfo LVal(Node node) {
    dumpNode(node);

    // LVal -> IDENT ExpList
    string varName = node->children[0]->data.str;
    symbolTableInfo info = FindVar(varName);
    auto info2 = ExpList(node->children[1], info, (int)info.arraySize.size());  // 计算偏移量
    
    if (info2.tempIndex != -1 && info.type == ARRAY) {   // 数组的赋值到temp
        auto newTemp = symbolTableInfo{};
        newTemp.type = INT;
        newTemp.tempIndex = tempVarCnt;
        tempVarCnt ++;

        if (info.isGlobal) {    // 全局数组要先取地址
            // tAddr = &t_label
            int tAddr = tempVarCnt++;
            ir.push_back("t" + to_string(tAddr) + " = &t" + to_string(info.tempIndex));
            // tOff = t_off * 4
            int tOff = tempVarCnt++;
            ir.push_back("t" + to_string(tOff) + " = t" + to_string(info2.tempIndex) + " * #4");
            // t_new = tAddr + tOff
            ir.push_back("t" + to_string(newTemp.tempIndex) + " = t" + to_string(tAddr) + " + t" + to_string(tOff));
            newTemp.isAddr = true;
            return newTemp;
        } 
        // 非全局数组直接取值
        // tOff = t_off * 4
        int tOff = tempVarCnt++;
        ir.push_back("t" + to_string(tOff) + " = t" + to_string(info2.tempIndex) + " * #4");
        // t_new = t_addr + tOff
        ir.push_back("t" + to_string(newTemp.tempIndex) + " = t" + to_string(info.tempIndex) + " + t" + to_string(tOff));
        newTemp.isAddr = true;
        newTemp.isArrParam = info2.isArrParam;
        return newTemp;
    }

    // 全局变量取地址
    if (info.isGlobal) {
        auto newTemp = symbolTableInfo{};
        newTemp.type = INT;
        newTemp.tempIndex = tempVarCnt;
        tempVarCnt ++;
        // t_new = &t_label
        ir.push_back("t" + to_string(newTemp.tempIndex) + " = &t" + to_string(info.tempIndex));
        newTemp.isAddr = true;
        newTemp.isArrParam = info2.isArrParam;
        return newTemp;
    }

    info.isArrParam = info2.isArrParam;
    return info;
}

symbolTableInfo ExpList(Node node, const symbolTableInfo& info, const int level) {
    if (node == nullptr) {
        auto newTemp = symbolTableInfo{};
        newTemp.type = INT;
        newTemp.isConst = true;
        newTemp.constValue = 0;
        newTemp.tempIndex = -1;
        if (info.type == ARRAY && level > 0) {
            newTemp.isArrParam = true;
        }
        return newTemp;
    }
    dumpNode(node);
    // ExpList -> [ Exp ] ExpList
    auto info2 = Exp(node->children[1]);
    if (info2.isAddr) {
        int newTemp = tempVarCnt++;
        ir.push_back("t" + to_string(newTemp) + " = *t" + to_string(info2.tempIndex));
        ir.push_back("t" + to_string(info2.tempIndex) + " = t" + to_string(newTemp));
    }
    auto info3 = ExpList(node->children[3], info, level - 1);

    // 计算偏移量 :
    // a[4][4][4]
    // [2][2][2] = 2 * 4 * 4 + 2 * 4 + 2
    auto newTemp = symbolTableInfo{};
    newTemp.type = INT;
    newTemp.tempIndex = tempVarCnt;
    tempVarCnt ++;
    if (info2.isConst) {
        // tLast = t_info2
        int tLast = tempVarCnt++;
        ir.push_back("t" + to_string(tLast) + " = #" + to_string(info2.constValue));  
        int arrSize = info.arraySize.size();
        for (int i = 1; i <= level - 1; i++) {
            // tNew = tLast * #info[arrSize - i]
            // tLast = tNew
            int tNew = tempVarCnt++;
            ir.push_back("t" + to_string(tNew) + " = t" + to_string(tLast) + " * #" + to_string(info.arraySize[arrSize-i]));
            tLast = tNew;
        }
        if (info3.isConst) {
            // t_new = tLast + #info3
            ir.push_back("t" + to_string(newTemp.tempIndex) + " = t" + to_string(tLast) + " + #" + to_string(info3.constValue));
        } else {
            // t_new = tLast + t_info3
            ir.push_back("t" + to_string(newTemp.tempIndex) + " = t" + to_string(tLast) + " + t" + to_string(info3.tempIndex));
        }
    } else {
        // tLast = t_info2
        int tLast = tempVarCnt++;
        ir.push_back("t" + to_string(tLast) + " = t" + to_string(info2.tempIndex));
        int arrSize = info.arraySize.size();
        for (int i = 1; i <= level - 1; i++) {
            // tNew = tLast * #info[arrSize - i]
            // tLast = tNew
            int tNew = tempVarCnt++;
            ir.push_back("t" + to_string(tNew) + " = t" + to_string(tLast) + " * #" + to_string(info.arraySize[arrSize-i]));
            tLast = tNew;
        }
        if (info3.isConst) {
            // t_new = tLast + #info3
            ir.push_back("t" + to_string(newTemp.tempIndex) + " = t" + to_string(tLast) + " + #" + to_string(info3.constValue));
        } else {
            // t_new = tLast + t_info3
            ir.push_back("t" + to_string(newTemp.tempIndex) + " = t" + to_string(tLast) + " + t" + to_string(info3.tempIndex));
        }
    }
    newTemp.isArrParam = info3.isArrParam;
    return newTemp;
}

symbolTableInfo PrimaryExp(Node node) {
    dumpNode(node);
    if (node->children[0]->name == "LPAREN") {  // PrimaryExp -> ( Exp )
        auto type = Exp(node->children[1]);
        return type;
    } 

    if (node->children[0]->name == "LVal") {    // PrimaryExp -> LVal
        auto type = LVal(node->children[0]);
        type.isConst = false;
        return type;
    }
    // PrimaryExp -> NUMBER
    Number(node->children[0]);
    auto type = symbolTableInfo{};
    type.type = INT;
    type.isConst = true;
    type.constValue = node->children[0]->children[0]->data.num;
    return type;
}

void Number(Node node) {
    dumpNode(node);
}

symbolTableInfo UnaryExp(Node node) {
    dumpNode(node);
    int sz = node->children.size();
    if (sz == 1) {  // UnaryExp -> PrimaryExp
        auto type = PrimaryExp(node->children[0]);
        return type;
    } 

    if (sz == 3) {  // UnaryExp -> IDENT （ ）
        string varName = node->children[0]->data.str;
        auto funcInfo = FindVar(varName);
        if (funcInfo.type == VOID) {
            ir.push_back("CALL " + varName);
            return symbolTableInfo{INT};
        }
        auto info = symbolTableInfo{};
        info.tempIndex = tempVarCnt;
        tempVarCnt ++;
        ir.push_back("t" + to_string(info.tempIndex) + " = CALL " + varName);
        return info;
    }

    if (sz == 4) {  // UnaryExp -> IDENT （ FuncRParams ）
        string varName = node->children[0]->data.str;
        auto funcInfo = FindVar(varName);
        if (funcInfo.type == VOID) {
            FuncRParams(node->children[2], funcInfo);
            ir.push_back("CALL " + varName);
            return symbolTableInfo{INT};
        }
        auto info = symbolTableInfo{};
        info.tempIndex = tempVarCnt;
        tempVarCnt ++;
        FuncRParams(node->children[2], info);
        ir.push_back("t" + to_string(info.tempIndex) + " = CALL " + varName);
        return info;
    }
    // UnaryExp -> UnaryOp UnaryExp
    auto op = node->children[0]->children[0]->name;
    auto type = UnaryExp(node->children[1]);

    if (type.isAddr) {
        auto newTemp = symbolTableInfo{};
        newTemp.type = INT;
        newTemp.tempIndex = tempVarCnt++;
        if (op == "!") {
            newTemp.isNot = !type.isNot;
        }
        ir.push_back("t" + to_string(newTemp.tempIndex) + " = *t" + to_string(type.tempIndex));
        return newTemp;
    }



    if (op == "!") {
        type.isNot = !type.isNot;
        return type;
    }
    auto newTemp = symbolTableInfo{};
    newTemp.type = INT;
    newTemp.tempIndex = tempVarCnt;
    tempVarCnt ++;
    if (type.isConst) {
        auto zeroTemp = tempVarCnt;
        tempVarCnt ++;
        ir.push_back("t" + to_string(zeroTemp) + " = #0");
        ir.push_back("t" + to_string(newTemp.tempIndex) + " = " + "t" + to_string(zeroTemp) + " " + op + " #" + to_string(type.constValue));
    } else {
        ir.push_back("t" + to_string(newTemp.tempIndex) + " = " + op + " t" + to_string(type.tempIndex));
    }
    return newTemp;
}

void UnaryOp(Node node) {
    dumpNode(node);
}

void FuncRParams(Node node, const symbolTableInfo& info) {
    dumpNode(node);
    // FuncRParams -> Exp ExpList2
    // 如果传入数组参数，还是回到LVal判断
    auto type = Exp(node->children[0]);

    if (type.isArrParam) {  // 函数传参，且传入参数是数组的指针
        ir.push_back("ARG t" + to_string(type.tempIndex));
        ExpList2(node->children[1], info);
        return;
    }

    if (type.isAddr) {  // 函数传参是a[1]这种形式
        int newTemp = tempVarCnt;
        tempVarCnt ++;
        ir.push_back("t" + to_string(newTemp) + " = *t" + to_string(type.tempIndex));
        ir.push_back("ARG t" + to_string(newTemp));
    } else if (type.isConst) {
        int newTemp = tempVarCnt;
        tempVarCnt ++;
        ir.push_back("t" + to_string(newTemp) + " = #" + to_string(type.constValue));
        ir.push_back("ARG t" + to_string(newTemp));
    } else { 
        ir.push_back("ARG t" + to_string(type.tempIndex));
    }
    ExpList2(node->children[1], info);
}

void ExpList2(Node node, const symbolTableInfo& info) {
    if (node == nullptr) {
        return;
    }
    dumpNode(node);
    // ExpList2 -> , Exp ExpList2
    auto type = Exp(node->children[1]);
    if (type.isConst) {
        int newTemp = tempVarCnt;
        tempVarCnt ++;
        ir.push_back("t" + to_string(newTemp) + " = #" + to_string(type.constValue));
        ir.push_back("ARG t" + to_string(newTemp));
    } else {
        ir.push_back("ARG t" + to_string(type.tempIndex));
    }
    ExpList2(node->children[2], info);
    return;
}

symbolTableInfo MulExp(Node node) {
    dumpNode(node);
    if (node->children[0]->name == "UnaryExp") {    // MulExp -> UnaryExp
        auto type = UnaryExp(node->children[0]);
        return type;
    }
    // MulExp -> MulExp */DIV/% UnaryExp
    auto type1 = MulExp(node->children[0]);
    auto op = node->children[1]->name;
    auto type2 = UnaryExp(node->children[2]);



    auto newTemp = symbolTableInfo{};
    newTemp.type = INT;
    if (type1.isConst && type2.isConst) {
        int constVal;
        if (op == "*") {
            constVal = type1.constValue * type2.constValue;
        } else if (op == "/") {
            constVal = type1.constValue / type2.constValue;
        } else {
            constVal = type1.constValue % type2.constValue;
        }
        newTemp.isConst = true;
        newTemp.constValue = constVal;
        return newTemp;
    } else if (type1.isConst) {
        newTemp.tempIndex = tempVarCnt;
        tempVarCnt ++;

        if (type2.isAddr) {
            // tVal是地址
            int tVal = tempVarCnt++;
            ir.push_back("t" + to_string(tVal) + " = *t" + to_string(type2.tempIndex));
            ir.push_back("t" + to_string(newTemp.tempIndex) + " = t" + to_string(tVal) + " " + op + " #" + to_string(type1.constValue));
            return newTemp;
        }

        ir.push_back("t" + to_string(newTemp.tempIndex) + " = t" + to_string(type2.tempIndex) + " " + op + " #" + to_string(type1.constValue));
    } else if (type2.isConst) {
        newTemp.tempIndex = tempVarCnt;
        tempVarCnt ++;

        if (type1.isAddr) {
            // tVal是地址
            int tVal = tempVarCnt++;
            ir.push_back("t" + to_string(tVal) + " = *t" + to_string(type1.tempIndex));
            ir.push_back("t" + to_string(newTemp.tempIndex) + " = t" + to_string(tVal) + " " + op + " #" + to_string(type2.constValue));
            return newTemp;
        }

        ir.push_back("t" + to_string(newTemp.tempIndex) + " = t" + to_string(type1.tempIndex) + " " + op + " #" + to_string(type2.constValue));
    } else {
        newTemp.tempIndex = tempVarCnt;
        tempVarCnt ++;

        if (type1.isAddr && type2.isAddr) {
            int tVal1 = tempVarCnt++;
            int tVal2 = tempVarCnt++;
            ir.push_back("t" + to_string(tVal1) + " = *t" + to_string(type1.tempIndex));
            ir.push_back("t" + to_string(tVal2) + " = *t" + to_string(type2.tempIndex));
            ir.push_back("t" + to_string(newTemp.tempIndex) + " = t" + to_string(tVal1) + " " + op + " t" + to_string(tVal2));
            return newTemp;
        } 

        if (type1.isAddr) {
            int tVal = tempVarCnt++;
            ir.push_back("t" + to_string(tVal) + " = *t" + to_string(type1.tempIndex));
            ir.push_back("t" + to_string(newTemp.tempIndex) + " = t" + to_string(tVal) + " " + op + " t" + to_string(type2.tempIndex));
            return newTemp;
        }

        if (type2.isAddr) {
            int tVal = tempVarCnt++;
            ir.push_back("t" + to_string(tVal) + " = *t" + to_string(type2.tempIndex));
            ir.push_back("t" + to_string(newTemp.tempIndex) + " = t" + to_string(type1.tempIndex) + " " + op + " t" + to_string(tVal));
            return newTemp;
        }

        ir.push_back("t" + to_string(newTemp.tempIndex) + " = t" + to_string(type1.tempIndex) + " " + op + " t" + to_string(type2.tempIndex));
    }

    return newTemp;
}

symbolTableInfo AddExp(Node node) {
    dumpNode(node);
    if (node->children[0]->name == "MulExp") { // AddExp -> MulExp
        auto type = MulExp(node->children[0]);
        return type;
    } 
    // AddExp -> AddExp +/- MulExp
    auto type1 = AddExp(node->children[0]);
    auto op = node->children[1]->name;
    auto type2 = MulExp(node->children[2]);
    auto newTemp = symbolTableInfo{};

    newTemp.type = INT;
    if (type1.isConst && type2.isConst) {
        int constVal;
        if (op == "+") {
            constVal = type1.constValue + type2.constValue;
        } else {
            constVal = type1.constValue - type2.constValue;
        } 
        newTemp.isConst = true;
        newTemp.constValue = constVal;
        return newTemp;
    } else if (type1.isConst) {
        newTemp.tempIndex = tempVarCnt;
        tempVarCnt ++;

        if (type2.isAddr) {
            // tVal是地址
            int tVal = tempVarCnt++;
            ir.push_back("t" + to_string(tVal) + " = *t" + to_string(type2.tempIndex));
            ir.push_back("t" + to_string(newTemp.tempIndex) + " = t" + to_string(tVal) + " " + op + " #" + to_string(type1.constValue));
            return newTemp;
        }

        ir.push_back("t" + to_string(newTemp.tempIndex) + " = t" + to_string(type2.tempIndex) + " " + op + " #" + to_string(type1.constValue));
    } else if (type2.isConst) {
        newTemp.tempIndex = tempVarCnt;
        tempVarCnt ++;

        if (type1.isAddr) {
            // tVal是地址
            int tVal = tempVarCnt++;
            ir.push_back("t" + to_string(tVal) + " = *t" + to_string(type1.tempIndex));
            ir.push_back("t" + to_string(newTemp.tempIndex) + " = t" + to_string(tVal) + " " + op + " #" + to_string(type2.constValue));
            return newTemp;
        }

        ir.push_back("t" + to_string(newTemp.tempIndex) + " = t" + to_string(type1.tempIndex) + " " + op + " #" + to_string(type2.constValue));
    } else {
        newTemp.tempIndex = tempVarCnt;
        tempVarCnt ++;

        if (type1.isAddr && type2.isAddr) {
            int tVal1 = tempVarCnt++;
            int tVal2 = tempVarCnt++;
            ir.push_back("t" + to_string(tVal1) + " = *t" + to_string(type1.tempIndex));
            ir.push_back("t" + to_string(tVal2) + " = *t" + to_string(type2.tempIndex));
            ir.push_back("t" + to_string(newTemp.tempIndex) + " = t" + to_string(tVal1) + " " + op + " t" + to_string(tVal2));
            return newTemp;
        }

        if (type1.isAddr) {
            int tVal = tempVarCnt++;
            ir.push_back("t" + to_string(tVal) + " = *t" + to_string(type1.tempIndex));
            ir.push_back("t" + to_string(newTemp.tempIndex) + " = t" + to_string(tVal) + " " + op + " t" + to_string(type2.tempIndex));
            return newTemp;
        }

        if (type2.isAddr) {
            int tVal = tempVarCnt++;
            ir.push_back("t" + to_string(tVal) + " = *t" + to_string(type2.tempIndex));
            ir.push_back("t" + to_string(newTemp.tempIndex) + " = t" + to_string(type1.tempIndex) + " " + op + " t" + to_string(tVal));
            return newTemp;
        }

        ir.push_back("t" + to_string(newTemp.tempIndex) + " = t" + to_string(type1.tempIndex) + " " + op + " t" + to_string(type2.tempIndex));
    }
    return newTemp;
}

symbolTableInfo RelExp(Node node, const int& trueLabel, const int& falseLabel) {
    dumpNode(node);
    if (node->children[0]->name == "AddExp") {  // RelExp -> AddExp
        auto type = AddExp(node->children[0]);
        return type;
    }
    // RelExp -> RelExp </<=/>/>= AddExp
    auto op = node->children[1]->name;
    auto type1 = RelExp(node->children[0], trueLabel, falseLabel);
    auto type2 = AddExp(node->children[2]);

    if (type1.isConst && type2.isConst) {
        int tempVar1 = tempVarCnt;
        tempVarCnt ++;
        int tempVar2 = tempVarCnt;
        tempVarCnt ++;
        ir.push_back("t" + to_string(tempVar1) + " = #" + to_string(type1.constValue));
        ir.push_back("t" + to_string(tempVar2) + " = #" + to_string(type2.constValue));
        ir.push_back("IF t" + to_string(tempVar1) + " " + op + " t" + to_string(tempVar2) + " GOTO label" + to_string(trueLabel));
    } else if (type1.isConst) {
        int tempVar = tempVarCnt;
        tempVarCnt ++;

        if (type2.isAddr) {
            int tVal = tempVarCnt++;
            ir.push_back("t" + to_string(tVal) + " = *t" + to_string(type2.tempIndex));
            ir.push_back("t" + to_string(tempVar) + " = #" + to_string(type1.constValue));
            ir.push_back("IF t" + to_string(tempVar) + " " + op + " t" + to_string(tVal) + " GOTO label" + to_string(trueLabel));
            ir.push_back("GOTO label" + to_string(falseLabel));
            return type1;
        }

        ir.push_back("t" + to_string(tempVar) + " = #" + to_string(type1.constValue));
        ir.push_back("IF t" + to_string(tempVar) + " " + op + " t" + to_string(type2.tempIndex) + " GOTO label" + to_string(trueLabel));
    } else if (type2.isConst) {
        int tempVar = tempVarCnt;
        tempVarCnt ++;

        if (type1.isAddr) {
            int tVal = tempVarCnt++;
            ir.push_back("t" + to_string(tVal) + " = *t" + to_string(type1.tempIndex));
            ir.push_back("t" + to_string(tempVar) + " = #" + to_string(type2.constValue));
            ir.push_back("IF t" + to_string(tVal) + " " + op + " t" + to_string(tempVar) + " GOTO label" + to_string(trueLabel));
            ir.push_back("GOTO label" + to_string(falseLabel));
            return type1;
        }

        ir.push_back("t" + to_string(tempVar) + " = #" + to_string(type2.constValue));
        ir.push_back("IF t" + to_string(type1.tempIndex) + " " + op + " t" + to_string(tempVar) + " GOTO label" + to_string(trueLabel));
    } else {

        if (type1.isAddr && type2.isAddr) {
            int tVal1 = tempVarCnt++;
            int tVal2 = tempVarCnt++;
            ir.push_back("t" + to_string(tVal1) + " = *t" + to_string(type1.tempIndex));
            ir.push_back("t" + to_string(tVal2) + " = *t" + to_string(type2.tempIndex));
            ir.push_back("IF t" + to_string(tVal1) + " " + op + " t" + to_string(tVal2) + " GOTO label" + to_string(trueLabel));
            ir.push_back("GOTO label" + to_string(falseLabel));
            return type1;
        }

        if (type1.isAddr) {
            int tVal = tempVarCnt++;
            ir.push_back("t" + to_string(tVal) + " = *t" + to_string(type1.tempIndex));
            ir.push_back("IF t" + to_string(tVal) + " " + op + " t" + to_string(type2.tempIndex) + " GOTO label" + to_string(trueLabel));
            ir.push_back("GOTO label" + to_string(falseLabel));
            return type1;
        }

        if (type2.isAddr) {
            int tVal = tempVarCnt++;
            ir.push_back("t" + to_string(tVal) + " = *t" + to_string(type2.tempIndex));
            ir.push_back("IF t" + to_string(type1.tempIndex) + " " + op + " t" + to_string(tVal) + " GOTO label" + to_string(trueLabel));
            ir.push_back("GOTO label" + to_string(falseLabel));
            return type1;
        }

        ir.push_back("IF t" + to_string(type1.tempIndex) + " " + op + " t" + to_string(type2.tempIndex) + " GOTO label" + to_string(trueLabel));
    }

    ir.push_back("GOTO label" + to_string(falseLabel));
    return type1;
}

symbolTableInfo EqExp(Node node, const int& trueLabel, const int& falseLabel) {
    dumpNode(node);
    if (node->children[0]->name == "RelExp") {  // EqExp -> RelExp
        auto type = RelExp(node->children[0], trueLabel, falseLabel);
        return type;
    }
    // EqExp -> EqExp ==/!= RelExp
    auto op = node->children[1]->name;
    auto type1 = EqExp(node->children[0], trueLabel, falseLabel);
    auto type2 = RelExp(node->children[2], trueLabel, falseLabel);

    if (type1.isConst && type2.isConst) {
        int tempVar1 = tempVarCnt;
        tempVarCnt ++;
        int tempVar2 = tempVarCnt;
        tempVarCnt ++;
        ir.push_back("t" + to_string(tempVar1) + " = #" + to_string(type1.constValue));
        ir.push_back("t" + to_string(tempVar2) + " = #" + to_string(type2.constValue));
        ir.push_back("IF t" + to_string(tempVar1) + " " + op + " t" + to_string(tempVar2) + " GOTO label" + to_string(trueLabel));
    } else if (type1.isConst) {
        int tempVar = tempVarCnt;
        tempVarCnt ++;

        if (type2.isAddr) {
            int tVal = tempVarCnt++;
            ir.push_back("t" + to_string(tVal) + " = *t" + to_string(type2.tempIndex));
            ir.push_back("t" + to_string(tempVar) + " = #" + to_string(type1.constValue));
            ir.push_back("IF t" + to_string(tempVar) + " " + op + " t" + to_string(tVal) + " GOTO label" + to_string(trueLabel));
            ir.push_back("GOTO label" + to_string(falseLabel));
            return type1;
        }

        ir.push_back("t" + to_string(tempVar) + " = #" + to_string(type1.constValue));
        ir.push_back("IF t" + to_string(tempVar) + " " + op + " t" + to_string(type2.tempIndex) + " GOTO label" + to_string(trueLabel));
    } else if (type2.isConst) {
        int tempVar = tempVarCnt;
        tempVarCnt ++;

        if (type1.isAddr) {
            int tVal = tempVarCnt++;
            ir.push_back("t" + to_string(tVal) + " = *t" + to_string(type1.tempIndex));
            ir.push_back("t" + to_string(tempVar) + " = #" + to_string(type2.constValue));
            ir.push_back("IF t" + to_string(tVal) + " " + op + " t" + to_string(tempVar) + " GOTO label" + to_string(trueLabel));
            ir.push_back("GOTO label" + to_string(falseLabel));
            return type1;
        }

        ir.push_back("t" + to_string(tempVar) + " = #" + to_string(type2.constValue));
        ir.push_back("IF t" + to_string(type1.tempIndex) + " " + op + " t" + to_string(tempVar) + " GOTO label" + to_string(trueLabel));
    } else {

        if (type1.isAddr && type2.isAddr) {
            int tVal1 = tempVarCnt++;
            int tVal2 = tempVarCnt++;
            ir.push_back("t" + to_string(tVal1) + " = *t" + to_string(type1.tempIndex));
            ir.push_back("t" + to_string(tVal2) + " = *t" + to_string(type2.tempIndex));
            ir.push_back("IF t" + to_string(tVal1) + " " + op + " t" + to_string(tVal2) + " GOTO label" + to_string(trueLabel));
            ir.push_back("GOTO label" + to_string(falseLabel));
            return type1;
        }

        if (type1.isAddr) {
            int tVal = tempVarCnt++;
            ir.push_back("t" + to_string(tVal) + " = *t" + to_string(type1.tempIndex));
            ir.push_back("IF t" + to_string(tVal) + " " + op + " t" + to_string(type2.tempIndex) + " GOTO label" + to_string(trueLabel));
            ir.push_back("GOTO label" + to_string(falseLabel));
            return type1;
        }

        if (type2.isAddr) {
            int tVal = tempVarCnt++;
            ir.push_back("t" + to_string(tVal) + " = *t" + to_string(type2.tempIndex));
            ir.push_back("IF t" + to_string(type1.tempIndex) + " " + op + " t" + to_string(tVal) + " GOTO label" + to_string(trueLabel));
            ir.push_back("GOTO label" + to_string(falseLabel));
            return type1;
        }

        ir.push_back("IF t" + to_string(type1.tempIndex) + " " + op + " t" + to_string(type2.tempIndex) + " GOTO label" + to_string(trueLabel));
    }
    ir.push_back("GOTO label" + to_string(falseLabel));
    return type1;
}

symbolTableInfo LAndExp(Node node, const int& trueLabel, const int& falseLabel) {
    dumpNode(node);
    if (node->children[0]->name == "EqExp") {   // LAndExp -> EqExp
        auto type = EqExp(node->children[0], trueLabel, falseLabel);
        return type;
    }
    // LAndExp -> LAndExp && EqExp
    auto labelFirst = labelCnt;
    labelCnt ++;
    auto type1 = LAndExp(node->children[0], labelFirst, falseLabel);
    ir.push_back("LABEL label" + to_string(labelFirst) + ":");
    auto type2 = EqExp(node->children[2], trueLabel, falseLabel);

    return type1;
}

symbolTableInfo LOrExp(Node node, const int& trueLabel, const int& falseLabel) {
    dumpNode(node);
    if (node->children[0]->name == "LAndExp") { // LOrExp -> LAndExp
        auto type = LAndExp(node->children[0], trueLabel, falseLabel);

        auto node1 = node->children[0]->children[0];
        auto node2 = node1->children[0];
        auto node3 = node2->children[0];
        if (node1->name == "EqExp" && node2->name == "RelExp" && node3->name == "AddExp") {
            int zeroTemp = tempVarCnt;
            tempVarCnt ++;
            ir.push_back("t" + to_string(zeroTemp) + " = #0");
            int goto1 = trueLabel;
            int goto2 = falseLabel;
            if (type.isNot) {
                goto1 = falseLabel;
                goto2 = trueLabel;
            } 
            if (type.isConst) {
                int tempVar = tempVarCnt;
                tempVarCnt ++;
                ir.push_back("t" + to_string(tempVar) + " = #" + to_string(type.constValue));
                ir.push_back("IF t" + to_string(tempVar) + " != t" + to_string(zeroTemp) + " GOTO label" + to_string(goto1));
                ir.push_back("GOTO label" + to_string(goto2));
            } else if (type.isAddr) {
                int tVal = tempVarCnt++;
                ir.push_back("t" + to_string(tVal) + " = *t" + to_string(type.tempIndex));
                ir.push_back("IF t" + to_string(tVal) + " != t" + to_string(zeroTemp) + " GOTO label" + to_string(goto1));
                ir.push_back("GOTO label" + to_string(goto2));
            } else {
                ir.push_back("IF t" + to_string(type.tempIndex) + " != t" + to_string(zeroTemp) + " GOTO label" + to_string(goto1));
                ir.push_back("GOTO label" + to_string(goto2));
            }

            return type;
        }

        return type;
    }
    // LOrExp -> LOrExp || LAndExp
    auto labelFirst = labelCnt;
    labelCnt ++;
    auto type1 = LOrExp(node->children[0], trueLabel, labelFirst);
    ir.push_back("LABEL label" + to_string(labelFirst) + ":");
    auto type2 = LAndExp(node->children[2], trueLabel, falseLabel);

    return type1;
}