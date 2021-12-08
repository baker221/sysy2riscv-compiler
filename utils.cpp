#include "utils.hpp"

void yyerror(const char *msg) {
  cerr << "Error at line " << yylineno << ",\t" << msg << endl;
  exit(1);
}

deque<string> mycode;
void emit(string s) { mycode.push_back(s); }
void emitLabel(const int &label_num) { emit("l" + to_string(label_num) + ":"); }
int genLabel() {
  static int label_count = 0;
  return label_count++;
}

Variable::Variable(bool is_const, deque<int> *_shape) {
  if (is_const) {
    this->type = v_const;
  } else {
    this->type = v_var;
  }
  seq_no = count++;
  this->shape = _shape;
  if (this->checkArray()) {
    int total_size = this->getTotalSize() / INT_SIZE;
    this->array_values = new deque<int>(total_size);
  }
  this->declare();
}
Variable::Variable(bool is_const) {
  if (is_const) {
    this->type = v_const;
  } else {
    this->type = v_var;
  }
  seq_no = count++; // TODO: scalar and const variable do not need seq_no and declaration
  this->shape = NULL;
  this->declare();
}
Variable::Variable(const int _val) {
  this->type = v_value;
  this->value = _val;
}
Variable::Variable(var_type _type, const int _no, deque<int> *_shape) {
  assert(_type == v_param);
  this->type = _type;
  this->seq_no = _no;
  this->shape = _shape;
}
Variable::Variable(Variable *_head, Variable *_offset) {
  this->type = v_access;
  this->array_head = _head;
  this->offset = _offset;
  this->seq_no = -1;
}
bool Variable::checkConst() {
  return (this->type == v_const || this->type == v_value);
}
bool Variable::checkArray() { return !(this->shape == NULL); }
string Variable::getName() {
  if (this->type == v_var || this->type == v_const) {
    return "T" + to_string(this->seq_no);
  } else if (this->type == v_param) {
    return "p" + to_string(this->seq_no);
  } else if (this->type == v_access) {
    return this->array_head->getName() + "[" + this->offset->getName() + "]";
  } else {
    return to_string(this->value);
  }
}
void Variable::declare() {
  if (!this->checkArray()) {
    emit("var " + this->getName());
  } else {
    int size = this->getTotalSize();
    emit("var " + to_string(size) + " " + this->getName());
  }
}
deque<int> *Variable::getSizes() {
  if (this->sizes != NULL) {
    return this->sizes;
  }
  this->sizes = new deque<int>();
  this->sizes->push_back(INT_SIZE);
  for (int i = this->shape->size() - 1; i >= 1; i--) {
    this->sizes->push_front(this->shape->at(i) * this->sizes->front());
  }
  return this->sizes;
}
int Variable::getTotalSize() {
  if (this->sizes == NULL) {
    this->getSizes();
  }
  return this->sizes->at(0) * this->shape->at(0);
}

void Environment::putVar(string name, Variable *var) {
  assert(var != NULL);
  if (variables.count(name)) {
    yyerror(("variable " + name + " redefined").c_str());
  }
  variables[name] = var;
}
Variable *Environment::getVar(string name) {
  Environment *env = this;
  while (env != NULL) {
    if (env->variables.count(name)) {
      return env->variables[name];
    } else {
      env = env->prev;
    }
  }
  yyerror(("variable " + name + " not defined").c_str());
  return NULL;
}

