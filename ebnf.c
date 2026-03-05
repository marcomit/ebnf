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

#include <readline/readline.h>
#include <readline/history.h>

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

int main() {
	return 0;
}
