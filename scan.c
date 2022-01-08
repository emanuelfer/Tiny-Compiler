/****************************************************/
/* File: scan.c                                     */
/* The scanner implementation for the TINY compiler */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "util.h"
#include "scan.h"

/* Corresponde aos estados do AFD da linguagem */
typedef enum
{
  START,
  INASSIGN,
  INCOMMENT,
  INNUM,
  INID,
  DONE
} StateType;

/* vetor de char para armazenamento do lexema do token corrente */
char tokenString[MAXTOKENLEN + 1];

/* Tamanho do vetor para armazenamento linha a linha do código fonte */
#define BUFLEN 256

static char lineBuf[BUFLEN]; /* vetor para armazenamento linha a linha do código fonte */
static int linepos = 0;      /* guarda a posição corrente de leitura em linebuf */
static int bufsize = 0;      /* tamanho da string corrente em linebuf em determinado momento */
static int EOF_flag = FALSE; /* corrects ungetNextChar behavior on EOF */

/* Função chamada para retornar o proximo caracter a partir de linebuf */
static int getNextChar(void)
{
  if (!(linepos < bufsize))
  {
    lineno++;
    if (fgets(lineBuf, BUFLEN - 1, source))
    {
      if (EchoSource)
        fprintf(listing, "%4d: %s", lineno, lineBuf);
      bufsize = strlen(lineBuf);
      linepos = 0;
      return lineBuf[linepos++];
    }
    else
    {
      EOF_flag = TRUE;
      return EOF;
    }
  }
  else
    return lineBuf[linepos++];
}

/*Retrocede uma posição no buffer */
static void ungetNextChar(void)
{
  if (!EOF_flag)
    linepos--;
}

/* Tabela de palavras reservadas contém todas as palavras reservadas com seu correspondente lexema */
static struct
{
  char *str;
  TokenType tok;
} reservedWords[MAXRESERVED] = {{"if", IF}, {"then", THEN}, {"else", ELSE}, {"endif", ENDIF}, {"repeat", REPEAT}, {"until", UNTIL}, {"read", READ}, {"write", WRITE}, {"while", WHILE}, {"endwhile", ENDWHILE}};

/*Recebe uma string e verifica se a string é uma palavra reservada*/
/* faz isso usando busca linear*/
static TokenType reservedLookup(char *s)
{
  int i;
  for (i = 0; i < MAXRESERVED; i++)
    if (!strcmp(s, reservedWords[i].str))
      return reservedWords[i].tok;
  return ID; // caso não seja retorna como sendo um identificador
}

/****************************************/
/* the primary function of the scanner  */
/****************************************/
/* corresponde a implementação do AFD
 */

TokenType getToken(void)
{ // armazena o token corrente 
  int tokenStringIndex = 0;
  /* holds current token to be returned */
  TokenType currentToken;
  // armazena o estado corrente do AFD 
  StateType state = START;
 //indica se um caracter que está sendo lido deva ou não compor o lexema
  int save;
  while (state != DONE)
  {
    int c = getNextChar(); // pega o caracter
    save = TRUE; // seta como true para entrar no switch
    switch (state)
    {
    case START: // corresponde ao primeiro estado do automato
      if (isdigit(c)) // se for um digito
        state = INNUM;
      else if (isalpha(c) || c == '_')
        state = INID;
      else if (c == ':')
        state = INASSIGN;
      else if ((c == ' ') || (c == '\t') || (c == '\n'))
        save = FALSE;
      else if (c == '{')
      {
        save = FALSE;
        state = INCOMMENT;
      }
      else
      {
        state = DONE;
        switch (c)
        {
        case EOF:
          save = FALSE;
          currentToken = ENDFILE;
          break;
        case '=':
          currentToken = EQ;
          break;
        case '<':
          currentToken = LT;
          break;
        case '+':
          currentToken = PLUS;
          break;
        case '-':
          currentToken = MINUS;
          break;
        case '*':
          currentToken = TIMES;
          break;
        case '/':
          currentToken = OVER;
          break;
        case ';':
          currentToken = SEMI;
          break;
        default:
          currentToken = ERROR;
          break;
        }
      }
      break;
    case INCOMMENT: // reconhecimento de comentario
      save = FALSE;
      if (c == EOF)
      {
        state = DONE;
        currentToken = ENDFILE;
      }
      else if (c == '}')
        state = START;
      break;
    case INASSIGN:
      state = DONE;
      if (c == '=')
        currentToken = ASSIGN;
      else
      { /* backup in the input */
        ungetNextChar();
        save = FALSE;
        currentToken = DDOT;
      }
      break;
    case INNUM: // reconhecimento de digito
      if (!isdigit(c))
      { /* backup in the input */
        ungetNextChar();
        save = FALSE;
        state = DONE;
        currentToken = NUM;
      }
      break;
    case INID: // aceitação de identificadores
      if (!isalpha(c) && !isdigit(c) && c != '_')
      { /* backup in the input */
        ungetNextChar();
        save = FALSE;
        state = DONE;
        currentToken = ID;
      }
      break;
    case DONE:
    default: /* should never happen */
      fprintf(listing, "Scanner Bug: state= %d\n", state);
      state = DONE;
      currentToken = ERROR;
      break;
    }

    if ((save) && (tokenStringIndex <= MAXTOKENLEN)) // se save for true e a posição no vetor do token for menor que MAXTOKENLEN
      tokenString[tokenStringIndex++] = (char)c; // acrescenta ao letor do lexema
    if (state == DONE) // se for o estado final
    {
      tokenString[tokenStringIndex] = '\0';
      if (currentToken == ID) // se for um identificador
        currentToken = reservedLookup(tokenString); // chama essa função para verificar se é uma palavra reservada ou um ID
    }
  }
  if (TraceScan) // se estiver setada como true é impresso o token e seu tipo
  {
    fprintf(listing, "\t%d: ", lineno);
    printToken(currentToken, tokenString);
  }
  return currentToken; // retorna o token corrente (reconhecido)
} /* end getToken */
