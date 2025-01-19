#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// Tokenizer
//
typedef enum {
  TK_PUNCT, // punctuation
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

// Input string.
static char* current_input;

// Reports an error and exit.
static void error(char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt,ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(1);
}

// Reports an error localtion and exit.
static void verror_at(char* loc, char* fmt, va_list ap) {
  int pos = loc - current_input;
  fprintf(stderr, "%s\n", current_input);
  fprintf(stderr, "%*s", pos, "");  // print pos spaces
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}


static void error_at(char* loc, char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(loc, fmt, ap);
  va_end(ap);
}

static void error_tok(Token* tok, char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(tok->loc, fmt, ap);
  va_end(ap);
}

// Consumes the current token if it matches `op`.
static bool equal(Token* tok, char* op) {
  return memcmp(tok->loc, op, tok->len) == 0 && op[tok->len] == '\0';
}

// Ensure that the current token is 's'.
static Token* skip(Token* tok, char* s) {
  if (!equal(tok, s)) {
    error_tok(tok, "expected '%s'", s);
  }
  return tok->next;
}

// Ensure that the current token is TK_NUM.
static int get_number(Token* tok) {
  if (tok->kind != TK_NUM) {
    error_tok(tok, "expected a number");
  }
  return tok->val;
}

// Create a new token.
static Token* new_token(TokenKind kind, char* start, char* end) {
  Token* tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->loc = start;
  tok->len = end - start;
  return tok;
}

static bool startswith(char* p, char* q) {
  return strncmp(p, q, strlen(q)) == 0;
}

// Read a punctuation
static int read_punct(char* p) {
  if (startswith(p, "==") || startswith(p, "!=") ||
      startswith(p, "<=") || startswith(p, ">=")) {
    return 2;
  }

  return ispunct(*p) ? 1 : 0;
}

// Tokenize `current_input` and returns new tokens.
static Token* tokenize() {
  char* p = current_input;
  Token head = {};
  Token* cur = &head;

  while (*p) {
    // Skip whitespace characters.
    if (isspace(*p)) {
      p++;
      continue;
    }

    // Numeric literal.
    if (isdigit(*p)) {
      cur = cur->next = new_token(TK_NUM, p, p);
      char* q = p;
      cur->val = strtoul(p, &p, 10);
      cur->len = p - q;
      continue;
    }

    // Punctuation
    int punct_len = read_punct(p);
    if (punct_len) {
      cur = cur->next = new_token(TK_PUNCT, p, p + punct_len);
      p += punct_len;
      continue;
    }

    error_at(p, "invalid token");
  }

  cur->next = new_token(TK_EOF, p, p);
  return head.next;
}

//
// Parser
//

typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_NEG, // unary -
  ND_EQ,  // ==
  ND_NE,  // !=
  ND_LT,  // <
  ND_LE,  // <=
  ND_NUM, // integer
} NodeKind;

// AST node type.
typedef struct Node Node;
struct Node {
  NodeKind kind;  // Node kind
  Node* lhs;      // Left-hand side
  Node* rhs;      // Right-hand size
  int val;        // Used if kind == ND_NUM
};

