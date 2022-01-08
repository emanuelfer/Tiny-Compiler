/****************************************************/
/* File: cgen.c                                     */
/* The code generator implementation                */
/* for the TINY compiler                            */
/* (generates code for the TM machine)              */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "symtab.h"
#include "code.h"
#include "cgen.h"

/* tmpOffset is the memory offset for temps
   It is decremented each time a temp is
   stored, and incremeted when loaded again
*/
static int tmpOffset = 0;

/* prototype for internal recursive code generator */
static void cGen(TreeNode *tree);

/*
função que gera código para instruções de sentença como read, write, if, repeat e atribuição
*/
static void genStmt(TreeNode *tree)
{
   // declara 3 ponteiros para os nós filhos
   TreeNode *p1, *p2, *p3, *curCase;
   int savedLoc1, savedLoc2, currentLoc, jmpToNextLoc, lastPos;
   // declara 3 variáveis inteiras savedLoc1,savedLoc2,currentLoc 
   // servem para guarda posições de geração de código, guardam o local para onde deve ser feito o salto
   int loc;
   switch (tree->kind.stmt) // função com o tipo de cada nó
   {
   case SwitchK:
      if (TraceCode)
         emitComment("-> switch");
      p1 = tree->child[0];
      p2 = tree->child[1];
      //get the variable
      cGen(p1);
      //generate Cases
      cGen(p2);
      break; /* switch_k */
   case CaseK:
      curCase = tree;

      if (TraceCode)
         emitComment("-> ");
      emitRM("LDA", ac1, 0, ac, "tentando colocar o valor de ac em ac1");
      //loop que gera os cases
      do
      {
         p1 = curCase->child[0];
         p2 = curCase->child[1];
         /* get constant */

         cGen(p1);

         emitRO("SUB", ac, ac1, ac, "op ==");    //subtrai AC de AC1
         emitRM("JEQ", ac, 1, pc, "br if true"); //PULA O PROXIMO COMANDO SE O RESULTADO FOR 0
         jmpToNextLoc = emitSkip(1);             //pula 1 linha pra deixar espaço pro jmp que leva pro proximo case

         cGen(p2);                                                             //gera os statements
         lastPos = emitSkip(0);                                                //salva ultima posição
         emitBackup(jmpToNextLoc);                                             //volta pra local do jmp pro proximo case
         emitRM("LDA", pc, (lastPos - jmpToNextLoc), pc, "unconditional jmp"); //pula pra posição do proximo case
         emitRestore();

         curCase = curCase->sibling;
      } while (curCase != NULL);

      break; /* switch_k */
   case IfK:
      if (TraceCode)
         emitComment("-> if");
      p1 = tree->child[0];
      p2 = tree->child[1];
      p3 = tree->child[2];
      /* generate code for test expression */
      cGen(p1);
      savedLoc1 = emitSkip(1);
      emitComment("if: jump to else belongs here");
      /* recurse on then part */
      cGen(p2);
      savedLoc2 = emitSkip(1);
      emitComment("if: jump to end belongs here");
      currentLoc = emitSkip(0);
      emitBackup(savedLoc1);
      emitRM_Abs("JEQ", ac, currentLoc, "if: jmp to else");
      emitRestore();
      /* recurse on else part */
      cGen(p3);
      currentLoc = emitSkip(0);
      emitBackup(savedLoc2);
      emitRM_Abs("LDA", pc, currentLoc, "jmp to end");
      emitRestore();
      if (TraceCode)
         emitComment("<- if");
      break; /* if_k */

   case RepeatK:
      if (TraceCode)
         emitComment("-> repeat");
      // cria dois ponteiros para os filhos do repeat
      p1 = tree->child[0]; // para instruções do corpo do repeat
      p2 = tree->child[1]; // para instruções da expressão do repeat
      savedLoc1 = emitSkip(0); // salva a instrução da primeira posição do corpo do repeat
      emitComment("repeat: jump after body comes back here");
      cGen(p1); // chama para gerar código para todo o corpo do repeat
      cGen(p2); // chama para gerar código para a expressão de controle
      /* emite a instrução JEQ (jump if equal) que serve para fazer um salto para a posição armazenada em saveLock1
         que é o inicio do corpo do repeat
         O salto é dado se o registrador ac que é o registrador de número 0, que no caso 
         da geração de código para expressões lógicas, vai conter o resultado da avaliação dessa expressão
         v = 1 e F = 0
         O PC faz um salto para a posição savelock1 se o conteúdo de AC for == 0 que corresponde a falso
      */
      emitRM_Abs("JEQ", ac, savedLoc1, "repeat: jmp back to body");
      if (TraceCode)
         emitComment("<- repeat");
      break; /* repeat */

   case WhileK:
      if (TraceCode) emitComment("-> while") ;
      p1 = tree->child[0] ; // para instrução da expressão do while
      p2 = tree->child[1] ; // para instruções do corpo do while
      // gera o código para a expressão
      cGen(p1);
      // verifica se pode entrar no corpo do while
      emitRM_Abs("JEQ",ac,savedLoc1,"while: entra no corpo");
      savedLoc1 = emitSkip(0); // salva a instrução da primeira posição do corpo do while
      cGen(p2); // gera código para o corpo
      emitComment("while: sai do corpo"); // emite um comentário
      if (TraceCode)  emitComment("<- while") ;
      break;
   case AssignK:
      if (TraceCode)
         emitComment("-> assign");
      /* generate code for rhs */
      cGen(tree->child[0]);
      /* now store value */
      loc = st_lookup(tree->attr.name);
      emitRM("ST", ac, loc, gp, "assign: store value");
      if (TraceCode)
         emitComment("<- assign");
      break; /* assign_k */

   case ReadK:
      emitRO("IN", ac, 0, 0, "read integer value");
      loc = st_lookup(tree->attr.name);
      emitRM("ST", ac, loc, gp, "read: store value");
      break;
   case WriteK:
      /* generate code for expression to write */
      cGen(tree->child[0]);
      /* now output it */
      emitRO("OUT", ac, 0, 0, "write ac");
      break;
   default:
      break;
   }
} /* genStmt */

