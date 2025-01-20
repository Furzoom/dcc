#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// tokenizer.c
//
typedef enum {
  TK_IDENT, // Identifiers
  TK_PUNCT, // punctuations
  TK_NUM,   // numeric literals
  TK_EOF,   // end-of-file markers
} TokenKind;

// Token type
typedef struct Token Token;
struct Token {
  TokenKind kind;   // token kind
  Token* next;      // next token
  int val;          // if kind is TK_NUM, its value
  char* loc;        // token location
  int len;          // token length
};

void error(char* fmt, ...);
void error_at(char* loc, char* fmt, ...);
void error_tok(Token* tok, char* fmt, ...);
bool equal(Token* tok, char* op);
Token* skip(Token* tok, char* op);
Token* tokenize(char* input);

//
// parser.c
//

typedef enum {
  ND_ADD,       // +
  ND_SUB,       // -
  ND_MUL,       // *
  ND_DIV,       // /
  ND_NEG,       // Unary -
  ND_EQ,        // ==
  ND_NE,        // !=
  ND_LT,        // <
  ND_LE,        // <=
  ND_ASSIGN,    // =
  ND_EXPR_STMT, // Expression statement
  ND_VAR,       // Variable
  ND_NUM,       // Integer
} NodeKind;

// AST node type.
typedef struct Node Node;
struct Node {
  NodeKind kind;  // Node kind
  Node* next;     // Next node
  Node* lhs;      // Left-hand side
  Node* rhs;      // Right-hand size
  char name;      // Used if kind == ND_VAR
  int val;        // Used if kind == ND_NUM
};

Node* parse(Token* tok);

//
// codegen.c
//

void codegen(Node* node);

