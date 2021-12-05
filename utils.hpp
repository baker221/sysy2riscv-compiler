#include <iostream>
#include <cstdio>
#include <string>
#include <cassert>
#include <unordered_map>
extern int yylineno;
extern int yylex();
extern FILE *yyin;
extern FILE *yyout;
#define YYSTYPE void* 
using namespace std;

void yyerror(const char *msg) {
  cerr << "Error at line " << yylineno << ",\t\t" << msg << endl;
  exit(1);
}

struct Variable {
  static int count;
  int seq_no;
  enum var_type {v_value, v_const, v_var, v_param, v_access} type;
  int value;
  Variable(bool is_const); // construct a var or const variable
  Variable(const int _val); // construct a tmp value
  string getName();
  bool checkConst();
};
int Variable::count = 0;

struct Environment {
  unordered_map<string, Variable *> variables;
  Environment *prev;
  Environment(Environment *p) : prev(p) {}
  void putVar(string name, Variable *var);
  void getVar();
};

struct Parser {
  Environment *top;
};

struct Initializer {
};

int genLabel() {
  static int label_count = 0;
  return label_count++;
}
