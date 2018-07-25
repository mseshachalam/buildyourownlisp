#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"

// enumerated values for the lval.type field
enum { LVAL_NUM, LVAL_ERR };

// division by zero, bad operation, too large to be represented by long
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };


// list value
// If type is 0 then the structure is a Number.", or "If type is 1 then the structure is an Error.
typedef struct {
	int type;
	union {
		int err;
		double num;
	} data;
} lval;

lval lval_num(double x) {
	lval v;
	v.type = LVAL_NUM;
	v.data.num = x;
	return v;
}

lval lval_err(int x) {
	lval v;
	v.type = LVAL_ERR;
	v.data.err = x;
	return v;
}

void lval_print(lval v) {
	switch (v.type) {
		case LVAL_NUM:
			printf("%f", v.data.num);
			break;
		case LVAL_ERR:
			if (v.data.err == LERR_DIV_ZERO) {
				printf("Error: Division By Zero!");
			} 
			if (v.data.err == LERR_BAD_OP) {
				printf("Error: Invalid Operator!");
			}
			if (v.data.err == LERR_BAD_NUM) {
				printf("Error: Invalid Number!");
			}
			break;
	}
}

void lval_println(lval v) {
	lval_print(v);
	putchar('\n');
}

lval eval_op(lval x, char* op, lval y) {
	if (x.type == LVAL_ERR) {
		return x;
	}
	if (y.type == LVAL_ERR) {
		return y;
	}

	if (strcmp(op, "+") == 0){
		return lval_num(x.data.num + y.data.num);
	}
	if (strcmp(op, "-") == 0){
		return lval_num(x.data.num - y.data.num);
	}
	if (strcmp(op, "*") == 0){
		return lval_num(x.data.num * y.data.num);
	}
	if (strcmp(op, "/") == 0){
		return	y.data.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.data.num / y.data.num);
	}
	if (strcmp(op, "%") == 0){
		return lval_num((x.data.num - y.data.num * floor(x.data.num / y.data.num)));
	}
	if (strcmp(op, "^") == 0){
		double raise = x.data.num;
		while (--y.data.num) {
			raise = raise * x.data.num;
		}
		return lval_num(raise);
	}
	if (strcmp(op, "max") == 0){
		if (x.data.num > y.data.num) {
			return lval_num(x.data.num);
		} else {
			return lval_num(y.data.num);
		}
	}
	if (strcmp(op, "min") == 0){
		if (x.data.num > y.data.num) {
			return lval_num(y.data.num);
		} else {
			return lval_num(x.data.num);
		}
	}

	return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {
	if (strstr(t->tag, "number")) {
		char *err;
		double x = strtod(t->contents, &err);
		if (err == t->contents) { 
			return lval_err(LERR_BAD_NUM);
		} else {
			return lval_num(x);
		}
	}

	char* op = t->children[1]->contents;
	lval x = eval(t->children[2]);
	
	int i = 3;
	while (strstr(t->children[i]->tag, "expr")) {
		x = eval_op(x, op, eval(t->children[i]));
		i++;
	}
	if (i == 3 && strcmp(op, "-") == 0) {
		x = lval_num(-1 * x.data.num);
	}

	return x;
}

int main(int argc, char** argv) {
	puts("lispy version 0.0.0.0.1");
	puts("press Ctrl+c to exit\n");

	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Operator = mpc_new("operator");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Lispy = mpc_new("lispy");
	
	mpca_lang(MPCA_LANG_DEFAULT,
	"			\
	 number : /-?[0-9]+/ ;  \
	 operator : '+' | '-' | '*' | '/' | '%' | '^' | \"min\" | \"max\" ; \
	 expr : <number> | '(' <operator> <expr>+ ')' ; \
	 lispy : /^/ <operator> <expr>+ /$/; \
	",
	Number, Operator, Expr, Lispy);
	
	while (1) {
		char* input = readline("lispy> ");
		add_history(input);
//		printf("No you'are a %s\n", input);
		mpc_result_t r;
		if (mpc_parse("<stdin>", input, Lispy, &r)) {
			lval result = eval(r.output);
			lval_println(result);
//			printf("%li\n", result);
//			mpc_ast_print(r.output);
			mpc_ast_delete(r.output);
		} else {
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		free(input);
	}

	mpc_cleanup(4, Number, Operator, Expr, Lispy);
	return 0;
}
