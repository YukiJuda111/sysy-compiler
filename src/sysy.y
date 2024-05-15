%code requires {
    #include "AST.h"
}
%{
#include "AST.h"

void yyerror(const char *s);
extern int yylex(void);
extern Node root;
%}


%define parse.error verbose
%locations

%union {
    Node node;
}
%type <node> Start
%type <node> CompUnit Decl BType VarDecl VarList 
%type <node> VarDef IntConstList InitVal InitValList 
%type <node> FuncDef FuncFParams FuncFParam FuncFParamList 
%type <node> Block BlockItemList BlockItem Stmt Exp Cond 
%type <node> LVal ExpList PrimaryExp Number UnaryExp UnaryOp 
%type <node> FuncRParams ExpList2 MulExp AddExp RelExp EqExp 
%type <node> LAndExp LOrExp
%token <node> INT_TYPE VOID_TYPE IF ELSE WHILE RETURN
%token <node> IDENT INTCONST
%token <node> EQ_OP NE_OP LE_OP GE_OP AND_OP OR_OP LT_OP GT_OP
%token <node> LBRACKET RBRACKET LPAREN RPAREN LBRACE RBRACE SEMI COMMA ASSIGN
%token <node> ADD_OP SUB_OP MUL_OP DIV_OP MOD_OP NOT_OP
%left AND_OP OR_OP
%left EQ_OP NE_OP
%left LT_OP GT_OP LE_OP GE_OP
%left ADD_OP SUB_OP
%left MUL_OP DIV_OP MOD_OP
%left ELSE

%start Start

%%

Start       : CompUnit
            {
                $$ = new AstNode("Start", {$1});
                root = $$;
            }
            ;


CompUnit    : /* empty */
            {
                $$ = nullptr;
            }
            | CompUnit Decl  
            {
                $$ = new AstNode("CompUnit", {$1, $2});
            }      
            | CompUnit FuncDef  
            {
                $$ = new AstNode("CompUnit", {$1, $2});
            }    
            ;


Decl        : VarDecl
            {
                $$ = new AstNode("Decl", {$1});
            }              
            ;                       

BType       : INT_TYPE
            {
                $$ = new AstNode("BType", {$1});
            }
            ;

VarDecl     : BType VarDef VarList SEMI
            {
                $$ = new AstNode("VarDecl", {$1, $2, $3, $4});
            }
            ;

VarList     : /* empty */
            {
                $$ = nullptr;
            }
            | COMMA VarDef VarList
            {
                $$ = new AstNode("VarList", {$1, $2, $3});
            }
            ;

VarDef      : IDENT IntConstList
            {
                $$ = new AstNode("VarDef", {$1, $2});
            }
            | IDENT IntConstList ASSIGN InitVal
            {
                $$ = new AstNode("VarDef", {$1, $2, $3, $4});
            }
            ;

IntConstList: /* empty */
            {
                $$ = nullptr;
            }
            | LBRACKET INTCONST RBRACKET IntConstList
            {
                $$ = new AstNode("IntConstList", {$1, $2, $3, $4});
            }
            ;
            
InitVal     : Exp
            {
                $$ = new AstNode("InitVal", {$1});
            }
            | LBRACE RBRACE
            {
                $$ = new AstNode("InitVal", {$1, $2});
            }
            | LBRACE InitVal InitValList RBRACE
            {
                $$ = new AstNode("InitVal", {$1, $2, $3, $4});
            }
            ;

InitValList : /* empty */
            {
                $$ = nullptr;
            }
            | COMMA InitVal InitValList
            {
                $$ = new AstNode("InitValList", {$1, $2, $3});
            }
            ;

FuncDef     : BType IDENT LPAREN RPAREN Block
            {
                $$ = new AstNode("FuncDef", {$1, $2, $3, $4, $5});
            }
            | BType IDENT LPAREN FuncFParams RPAREN Block
            {
                $$ = new AstNode("FuncDef", {$1, $2, $3, $4, $5, $6});
            }
            | VOID_TYPE IDENT LPAREN RPAREN Block
            {
                $$ = new AstNode("FuncDef", {$1, $2, $3, $4, $5});
            }
            | VOID_TYPE IDENT LPAREN FuncFParams RPAREN Block
            {
                $$ = new AstNode("FuncDef", {$1, $2, $3, $4, $5, $6});
            }
            ;

FuncFParams : FuncFParam FuncFParamList
            {
                $$ = new AstNode("FuncFParams", {$1, $2});
            }
            ;

FuncFParamList : /* empty */
            {
                $$ = nullptr;
            }
            | COMMA FuncFParam FuncFParamList
            {
                $$ = new AstNode("FuncFParamList", {$1, $2, $3});
            }
            ;

FuncFParam  : BType IDENT
            {
                $$ = new AstNode("FuncFParam", {$1, $2});
            }
            | BType IDENT LBRACKET RBRACKET IntConstList
            {
                $$ = new AstNode("FuncFParam", {$1, $2, $3, $4, $5});
            }
            ;

Block       : LBRACE BlockItemList RBRACE
            {
                $$ = new AstNode("Block", {$1, $2, $3});
            }
            ;

BlockItemList : /* empty */
            {
                $$ = nullptr;
            }
            | BlockItem BlockItemList
            {
                $$ = new AstNode("BlockItemList", {$1, $2});
            }
            ;

BlockItem   : Decl 
            {
                $$ = new AstNode("BlockItem", {$1});
            }
            | Stmt
            {
                $$ = new AstNode("BlockItem", {$1});
            }
            ;

