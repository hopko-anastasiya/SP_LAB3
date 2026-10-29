%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "parser.tab.h"

int line_num = 1;
%}

%option noyywrap nounput noinput

%%

"int"           { return T_INT; }
"if"            { return T_IF; }
"else"          { return T_ELSE; }
"while"         { return T_WHILE; }
"printf"        { return T_PRINT; }
"print"         { return T_PRINT; }

"=="            { return T_EQ; }
"!="            { return T_NE; }
"<="            { return T_LE; }
">="            { return T_GE; }
"<"             { return T_LT; }
">"             { return T_GT; }

"+"             { return '+'; }
"-"             { return '-'; }
"*"             { return '*'; }
"/"             { return '/'; }
"="             { return '='; }
";"             { return ';'; }
","             { return ','; }
"("             { return '('; }
")"             { return ')'; }
"{"             { return '{'; }
"}"             { return '}'; }

[0-9]+          {
                    yylval.num = atoi(yytext);
                    return T_NUM;
                }

[a-zA-Z_][a-zA-Z0-9_]* {
                    yylval.id = strdup(yytext);
                    return T_ID;
                }

\"(\\.|[^"\\\n])*\" {
                    yylval.id = strdup(yytext);
                    return T_STRING;
                }

"//".*          { }

"/*"([^*]|\*+[^*/])*\*+"/" {
                    for (char *p = yytext; *p; p++)
                        if (*p == '\n') line_num++;
                }

[ \t\r]+        { }
\n              { line_num++; }

.               {
                    fprintf(stderr, "[Лексична помилка] Рядок %d: Невідомий символ '%s'\n", line_num, yytext);
                    exit(1);
                }

%%
