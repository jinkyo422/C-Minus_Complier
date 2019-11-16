/****************************************************/
/* File: tiny.y                                     */
/* The TINY Yacc/Bison specification file           */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/
%{
#define YYPARSER /* distinguishes Yacc output from other code files */

#include "globals.h"
#include "util.h"
#include "scan.h"
#include "parse.h"

#define YYSTYPE TreeNode *
static char * savedName; /* for use in assignments */
static int savedLineNo;  /* ditto */
static TreeNode * savedTree; /* stores syntax tree for later return */
static int yylex(void); // added 11/2/11 to ensure no conflict with lex
static int savedSize;
static char * savedType;

%}

%token IF ELSE WHILE RETURN INT VOID
%token THEN END REPEAT UNTIL READ WRITE 
%token ID NUM 
%token ASSIGN EQ NE LT LE GE GT PLUS MINUS TIMES OVER LPAREN RPAREN LBRACE RBRACE LCURLY RCURLY SEMI COMMA
%token ERROR 

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%% /* Grammar for TINY */

program     : decl_list
                { savedTree = $1;}
            ;
decl_list   : decl_list decl
                { YYSTYPE t = $1;
                  if (t != NULL)
                  { while (t->sibling != NULL)
                        t = t->sibling;
                    t->sibling = $2;
                    $$ = $1; }
                  else $$ = $2;
                }
            | decl  { $$ = $1; }
            ;
decl        : var_decl
                { $$ = $1; }
            | fun_decl
                { $$ = $1; }
            ;
var_decl    : type_spec id SEMI
                { $$ = newStmtNode(VarDeclK);
                  $$->attr.name = savedName;
                  $$->attr.type = savedType;
                  $$->lineno = lineno;
                }
            | type_spec id LBRACE num RBRACE
                { $$ = newStmtNode(ArrVarDeclK);
                  $$->attr.name = savedName;
                  $$->attr.val = savedSize;
                  $$->attr.type = savedType;
                  $$->lineno = lineno;
                }
            ;
type_spec   : INT
                { savedType = copyString(tokenString);}
            | VOID
                { savedType = copyString(tokenString);}
            ;
fun_decl    : type_spec id
                { $$ = newStmtNode(FunK);
                  $$->attr.name = savedName;
                  $$->attr.type = savedType;
                }
              LPAREN params RPAREN comp_stmt
                { $$ = $3;
                  $$->lineno = lineno;
                  $$->child[0] = $5;
                  $$->child[1] = $7;
                }
            ;
params      : param_list
                { $$ = $1; }
            | VOID
                { $$ = newStmtNode(ParamK);
                  $$->attr.name = "(null)";
                  $$->attr.type = "void";
                  $$->lineno = lineno;
                }
            ;
param_list  : param_list COMMA param
                { YYSTYPE t = $1;
                  if (t != NULL)
                  { while (t->sibling != NULL)
                        t = t->sibling;
                    t->sibling = $3;
                    $$ = $1; }
                  else $$ = $3;
                }
            | param
                { $$ = $1; }
            ;
param       : type_spec id
                { $$ = newStmtNode(ParamK);
                  $$->attr.name = savedName;
                  $$->attr.type = savedType;
                  $$->lineno = lineno;
                }
            | type_spec id LBRACE RBRACE
                { $$ = newStmtNode(ArrParamK);
                  $$->attr.name = savedName;
                  $$->attr.type = savedType;
                  $$->lineno = lineno;
                }
            ;
comp_stmt   : LCURLY local_decl stmt_list RCURLY
                { $$ = newStmtNode(CompK);
                  $$->child[0] = $2;
                  $$->child[1] = $3;
                }
            ;
local_decl  : local_decl var_decl
                { YYSTYPE t = $1;
                  if (t != NULL)
                  { while (t->sibling != NULL)
                        t = t->sibling;
                    t->sibling = $2;
                    $$ = $1; }
                  else $$ = $2;
                }
            | { $$ = NULL; }
            ;
stmt_list   : stmt_list stmt
                { YYSTYPE t = $1;
                  if (t != NULL)
                  { while (t->sibling != NULL)
                        t = t->sibling;
                    t->sibling = $2;
                    $$ = $1; }
                  else $$ = $2;
                }
            | { $$ = NULL; }
            ;
stmt        : exp_stmt
                { $$ = $1; }
            | comp_stmt
                { $$ = $1; }
            | sel_stmt
                { $$ = $1; }
            | iter_stmt
                { $$ = $1; }
            | ret_stmt
                { $$ = $1; }
            ;
exp_stmt    : exp SEMI
                { $$ = $1; }
            | SEMI
                { $$ = NULL; }
            ;
