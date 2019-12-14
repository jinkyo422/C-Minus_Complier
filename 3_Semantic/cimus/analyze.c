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
#include "util.h"

static Scope globalScope;
static Scope curScope;
static Bucket curBucket;
static char *name;
static char errorMsg[100];

/* Procedure traverse is a generic recursive 
 * syntax tree traversal routine:
 * it applies preProc in preorder and postProc 
 * in postorder to tree pointed to by t
 */
static void traverse( TreeNode * t,
               void (* preProc) (TreeNode *),
               void (* postProc) (TreeNode *) )
{ if (t != NULL)
  { preProc(t);
    { int i;
      for (i=0; i < MAXCHILDREN; i++)
        traverse(t->child[i],preProc,postProc);
    }
    postProc(t);
    traverse(t->sibling,preProc,postProc);
  }
}

/* nullProc is a do-nothing procedure to 
 * generate preorder-only or postorder-only
 * traversals from traverse
 */
static void nullProc(TreeNode * t)
{ if (t==NULL) return;
  else return;
}

/* print received error message and exit the program */
static void buildingError(TreeNode *t, char *message)
{
  fprintf(listing,"Error: %s at line %d\n", message, t->lineno);
  Error = TRUE;
}

/* Procedure insertNode inserts
 * identifiers stored in t into
 * the symbol table
 */