/* Procedure genExp generates code at an expression node */
static void genExp(TreeNode *tree)
{
   int loc;
   TreeNode *p1, *p2;
   switch (tree->kind.exp)
   {

   case ConstK:
      if (TraceCode)
         emitComment("-> Const");
      /* gen code to load integer constant using LDC */
      emitRM("LDC", ac, tree->attr.val, 0, "load const");
      if (TraceCode)
         emitComment("<- Const");
      break; /* ConstK */

   case IdK:
      if (TraceCode)
         emitComment("-> Id");
      loc = st_lookup(tree->attr.name);
      emitRM("LD", ac, loc, gp, "load id value");
      if (TraceCode)
         emitComment("<- Id");
      break; /* IdK */

   case OpK:
      if (TraceCode)
         emitComment("-> Op");
      p1 = tree->child[0];
      p2 = tree->child[1];
      /* gen code for ac = left arg */
      cGen(p1);
      /* gen code to push left operand */
      emitRM("ST", ac, tmpOffset--, mp, "op: push left");
      /* gen code for ac = right operand */
      cGen(p2);
      /* now load left operand */
      emitRM("LD", ac1, ++tmpOffset, mp, "op: load left");
      switch (tree->attr.op)
      {
      case PLUS:
         emitRO("ADD", ac, ac1, ac, "op +");
         break;
      case MINUS:
         emitRO("SUB", ac, ac1, ac, "op -");
         break;
      case TIMES:
         emitRO("MUL", ac, ac1, ac, "op *");
         break;
      case OVER:
         emitRO("DIV", ac, ac1, ac, "op /");
         break;
      case LT:
         emitRO("SUB", ac, ac1, ac, "op <");
         emitRM("JLT", ac, 2, pc, "br if true");
         emitRM("LDC", ac, 0, ac, "false case");
         emitRM("LDA", pc, 1, pc, "unconditional jmp");
         emitRM("LDC", ac, 1, ac, "true case");
         break;
      case EQ:
         emitRO("SUB", ac, ac1, ac, "op ==");
         emitRM("JEQ", ac, 2, pc, "br if true");
         emitRM("LDC", ac, 0, ac, "false case");
         emitRM("LDA", pc, 1, pc, "unconditional jmp");
         emitRM("LDC", ac, 1, ac, "true case");
         break;
      default:
         emitComment("BUG: Unknown operator");
         break;
      } /* case op */
      if (TraceCode)
         emitComment("<- Op");
      break; /* OpK */

   default:
      break;
   }
} /* genExp */

/* gera recursivamente código pelo percurso na árvore
 */
static void cGen(TreeNode *tree)
{
   if (tree != NULL)
   {
      switch (tree->nodekind)
      {
      case StmtK: // se for um nó de sentença
         genStmt(tree); // chama essa função para gerar código para esse nó
         break;
      case ExpK:  // se for expressão
         genExp(tree);
         break;
      default:
         break;
      }
      cGen(tree->sibling); // chama recursivamente cgen passando o nó irmão como argumento
   }
}

/**********************************************/
/* the primary function of the code generator */
/**********************************************/
/* Procedure codeGen generates code to a code
 * file by traversal of the syntax tree. The
 * second parameter (codefile) is the file name
 * of the code file, and is used to print the
 * file name as a comment in the code file
 */
void codeGen(TreeNode *syntaxTree, char *codefile)
{
   char *s = malloc(strlen(codefile) + 7);
   strcpy(s, "File: ");
   strcat(s, codefile);
   emitComment("TINY Compilation to TM Code");
   emitComment(s);
   /* generate standard prelude */
   emitComment("Standard prelude:");
   emitRM("LD", mp, 0, ac, "load maxaddress from location 0");
   emitRM("ST", ac, 0, ac, "clear location 0");
   emitComment("End of standard prelude.");
   /* generate code for TINY program */
   cGen(syntaxTree);
   /* finish */
   emitComment("End of execution.");
   emitRO("HALT", 0, 0, 0, "");
}
