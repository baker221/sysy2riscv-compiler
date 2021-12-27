#include "utils.hpp"
#include "tiggerutils.hpp"

void yyerror(const char *msg) {
  cerr << "Error at line " << yylineno << ",\t" << msg << endl;
  exit(1);
}

deque<string> mycode;
deque<string> eeyore_code;
void emit(string s) { mycode.push_back(s); }
void emitLabel(const int &label_num) { emit("l" + to_string(label_num) + ":"); }
int genLabel() {
  static int label_count = 0;
  return label_count++;
}

Variable::Variable(bool is_const, deque<int> *_shape) {
  assert(_shape != NULL);
  if (is_const) {
    type = v_const;
  } else {
    type = v_var;
  }
  seq_no = count++;
  shape = _shape;
  if (_shape != NULL) {
    sizes = new deque<int>(); // get total sizes
    sizes->push_back(INT_SIZE);
    for (int i = shape->size() - 1; i >= 1; i--) {
      sizes->push_front(shape->at(i) * sizes->front());
    }
    int total_size = sizes->at(0) * shape->at(0) / INT_SIZE;
    array_values = new deque<int>(total_size);
  }
  declare();
}
Variable::Variable(bool is_const, bool _nameless) {
  shape = NULL;
  nameless = _nameless;
  if (is_const) {
    type = v_const;
    seq_no = -1;
  } else {
    type = v_var;
    if (nameless) {
      if (available_count.empty()) {
        seq_no = count++;
      } else {
        seq_no = *available_count.begin();
        available_count.erase(available_count.begin());
      }
    } else {
      seq_no = count++;
    }
    declare();
  }
}
Variable::Variable(const int _val) {
  type = v_value;
  value = _val;
}
Variable::Variable(var_type _type, const int _no, deque<int> *_shape) {
  assert(_type == v_param);
  type = _type;
  seq_no = _no;
  shape = _shape;
  sizes = NULL;
}
Variable::Variable(Variable *_head, Variable *_offset) {
  type = v_access;
  array_head = _head;
  offset = _offset;
  seq_no = -1;
}
bool Variable::checkConst() { return (type == v_const || type == v_value); }
bool Variable::checkArray() { return !(shape == NULL); }
string Variable::getName() {
  if (type == v_var || (type == v_const && checkArray())) {
    return "T" + to_string(seq_no);
  } else if (type == v_param) {
    return "p" + to_string(seq_no);
  } else if (type == v_access) {
    return array_head->getName() + "[" + offset->getName() + "]";
  } else { // scalar v_const and v_value
    return to_string(value);
  }
}
void Variable::declare() {
  if (!checkArray()) {
    emit("var " + getName());
  } else {
    int size = sizes->at(0) * shape->at(0);
    emit("var " + to_string(size) + " " + getName());
  }
}
deque<int> *Variable::getSizes() {
  if (sizes != NULL) {
    return sizes;
  }
  sizes = new deque<int>();
  sizes->push_back(INT_SIZE);
  for (int i = shape->size() - 1; i >= 1; i--) {
    sizes->push_front(shape->at(i) * sizes->front());
  }
  return sizes;
}
void Variable::releaseCount() {
  assert(nameless == true);
  available_count.insert(seq_no);
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
  return NULL;
}

void Parser::pushEnv(bool is_param) {
  Environment *env = new Environment(top, is_param);
  top = env;
}
void Parser::popEnv() {
  assert(top != NULL);
  top = top->prev;
}
Variable *Parser::registerVar(string name, deque<int> *shape, bool is_const) {
  Variable *v;
  if (shape->size() == 0) {
    v = new Variable(is_const, false);
  } else {
    v = new Variable(is_const, shape);
  }
  top->putVar(name, v);
  return v;
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
  var = _var;
  element_num.clear();
  auto shape = var->shape;
  is_array = !(shape == NULL);
  if (is_array) {
    element_num.push_front(shape->back());
    for (int i = shape->size() - 2; i >= 0; i--) {
      element_num.push_front(shape->at(i) * element_num.front());
    }
  }
  level = -1;
  pos = 0;
}
void Initializer::initialize(Variable *t, bool is_const) {
  if (is_array) { // array
    emit(var->getName() + "[" + to_string(pos * INT_SIZE) +
         "] = " + t->getName());
    if (var->checkConst() && is_const) {
      var->array_values->at(pos) = t->value;
    }
    pos++;
  } else { // scalar
    if (var->checkConst() && is_const) {
      var->value = t->value;
    } else {
      emit(var->getName() + " = " + t->getName());
    }
  }
  if (!t->checkConst() && t->nameless) {
    t->releaseCount();
  }
}
void Initializer::fillZero(bool all_blank) {
  // 将未填满的初始化为0
  int num;
  if (all_blank) {
    num = element_num[level];
  } else {
    num = element_num[level] - (pos) % element_num[level];
    if (num == element_num[level]) {
      return; // do not need to fill zero.
    }
  }
  int begin_label = genLabel();
  int after_label = genLabel();
  Variable *i = new Variable(false, true);
  Variable *t = new Variable(false, true);
  emit("// above two line is duplicate var define");
  emit(i->getName() + " = 0");
  emitLabel(begin_label);
  emit(t->getName() + " = " + i->getName() + " < " + to_string(num));
  emit("if " + t->getName() + " == 0 goto l" + to_string(after_label));
  emit(t->getName() + " = " + to_string(pos) + " + " + i->getName());
  emit(t->getName() + " = " + t->getName() + " * " + to_string(INT_SIZE));
  emit(var->getName() + "[" + t->getName() + "] = 0");
  emit(i->getName() + " = " + i->getName() + " + 1");
  emit("goto l" + to_string(begin_label));
  emitLabel(after_label);
  pos += num;
  i->releaseCount();
  t->releaseCount();
}