sel_stmt    : IF LPAREN exp RPAREN stmt %prec LOWER_THAN_ELSE
                { $$ = newStmtNode(IfK);
                  $$->child[0] = $3;
                  $$->child[1] = $5;
                  $$->child[2] = NULL;
                }
            | IF LPAREN exp RPAREN stmt ELSE stmt
                { $$ = newStmtNode(IfK);
                  $$->child[0] = $3;
                  $$->child[1] = $5;
                  $$->child[2] = $7;
                }
            ;
iter_stmt   : WHILE LPAREN exp RPAREN stmt
                { $$ = newStmtNode(WhileK);
                  $$->child[0] = $3;
                  $$->child[1] = $5;
                }
            ;
ret_stmt    : RETURN SEMI
                { $$ = newStmtNode(RetK);
                  $$->child[0] = NULL;
                }
            | RETURN exp SEMI
                { $$ = newStmtNode(RetK);
                  $$->child[0] = $2;
                }
            ;
exp         : var ASSIGN exp
                { $$ = newStmtNode(AssignK);
                  $$->child[0] = $1;
                  $$->child[1] = $3;
                }
            | simple_exp { $$ = $1; }
            ;
var         : id
                { $$ = newExpNode(IdK);
                  $$->attr.name = savedName;
                }
            | id
                { $$ = newExpNode(ArrIdK);
                  $$->attr.name = savedName;
                }
              LBRACE exp RBRACE
                { $$->child[0] = $3; }
            ;
simple_exp  : add_exp LE add_exp
                { $$ = newExpNode(OpK);
                  $$->child[0] = $1;
                  $$->child[1] = $3;
                  $$->attr.op = LE;
                }
            | add_exp LT add_exp
                { $$ = newExpNode(OpK);
                  $$->child[0] = $1;
                  $$->child[1] = $3;
                  $$->attr.op = LT;
                }
            | add_exp GT add_exp
                { $$ = newExpNode(OpK);
                  $$->child[0] = $1;
                  $$->child[1] = $3;
                  $$->attr.op = GT;
                }
            | add_exp GE add_exp
                { $$ = newExpNode(OpK);
                  $$->child[0] = $1;
                  $$->child[1] = $3;
                  $$->attr.op = GE;
                }
            | add_exp EQ add_exp
                { $$ = newExpNode(OpK);
                  $$->child[0] = $1;
                  $$->child[1] = $3;
                  $$->attr.op = EQ;
                }
            | add_exp NE add_exp
                { $$ = newExpNode(OpK);
                  $$->child[0] = $1;
                  $$->child[1] = $3;
                  $$->attr.op = NE;
                }
            | add_exp
                { $$ = $1; }
            ;
add_exp     : add_exp PLUS term
                { $$ = newExpNode(OpK);
                  $$->child[0] = $1;
                  $$->child[1] = $3;
                  $$->attr.op = PLUS;
                }
            | add_exp MINUS term
                { $$ = newExpNode(OpK);
                  $$->child[0] = $1;
                  $$->child[1] = $3;
                  $$->attr.op = MINUS;
                }
            | term
                { $$ = $1; }
            ;
term        : term TIMES factor
                { $$ = newExpNode(OpK);
                  $$->child[0] = $1;
                  $$->child[1] = $3;
                  $$->attr.op = TIMES;
                }
            | term OVER factor
                { $$ = newExpNode(OpK);
                  $$->child[0] = $1;
                  $$->child[1] = $3;
                  $$->attr.op = OVER;
                }
            | factor
                { $$ = $1; }
            ;
factor      : LPAREN exp RPAREN
                { $$ = $2; }
            | var
                { $$ = $1; }
            | call
                { $$ = $1; }
            | NUM
                { $$ = newExpNode(ConstK);
                  $$->attr.val = atoi(tokenString);
                }
            ;
call        : id
                { $$ = newExpNode(CallK);
                  $$->attr.name = savedName;
                }
              LPAREN args RPAREN
                { $$ = $2;
                  $$->child[0] = $4;
                }
            ;
args        : arg_list
                { $$ = $1;}
            | { $$ = NULL; }
            ;
arg_list    : arg_list COMMA exp
                { YYSTYPE t = $1;
                  if (t != NULL)
                  { while (t->sibling != NULL)
                        t = t->sibling;
                    t->sibling = $3;
                    $$ = $1; }
                  else $$ = $3;
                }
            | exp
                { $$ = $1; }
            ;
id          : ID
                { savedName = copyString(tokenString);
                  savedLineNo = lineno;
                }
            ;

num         : NUM
                {savedSize = atoi(tokenString);
                  savedLineNo = lineno;
                }
            ;

%%

int yyerror(char * message)
{ fprintf(listing,"Syntax error at line %d: %s\n",lineno,message);
  fprintf(listing,"Current token: ");
  printToken(yychar,tokenString);
  Error = TRUE;
  return 0;
}

/* yylex calls getToken to make Yacc/Bison output
 * compatible with ealier versions of the TINY scanner
 */
static int yylex(void)
{ return getToken(); }

TreeNode * parse(void)
{ yyparse();
  return savedTree;
}

