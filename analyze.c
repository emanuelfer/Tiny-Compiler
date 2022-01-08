/****************************************************/
/* File: analyze.c                                  */
/* Semantic analyzer implementation                 */
/* for the TINY compiler                            */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "symtab.h"
#include "analyze.h"

/* counter for variable memory locations */
static int location = 0;

/* Procedure traverse is a generic recursive
 * syntax tree traversal routine:
 * it applies preProc in preorder and postProc
 * in postorder to tree pointed to by t
 */
static void traverse(TreeNode *t,
                     void (*preProc)(TreeNode *),
                     void (*postProc)(TreeNode *))
{
  if (t != NULL)
  {
    preProc(t);
    {
      int i;
      for (i = 0; i < MAXCHILDREN; i++)
        traverse(t->child[i], preProc, postProc);
    }
    postProc(t);
    traverse(t->sibling, preProc, postProc);
  }
}

/* não faz nada apenas serve para ser 
utilizada como argumento da função traverse, 
quando colocada no segundo argumento indica que a árvore 
não terá uma função para percorre-la em pre-ordem
e quando usada como terceiro argumento, significa 
que não será percorrida em pós-ordem.
 */
static void nullProc(TreeNode *t)
{
  if (t == NULL)
    return;
  else
    return;
}

/*insere identificadores que estejam em 
nós na árvore na tabela de símbolos
 */
static void insertNode(TreeNode *t)
{
  switch (t->nodekind)
  {
  case StmtK:
    switch (t->kind.stmt)
    {
    case AssignK:
    case ReadK:
      if (st_lookup(t->attr.name) == -1)// se o identificador ainda não estiver na TS
           // Insere o identificador pela linha abaixo 
        st_insert(t->attr.name, t->lineno, location++);
        // insere o nome da variável, o número da linha que a variável está e 
        // a localização da variável na tabela de simbolos
      else // se já estiver na tabela
        // adiciona o número da linha onde a variável está aparecendo novamente no código fonte
        st_insert(t->attr.name, t->lineno, 0);
      break;
    default:
      break;
    }
    break;
  case ExpK:
    switch (t->kind.exp)
    {
    case IdK:
      if (st_lookup(t->attr.name) == -1)
        /* not yet in table, so treat as new definition */
        st_insert(t->attr.name, t->lineno, location++);
      else
        /* already in table, so ignore location,
             add line number of use only */
        st_insert(t->attr.name, t->lineno, 0);
      break;
    default:
      break;
    }
    break;
  default:
    break;
  }
}

/* constroi a tabela de símbolos de acordo com 
a árvore sintática passada
 */
void buildSymtab(TreeNode *syntaxTree)
{
  traverse(syntaxTree, insertNode, nullProc);// percore a arvore síntática fazendo a inserção de identificadores na tabela de simbolos
// posssui três argumentos arvore sintática, função insertNode (insere na TS as informações sobre os nós de identificadores), nullProc nada é feito 
  if (TraceAnalyze)
  {
    fprintf(listing, "\nSymbol table:\n\n");
    printSymTab(listing);
  }
}

/* Emite uma mensagem de erro semantico indicando o número da linha e 
seta a variável erro como true para signigficar que há um erro semantico
*/
static void typeError(TreeNode *t, char *message)
{
  fprintf(listing, "Type error at line %d: %s\n", t->lineno, message);
  Error = TRUE;
}

/* Faz a checagem de tipo a cada nó da arvore
 */
static void checkNode(TreeNode *t)
{
  switch (t->nodekind)
  {
  case ExpK:
    switch (t->kind.exp)
    {
    case OpK:
      // se o nó é do tipo operador verifica se os filhos são do tipo integer
      if ((t->child[0]->type != Integer) ||
          (t->child[1]->type != Integer))
        typeError(t, "Op applied to non-integer");
      if ((t->attr.op == EQ) || (t->attr.op == LT)) //verifica se o operador é < ou =
        t->type = Boolean; // se for a expressão á boleana
      else
        t->type = Integer; // se não inteira
      break;
    case ConstK:
    case IdK:
      t->type = Integer;
      break;
    default:
      break;
    }
    break;
  case StmtK:
    switch (t->kind.stmt)
    {
    case IfK:
      if (t->child[0]->type == Integer)
        typeError(t->child[0], "if test is not Boolean");
      break;
    case AssignK:
      if (t->child[0]->type != Integer)
        typeError(t->child[0], "assignment of non-integer value");
      break;
    case WriteK:
      if (t->child[0]->type != Integer)
        typeError(t->child[0], "write of non-integer value");
      break;
    case RepeatK:
      if (t->child[1]->type == Integer)
        typeError(t->child[1], "repeat test is not Boolean");
      break;
    case WhileK:
      if (t->child[1]->type == Integer)
        typeError(t->child[1], "while test is not Boolean");
      break;
    default:
      break;
    }
    break;
  default:
    break;
  }
}

/* faz a verificação de tipos em relação a arvore sintatica que recebe como argumento
 */
void typeCheck(TreeNode *syntaxTree)
{
  traverse(syntaxTree, nullProc, checkNode);  // traverse aplica a função checkNode sobre cada nó enquanto percorre a árvore
}
