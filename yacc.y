%{
#include "utils.hpp"
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
ConstDef        : IDENT ConstExps { // const variables
                    string name = *(string *)$1;
                    deque<int> *shape = (deque<int> *)$2;
                    if (shape->size() == 0) {
                      $$ = new Variable(true);
                    } else {
                      $$ = new Variable(true, shape);
                    }
                    parser.top->putVar(name, (Variable *)$$);
                  } '=' ConstInitVal
                ;
ConstExps       : ConstExps '[' ConstExp ']' {
                    ((deque<int> *)$1)->push_back(((Variable *)$3)->value);
                    $$ = $1;
                  }
                | { $$ = new deque<int>(); }
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
VarDef          : IDENT ConstExps { // var variables
                    string name = *(string *)$1;
                    deque<int> *shape = (deque<int> *)$2;
                    if (shape->size() == 0) {
                      $$ = new Variable(false);
                    } else {
                      $$ = new Variable(false, shape);
                    }
                    parser.top->putVar(name, (Variable *)$$);
                  }
                | IDENT ConstExps {
                    string name = *(string *)$1;
                    deque<int> *shape = (deque<int> *)$2;
                    if (shape->size() == 0) {
                      $$ = new Variable(false);
                    } else {
                      $$ = new Variable(false, shape);
                    }
                    parser.top->putVar(name, (Variable *)$$);
                  }'=' InitVal
                ;
InitVal         : Exp
                | '{' InitVals '}'
                | '{' '}'
                ;
InitVals        : InitVals ',' InitVal
                | InitVal
                ;

FuncDef         : INT IDENT '(' ')' {
                    string name = *(string *) $2;
                    int param_num = 0;
                    Function *func = new Function(param_num, 1);
                    parser.putFunc(name, func);
                    emit("f_" + name + " [" + to_string(param_num) + "]");
                  }
                  Block {
                    string name = *(string *) $2;
                    emit("return 0");
                    emit("end f_" + name)
                  }
                | VOID IDENT '(' ')' {
                    string name = *(string *) $2;
                    int param_num = 0;
                    Function *func = new Function(param_num, 0);
                    parser.putFunc(name, func);
                    emit("f_" + name + " [" + to_string(param_num) + "]");
                  }
                  Block {
                    string name = *(string *) $2;
                    emit("return");
                    emit("end f_" + name)
                  }
                | INT IDENT '(' {
                    parser.pushEnv(true);
                  }
                  FuncFParams ')' {
                    string name = *(string *) $2;
                    int param_num = *(int *)$5;
                    Function *func = new Function(param_num, 1);
                    parser.putFunc(name, func);
                    emit("f_" + name + " [" + to_string(param_num) + "]");
                  }
                  Block {
                    parser.popEnv();
                    string name = *(string *) $2;
                    emit("return 0");
                    emit("end f_" + name)
                  }
                | VOID IDENT '(' {
                    parser.pushEnv(true);
                  }
                  FuncFParams ')' {
                    string name = *(string *) $2;
                    int param_num = *(int *)$5;
                    Function *func = new Function(param_num, 0);
                    parser.putFunc(name, func);
                    emit("f_" + name + " [" + to_string(param_num) + "]");
                  }
                  Block {
                    parser.popEnv();
                    string name = *(string *) $2;
                    emit("return");
                    emit("end f_" + name)
                  }
                ;
FuncFParams     : FuncFParams ',' INT IDENT { // 如果将FuncFParam分开表示，无法为函数参数编号
                    $$ = new int(*(int *)$1 + 1);
                    string name = *(string *) $4;
                    parser.top->putVar(name, new Variable(v_param, *(int *)$1 - 1, NULL));
                  }
                | FuncFParams ',' INT IDENT '[' ']' ConstExps {
                    $$ = new int(*(int *)$1 + 1);
                    string name = *(string *) $4;
                    deque<int> *t = (deque<int> *)$7;
                    t->push_front(0);
                    parser.top->putVar(name, new Variable(v_param, *(int *)$1 - 1,t));
                  }
                | INT IDENT {
                    $$ = new int(1);
                    string name = *(string *) $2;
                    parser.top->putVar(name, new Variable(v_param, 0, NULL));
                  }
                | INT IDENT '[' ']' ConstExps {
                    $$ = new int(1);
                    string name = *(string *) $2;
                    deque<int> *t = (deque<int> *)$5;
                    t->push_front(0);
                    parser.top->putVar(name, new Variable(v_param, 0, t));
                  }
                ;

Block           : '{' {
                    parser.pushEnv(false);
                  }
                  BlockItems '}' {
                    parser.popEnv();
                  }
                ;
BlockItems      : BlockItems BlockItem
                |
                ;
BlockItem       : Decl 
                | Stmt
                ;
Stmt            : LVal '=' Exp ';' {
                    emit(((Variable *)$1)->getName() + "=" + ((Variable *)$3)->getName());
                  }
                | Exp ';'
                | ';'
                | Block
                | IF '(' Cond ')' {
                    $1 = new IfStmt(*(int *)$3, genLabel(), genLabel());
                    emit("goto l" + to_string(((IfStmt *)$1)->label_false)); // goto false
                    emitLabel(((IfStmt *)$1)->label_true); // generate true label
                  }
                  Stmt {
                    emit("goto l" + to_string(((IfStmt *)$1)->label_after)); 
                    emitLabel(((IfStmt *)$1)->label_false); // generate false label
                  }
                  Danglingelse {
                    emitLabel(((IfStmt *)$1)->label_after); // generate after label
                  }
                | WHILE {
                    $1 = new WhileStmt(genLabel(), 0, genLabel());
                    emitLabel(((WhileStmt *)$1)->label_begin); // generate begin label
                    parser.while_stack.push_back((WhileStmt *)$1);
                  }
                  '(' Cond ')' {
                    ((WhileStmt *)$1)->label_body = *(int *)$4;
                    emit("goto l" + to_string(((WhileStmt *)$1)->label_after)); // goto after
                    emitLabel(((WhileStmt *)$1)->label_body); // generate body label
                  }
                  Stmt {
                    emit("goto l" + to_string(((WhileStmt *)$1)->label_begin)); // goto begin
                    emitLabel(((WhileStmt *)$1)->label_after); // generate after label
                    parser.while_stack.pop_back();
                  }
                | BREAK ';' {
                    if (parser.while_stack.size() == 0) {
                      yyerror("Not in while loop");
                    }
                    emit("goto l" + to_string(parser.while_stack.back()->label_after));
                  }
                | CONTINUE ';' {
                    if (parser.while_stack.size() == 0) {
                      yyerror("Not in while loop");
                    }
                    emit("goto l" + to_string(parser.while_stack.back()->label_begin));
                  }
                | RETURN Exp ';'
                | RETURN ';'
                ;
Danglingelse    : ELSE Stmt
                |
                ;

Exp             : AddExp { $$ = $1; }
                ;
Cond            : LOrExp { $$ = $1; }
                ;
LVal            : IDENT Exps {
                    string name = *(string *) $1;
                    $$ = parser.top->getVar(name);
                    deque<Variable *> *t = (deque<Variable *> *)$2;
                    if (t->size() != 0) { // TODO array access condition

                    }
                  }
                ;
Exps            : Exps '[' Exp ']' {
                    ((deque<Variable *> *)$1)->push_back((Variable *)$3); // a[i] condition, i can be variable
                    $$ = $1;
                  }
                | { $$ = new deque<Variable *>(); }
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
UnaryExp        : PrimaryExp { $$ = $1; }
                | IDENT '(' ')'
                | IDENT '(' FuncRParams ')'
                | '+' UnaryExp { $$ = $2; }
                | '-' UnaryExp {
                    if (((Variable *)$2)->checkConst()) {
                      $$ = new Variable(-((Variable *)$2)->value);
                    } else {
                      $$ = new Variable(false);
                      emit(((Variable *)$$)->getName() + "=-" + ((Variable *)$2)->getName());
                    }
                  }
                | '!' UnaryExp {
                    if (((Variable *)$2)->checkConst()) {
                      $$ = new Variable(!((Variable *)$2)->value);
                    } else {
                      $$ = new Variable(false);
                      emit(((Variable *)$$)->getName() + "=!" + ((Variable *)$2)->getName());
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
                      emit(((Variable *)$$)->getName() + "=" + ((Variable *)$1)->getName() + "*" + ((Variable *)$3)->getName());
                    }
                  }
                | MulExp '/' UnaryExp {
                    if (((Variable *)$1)->checkConst() && ((Variable *)$3)->checkConst()) {
                      $$ = new Variable(((Variable *)$1)->value / ((Variable *)$3)->value);
                    } else {
                      $$ = new Variable(false);
                      emit(((Variable *)$$)->getName() + "=" + ((Variable *)$1)->getName() + "/" + ((Variable *)$3)->getName());
                    }
                  }
                | MulExp '%' UnaryExp {
                    if (((Variable *)$1)->checkConst() && ((Variable *)$3)->checkConst()) {
                      $$ = new Variable(((Variable *)$1)->value % ((Variable *)$3)->value);
                    } else {
                      $$ = new Variable(false);
                      emit(((Variable *)$$)->getName() + "=" + ((Variable *)$1)->getName() + "%" + ((Variable *)$3)->getName());
                    }
                  }
                ;
AddExp          : MulExp { $$ = $1; }
                | AddExp '+' MulExp {
                    if (((Variable *)$1)->checkConst() && ((Variable *)$3)->checkConst()) {
                      $$ = new Variable(((Variable *)$1)->value + ((Variable *)$3)->value);
                    } else {
                      $$ = new Variable(false);
                      emit(((Variable *)$$)->getName() + "=" + ((Variable *)$1)->getName() + "+" + ((Variable *)$3)->getName());
                    }
                  }
                | AddExp '-' MulExp {
                    if (((Variable *)$1)->checkConst() && ((Variable *)$3)->checkConst()) {
                      $$ = new Variable(((Variable *)$1)->value - ((Variable *)$3)->value);
                    } else {
                      $$ = new Variable(false);
                      emit(((Variable *)$$)->getName() + "=" + ((Variable *)$1)->getName() + "-" + ((Variable *)$3)->getName());
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
                      emit(((Variable *)$$)->getName() + "=" + ((Variable *)$1)->getName() + "<" + ((Variable *)$3)->getName());
                    }
                  }
                | RelExp '>' AddExp {
                    if (((Variable *)$1)->checkConst() && ((Variable *)$3)->checkConst()) {
                      int t = (int)(((Variable *)$1)->value > ((Variable *)$3)->value);
                      $$ = new Variable(t);
                    } else {
                      $$ = new Variable(false);
                      emit(((Variable *)$$)->getName() + "=" + ((Variable *)$1)->getName() + ">" + ((Variable *)$3)->getName());
                    }
                  }
                | RelExp LEQ AddExp {
                    if (((Variable *)$1)->checkConst() && ((Variable *)$3)->checkConst()) {
                      int t = (int)(((Variable *)$1)->value <= ((Variable *)$3)->value);
                      $$ = new Variable(t);
                    } else {
                      $$ = new Variable(false);
                      emit(((Variable *)$$)->getName() + "=" + ((Variable *)$1)->getName() + "<=" + ((Variable *)$3)->getName());
                    }
                  }
                | RelExp GEQ AddExp {
                    if (((Variable *)$1)->checkConst() && ((Variable *)$3)->checkConst()) {
                      int t = (int)(((Variable *)$1)->value >= ((Variable *)$3)->value);
                      $$ = new Variable(t);
                    } else {
                      $$ = new Variable(false);
                      emit(((Variable *)$$)->getName() + "=" + ((Variable *)$1)->getName() + ">=" + ((Variable *)$3)->getName());
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
                      emit(((Variable *)$$)->getName() + "=" + ((Variable *)$1)->getName() + "==" + ((Variable *)$3)->getName());
                    }
                  }
                | EqExp NEQ RelExp {
                    if (((Variable *)$1)->checkConst() && ((Variable *)$3)->checkConst()) {
                      int t = (int)(((Variable *)$1)->value != ((Variable *)$3)->value);
                      $$ = new Variable(t);
                    } else {
                      $$ = new Variable(false);
                      emit(((Variable *)$$)->getName() + "=" + ((Variable *)$1)->getName() + "!=" + ((Variable *)$3)->getName());
                    }
                  }
                ;
LAndExp         : EqExp {
                    int cur_label_num = genLabel();
                    $$ = new int(cur_label_num); // For And expression, store false label here.
                    emit("if " + ((Variable *)$1)->getName() + "==0 goto l" + to_string(cur_label_num));
                  }
                | LAndExp AND EqExp {
                    $$ = $1;
                    emit("if " + ((Variable *)$3)->getName() + "==0 goto l" + to_string(*(int *)$$));
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