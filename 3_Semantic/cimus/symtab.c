/****************************************************/
/* File: symtab.c                                   */
/* Symbol table implementation for the TINY compiler*/
/* (allows only one symbol table)                   */
/* Symbol table is implemented as a chained         */
/* hash table                                       */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"

/* SHIFT is the power of two used as multiplier
   in hash function  */
#define SHIFT 4

static Scope scopes[MAX_SCOPE];
static int nScope = 0;

static Scope scopeStack[MAX_SCOPE];
static int nScopeStack = 0;

static int location[MAX_SCOPE];

/* the hash function */
static int hash ( char * key )
{ int temp = 0;
  int i = 0;
  while (key[i] != '\0')
  { temp = ((temp << SHIFT) + key[i]) % SIZE;
    ++i;
  }
  return temp;
}

/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored
 */
void st_insert( Scope scope, char *name, TreeNode *t, ExpType type, int lineno, int loc )
{
  int h = hash(name);
  Bucket b = st_lookup_excluding_parent(scope, name);
  
  if (b == NULL) {
    b = (Bucket) malloc(sizeof(struct BucketListRec));
    b->name = name;
    b->t = t;
    b->type = type;
    b->lines = (LineList) malloc(sizeof(struct LineListRec));
    b->lines->lineno = lineno;
    b->memloc = loc;
    b->lines->next = NULL;
    b->next = scope->bucket[h];
    scope->bucket[h] = b;
  }
  else {
    LineList l = b->lines;
    while (l->next != NULL)
      l = l->next;
    l->next = (LineList) malloc(sizeof(struct LineListRec));
    l->next->lineno = lineno;
    l->next->next = NULL;
  }
} /* st_insert */

/* Function st_lookup returns the memory 
 * location of a variable or -1 if not found
 */
Bucket st_lookup( Scope scope, char *name )
{
  if (name == NULL)
    return NULL;
  int h = hash(name);
  while (scope != NULL) {
    Bucket b = scope->bucket[h];
    while ((b != NULL) && (strcmp(name, b->name) != 0))
    {   
        fprintf(listing,"test: %s\n",b->name);
        b = b->next;
    }
    if (b == NULL)
      scope = scope->parent;
    else
      return b;
  }
  return NULL;
}

Bucket st_lookup_excluding_parent( Scope scope, char *name )
{
  int h = hash(name);
  Bucket b = scope->bucket[h];
  while ((b != NULL) && (strcmp(name, b->name) != 0))
    b = b->next;

  return b;
}

Scope sc_create( char *funcName, TreeNode *t )
{
  Scope newScope = (Scope)malloc(sizeof(ScopeListRec));
  if (newScope == NULL) {
    fprintf(stderr, "failed to create new scope\n");
    exit(1);
  }

  newScope->name = funcName;
  newScope->t = t;
  for (int i = 0; i < SIZE; i++)
    newScope->bucket[i] = NULL;
  Scope parent = sc_top();
  if (parent)
    newScope->nestedLevel = sc_top()->nestedLevel + 1;
  else
    newScope->nestedLevel = 0;
  newScope->parent = parent;
  newScope->index = nScope;
  scopes[nScope++] = newScope;
  newScope->scopeCreated = FALSE;
  return newScope;
}

Scope sc_top()
{
  if (nScopeStack == 0)
    return NULL;
  else
    return scopeStack[nScopeStack - 1];
}

Scope sc_push( Scope scope )
{
  return scopeStack[nScopeStack++] = scope;
}

Scope sc_pop()
{
  return scopeStack[--nScopeStack];
}

int nextLocation( Scope scope )
{
  return location[scope->index]++;
}

void printBucket(Bucket b)
{
  LineList l = b->lines;

  printf("name: %s\n", b->name);
  printf("type: %s\n", printType(b->type));
  printf("lines: ");
  while (l) {
    printf("%d ", l->lineno);
    l = l->next;
  }
  printf("\n");
  printf("memloc: %d\n", b->memloc);
}

