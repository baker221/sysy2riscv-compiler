%{
#include "utils.hpp"
#include "yacc.tab.hpp"
extern "C" {
    int yylex();
    int yyparse();
}
%}
%option yylineno

comment1 "//"[^\n]*
comment2 "/*"([^\*]|(\*)*[^\*/])*(\*)*"*/"
space [ \t]
newline [\r\n]
digit10 [0-9]
digit8 [0-7]
digit16 [0-9A-Fa-f]
integer10 [1-9]{digit10}*
integer8 0{digit8}*
integer16 (0x|0X){digit16}+
identifier [_a-zA-Z]([_a-zA-Z0-9])*

%%

{space}                            {}
{newline}                          {}
{comment1}                         {}
{comment2}                         {printf("comment2 here.\n");}
int                                {return INT;}
const                              {return CONST;}
void                               {return VOID;}
if                                 {return IF;}
else                               {return ELSE;}
while                              {return WHILE;}
break                              {return BREAK;}
continue                           {return CONTINUE;}
return                             {return RETURN;}
"&&"                               {return AND;}
"||"                               {return OR;}
"=="                               {return EQ;}
"!="                               {return NEQ;}
">="                               {return GEQ;}
"<="                               {return LEQ;}
{identifier}                       {return IDENT;}
{integer10}|{integer8}|{integer16} {
    yylval = new int;
    sscanf(yytext, "%i", (int*)yylval);
    return INT_CONST;
}
.                                  {return yytext[0];}

%%
int yywrap(void) {
    return 1;
}