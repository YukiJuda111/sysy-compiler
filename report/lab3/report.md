# lab3 IR生成
3210105488 刘扬

## 1 编译及测试
```bash
make
python3 test.py ./compiler lab3
```

## 2 实现功能
#### ir存储
通过链表分别存储全局变量ir和其他ir
```cpp
list<string> globalIr;
list<string> ir;
```

#### 表达式代码产生
分别在`AddExp`,`MulExp`,`UnaryExp`等函数中产生代码，大致实现思路如下：
```
以AddExp1 -> AddExp2 + MulExp为例:
1. 在AddExp1节点调用AddExp2节点，AddExp2会递归地产生自己的IR
2. 读取运算符+
3. 在AddExp1节点调用MulExp节点，MulExp会递归地产生自己的IR
4. 根据AddExp2和MulExp的返回值获取临时变量t1和t2
5. 产生IR： t3 = t1 + t2
(当然数组等特殊情况需要特殊处理，这里只是简单的例子)
```
#### 语句代码产生
参考了实验文档的做法，大致思路如下：
```
以 Stmt -> IF Cond Stmt1 ELSE Stmt2为例，其他做法完全按照实验文档中的表格翻译成我的框架下的代码：
1. 产生两个label，label_true和label_false
2. 在Cond节点调用Stmt1节点，Stmt1会递归地产生自己的IR
3. 产生IR：LABEL label_true
4. 在Cond节点调用Stmt2节点，Stmt2会递归地产生自己的IR
5. 产生IR：LABEL label_false
```

#### 条件判断语句产生
 参考了实验文档的做法，大致思路如下：
```
以 RelExp1 -> RelExp2 < AddExp为例，其他做法完全按照实验文档中的表格翻译成我的框架下的代码：
1. 在RelExp1节点调用RelExp2节点，RelExp2会递归地产生自己的IR
2. 读取运算符<
3. 在RelExp1节点调用AddExp节点，AddExp会递归地产生自己的IR
4. 根据RelExp2和AddExp的返回值获取临时变量t1和t2
5. 根据传入的参数label_true和label_false产生IR：
    5.1 IF t1 < t2 GOTO label_true
    5.2 GOTO label_false
（同样需要特殊处理数组等特殊情况）
```

#### 数组和全局变量
首先需要在之前所有的表达式产生和条件判断产生中加入对数组的处理，这里的处理比较复杂，需要考虑的情况包括但不限于：
- 数组在是左值还是右值，这可以在`Stmt`和`VarDef`阶段判断出来，如果出现在等号左边则是左值，否则是右值
- 对于左值而言，需要得到数组的地址，对于右值而言，需要得到数组的值          
- 数组作为参数传递给函数时，要看函数需要的参数是指针还是值，如果是指针则传递数组的地址，否则传递数组的值
- 全局变量需要用`&`来取地址

## 3 程序亮点
#### 数组初始化
```
1.通过lab2的符号表统计数组的大小，根据数组的大小初始化一个vector<int> initValues, initValues先全部初始化为0
2.在InitValue节点，有三种情况：
    2.1 InitValue -> Exp: 将Exp的值赋给initValues
    2.2 InitValue -> { InitValueList }: 递归地处理InitValueList，同时在最后根据当前的维数插入0
    2.3 InitValue -> {}: 根据当前的维数插入0
```

#### 偏移量计算
```
ExpList1 -> [Exp] ExpList2:
1. 在ExpList1节点统计当前所处维数：d
2. 得到Exp的值：v
3. 递归地调用ExpList2节点，得到ExpList2的值：t
4. 通过传入的SymbolTable中的数组大小信息，比如数组是三维的，每个维度的大小分别是a,b,c，而当前维数d = 2
5. 返回: t + v * b * c + v * c
```


#### 判断数组是作为指针传递还是作为值传递
在ExpList中统计了数组的维数，如果递归的次数少于数组的维数，则说明数组是作为指针传递，否则是作为值传递

#### `!`运算符的处理
`!`就是将判断条件取反，我在SymbolTable中维护了一个bool变量`isNot`，如果`isNot`为true，则说明当前的判断条件需要取反。

每次在UnaryExp中遇到`!`，就执行一次`isNot = !isNot`。
