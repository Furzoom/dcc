#include <ctype.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  TK_PUNCT, // punctuators
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

// Reports an error and exit.
static void error(char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt,ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(1);
}

// Consumes the current token if it matches `op`.
static bool equal(Token* tok, char* op) {
  return memcmp(tok->loc, op, tok->len) == 0 && op[tok->len] == '\0';
}

// Ensure that the current token is 's'.
static Token* skip(Token* tok, char* s) {
  if (!equal(tok, s)) {
    error("expected '%s'", s);
  }
  return tok->next;
}

// Ensure that the current token is TK_NUM.
static int get_number(Token* tok) {
  if (tok->kind != TK_NUM) {
    error("expected a number");
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

// Tokenize `p` and returns new tokens.
static Token* tokenize(char* p) {
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
      cur->len = q - p;
      continue;
    }

    if (*p == '+' || *p == '-') {
      cur = cur->next = new_token(TK_PUNCT, p, p + 1);
      p++;
      continue;
    }

    error("invalid token");
  }

  cur->next = new_token(TK_EOF, p, p);
  return head.next;
}

int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "%s: invalid number of arguments\n", argv[0]);
    return 1;
  }

  Token* tok = tokenize(argv[1]);

  printf("  .globl main\n");
  printf("main:\n");

  // The first token must be number.
  printf("  mov $%d, %%rax\n", get_number(tok));
  tok = tok->next;

  // ... followed by either `+ <number>` or `- <number>`.
  while (tok->kind != TK_EOF) {
    if (equal(tok, "+")) {
      printf("  add $%d, %%rax\n", get_number(tok->next));
      tok = tok->next->next;
      continue;
    }

    tok = skip(tok, "-");
    printf("  sub $%d, %%rax\n", get_number(tok));
    tok = tok->next;
  }

  printf("  ret\n");
  return 0;
}