void Parser::pushEnv(bool is_param) {
  Environment *env = new Environment(this->top, is_param);
  this->top = env;
}
void Parser::popEnv() {
  assert(this->top != NULL);
  this->top = this->top->prev;
}
void Parser::putFunc(string name, Function *func) {
  assert(func != NULL);
  if (functions.count(name)) {
    yyerror(("function " + name + " redefined").c_str());
  }
  functions[name] = func;
}
Function *Parser::getFunc(string name) {
  if (functions.count(name)) {
    return functions[name];
  }
  yyerror(("function " + name + " not defined").c_str());
  return NULL;
}
void Initializer::set(Variable *_var) {
  assert(_var != NULL);
  this->var = _var;
  auto shape = this->var->shape;
  this->is_array = !(shape == NULL);
  if (this->is_array) {
    this->element_num.push_front(shape->back());
    for (int i = shape->size() - 2; i >= 0; i--) {
      this->element_num.push_front(shape->at(i) * this->element_num.front());
    }
  }
  this->level = -1;
  this->pos = 0;
}
void Initializer::initialize(Variable *t, bool is_const) {
  if (this->is_array) { // array
    emit(this->var->getName() + "[" + to_string(this->pos * INT_SIZE) +
         "]=" + t->getName());
    if (this->var->checkConst()) {
      this->var->array_values->at(this->pos) = t->value;
    }
    this->pos++;
  } else { // scalar
    emit(this->var->getName() + "=" + t->getName());
    if (this->var->checkConst()) {
      this->var->value = t->value;
    }
  }
}
void Initializer::fillZero(bool all_blank) {
  // 将未填满的初始化为0
  int num;
  if (all_blank) {
    num = element_num[this->level];
  } else {
    num = element_num[this->level] - (this->pos) % this->element_num[this->level];
  }
  int begin_label = genLabel();
  int after_label = genLabel();
  Variable *i = new Variable(false);
  emit(i->getName() + "=0");
  Variable *t = new Variable(false);
  emitLabel(begin_label);
  emit(t->getName() + "=" + i->getName() + "<" + to_string(num));
  emit("if " + t->getName() + "==0 goto l" + to_string(after_label)); 
  emit(t->getName() + "=" + to_string(pos) + "+" + i->getName());
  emit(t->getName() + "=" + t->getName() + "*" + to_string(INT_SIZE));
  emit(this->var->getName() + "[" + t->getName() + "]=0");
  emit(i->getName() + "=" + i->getName() + "+1");
  emit("goto l" + to_string(begin_label));
  emitLabel(after_label);
  pos += num;
}
string final_code;
void output(const string &s) {
  final_code += s + "\n";
}
bool isFuncHeader(const string &s) {
  return s.substr(0, 2) == "f_";
}
bool isVarDefine(const string &s) {
  return s.substr(0, 4) == "var ";
}
bool isFuncEnd(const string &s) {
  return s.substr(0, 6) == "end f_";
}
bool isMain(const string &s) {
  return s.substr(0, 7) == "f_main ";
}
void postProcess(const deque<string> &codes) {
  bool is_global = true;
  deque<string> global_init;
  for (auto code : codes) {
    if (isFuncHeader(code)) {
      is_global = false;
    }
    else if (isFuncEnd(code)) {
      is_global = true;
    }
    else if (is_global && isVarDefine(code)) {
      output(code);
    }
    else if (is_global && !isVarDefine(code)) { // global initilization
      global_init.push_back(code);
    }
  }
  auto i = codes.begin();
  while(i != codes.end()) {
    if (isFuncHeader(*i)) {
      output(*i);
      auto j = i;
      while(j != codes.end() && !isFuncEnd(*j)) {
        j++;
      }
      for (auto k = i + 1; k != j; k++) { // local variable define
        if (isVarDefine(*k)) {
          output("\t" + *k);
        }
      }
      if (isMain(*i)) {
        for (auto k = global_init.begin(); k != global_init.end(); k++) {
          output("\t" + *k);
        }
      }
      for (auto k = i + 1; k != j; k++) {
        if (!isVarDefine(*k)) {
          output("\t" + *k);
        }
      }
      output(*j);
      if (j != codes.end()) {
        i = j;
      }
    }
    else {
      i++;
    }
  }
}

Parser parser;
Initializer initializer;
int Variable::count = 0;
int main(int argc, char **argv) {
  for (int c; (c = getopt(argc, argv, "Se:o:")) != EOF;) {
    switch (c) {
    case 'S':
      break;
    case 'e':
      yyin = fopen(optarg, "r");
      break;
    case 'o':
      yyout = fopen(optarg, "w");
      break;
    default:
      yyerror("unknown argument");
    }
  }
  if (yyin == NULL || yyout == NULL) {
    yyerror("failed to open files\n");
  }

  parser.putFunc("getint", new Function(0, type_int));
  parser.putFunc("getch", new Function(0, type_int));
  parser.putFunc("getarray", new Function(1, type_int));
  parser.putFunc("putint", new Function(1, type_void));
  parser.putFunc("putch", new Function(1, type_void));
  parser.putFunc("putarray", new Function(2, type_void));
  yyparse();
  final_code = "";
  postProcess(mycode);
  fprintf(yyout, "%s", final_code.c_str());
  return 0;
}