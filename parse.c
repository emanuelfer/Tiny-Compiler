/****************************************************/
/* File: parse.c                                    */
/* The parser implementation for the TINY compiler  */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/
#include "globals.h"
#include "util.h"
#include "scan.h"
#include "parse.h"

static TokenType token; /* holds current token */

// protótipo de todas as funções que compoem o analisador sintático
// ca da função corresponte a um símbolo variável da gramática
static TreeNode *stmt_sequence(void);
static TreeNode *statement(void);
static TreeNode *if_stmt(void);
static TreeNode *repeat_stmt(void);
static TreeNode *while_stmt(void);
static TreeNode *assign_stmt(void);
static TreeNode *read_stmt(void);
static TreeNode *write_stmt(void);
static TreeNode *exp(void);
static TreeNode *simple_exp(void);
static TreeNode *term(void);
static TreeNode *factor(void);

static void syntaxError(char *message)
{
  fprintf(listing, "\n>>> ");
  fprintf(listing, "Syntax error at line %d: %s", lineno, message);
  Error = TRUE;
}

// verifica se o token que foi retornado pelo análizador léxico corresponde ao token esperado naquele momento da análise
static void match(TokenType expected)
{
  if (token == expected)
    token = getToken();
  else
  {
    syntaxError("Unexpected token -> ");
    printToken(token, tokenString);
    fprintf(listing, "      ");
  }
}

// função que corresponde ao símbolo inicial da gramática
TreeNode *stmt_sequence(void)
{
  TreeNode *t = statement();
  TreeNode *p = t;
  match(SEMI);
  while ((token != ENDFILE) && (token != ENDIF) && (token != ENDWHILE) &&
         (token != ELSE) && (token != UNTIL) && (token != CASE) && (token != ENDSWITCH))
  {
    TreeNode *q;
    q = statement();
    if (q != NULL)
    {
      if (t == NULL)
        t = p = q;
      else /* now p cannot be NULL either */
      {
        p->sibling = q;
        p = q;
      }
    }
    match(SEMI);
  }
  return t;
}

// encontra qual é o tipo de declaração báseado no token que foi lido e nos primeiros token esperados para cada tipo 
// de declaração
TreeNode *statement(void)
{
  TreeNode *t = NULL;
  switch (token)
  {
  case IF:
    t = if_stmt();
    break;
  case REPEAT:
    t = repeat_stmt();
    break;
  case WHILE:
    t = while_stmt();
    break;
  case ID:
    t = assign_stmt();
    break;
  case READ:
    t = read_stmt();
    break;
  case WRITE:
    t = write_stmt();
    break;
  default:
    syntaxError("Unexpected token1 -> ");
    printToken(token, tokenString);
    token = getToken();
    break;
  } /* end case */
  return t;
}

// reconhecer a declaração de if
TreeNode *if_stmt(void)
{
  TreeNode *t = newStmtNode(IfK); // cria um nó de declaração com a função newStmtNode
  match(IF);
  if (t != NULL)
    t->child[0] = exp(); // chamada para construir a subarvore referente a expressão de controle
  match(THEN);
  if (t != NULL)
    t->child[1] = stmt_sequence(); // construir a subarvore referente ao corpo do if
  if (token == ELSE)
  {
    match(ELSE);
    if (t != NULL)
      t->child[2] = stmt_sequence(); // construir a subarvore referente ao corpo do else
  }
  match(ENDIF);

  return t;
}

// reconhecer a declaração de repeat
TreeNode *repeat_stmt(void)
{
  TreeNode *t = newStmtNode(RepeatK);
  match(REPEAT);
  if (t != NULL)
    t->child[0] = stmt_sequence();
  match(UNTIL);
  if (t != NULL)
    t->child[1] = exp();
  return t;
}

// reconhece a declaração while
TreeNode *while_stmt(void)
{
  TreeNode *t = newStmtNode(WhileK);
  match(WHILE); // verifica o reconhecimento da marca WHILE
  if (t != NULL)
    t->child[0] = exp(); // constroi a árvore da expressão de controle
  if (t != NULL)
    t->child[1] = stmt_sequence();  // declarações dentro do while
  match(ENDWHILE);  // reconhece a marca de finalização do while
  return t;
}

// reconhecer a declaração de atribuição
TreeNode *assign_stmt(void)
{
  TreeNode *t = newStmtNode(AssignK);
  if ((t != NULL) && (token == ID))
    t->attr.name = copyString(tokenString);
  match(ID);
  match(ASSIGN);
  if (t != NULL)
    t->child[0] = exp();
  return t;
}

// reconhecer a declaração de leitura
TreeNode *read_stmt(void)
{
  TreeNode *t = newStmtNode(ReadK);
  match(READ);
  if ((t != NULL) && (token == ID))
    t->attr.name = copyString(tokenString);
  match(ID);
  return t;
}

// reconhecer a declaração de escrita
TreeNode *write_stmt(void)
{
  TreeNode *t = newStmtNode(WriteK);
  match(WRITE);
  if (t != NULL)
    t->child[0] = exp();
  return t;
}

// retorna um ponteiro para a árvore de expressão de comparação
TreeNode *exp(void)
{
  TreeNode *t = simple_exp();
  if ((token == LT) || (token == EQ))
  {
    TreeNode *p = newExpNode(OpK);
    if (p != NULL)
    {
      p->child[0] = t;
      p->attr.op = token;
      t = p;
    }
    match(token);
    if (t != NULL)
      t->child[1] = simple_exp();
  }
  return t;
}

// retorna um nó para uma expressão aritimetica
TreeNode *simple_exp(void)
{
  TreeNode *t = term();
  while ((token == PLUS) || (token == MINUS))
  {
    TreeNode *p = newExpNode(OpK);
    if (p != NULL)
    {
      p->child[0] = t;
      p->attr.op = token;
      t = p;
      match(token);
      t->child[1] = term();
    }
  }
  return t;
}

// retorna um ponteiro para uma expressão
TreeNode *term(void)
{
  TreeNode *t = factor();
  while ((token == TIMES) || (token == OVER))
  {
    TreeNode *p = newExpNode(OpK);
    if (p != NULL)
    {
      p->child[0] = t;
      p->attr.op = token;
      t = p;
      match(token);
      p->child[1] = factor();
    }
  }
  return t;
}

// reconhece três tipos de tokens um número, um identificador ou um parêntese abrindo
TreeNode *factor(void)
{
  TreeNode *t = NULL;
  switch (token)
  {
  case NUM:
    t = newExpNode(ConstK);
    if ((t != NULL) && (token == NUM))
      t->attr.val = atoi(tokenString);
    match(NUM);
    break;
  case ID:
    t = newExpNode(IdK);
    if ((t != NULL) && (token == ID))
      t->attr.name = copyString(tokenString);
    match(ID);
    break;
  default:
    syntaxError("unexpected token -> ");
    printToken(token, tokenString);
    token = getToken();
    break;
  }
  return t;
}

/****************************************/
/* the primary function of the parser   */
/****************************************/
/* Retorna um ponteiro para a árvore sintática construída
 */
TreeNode *parse(void)
{
  TreeNode *t;
  token = getToken(); // reconhecimento de token
  t = stmt_sequence(); // iniciar o reconhecimento sintático
  if (token != ENDFILE)
    syntaxError("Code ends before file\n"); // retorna o ponteiro para a árvore sintática
  return t;
}
