/* = definition
 * ; end rule
 * <space> concatenation
 * | alternative
 * [ ... ] optional 0 or 1
 * { ... } repetition 0 or more
 * ( ... ) group
 *
 * name = any character
 *
 * rule = name "=" pattern
 *
 * pattern 			= alternative
 * 							| optional
 * 							| repetition
 * 							| group
 *
 * sequence 		= pattern  [ " " pattern ]
 *
 * alternative 	= sequence [ "|" sequence ]
 *
 * optional 		= "[" pattern "]"
 *
 * repetition		= "{" pattern "}"
 * 
 * group				= "(" pattern ")"
 */

#include <ctype.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stddef.h>

#include "zvec.h"

#define RULE_TERM_MASK	(1 << 8)
#define RULE_UNARY_MASK (1 << 9)
#define RULE_MULTI_MASK (1 << 10)

typedef enum {
	TOK_IDENT,
	TOK_LIT,
	TOK_PIPE,
	TOK_LPAREN,
	TOK_RPAREN,
	TOK_LBRACKET,
	TOK_RBRACKET,
	TOK_LSPAREN,
	TOK_RSPAREN,
	TOK_EQ
} toktype;

typedef struct {
	toktype type;
	union {
		char *lexeme;
	};
} token;

typedef enum {
	RULE_TERM					= RULE_TERM_MASK,

	RULE_GROUP 				= RULE_UNARY_MASK,
	RULE_OPTIONAL,
	RULE_REPETITION,

	RULE_SEQUENCE 		= RULE_MULTI_MASK,
	RULE_ALTERNATIVE,
} ruletype;

typedef struct rule {
	ruletype type;
	union {
		token 			*tok;
		struct rule *child;
		struct rule **children;
	};
} rule;

typedef struct {
	token 	**iter;
	size_t 	current;
} context;

token *maketok(toktype type, char *lexeme) {
	token *self = calloc(1, sizeof(token));
	*self = (token){
		.type = type,
		.lexeme = lexeme
	};
	return self;
}

rule *makerule(ruletype type) {
	rule *self = calloc(1, sizeof(rule));
	self->type = type;
	return self;
}

token *scanLiteral(context *ctx, char **src, char bound) {
	if (**src != bound) return NULL;

	(*src)++;
	char *start = *src;
	while (**src && **src != bound) (*src)++;

	if (!**src) return NULL;

	return maketok(TOK_LIT, strndup(start, *src - start));
}

void skipUnuseless(context *ctx, char **src) {
	char *start = NULL;
	do {
		start = *src;
		while (isspace(**src)) (*src)++;
		
		if (**src == '/' && (*src)[1] == '/') {
			while (**src && **src != '\n') (*src)++;
		}
	} while (start != *src);
}

void scan(context *ctx, char **src) {
	while (**src) {
		skipUnuseless(ctx, src);

		if (!**src) break;

		token *tok = NULL;
		char *start = *src;

		if (**src == '\'') {
			tok = scanLiteral(ctx, src, '\'');
		} else if (**src == '"') {
			tok = scanLiteral(ctx, src, '"');
		} else {
			if (**src == '=') {
				tok = maketok(TOK_EQ, NULL);
			} else if (**src == '|') {
				tok = maketok(TOK_PIPE, NULL);
			} else if (**src == '(') {
				tok = maketok(TOK_LPAREN, NULL);
			} else if (**src == ')') {
				tok = maketok(TOK_RPAREN, NULL);
			} else if (**src == '[') {
				tok = maketok(TOK_LSPAREN, NULL);
			} else if (**src == '{') {
				tok = maketok(TOK_LBRACKET, NULL);
			} else if (**src == '}') {
				tok = maketok(TOK_RBRACKET, NULL);
			}
			(*src)++;
		}

		if (!tok) {
			printf("Error near %s\n", start);
			while (**src && **src != '\n')(*src)++;
		} else {
			vecpush(ctx->iter, tok);
		}
	}
}

int main() {
	context *ctx = calloc(1, sizeof(context));

		

	return 0;
}
