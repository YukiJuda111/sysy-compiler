#include "AST.h"
#include "semant.h"
#include "translate.h"
extern int yyparse();
extern FILE* yyin;
Node root = nullptr;
int main(int argc, char** argv) {
  if (argc > 1) {
    yyin = fopen(argv[1], "r");
    if (!yyin) {
      printf("Cannot open file %s\n", argv[1]);
      return 1;
    }
  }
  int err = yyparse();
  
  fclose(yyin);
  if (err) {
    printf("Parse failed\n");
    return err;
  }
  // 语义检查
  if(root) {
    semant_Start(root);
  }
  // 生成IR
  if (root) {
    Start(root);
  }

  FILE* output = fopen(argv[2], "w");
  if (!output) {
    printf("Cannot open file %s\n", argv[2]);
    return 1;
  }

  printFileIr(output);

  return 0;
}