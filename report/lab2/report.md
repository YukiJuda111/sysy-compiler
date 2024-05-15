# lab2实验报告
## 编译及运行
```shell
cd path_to_SP24-STARTER
make
python3 test.py ./compiler lab2
```

## 实现功能
- AST
    
    通过递归的方式构建AST，每个节点都有一个`vector<Node> children`成员变量，用于存储子节点

- 符号表

    通过cpp的stl实现hashmap
    ```cpp
    vector<unordered_map<string, symbolTableInfo>> symbolTable;
    ```
    每次在调用`Block()`时，创建一个新的符号表，Block结束时，删除该符号表
    ```cpp
    symbolTable.push_back(unordered_map<string, symbolTableInfo>());
    ...
    Block();
    ...
    symbolTable.pop_back();
    ```
    

- 重复定义检查

    通过在符号表中查找是否已经定义过

- 类型检查

    1. 变量定义或赋值中左右表达式类型不匹配
        
        在`VarDef`和`Stmt`两个节点检查左右表达式返回的类型，如果不匹配则报错

    2. 操作数类型不匹配或操作数类型与操作符不匹配，如整型变量与数组变量相加减
        
        在`AddExp` `MulExp`...等节点检查操作数类型是否匹配，如果不匹配则报错


    3. return 语句的返回类型与函数定义的返回类型不匹配
     
        在`Stmt`的return分支做类型检查，对于后续语法解析为`Stmt->RETURN`的节点，返回类型为`void`，对于`Stmt->RETURN Exp`的节点，返回类型为`int`

    5. 函数调用时实参与形参的数目或类型不匹配; 特别的，函数调用时数组对应维数不匹配
        
        在符号表的第0层中存储函数的参数信息`vector<DeclType> params`，每次调用函数时，检查实参与形参的数目是否匹配，以及数组对应维数是否匹配

    6. 对非数组型变量使用“[...]”(数组访问)操作符
 
    7. 对普通变量使用“(...)”或“()”(函数调用)操作符，也可以认为是调用未定义的函数

- 数组初始化
    
    用一个hashmap统计数组初始化列表每层的大小: `unordered_map<int, int> initMap`，在遍历AST调用`InitVal`时，将初始化列表的层数作为参数传入，hashmap的映射即为：`层数level -> 该层的大小cnt`，如果初始化列表的大小超过了数组的大小；则报错,如果初始化列表的大小小于数组的大小，则用0填充

## 程序亮点
- 用hashmap来做数组初始化列表的大小统计

- 符号表和数组初始化列表的hashmap采用了cpp的`unordered_map`，查找速度快