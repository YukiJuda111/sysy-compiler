%option noinput
%option nounput
%option noyywrap

%{
#include "sysy.tab.hh"
#include "AST.h"
void yyerror(const char *s);
void addTerminal(string name, enum DATATYPE datatype, string val);
%}

%%

"int"             { addTerminal("TYPE", TYPEDECL, "int"); return INT_TYPE;}
"void"            { addTerminal("TYPE", TYPEDECL, "void"); return VOID_TYPE; }
"if"              { addTerminal("IF", NOTCARE, ""); return IF; }
"else"            { addTerminal("ELSE", NOTCARE, ""); return ELSE; }
"while"           { addTerminal("WHILE", NOTCARE, ""); return WHILE; }
"return"          { addTerminal("RETURN", NOTCARE, ""); return RETURN; }
"=="              { addTerminal("==", NOTCARE, ""); return EQ_OP; }
"!="              { addTerminal("!=", NOTCARE, ""); return NE_OP;}
"<="              { addTerminal("<=", NOTCARE, ""); return LE_OP; }
">="              { addTerminal(">=", NOTCARE, ""); return GE_OP;}
"<"               { addTerminal("<", NOTCARE, ""); return LT_OP;}
">"               { addTerminal(">", NOTCARE, ""); return GT_OP;}
"&&"              { addTerminal("&&", NOTCARE, ""); return AND_OP;}
"||"              { addTerminal("||", NOTCARE, ""); return OR_OP;}
"["               { addTerminal("LBRACKET", NOTCARE, ""); return LBRACKET;}
"]"               { addTerminal("RBRACKET", NOTCARE, ""); return RBRACKET;}
"("               { addTerminal("LPAREN", NOTCARE, ""); return LPAREN;}
")"               { addTerminal("RPAREN", NOTCARE, ""); return RPAREN;}
"{"               { addTerminal("LBRACE", NOTCARE, ""); return LBRACE;}
"}"               { addTerminal("RBRACE", NOTCARE, ""); return RBRACE;}
";"               { addTerminal("SEMICOLON", NOTCARE, ""); return SEMI;}
","               { addTerminal("COMMA", NOTCARE, ""); return COMMA;}
"="               { addTerminal("ASSIGN", NOTCARE, ""); return ASSIGN;}
"+"               { addTerminal("+", NOTCARE, ""); return ADD_OP; }
"-"               { addTerminal("-", NOTCARE, ""); return SUB_OP; }
"%"               { addTerminal("%", NOTCARE, ""); return MOD_OP; }
"!"               { addTerminal("!", NOTCARE, ""); return NOT_OP; }
"*"               { addTerminal("*", NOTCARE, ""); return MUL_OP; }
"/"               { addTerminal("/", NOTCARE, ""); return DIV_OP; }              

0[xX][0-9a-fA-F]+  { addTerminal("INT", HEX, yytext); return INTCONST; }  // 匹配十六进制整数
0[0-7]+            { addTerminal("INT", OCT, yytext); return INTCONST;}  // 匹配八进制整数
[0-9]+             { addTerminal("INT", DEC, yytext); return INTCONST; }  // 匹配十进制整数
[a-zA-Z_][a-zA-Z0-9_]*  { addTerminal("ID", ID, yytext); return IDENT;}
[ \t]             ; // 忽略空格和制表符


"//".*            ;
"/*"([^*]*|(\*+[^/]))*"*/"  ;
.                 {yyerror("something goes wrong...");} 

%%

void addTerminal(string name, enum DATATYPE datatype, string val)
{
    yylval.node = new AstNode(name, datatype, val);
}
