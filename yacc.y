%{
#include "utils.hpp"
void yyerror(const char *msg) {
    printf("Error at line %d:\n\t\t%s\n", yylineno, msg);
    exit(1);
}
%}

%token INT CONST VOID
%token IF ELSE WHILE BREAK CONTINUE RETURN
%token AND OR EQ NEQ LEQ GEQ
%token IDENT INT_CONST
%%

CompUnits       : CompUnits CompUnit
                |
                ;
CompUnit        : Decl
                | FuncDef
                ;

Decl            : ConstDecl
                | VarDecl
                ;
ConstDecl       : CONST INT ConstDefs ';'
                ;
ConstDefs       : ConstDefs ',' ConstDef
                | ConstDef
                ;
ConstDef        : IDENT ConstExps '=' ConstInitVal
                ;
ConstExps       : ConstExps '[' ConstExp ']'
                |
                ;
ConstInitVal    : ConstExp
                | '{' ConstInitVals '}'
                | '{' '}'
                ;
ConstInitVals   : ConstInitVals ',' ConstInitVal
                | ConstInitVal
                ;
VarDecl         : INT VarDefs ';'
                ;
VarDefs         : VarDefs ',' VarDef
                | VarDef
                ;
VarDef          : IDENT ConstExps
                | IDENT ConstExps '=' InitVal
                ;
InitVal         : Exp
                | '{' InitVals '}'
                | '{' '}'
                ;
InitVals        : InitVals ',' InitVal
                | InitVal
                ;

FuncDef         : INT IDENT '(' ')' Block
                | VOID IDENT '(' ')' Block
                | INT IDENT '(' FuncFParams ')' Block
                | VOID IDENT '(' FuncFParams ')' Block
                ;
FuncFParams     : FuncFParams ',' FuncFParam
                | FuncFParam
                ;
FuncFParam      : INT IDENT
                | INT IDENT '[' ']' ConstExps
                ;

Block           : '{' BlockItems '}'
                ;
BlockItems      : BlockItems BlockItem
                |
                ;
BlockItem       : Decl 
                | Stmt
                ;
Stmt            : LVal '=' Exp ';'
                | Exp ';'
                | ';'
                | Block
                | IF '(' Cond ')' Stmt
                | IF '(' Cond ')' Stmt ELSE Stmt
                | WHILE '(' Cond ')' Stmt
                | BREAK ';'
                | CONTINUE ';'
                | RETURN Exp ';'
                | RETURN ';'
                ;

Exp             : AddExp
                ;
Cond            : LOrExp
                ;
LVal            : IDENT Exps
                ;
Exps            : Exps '[' Exp ']'
                |
                ;
PrimaryExp      : '(' Exp ')'
                | LVal
                | Number
                ;
Number          : INT_CONST
                ;
UnaryExp        : PrimaryExp
                | IDENT '(' ')'
                | IDENT '(' FuncRParams ')'
                | UnaryOp UnaryExp
                ;
UnaryOp         : '+'
                | '-'
                | '!'
                ;
FuncRParams     : FuncRParams ',' Exp
                | Exp
                ;
MulExp          : UnaryExp
                | MulExp '*' UnaryExp
                | MulExp '/' UnaryExp
                | MulExp '%' UnaryExp
                ;
AddExp          : MulExp
                | AddExp '+' MulExp
                | AddExp '-' MulExp
                ;
RelExp          : AddExp
                | RelExp '<' AddExp
                | RelExp '>' AddExp
                | RelExp LEQ AddExp
                | RelExp GEQ AddExp
                ;
EqExp           : RelExp
                | EqExp EQ RelExp
                | EqExp NEQ RelExp
                ;
LAndExp         : EqExp
                | LAndExp AND EqExp
                ;
LOrExp          : LAndExp
                | LOrExp OR LAndExp
                ;
ConstExp        : AddExp
                ;



%%