static Node* new_node(NodeKind kind) {
  Node* node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

static Node* new_binary(NodeKind kind, Node* lhs, Node* rhs) {
  Node* node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static Node* new_unary(NodeKind kind, Node* expr) {
  Node* node = new_node(kind);
  node->lhs = expr;
  return node;
}

static Node* new_num(int val) {
  Node* node = new_node(ND_NUM);
  node->val = val;
  return node;
}

static Node* expr(Token** rest, Token* tok);
static Node* equality(Token** rest, Token* tok);
static Node* relational(Token** rest, Token* tok);
static Node* add(Token** rest, Token* tok);
static Node* mul(Token** rest, Token* tok);
static Node* unary(Token** rest, Token* tok);
static Node* primary(Token** rest, Token* tok);

// expr = equality
static Node* expr(Token** rest, Token* tok) {
  return equality(rest, tok);
}

// equality = relational ("==" relational | "!=" relational)
static Node* equality(Token** rest, Token* tok) {
  Node* node = relational(&tok, tok);

  for (;;) {
    if (equal(tok, "==")) {
      node = new_binary(ND_EQ, node, relational(&tok, tok->next));
      continue;
    }

    if (equal(tok, "!=")) {
      node = new_binary(ND_NE, node, relational(&tok, tok->next));
      continue;
    }

    *rest = tok;
    return node;
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)
static Node* relational(Token** rest, Token* tok) {
  Node* node = add(&tok, tok);

  for (;;) {
    if (equal(tok, "<")) {
      node = new_binary(ND_LT, node, add(&tok, tok->next));
      continue;
    }

    if (equal(tok, "<=")) {
      node = new_binary(ND_LE, node, add(&tok, tok->next));
      continue;
    }

    if (equal(tok, ">")) {
      node = new_binary(ND_LT, add(&tok, tok->next), node);
      continue;
    }

    if (equal(tok, ">=")) {
      node = new_binary(ND_LE, add(&tok, tok->next), node);
      continue;
    }

    *rest = tok;
    return node;
  }
}

// expr = mul ("+" mul | "-" mul)*
static Node* add(Token** rest, Token* tok) {
  Node* node = mul(&tok, tok);

  for (;;) {
    if (equal(tok, "+")) {
      node = new_binary(ND_ADD, node, mul(&tok, tok->next));
      continue;
    }

    if (equal(tok, "-")) {
      node = new_binary(ND_SUB, node, mul(&tok, tok->next));
      continue;
    }

    *rest = tok;
    return node;
  }
}

// mul = unary ("*" unary | "/" unary)*
static Node* mul(Token** rest, Token* tok) {
  Node* node = unary(&tok, tok);

  for (;;) {
    if (equal(tok, "*")) {
      node = new_binary(ND_MUL, node, unary(&tok, tok->next));
      continue;
    }

    if (equal(tok, "/")) {
      node = new_binary(ND_DIV, node, unary(&tok, tok->next));
      continue;
    }

    *rest = tok;
    return node;
  }
}

// unary = ("+" | "-") unary
//       | primary
static Node* unary(Token** rest, Token* tok) {
  if (equal(tok, "+")) {
    return unary(rest, tok->next);
  }

  if (equal(tok, "-")) {
    return new_unary(ND_NEG, unary(rest, tok->next));
  }

  return primary(rest, tok);
}

// primary = "(" expr ")" | num
static Node* primary(Token** rest, Token* tok) {
  if (equal(tok, "(")) {
    Node* node = expr(&tok, tok->next);
    *rest = skip(tok, ")");
    return node;
  }

  if (tok->kind == TK_NUM) {
    Node* node = new_num(tok->val);
    *rest = tok->next;
    return node;
  }

  error_tok(tok, "expected an expression");
}

//
// Code generator
//

static int depth;

static void push() {
  printf("  push %%rax\n");
  depth++;
}

static void pop(char* arg) {
  printf("  pop %s\n", arg);
  depth--;
}

static void gen_expr(Node* node) {
  switch (node->kind) {
    case ND_NUM:
      printf("  mov $%d, %%rax\n", node->val);
      return;
    case ND_NEG:
      gen_expr(node->lhs);
      printf("  neg %%rax\n");
      return;
  }

  gen_expr(node->rhs);
  push();
  gen_expr(node->lhs);
  pop("%rdi");

  switch (node->kind) {
    case ND_ADD:
      printf("  add %%rdi, %%rax\n");
      return;
    case ND_SUB:
      printf("  sub %%rdi, %%rax\n");
      return;
    case ND_MUL:
      printf("  imul %%rdi, %%rax\n");
      return;
    case ND_DIV:
      printf("  cqo\n");  // RDX:RAX:= sign-extend of RAX.
      printf("  idiv %%rdi\n");
      return;
    case ND_EQ:  // fall-through
    case ND_NE:  // fall-through
    case ND_LT:  // fall-through
    case ND_LE:
      printf("  cmp %%rdi, %%rax\n");
      if (node->kind == ND_EQ) {
        printf("  sete %%al\n");
      } else if (node->kind == ND_NE) {
        printf("  setne %%al\n");
      } else if (node->kind == ND_LT) {
        printf("  setl %%al\n");
      } else if (node->kind == ND_LE) {
        printf("  setle %%al\n");
      }

      printf("  movzb %%al, %%rax\n");
      return;
  }

  error("invalid expression");
}

int main(int argc, char** argv) {
  if (argc != 2) {
    error("%s: invalid number of arguments\n", argv[0]);
  }

  current_input = argv[1];
  Token* tok = tokenize();
  Node* node = expr(&tok, tok);

  if (tok->kind != TK_EOF) {
    error_tok(tok, "extra token");
  }

  printf("  .globl main\n");
  printf("main:\n");

  // Traverse the AST to emit assembly.
  gen_expr(node);
  printf("  ret\n");

  assert(depth == 0);

  return 0;
}