Variable *performOp(Variable *v1, Variable *v2, string op) {
  Variable *v = new Variable(false, true);
  string name1 = v->getName();
  string name2 = v1->getName();
  string name3 = v2->getName();
  emit(name1 + " = " + name2 + " " + op + " " + name3);
  if (!v1->checkConst() && v1->nameless) {
    v1->releaseCount();
  }
  if (!v2->checkConst() && v2->nameless) {
    v2->releaseCount();
  }
  return v;
}

void output(const string &s) { eeyore_code.push_back(s); }
bool isFuncHeader(const string &s) { return s.substr(0, 2) == "f_"; }
bool isVarDefine(const string &s) { return s.substr(0, 4) == "var "; }
bool isFuncEnd(const string &s) { return s.substr(0, 6) == "end f_"; }
bool isMain(const string &s) { return s.substr(0, 7) == "f_main "; }
void toEeyore(const deque<string> &codes) {
  bool is_global = true;
  deque<string> global_init;
  deque<string> fillzero_var_dec;
  for (int i = 0; i < codes.size(); i++) {
    auto code = codes[i];
    if (isFuncHeader(code)) {
      is_global = false;
    } else if (isFuncEnd(code)) {
      is_global = true;
    } else if (is_global && isVarDefine(code)) {
      output(code);
    } else if (is_global && !isVarDefine(code)) { // global initilization
      global_init.push_back(code); // TODO 全局变量初始化不必放在main函数里，可以直接初始化
      // BUT 这样处理过后可以保证tigger和riscv的赋值只在函数中
    }
    if (is_global &&
        code.substr(0, 2) == "//") { // dirty trick for global fillzero var
      fillzero_var_dec.push_back(codes[i - 1]);
      fillzero_var_dec.push_back(codes[i - 2]);
      eeyore_code.pop_back();
      eeyore_code.pop_back();
    }
  }
  auto i = codes.begin();
  while (i != codes.end()) {
    if (isFuncHeader(*i)) {
      output(*i);
      auto j = i;
      while (j != codes.end() && !isFuncEnd(*j)) {
        j++;
      }
      unordered_set<int> vars;
      if (isMain(*i)) {
        for (auto var_dec : fillzero_var_dec) {
          string name = var_dec.substr(5);
          if (vars.count(stoi(name))) {
            continue;
          }
          vars.insert(stoi(name));
          output(var_dec);
        }
      }
      for (auto k = i + 1; k != j; k++) { // local variable define
        if (isVarDefine(*k)) {
          if ((*k)[4] == 'T') { // scalar
            string name = (*k).substr(5);
            if (vars.count(stoi(name))) {
              continue;
            }
            vars.insert(stoi(name));
          }
          output(*k);
        }
      }
      if (isMain(*i)) {
        for (auto k = global_init.begin(); k != global_init.end(); k++) {
          output(*k);
        }
      }
      for (auto k = i + 1; k != j; k++) {
        if (!isVarDefine(*k)) {
          if ((*k).substr(0, 6) == "return" &&
              eeyore_code.back().substr(0, 6) == "return") {
            continue;
          }
          output(*k);
        }
      }
      output(*j);
      if (j != codes.end()) {
        i = j;
      }
    } else {
      i++;
    }
  }
}

Parser parser;
Initializer initializer;
int Variable::count = 0;
unordered_set<int> Variable::available_count;
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
  int mode = 2; // 1: to eeyore, 2: to tigger, 3: to riscv
  deque<string> tigger_code;
  deque<string> riscv_code;
  if (mode >= 1) {
    eeyore_code.clear();
    toEeyore(mycode);
  }
  if (mode >= 2) {
    tigger_code = toTigger(eeyore_code);
  }
  if (mode >= 3) {}
  string code = "";
  if (mode == 1) {
    for (auto &s : eeyore_code) {
      code += s + "\n";
    }
  } else if (mode == 2) {
    for (auto &s : tigger_code) {
      code += s + "\n";
    }
  }
  fprintf(yyout, "%s", code.c_str());
  return 0;
}