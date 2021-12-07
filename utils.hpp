#include <cassert>
#include <cstdio>
#include <deque>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <unistd.h>
extern int yylineno;
extern "C" {
int yylex();
int yyparse();
}
extern FILE *yyin;
extern FILE *yyout;
#define YYSTYPE void *
using namespace std;
void emit(string s);
void emitLabel(int label_num);
int genLabel();
void yyerror(const char *msg);

enum var_type {
  v_value, // tmp value
  v_const, // const variable
  v_var,
  v_param, // function param
  v_access
};
const int INT_SIZE = 4;

struct Variable {
  static int count;
  int seq_no;
  var_type type;
  int value;
  deque<int> *shape;
  Variable(bool is_const,
           deque<int> *_shape); // construct a var or const variable
  Variable(bool is_const); // var or const variable and scalar
  Variable(var_type _type, int _no, deque<int> *_shape);
  Variable(const int _val); // construct a tmp value
  string getName();
  bool checkConst();
  bool checkArray();
  void declare();
};

struct Function {
  int param_num;
  int func_type; // 0: void, 1: int
  Function(int p, int t) : param_num(p), func_type(t) {}
};

struct IfStmt {
  int label_true;
  int label_false;
  int label_after;
  IfStmt(int t, int f, int a) : label_true(t), label_false(f), label_after(a) {}
};

struct WhileStmt {
  int label_begin;
  int label_body;
  int label_after;
  WhileStmt(int begin, int body, int after)
      : label_begin(begin), label_body(body), label_after(after) {}
};

struct Environment {
  unordered_map<string, Variable *> variables;
  Environment *prev; // the upper level env
  bool is_param;     // indicate whether this env is for param declaration
  Environment(Environment *p, bool _is_param) : prev(p), is_param(_is_param) {}
  void putVar(string name, Variable *var);
  Variable *getVar(string name);
};

struct Parser {
  Environment *top;
  vector<WhileStmt *> while_stack;
  unordered_map<string, Function *> functions;
  Parser() { top = new Environment(NULL, false); }
  void pushEnv(bool is_param);
  void popEnv();
  void putFunc(string name, Function *func);
  Function *getFunc(string name);
};

struct Initializer {};

extern Parser parser;