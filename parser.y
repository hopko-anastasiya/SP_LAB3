%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

extern int yylex(void);
extern int line_num;
extern FILE *yyin;

void yyerror(const char *s);
ASTNode *ast_root = NULL;
%}

%union {
    int num;
    char *id;
    struct ASTNode *node;
}

%token <num> T_NUM
%token <id>  T_ID T_STRING
%token T_INT T_IF T_ELSE T_WHILE T_PRINT
%token T_EQ T_NE T_LT T_GT T_LE T_GE

%type <node> program stmt_list stmt block expr

%nonassoc LOWER_THAN_ELSE
%nonassoc T_ELSE

%left T_EQ T_NE
%left T_LT T_GT T_LE T_GE
%left '+' '-'
%left '*' '/'
%right UMINUS

%%

program:
    stmt_list               { ast_root = $1; }
    ;

stmt_list:
    stmt                    { $$ = $1; }
    | stmt_list stmt        { $$ = make_seq($1, $2); }
    ;

stmt:
    T_INT T_ID ';'          { $$ = make_decl($2, NULL); free($2); }
    | T_INT T_ID '=' expr ';' { $$ = make_decl($2, $4); free($2); }
    | T_ID '=' expr ';'     { $$ = make_assign($1, $3); free($1); }
    | T_PRINT '(' expr ')' ';' { $$ = make_print($3); }
    | T_PRINT '(' T_STRING ',' expr ')' ';' { $$ = make_print($5); free($3); }
    | T_IF '(' expr ')' block %prec LOWER_THAN_ELSE { $$ = make_if($3, $5, NULL); }
    | T_IF '(' expr ')' block T_ELSE block          { $$ = make_if($3, $5, $7); }
    | T_WHILE '(' expr ')' block                    { $$ = make_while($3, $5); }
    ;

block:
    '{' stmt_list '}'       { $$ = $2; }
    | '{' '}'               { $$ = NULL; }
    | stmt                  { $$ = $1; }
    ;

expr:
    T_NUM                   { $$ = make_num($1); }
    | T_ID                  { $$ = make_var($1); free($1); }
    | expr '+' expr         { $$ = make_binop("+", $1, $3); }
    | expr '-' expr         { $$ = make_binop("-", $1, $3); }
    | expr '*' expr         { $$ = make_binop("*", $1, $3); }
    | expr '/' expr         { $$ = make_binop("/", $1, $3); }
    | expr T_LT expr        { $$ = make_binop("<", $1, $3); }
    | expr T_GT expr        { $$ = make_binop(">", $1, $3); }
    | expr T_LE expr        { $$ = make_binop("<=", $1, $3); }
    | expr T_GE expr        { $$ = make_binop(">=", $1, $3); }
    | expr T_EQ expr        { $$ = make_binop("==", $1, $3); }
    | expr T_NE expr        { $$ = make_binop("!=", $1, $3); }
    | '-' expr %prec UMINUS { $$ = make_binop("-", make_num(0), $2); }
    | '(' expr ')'          { $$ = $2; }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "[Синтаксична помилка] Рядок %d: %s\n", line_num, s);
}

int main(int argc, char **argv) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            perror("Не вдалося відкрити вхідний файл");
            return 1;
        }
    }

    printf("ЗАПУСК СИНТАКСИЧНОГО АНАЛІЗАТОРА\n");
    if (yyparse() != 0 || ast_root == NULL) {
        fprintf(stderr, "Аналіз завершено з помилками.\n");
        return 1;
    }
    printf("Синтаксичний аналіз успішно завершено! AST побудовано.\n\n");

    printf("ІНТЕРПРЕТАЦІЯ AST\n");
    eval_ast(ast_root);
    printf("\n");

    printf("ГЕНЕРАЦІЯ ЦІЛЬОВОГО КОДУ (JAVASCRIPT)\n");
    FILE *js = fopen("out.js", "w");
    if (!js) {
        perror("Не вдалося створити out.js");
        return 1;
    }
    gen_js(ast_root, js);
    fclose(js);
    printf("Файл 'out.js' згенеровано! Запуск: node out.js\n\n");

    generate_d3_html(ast_root, "ast_visualizer.html");
    printf("ВІЗУАЛІЗАЦІЯ AST (D3.JS)\n");
    printf("Файл 'ast_visualizer.html' успішно згенеровано! Відкрийте його у браузері.\n");
    return 0;
}
