%{
#include "utils.hpp"
%}
%%
%%
void yyerror(const char *msg) {
    printf("Error at line %d:\n\t\t%s\n", line_num, msg);
    exit(1);
}