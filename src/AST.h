#pragma once
#include <iostream>
#include <string>
#include <vector>
using namespace std;
enum DATATYPE { DEC, OCT, HEX, ID, TYPEDECL, NOTCARE };


class AstNode;
typedef AstNode* Node;
class AstNode {
public:
    string name;        
    bool isTerminal;          
    enum DATATYPE datatype;  
    struct data_ {                  
        int num;
        string str;
    } data;
    vector<Node> children;  

    // terminal的默认构造函数
    AstNode(const string& name, DATATYPE datatype, const string& val)
        : name(name), isTerminal(true), datatype(datatype)
    {
        if (!val.empty()) {
            switch (datatype) {
                case ID:
                    data.str = val;
                    break;
                case TYPEDECL:
                    data.str = val;
                    break;
                case DEC:
                    data.num = std::stoul(val);
                    break;
                case OCT:
                    data.num = std::stoul(val, nullptr, 8);
                    datatype = DEC;
                    break;
                case HEX:
                    data.num = std::stoul(val, nullptr, 16);
                    datatype = DEC;
                    break;
                default:
                    break;
            }
        }
    }

    // non-erminal的默认构造函数
    //Node mynode = new AstNode("example", 1, {child1, child2, child3});
    AstNode(const string& name, std::initializer_list<Node> childs)
        : name(name), isTerminal(false), children(childs) {}
            

    void dumpTree(int depth = 0) {
        for (int i = 0; i < depth; i++) {
            cout << "  ";
        }
        cout << name;
        if (isTerminal) {
            switch (datatype) {
                case ID:
                    cout << ": " << data.str;
                    break;
                case TYPEDECL:
                    cout << ": " << data.str;
                    break;
                case DEC:
                    cout << ": " << data.num;
                    break;
                default:
                    break;
            }
        } else {
            cout << "(non-terminal)";
        }
        cout << endl;
        if (!isTerminal) {
            for (std::vector<Node>::size_type i = 0; i < children.size(); i++) {
                if(children[i]) children[i]->dumpTree(depth + 1);
            }
        }
    }
};