Stmt        : LVal ASSIGN Exp SEMI
            {
                $$ = new AstNode("Stmt", {$1, $2, $3, $4});
            }
            | SEMI
            {
                $$ = new AstNode("Stmt", {$1});
            }
            | Exp SEMI
            {
                $$ = new AstNode("Stmt", {$1, $2});
            }
            | Block
            {
                $$ = new AstNode("Stmt", {$1});
            }
            | IF LPAREN Cond RPAREN Stmt %prec ADD_OP   
            {
                $$ = new AstNode("Stmt", {$1, $2, $3, $4, $5});
            }
            | IF LPAREN Cond RPAREN Stmt ELSE Stmt
            {
                $$ = new AstNode("Stmt", {$1, $2, $3, $4, $5, $6, $7});
            }
            | WHILE LPAREN Cond RPAREN Stmt
            {
                $$ = new AstNode("Stmt", {$1, $2, $3, $4, $5});
            }
            | RETURN SEMI
            {
                $$ = new AstNode("Stmt", {$1, $2});
            }
            | RETURN Exp SEMI
            {
                $$ = new AstNode("Stmt", {$1, $2, $3});
            }
            ;

Exp         : AddExp
            {
                $$ = new AstNode("Exp", {$1});
            }
            ;

Cond        : LOrExp
            {
                $$ = new AstNode("Cond", {$1});
            }
            ;

LVal        : IDENT ExpList
            {
                $$ = new AstNode("LVal", {$1, $2});
            }
            ;

ExpList     : /* empty */
            {
                $$ = nullptr;
            }
            | LBRACKET Exp RBRACKET ExpList
            {
                $$ = new AstNode("ExpList", {$1, $2, $3, $4});
            }
            ;

PrimaryExp  : LPAREN Exp RPAREN
            {
                $$ = new AstNode("PrimaryExp", {$1, $2, $3});
            }
            | LVal
            {
                $$ = new AstNode("PrimaryExp", {$1});
            }
            | Number
            {
                $$ = new AstNode("PrimaryExp", {$1});
            }
            ;

Number      : INTCONST
            {
                $$ = new AstNode("Number", {$1});
            }
            ;

UnaryExp    : PrimaryExp
            {
                $$ = new AstNode("UnaryExp", {$1});
            }
            | IDENT LPAREN RPAREN
            {
                $$ = new AstNode("UnaryExp", {$1, $2, $3});
            }
            | IDENT LPAREN FuncRParams RPAREN
            {
                $$ = new AstNode("UnaryExp", {$1, $2, $3, $4});
            }
            | UnaryOp UnaryExp
            {
                $$ = new AstNode("UnaryExp", {$1, $2});
            }
            ;

UnaryOp     : ADD_OP
            {
                $$ = new AstNode("UnaryOp", {$1});
            }
            | SUB_OP
            {
                $$ = new AstNode("UnaryOp", {$1});
            }
            | NOT_OP
            {
                $$ = new AstNode("UnaryOp", {$1});
            }
            ;

FuncRParams : Exp ExpList2
            {
                $$ = new AstNode("FuncRParams", {$1, $2});
            }   
            ;

ExpList2    : /* empty */
            {
                $$ = nullptr;
            }
            | COMMA Exp ExpList2
            {
                $$ = new AstNode("ExpList2", {$1, $2, $3});
            }
            ;

MulExp      : UnaryExp
            {
                $$ = new AstNode("MulExp", {$1});
            }
            | MulExp MUL_OP UnaryExp
            {
                $$ = new AstNode("MulExp", {$1, $2, $3});
            }
            | MulExp DIV_OP UnaryExp
            {
                $$ = new AstNode("MulExp", {$1, $2, $3});
            }
            | MulExp MOD_OP UnaryExp
            {
                $$ = new AstNode("MulExp", {$1, $2, $3});
            }
            ;

AddExp      : MulExp
            {
                $$ = new AstNode("AddExp", {$1});
            }
            | AddExp ADD_OP MulExp
            {
                $$ = new AstNode("AddExp", {$1, $2, $3});
            }
            | AddExp SUB_OP MulExp
            {
                $$ = new AstNode("AddExp", {$1, $2, $3});
            }
            ;

RelExp      : AddExp
            {
                $$ = new AstNode("RelExp", {$1});
            }
            | RelExp LT_OP AddExp
            {
                $$ = new AstNode("RelExp", {$1, $2, $3});
            }
            | RelExp GT_OP AddExp
            {
                $$ = new AstNode("RelExp", {$1, $2, $3});
            }
            | RelExp LE_OP AddExp
            {
                $$ = new AstNode("RelExp", {$1, $2, $3});
            }
            | RelExp GE_OP AddExp
            {
                $$ = new AstNode("RelExp", {$1, $2, $3});
            }
            ;           

EqExp       : RelExp
            {
                $$ = new AstNode("EqExp", {$1});
            }
            | EqExp EQ_OP RelExp
            {
                $$ = new AstNode("EqExp", {$1, $2, $3});
            }
            | EqExp NE_OP RelExp
            {
                $$ = new AstNode("EqExp", {$1, $2, $3});
            }
            ;

LAndExp     : EqExp
            {
                $$ = new AstNode("LAndExp", {$1});
            }
            | LAndExp AND_OP EqExp
            {
                $$ = new AstNode("LAndExp", {$1, $2, $3});
            }
            ;

LOrExp      : LAndExp   
            {
                $$ = new AstNode("LOrExp", {$1});
            }
            | LOrExp OR_OP LAndExp
            {
                $$ = new AstNode("LOrExp", {$1, $2, $3});
            }
            ;
%%

void yyerror(const char *s) {
    fprintf(stderr, "Parse error :  %s\n", s);
}