static void insertNode(TreeNode *t)
{
  curScope = sc_top();
  t->scope = curScope;
  switch (t->nodekind) {
    case StmtK:
      switch (t->kind.stmt) {
        case FunK:
          name = t->attr.name;
          if (st_lookup(curScope, name) != NULL) {
            strcpy(errorMsg, "Redefinition of Function ");
            buildingError(t, strcat(errorMsg, name));
          }
          if (curScope != globalScope) {
            buildingError(t, "Function Definition is not allowed here");
          }
          if(!strcmp(t->attr.type, "int"))	t->type = Integer;
          else if(!strcmp(t->attr.type, "void"))	t->type = Void;
          
          st_insert(curScope, name, t, t->type, t->lineno, nextLocation(curScope));
          curScope = sc_push(sc_create(name, t));
          t->scope = curScope;
          break;
        case VarDeclK:
        case ArrVarDeclK:
          name = t->attr.name;
          curBucket = st_lookup_excluding_parent(curScope, name);
          if (!strcmp(t->attr.type, "void")) {
            if (t->kind.stmt == VarDeclK) {
              strcpy(errorMsg, " Variable Type cannot be Void");
              buildingError(t, strcat(name, errorMsg));
            }
            else{
              strcpy(errorMsg, " Array Type cannot be Void");
              buildingError(t, strcat(name, errorMsg));
            }
            break;
          }
          if (curBucket != NULL) {
            strcpy(errorMsg, "Redefinition of ");
            buildingError(t, strcat(errorMsg, name));
          }

          if(!strcmp(t->attr.type, "int")){
            if(t->attr.val == 0)  t->type = Integer;
            else  t->type = IntegerArray;
          }
          else if(!strcmp(t->attr.type, "void"))	t->type = Void;
          
          st_insert(curScope, name, t, t->type, t->lineno, nextLocation(curScope));
          t->scope = curScope;
          break;
        case CompK:
          if (curScope->scopeCreated == FALSE) {
            curScope->scopeCreated = TRUE;
          }
          else {
            curScope = sc_push(sc_create(curScope->name, t));
            t->scope = curScope;
            curScope->scopeCreated = TRUE;
          }
          break;
        case ParamK:
        case ArrParamK:
          name = t->attr.name;
          curBucket = st_lookup_excluding_parent(curScope, name);
          if (!strcmp(t->attr.type, "void")) {
            if (!strcmp(curScope->name, "main"))    break;
            buildingError(t, "Parameter Type cannot be Void");
          }
          if (curBucket != NULL) {
            strcpy(errorMsg, "Redefinition of Parameter ");
            buildingError(t, strcat(errorMsg, name));
          }
          if(!strcmp(t->attr.type, "int")){
            if(t->kind.stmt == ParamK)
              t->type = Integer;
          	else
          		t->type = IntegerArray;
          }
          else if(!strcmp(t->attr.type, "void"))	t->type = Void;

          st_insert(curScope, name, t, t->type, t->lineno, nextLocation(curScope));
          t->scope = curScope;
          break;
        case IfK:
          break;
        case WhileK:
          break;
        case RetK:
          break;
      	case AssignK:
          break;
        default:
          break;
      }
      break;
    case ExpK:
      switch (t->kind.exp) {
        case IdK:
      	case ArrIdK:
          name = t->attr.name;
          curBucket = st_lookup(curScope, name);
          if (curBucket == NULL) {
            strcpy(errorMsg, "Undeclared Variable ");
            buildingError(t, strcat(errorMsg, name));
          }
          if (curBucket)
            t->type = curBucket->type;
          st_insert(curScope, name, t, t->type, t->lineno, nextLocation(curScope));
          t->scope = curScope;
          break;
        case CallK: {
          name = t->attr.name;
          curBucket = st_lookup(curScope, name);
          if (curBucket == NULL) {
            strcpy(errorMsg, "Undeclared Function ");
            buildingError(t, strcat(errorMsg, name));
          }
          if (curBucket)
            t->type = curBucket->type;
          st_insert(curScope, name, t, t->type, t->lineno, nextLocation(curScope));
          t->scope = curScope;
          break;
        }  
        case OpK:
          break;
        case ConstK:
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

static void afterInsertNode(TreeNode *t)
{
  if (t->nodekind == StmtK && t->kind.stmt == CompK)
    sc_pop();
  else if (t->nodekind == ExpK && (t->kind.exp == AssignK || t->kind.exp == OpK))
    t->type = t->child[0]->type;
}

/* Function initBuildSymtab constructs 2 necessary functions
 * input() && output(). And also creates globalScope before anything else.
 */
static void initBuildSymtab()
{
  /* global scope */
  globalScope = sc_create(NULL, NULL);
  curScope = globalScope;
  sc_push(globalScope);

  TreeNode *inpFunc = newStmtNode(FunK);
  TreeNode *outFunc = newStmtNode(FunK);
  TreeNode *compStmt;
  TreeNode *param;

  /* input() */
  compStmt = newStmtNode(CompK);
  compStmt->lineno = 0;
  compStmt->child[0] = compStmt->child[1] = NULL;

  inpFunc = newStmtNode(FunK);
  inpFunc->lineno = 0;
  inpFunc->attr.name = "input";
  inpFunc->type = Integer;
  inpFunc->scope = globalScope;
  inpFunc->child[0] = NULL;
  inpFunc->child[1] = compStmt;
  st_insert(globalScope, inpFunc->attr.name, inpFunc, inpFunc->type,
            inpFunc->lineno, nextLocation(curScope));
  curScope = sc_push(sc_create(inpFunc->attr.name, inpFunc));

  compStmt->scope = curScope;
  sc_pop();

  /* output() */
  compStmt = newStmtNode(CompK);
  compStmt->lineno = 0;
  compStmt->child[0] = compStmt->child[1] = NULL;

  param = newStmtNode(ParamK);
  param->attr.name = "arg";
  param->type = Integer;

  outFunc->lineno = 0;
  outFunc->attr.name = "output";
  outFunc->type = Void;
  outFunc->scope = globalScope;
  outFunc->child[0] = param;
  outFunc->child[1] = compStmt;
  st_insert(globalScope, outFunc->attr.name, outFunc, outFunc->type,
            outFunc->lineno, nextLocation(curScope));
  curScope = sc_push(sc_create(outFunc->attr.name, outFunc));

  compStmt->scope = param->scope = curScope;
  sc_pop();
  curScope = globalScope;
}

/* Function buildSymtab constructs the symbol
 * table by preorder traversal of the syntax tree
 */
void buildSymtab(TreeNode *syntaxTree)
{
  initBuildSymtab();
  traverse(syntaxTree,insertNode,afterInsertNode);
  sc_pop();
  if (TraceAnalyze)
  {
    printSymTab(listing);
  }
}

static void typeError(TreeNode *t, char *message)
{ fprintf(listing,"Error: Type error at line %d: %s\n",t->lineno,message);
  Error = TRUE;
}

/* Procedure checkNode performs
 * type checking at a single tree node
 */
static void checkNode(TreeNode *t)
{
  switch (t->nodekind) {
    case StmtK:
      switch (t->kind.stmt) {
        case CompK:
          break;
        case IfK:
        case WhileK:
          if (t->child[0] == NULL)
            typeError(t, "expected expression");
          else if (t->child[0]->type == Void)
            typeError(t, "statement requires expression of scalar type ('void' invalid)");
          break;
        case RetK:{
          Bucket funcBucket = st_lookup(t->scope, t->scope->name);
          ExpType funcType = funcBucket->type;
          TreeNode *expr = t->child[0];
          if (expr->type == Void && expr->kind.exp == ConstK)
            expr->type = Integer;

          if (funcType == Void && expr != NULL && expr->type != Void)
            typeError(t, "invalid return type");
          else if (funcType == Integer && (expr == NULL || expr->type != Integer)) {
            if (expr == NULL)
              typeError(t, "invalid return type");
            else if (expr->type == IntegerArray && expr->child[0] != NULL)
              break;
            else
              typeError(t, "invalid return type");
          }
          break;
        }
        case AssignK:{
          TreeNode *leftOp = t->child[0];
          TreeNode *rightOp = t->child[1];
          
          if (leftOp->type == Void && leftOp->kind.exp == ConstK)
            leftOp->type = Integer;
          if (rightOp->type == Void && rightOp->kind.exp == ConstK)
            rightOp->type = Integer;
          
          if (leftOp->type == Void || rightOp->type == Void)
            typeError(t, "expression is not assignable");
          else if (leftOp->type == IntegerArray && leftOp->child[0] == NULL)
            typeError(t, "type inconsistance");
          else if (rightOp->type == IntegerArray && rightOp->child[0] == NULL)
            typeError(t, "type inconsistance");
          break;
        }
        default:
          break;
      }
      break;
    case ExpK:
      switch (t->kind.exp) {
        case IdK:
          curBucket = st_lookup(t->scope, t->attr.name);
          t->type = curBucket->type;
          if (t->child[0] != NULL) {
            if (t->child[0]->type != Integer) {
              strcpy(errorMsg, "array subscript is not an integer");
              typeError(t, strcat(errorMsg, t->attr.name));
              break;
            }
          }
          break;
        case CallK:
          curBucket = st_lookup(t->scope->parent, t->attr.name);
          if (curBucket == NULL) {
            strcpy(errorMsg, "implicit declaration of function ");
            typeError(t, strcat(errorMsg, t->attr.name));
            break;
          }
          t->type = curBucket->type;

          TreeNode *func = curBucket->t;
          TreeNode *param = func->child[0];
          TreeNode *arg = t->child[0];
          while (arg) {
            if (!param) {
              typeError(t, "invalid function call");
              break;
            }
            else if (param->type != arg->type) {
              if((param->type == Integer || param->type == IntegerArray) 
                      && (arg->type == Integer || arg->type == IntegerArray || arg->kind.exp == ConstK))	break;
              typeError(t, "invalid function call");
              break;
            }
            else {
              arg = arg->sibling;
              param = param->sibling;
            }
          }
          if (arg == NULL && param != NULL)
            typeError(t, "invalid function call");
          break;
        case OpK:{
          ExpType leftType = t->child[0]->type;
          ExpType rightType = t->child[1]->type;
          if (leftType == IntegerArray && t->child[0]->child[0] != NULL)
            leftType = Integer;
          else if (leftType == Void && t->child[0]->kind.exp == ConstK)
            leftType = Integer;
          if (rightType == IntegerArray && t->child[1]->child[0] != NULL)
            rightType = Integer;
          else if (rightType == Void && t->child[1]->kind.exp == ConstK)
            rightType = Integer;
          
          if (leftType == Void || rightType == Void){
            typeError(t, "invalid expression");}
          else if (leftType != rightType)
            typeError(t, "invalid expression");
          else
            t->type = Integer;
          break;
        }
        case ConstK:
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

/* Procedure typeCheck performs type checking
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode *syntaxTree)
{
  sc_push(globalScope);
  traverse(syntaxTree,nullProc,checkNode);
  sc_pop();
}
