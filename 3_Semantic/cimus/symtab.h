/****************************************************/
/* File: symtab.h                                   */
/* Symbol table interface for the TINY compiler     */
/* (allows only one symbol table)                   */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"

#ifndef _SYMTAB_H_
#define _SYMTAB_H_

#define MAX_SCOPE 500
#define SIZE 211

/* the list of line numbers of the source 
 * code in which a variable is referenced
 */
typedef struct LineListRec
   { int lineno;
     struct LineListRec * next;
   } LineListRec, *LineList;

/* The record in the bucket lists for
 * each variable, including name,
 * assigned memory location, and
 * the list of line numbers in which
 * it appears in the source code
 */
typedef struct BucketListRec
{
  char *name;
  TreeNode *t;
  ExpType type;
  LineList lines;
  int memloc;
  struct BucketListRec *next;
} BucketListRec, *Bucket;

typedef struct ScopeListRec
{
  char *name;
  TreeNode *t;
  Bucket bucket[SIZE];
  int nestedLevel;
  struct ScopeListRec *parent;
  int index;
  int scopeCreated;
} ScopeListRec, *Scope;

/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored
 */
void st_insert( Scope scope, char *name, TreeNode *t, ExpType type, int lineno, int loc );

/* Function st_lookup returns the memory 
 * location of a variable or -1 if not found
 */
Bucket st_lookup( Scope scope, char *name );
Bucket st_lookup_excluding_parent( Scope scope, char *name );

Scope sc_create( char *funcName, TreeNode *t );
Scope sc_top();
Scope sc_push( Scope scope );
Scope sc_pop();

int nextLocation( Scope scope );

void printBucket(Bucket b);

char *printType(ExpType type);

/* Procedure printSymTab prints a formatted 
 * listing of the symbol table contents 
 * to the listing file
 */
void printSymTab(FILE * listing);

#endif
