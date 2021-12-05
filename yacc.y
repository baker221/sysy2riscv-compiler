%{
#include "utils.hpp"
vector<string> mycode;
void emit(string s) {
  mycode.push_back(s);
}
void emitLabel(int label_num) {
  emit("l" + to_string(label_num) + ":");
}
Parser parser;
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
ConstDef        : IDENT ConstExps {
                    $$ = new Variable(true);
                    parser.top->putVar(*(string *) $1, (Variable *)$$);
                  } '=' ConstInitVal
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
VarDef          : IDENT ConstExps {
                    $$ = new Variable(false);
                    parser.top->putVar(*(string *) $1, (Variable *)$$);
                  }
                | IDENT ConstExps {
                    $$ = new Variable(false);
                    parser.top->putVar(*(string *) $1, (Variable *)$$);
                  }'=' InitVal
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
Stmt            : LVal '=' Exp ';' {
                    emit(((Variable *)$1)->getname() + "=" + ((Variable *)$3)->getname());
                  }
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

Exp             : AddExp { $$ = $1; }
                ;
Cond            : LOrExp { $$ = $1; /* TODO only get true label here, false label need to be generated upper level. */}
                ;
LVal            : IDENT Exps
                ;
Exps            : Exps '[' Exp ']'
                |
                ;
PrimaryExp      : '(' Exp ')' { $$ = $2; }
                | LVal {
                    $$ = $1; // TODO: array condition
                  }
                | Number { $$ = $1; }
                ;
Number          : INT_CONST {
                    $$ = new Variable(*(int *)$1);
                  }
                ;
UnaryExp        : PrimaryExp
                | IDENT '(' ')'
                | IDENT '(' FuncRParams ')'
                | '+' UnaryExp { $$ = $2; }
                | '-' UnaryExp {
                    if (((Variable *)$2)->checkConst()) {
                      $$ = new Variable(-((Variable *)$2)->value);
                    } else {
                      $$ = new Variable(false);
                      emit(((Variable *)$$)->getname() + "=-" + ((Variable *)$2)->getname());
                    }
                  }
                | '!' UnaryExp {
                    if (((Variable *)$2)->checkConst()) {
                      $$ = new Variable(!((Variable *)$2)->value);
                    } else {
                      $$ = new Variable(false);
                      emit(((Variable *)$$)->getname() + "=!" + ((Variable *)$2)->getname());
                    }
                  }
                ;
FuncRParams     : FuncRParams ',' Exp
                | Exp
                ;
MulExp          : UnaryExp { $$ = $1; }
                | MulExp '*' UnaryExp {
                    if (((Variable *)$1)->checkConst() && ((Variable *)$3)->checkConst()) {
                      $$ = new Variable(((Variable *)$1)->value * ((Variable *)$3)->value);
                    } else {
                      $$ = new Variable(false);
                      emit(((Variable *)$$)->getname() + "=" + ((Variable *)$1)->getname() + "*" + ((Variable *)$3)->getname());
                    }
                  }
                | MulExp '/' UnaryExp {
                    if (((Variable *)$1)->checkConst() && ((Variable *)$3)->checkConst()) {
                      $$ = new Variable(((Variable *)$1)->value / ((Variable *)$3)->value);
                    } else {
                      $$ = new Variable(false);
                      emit(((Variable *)$$)->getname() + "=" + ((Variable *)$1)->getname() + "/" + ((Variable *)$3)->getname());
                    }
                  }
                | MulExp '%' UnaryExp {
                    if (((Variable *)$1)->checkConst() && ((Variable *)$3)->checkConst()) {
                      $$ = new Variable(((Variable *)$1)->value % ((Variable *)$3)->value);
                    } else {
                      $$ = new Variable(false);
                      emit(((Variable *)$$)->getname() + "=" + ((Variable *)$1)->getname() + "%" + ((Variable *)$3)->getname());
                    }
                  }
                ;
AddExp          : MulExp { $$ = $1; }
                | AddExp '+' MulExp {
                    if (((Variable *)$1)->checkConst() && ((Variable *)$3)->checkConst()) {
                      $$ = new Variable(((Variable *)$1)->value + ((Variable *)$3)->value);
                    } else {
                      $$ = new Variable(false);
                      emit(((Variable *)$$)->getname() + "=" + ((Variable *)$1)->getname() + "+" + ((Variable *)$3)->getname());
                    }
                  }
                | AddExp '-' MulExp {
                    if (((Variable *)$1)->checkConst() && ((Variable *)$3)->checkConst()) {
                      $$ = new Variable(((Variable *)$1)->value - ((Variable *)$3)->value);
                    } else {
                      $$ = new Variable(false);
                      emit(((Variable *)$$)->getname() + "=" + ((Variable *)$1)->getname() + "-" + ((Variable *)$3)->getname());
                    }
                  }
                ;
RelExp          : AddExp { $$ = $1; }
                | RelExp '<' AddExp {
                    if (((Variable *)$1)->checkConst() && ((Variable *)$3)->checkConst()) {
                      int t = (int)(((Variable *)$1)->value < ((Variable *)$3)->value);
                      $$ = new Variable(t);
                    } else {
                      $$ = new Variable(false);
                      emit(((Variable *)$$)->getname() + "=" + ((Variable *)$1)->getname() + "<" + ((Variable *)$3)->getname());
                    }
                  }
                | RelExp '>' AddExp {
                    if (((Variable *)$1)->checkConst() && ((Variable *)$3)->checkConst()) {
                      int t = (int)(((Variable *)$1)->value > ((Variable *)$3)->value);
                      $$ = new Variable(t);
                    } else {
                      $$ = new Variable(false);
                      emit(((Variable *)$$)->getname() + "=" + ((Variable *)$1)->getname() + ">" + ((Variable *)$3)->getname());
                    }
                  }
                | RelExp LEQ AddExp {
                    if (((Variable *)$1)->checkConst() && ((Variable *)$3)->checkConst()) {
                      int t = (int)(((Variable *)$1)->value <= ((Variable *)$3)->value);
                      $$ = new Variable(t);
                    } else {
                      $$ = new Variable(false);
                      emit(((Variable *)$$)->getname() + "=" + ((Variable *)$1)->getname() + "<=" + ((Variable *)$3)->getname());
                    }
                  }
                | RelExp GEQ AddExp {
                    if (((Variable *)$1)->checkConst() && ((Variable *)$3)->checkConst()) {
                      int t = (int)(((Variable *)$1)->value >= ((Variable *)$3)->value);
                      $$ = new Variable(t);
                    } else {
                      $$ = new Variable(false);
                      emit(((Variable *)$$)->getname() + "=" + ((Variable *)$1)->getname() + ">=" + ((Variable *)$3)->getname());
                    }
                  }
                ;
EqExp           : RelExp { $$ = $1; }
                | EqExp EQ RelExp {
                    if (((Variable *)$1)->checkConst() && ((Variable *)$3)->checkConst()) {
                      int t = (int)(((Variable *)$1)->value == ((Variable *)$3)->value);
                      $$ = new Variable(t);
                    } else {
                      $$ = new Variable(false);
                      emit(((Variable *)$$)->getname() + "=" + ((Variable *)$1)->getname() + "==" + ((Variable *)$3)->getname());
                    }
                  }
                | EqExp NEQ RelExp {
                    if (((Variable *)$1)->checkConst() && ((Variable *)$3)->checkConst()) {
                      int t = (int)(((Variable *)$1)->value != ((Variable *)$3)->value);
                      $$ = new Variable(t);
                    } else {
                      $$ = new Variable(false);
                      emit(((Variable *)$$)->getname() + "=" + ((Variable *)$1)->getname() + "!=" + ((Variable *)$3)->getname());
                    }
                  }
                ;
LAndExp         : EqExp {
                    int cur_label_num = genLabel();
                    $$ = new int(cur_label_num); // For And expression, store false label here.
                    emit("if " + ((Variable *)$1)->getname() + "==0 goto l" + to_string(cur_label_num));
                  }
                | LAndExp AND EqExp {
                    $$ = $1;
                    emit("if " + ((Variable *)$3)->getname() + "==0 goto l" + to_string(*(int *)$$));
                  }
                ;
LOrExp          : LAndExp {
                    int cur_label_num = genLabel();
                    $$ = new int(cur_label_num); // For Or expression, store true label here.
                    emit("goto l" + to_string(cur_label_num));
                    emitLabel(*(int *)$1); // false label
                  }
                | LOrExp OR LAndExp {
                    $$ = $1;
                    emit("goto l" + to_string(*(int *)$$));
                    emitLabel(*(int *)$3); // false label
                  }
                ;
ConstExp        : AddExp {
                    assert(((Variable *)$1)->checkConst());
                    $$ = $1;
                  }
                ;

%%