char *printType(ExpType type)
{
  switch (type) {
    case Void:
      return "Void";
    case Integer:
      return "Integer";
    case IntegerArray:
      return "IntegerArray";
    default:
      return "UNKNOWN";
  }
}

static void printFunctionDeclaration(FILE *listing)
{
  for (int i = 0; i < nScope; i++) {
    Scope tmpScope = scopes[i];
    TreeNode *t = tmpScope->t;
    TreeNode *param;
    if (t != NULL)
      param = t->child[0];
    else
      param = NULL;
    if (tmpScope->parent == NULL)
      continue;
    if (tmpScope->parent->name != NULL)
      continue;
    fprintf(listing, "%-15sglobal\t   %-14s", tmpScope->name, printType(t->type));
    if (param == NULL)
      fprintf(listing, "\t\t%-19s\n", "Void");
    else {
	fprintf(listing, "\n");
      while(param) {
        fprintf(listing, "\t\t\t\t\t %-15s%s\n", param->attr.name, printType(param->type));
        param = param->sibling;
      }
    }
  }
}

static void printGlobalDeclarations(FILE *listing)
{
  Scope globalScope;
  for (int i = 0; i < nScope; i++) {
    if (scopes[i]->name == NULL) {
      globalScope = scopes[i];
      break;
    }
  }
  for (int i = 0; i < SIZE; i++) {
    Bucket curBucket = globalScope->bucket[i];
    while (curBucket) {
      TreeNode *t = curBucket->t;
      if (t->nodekind == StmtK) {
        fprintf(listing, "%-15s", t->attr.name);
        if (t->kind.stmt == FunK)
          fprintf(listing, "%-11s", "Function");
        else
          fprintf(listing, "%-11s", "Variable");
        fprintf(listing, "%s\n", printType(t->type));
      }
      curBucket = curBucket->next;
    }
  }
  fprintf(listing, "\n");
}

static void printScopeInfo(FILE *listing, Scope curScope)
{
  for (int i = 0; i < SIZE; i++) {
    Bucket curBucket = curScope->bucket[i];
    while (curBucket) {
      TreeNode *t = curBucket->t;
      if ((t->nodekind == StmtK && t->kind.stmt != FunK) || t->nodekind == ParamK) {
        fprintf(listing, "%-17s", curScope->name);
        fprintf(listing, "%-15d", curScope->nestedLevel);  
        fprintf(listing, "%-14s", t->attr.name);
        fprintf(listing, "%s\n", printType(t->type));
      }
      curBucket = curBucket->next;
    }
  }
}

/* Procedure printSymTab prints a formatted
 * listing of the symbol table contents
 * to the listing file
 */
void printSymTab(FILE *listing)
{
fprintf(listing, "\n\n< Symbol Table >\n");
  fprintf(listing, "Function Name  Scope Name  Return Type   Paramter Name  Paramter Type\n");
  fprintf(listing, "-------------  ----------  -----------   -------------  -------------\n\n");
  //printSymbol(listing);

  fprintf(listing, "< Function Table >\n");
  fprintf(listing, "Function Name  Scope Name  Return Type   Paramter Name  Paramter Type\n");
  fprintf(listing, "-------------  ----------  -----------   -------------  -------------\n");
  printFunctionDeclaration(listing);
  fprintf(listing, "\n");

  fprintf(listing, "< Function and Global Variables >\n");
  fprintf(listing, "   ID Name      ID type    Data Type\n");
  fprintf(listing, "-------------  ---------  -----------\n");
  printGlobalDeclarations(listing);

  fprintf(listing, "< Function Parameters and Local Variables >\n");
  fprintf(listing, "  Scope Name     Nested Level     ID Name     Data Type\n");
  fprintf(listing, "--------------   ------------   -----------   ---------\n");
  for (int i = 0; i < nScope; i++) {
    if (scopes[i]->name == NULL)
      continue;
    printScopeInfo(listing, scopes[i]);
  }
} /* printSymTab */
