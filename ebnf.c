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

#define print(x) _Generic((x),																		\
	token *: printTok,																							\
	rule *: printRule																								\
)(x)


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
	RULE_DEFINITION,
	RULE_TERM					= RULE_TERM_MASK,
	RULE_REFERENCE,

	RULE_OPTIONAL			= RULE_UNARY_MASK,
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

	/* A list of RULE_DEFINITION */
	rule  	**rules;
} context;

void printRule(rule *);
void printTok(token *);
rule *parsePattern(context *);

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

rule *maketerm(ruletype type, token *tok) {
	rule *self = makerule(type);
	self->tok = tok;
	return self;
}

rule *makeunary(ruletype type, rule *child) {
	rule *self = makerule(type);
	self->child = child;
	return self;
}

rule *makemulti(ruletype type, rule **children) {
	rule *self = makerule(type);
	self->children = children;
	return self;
}

token *scanLiteral(context *ctx, char **src, char bound) {
	if (**src != bound) return NULL;

	(*src)++;
	char *start = *src;
	while (**src && **src != bound) (*src)++;

	if (!**src) return NULL;
	(*src)++;

	return maketok(TOK_LIT, strndup(start, *src - start - 1));
}

token *scanIdent(context *ctx, char **src) {
	char *start = *src;
	while (isalnum(**src) || **src == '_') (*src)++;
	
	return maketok(TOK_IDENT, strndup(start, *src - start));
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

int scan(context *ctx, char **src) {
	while (**src) {
		skipUnuseless(ctx, src);

		if (!**src) break;

		token *tok = NULL;
		char *start = *src;

		if (**src == '\'') {
			tok = scanLiteral(ctx, src, '\'');
		} else if (**src == '"') {
			tok = scanLiteral(ctx, src, '"');
		} else if (isalpha(**src) || **src == '_') {
			tok = scanIdent(ctx, src);
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
			} else if (**src == ']') {
				tok = maketok(TOK_RSPAREN, NULL);
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
	return **src;
}

int canPeek(context *ctx) { return ctx->current < veclen(ctx->iter); }

int check(context *ctx, toktype type) {
	if (!canPeek(ctx)) return 0;
	return ctx->iter[ctx->current]->type == type;
}

int match(context *ctx, toktype type) {
	if (!check(ctx, type)) return 0;
	ctx->current++;
	return 1;
}

token *consume(context *ctx) {
	token *tok = ctx->iter[ctx->current];
	ctx->current++;
	return tok;
}

rule *parseSeq(context *ctx) {
	rule **children = NULL;
	rule *current = NULL;

	while (( current = parsePattern(ctx) )) {
		vecpush(children, current);
	}

	if (veclen(children) == 1) {
		current = children[0];
		vecfree(children);
		return current;
	}

	return makemulti(RULE_SEQUENCE, children);
}

rule *parseOr(context *ctx) {
	rule **children = NULL;
	rule *pattern = parseSeq(ctx);

	if (!pattern) return NULL;
	if (!check(ctx, TOK_PIPE)) return pattern;

	while (match(ctx, TOK_PIPE)) {
		pattern = parseSeq(ctx);
		if (!pattern) {
			return NULL;
		}
		vecpush(children, pattern);
	}

	return makemulti(RULE_ALTERNATIVE, children);
}

rule *parseWrapped(context *ctx, toktype left, toktype right) {
	if (!match(ctx, left)) {
		return NULL;
	}
	rule *pattern = parseOr(ctx);
	if (!pattern) return NULL;
	if (!match(ctx, right)) {
		return NULL;
	}

	return pattern;
}

rule *parseOptional(context *ctx) {
	rule *child = parseWrapped(ctx, TOK_LSPAREN, TOK_RSPAREN);
	if (!child) return NULL;
	return makeunary(RULE_OPTIONAL, child);
}

rule *parseRepetition(context *ctx) {
	rule *child = parseWrapped(ctx, TOK_LBRACKET, TOK_RBRACKET);
	if (!child) return NULL;
	return makeunary(RULE_REPETITION, child);
}

rule *parseGroup(context *ctx) {
	return parseWrapped(ctx, TOK_LPAREN, TOK_RPAREN);
}

rule *parseReference(context *ctx) {
	if (!check(ctx, TOK_IDENT)) return NULL;
	return maketerm(RULE_REFERENCE, consume(ctx));
}

rule *parseTerm(context *ctx) {
	if (!check(ctx, TOK_LIT)) return NULL;
	return maketerm(RULE_TERM, consume(ctx));
}

rule *parsePattern(context *ctx) {
	rule *(*funcs[])(context *) = {
		parseGroup,
		parseRepetition,
		parseOptional,
		parseReference,
		parseTerm
	};
	size_t len = sizeof(funcs) / sizeof(funcs[0]);
	size_t saved = ctx->current;
	for (size_t i = 0; i < len; i++) {
		rule *parsed = funcs[i](ctx);
		if (parsed) return parsed;

		ctx->current = saved;
	}
	return NULL;
}

rule *parseRuleDecl(context *ctx) {
	if (!check(ctx, TOK_IDENT)) {
		printf("Expected an identifier\n");
		return NULL;
	}

	rule *name = maketerm(RULE_REFERENCE, consume(ctx));

	if (!match(ctx, TOK_EQ)) {
		printf("Expected '='");
		return NULL;
	}

	rule *pattern = parseOr(ctx);

	if (!pattern) {
		return NULL;
	}

	rule **children = NULL;
	vecpush(children, name);
	vecpush(children, pattern);

	return makemulti(RULE_DEFINITION, children);
}

void parse(context *ctx) {
	rule *r = NULL;
	while (canPeek(ctx)) {
		r = parseRuleDecl(ctx);
		if (!r) {
			print(ctx->iter[ctx->current]);
			printf("\nInvalid expression\n");
			return;
		}
		print(r);
	}
}

void printTok(token *tok) {
	switch (tok->type) {
	case TOK_EQ: 				printf("="); 										break;
	case TOK_LBRACKET: 	printf("{"); 										break;
	case TOK_RBRACKET: 	printf("}"); 										break;
	case TOK_LSPAREN: 	printf("["); 										break;
	case TOK_RSPAREN: 	printf("]"); 										break;
	case TOK_LPAREN: 		printf("("); 										break;
	case TOK_RPAREN: 		printf(")"); 										break;
	case TOK_PIPE: 			printf("|"); 										break;
	case TOK_IDENT: 		printf("%s", tok->lexeme); 			break;
	case TOK_LIT: 			printf("\"%s\"", tok->lexeme); 	break;
	}
}

void printRuleWithIndent(rule *r, int depth) {
	if (!r) return;
	for (int i = 0; i < depth; i++) printf("  ");

	depth++;
	
	switch (r->type) {
	case RULE_ALTERNATIVE:
		printf("or:\n");
		break;
	case RULE_DEFINITION:
		printf("rule[%s]:\n", r->children[0]->tok->lexeme);
		printRuleWithIndent(r->children[1], depth);
		break;
	case RULE_OPTIONAL:
		printf("optional:\n");
		break;
	case RULE_REFERENCE:
		break;
	case RULE_REPETITION:
		printf("repetition:\n");
		break;
	case RULE_SEQUENCE:
		printf("sequence:\n");
		break;
	default: break;
	}
	if (r->type & RULE_TERM_MASK) {
		print(r->tok);
		printf("\n");
	} else if (r->type & RULE_UNARY_MASK) {
		printRuleWithIndent(r->child, depth);
	} else if (r->type & RULE_MULTI_MASK) {
		for (size_t i = 0; i < veclen(r->children); i++) {
			printRuleWithIndent(r->children[i], depth);
		}
	}
}

void printRule(rule *r) { printRuleWithIndent(r, 0); }


void repl(context *ctx) {
	while (1) {
		ctx->current = 0;

		char *line = readline(">> ");

		if (!line) break;
		if (strcmp(line, "exit") == 0) {
			printf("Goodbye by marcomit\n");
			return;
		} else if (strcmp(line, "clear") == 0) {
			printf("\033[2J\033[0H");
			continue;
		} else if (strcmp(line, "exec ") == 0) {
			
		}

		scan(ctx, &line);

		parse(ctx);
	}
}

int main() {
	context *ctx = calloc(1, sizeof(context));

	repl(ctx);

	return 0;
}
