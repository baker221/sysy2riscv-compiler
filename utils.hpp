#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
extern int yylineno;
extern int yylex();
extern FILE *yyin;
extern FILE *yyout;
#define YYSTYPE void *
using namespace std;

void yyerror(const char *msg) {
  cerr << "Error at line " << yylineno << ",\t\t" << msg << endl;
  exit(1);
}

struct Variable {
  static int count;
  int seq_no;
  enum var_type { v_value, v_const, v_var, v_param, v_access } type;
  int value;
  Variable(bool is_const);  // construct a var or const variable
  Variable(const int _val); // construct a tmp value
  string getName();
  bool checkConst();
};
int Variable::count = 0;

struct Function {
  int param_num;
  int ret_val;
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
  void getVar();
};

struct Parser {
  Environment *top;
  vector<WhileStmt *> while_stack;
  unordered_map<string, Function *> functions;
  Parser() {
    top = new Environment(NULL, false);
  }
  void pushEnv(bool is_param);
  void popEnv();
  void putFunc(string name, Function *func);
};

struct Initializer {};

int genLabel() {
  static int label_count = 0;
  return label_count++;
}
