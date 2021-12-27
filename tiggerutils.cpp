#include "tiggerutils.hpp"

deque<string> tigger_code;
deque<string> buffer;
void flush() {
  for (auto &code : buffer) {
    tigger_code.push_back(code);
  }
  buffer.clear();
}
void toBuffer(const string &code) { buffer.push_back(code); }

const int n_registers = 28;
const string registers[n_registers] = {
    "x0", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "s0",  "s1",
    "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"};
unordered_map<string, bool> register_used;

string getReg() {
  for (int i = 0; i < n_registers; i++) {
    if (!register_used[registers[i]]) {
      register_used[registers[i]] = true;
      return registers[i];
    }
  }
  return "";
}

bool getReg(const string &reg) { // only use for a0-a7
  if (register_used[reg] == false) {
    register_used[reg] = true;
    return true;
  }
  return false;
}

void freeReg(const string &reg) { register_used[reg] = false; }

void bindReg(Var *var) {
  if (var->reg == "") {
    var->reg = getReg();
  }
  if (var->reg == "") {
    cout << "no register available for var " << var->name << endl;
    exit(1);
  }
}

void unbindReg(Var *var) {
  if (var->reg != "") {
    freeReg(var->reg);
    var->reg = "";
  }
}

typedef unordered_map<string, Var *> VarMap;
unordered_map<string, VarMap> var_table;
int global_var_count;
string cur_func;
int local_var_count;
int param_count;

void init() {
  for (int i = 0; i < n_registers; i++) {
    register_used[registers[i]] = false;
  }
  register_used["x0"] = true;
  register_used["t0"] = true; // use t0 as tmp register
  register_used["s0"] = true; // store for tigger2riscv2 tmp reg
  cur_func = "global";
  var_table["global"] = VarMap();
  var_table["number"] = VarMap(); // 不储存常数创建的变量的话，无法正确释放寄存器
  global_var_count = 0;
  param_count = 0;
  local_var_count = 0;
}

deque<string> split(const string &str, const string &sep = " ") {
  deque<string> ret;
  char *cstr = const_cast<char *>(str.c_str());
  char *current;
  current = strtok(cstr, sep.c_str());
  while (current != NULL) {
    ret.push_back(current);
    current = strtok(NULL, sep.c_str());
  }
  return ret;
}

Var *varFind(const string &name) {
  if (isdigit(name[0]) || name[0] == '-') { // number
    if (var_table["number"].count(name)) {
      return var_table["number"][name];
    }
    Var *var = new Var(name, stoi(name), false, false, true);
    var_table["number"][name] = var;
    return var;
  }
  VarMap &local = var_table[cur_func];
  VarMap &global = var_table["global"];
  if (local.count(name)) {
    return local.find(name)->second;
  } else if (global.count(name)) {
    return global.find(name)->second;
  } else {
    return NULL;
  }
}

string varLoad(const string &name) { // load a var from stack to register
  Var *var = varFind(name);
  bindReg(var);
  if (var->is_array) {
    toBuffer("loadaddr " + var->addr + " " + var->reg);
  } else if (var->is_num) {
    toBuffer(var->reg + " = " + var->name);
  } else if (var->is_global) {
    toBuffer("load " + var->name + " " + var->reg);
  } else {
    toBuffer("load " + var->addr + " " + var->reg);
  }
  var->dirty = false;
  return var->reg;
}

void varSave(const string &name) {
  Var *var = varFind(name);
  if (var->reg == "" || !var->dirty || var->is_num || var->is_array) {
  } else if (var->is_global) {
    string reg = "t0";
    toBuffer("loadaddr " + var->name + " " + reg);
    toBuffer(reg + "[0] = " + var->reg);
  } else {
    toBuffer("store " + var->reg + " " + var->addr);
  }
  unbindReg(var);
}

void varSetDirty(const string &name) { varFind(name)->dirty = true; }

void decl(string sym) { // var sym
  if (cur_func == "global") {
    string name = "v" + to_string(global_var_count++);
    var_table["global"][sym] = new Var(name, name, true, false, false);
    toBuffer(name + " = 0");
  } else {
    var_table[cur_func][sym] =
        new Var(sym, local_var_count++, false, false, false);
  }
}

void decl(string sym, string num) { // var num sym
  if (cur_func == "global") {
    string name = "v" + to_string(global_var_count++);
    var_table["global"][sym] = new Var(name, name, true, true, false);
    toBuffer(name + " = malloc " + num);
  } else {
    var_table[cur_func][sym] =
        new Var(sym, local_var_count, false, true, false);
    local_var_count += stoi(num) / 4;
  }
}

void funcHeader(string func_name, string num) { // func [num]
  flush();
  cur_func = func_name;
  var_table[cur_func] = VarMap();
  local_var_count = stoi(num);
  param_count = 0;
  toBuffer(func_name + " [" + num + "]");
  for (int i = 0; i < local_var_count; i++) {
    string sym = "p" + to_string(i);
    string reg = "a" + to_string(i);
    var_table[cur_func][sym] = new Var(sym, i, false, false, false, reg);
    // save all parameters to stack
    varSetDirty(sym);
    varSave(sym);
  }
}

void funcEnd(string func_name) {
  buffer[0] += (" [" + to_string(local_var_count) + "]");
  toBuffer("end " + func_name);
  flush();
  cur_func = "global";
}

void ifStmt(string val1, string op, string val2, string label) {
  string reg1 = varLoad(val1);
  string reg2 = varLoad(val2);
  toBuffer("if " + reg1 + " " + op + " " + reg2 + " goto " + label);
  varSave(val1);
  varSave(val2);
}

void paramDec(string val) {
  string reg = "a" + to_string(param_count++);
  if (!getReg(reg)) {
    cout << "in param declare" << endl;
    cout << "register " << reg << " not available" << endl;
    exit(1);
  }
  string reg2 = varLoad(val);
  toBuffer(reg + " = " + reg2);
  varSave(val);
}

void returnStmt(string val) {
  string reg = "a0";
  if (!getReg(reg)) {
    cout << "in return" << endl;
    cout << "register " << reg << " not available" << endl;
    exit(2);
  }
  string reg2 = varLoad(val);
  toBuffer(reg + " = " + reg2);
  toBuffer("return");
  freeReg("a0");
  varSave(val);
}

void callFunc(string func_name) {
  for (int i = 0; i < param_count; i++) {
    freeReg("a" + to_string(i));
  }
  toBuffer("call " + func_name);
  param_count = 0;
}

void callFunc(string func_name, string sym) {
  for (int i = 1; i < param_count; i++) {
    freeReg("a" + to_string(i));
  }
  toBuffer("call " + func_name);
  param_count = 0;
  string reg = varLoad(sym);
  toBuffer(reg + " = a0");
  freeReg("a0");
  varSetDirty(sym);
  varSave(sym);
}

void assignStmt(string sym, string val) {
  string reg1 = varLoad(sym);
  string reg2 = varLoad(val);
  toBuffer(reg1 + " = " + reg2);
  varSetDirty(sym);
  varSave(sym);
  varSave(val);
}

void assignStmt(string sym, string val, string sinop) {
  string reg1 = varLoad(sym);
  string reg2 = varLoad(val);
  toBuffer(reg1 + " = " + sinop + " " + reg2);
  varSetDirty(sym);
  varSave(sym);
  varSave(val);
}

void arrayAssign(string sym, string val1, string val2) {
  string reg1 = varLoad(sym);
  string reg2 = varLoad(val1);
  toBuffer("t0 = " + reg1 + " + " + reg2);
  varSave(sym);
  varSave(val1);
  string reg3 = varLoad(val2);
  toBuffer("t0[0] = " + reg3);
  varSave(val2);
}

void assignArray(string sym1, string sym2, string val) {
  string reg2 = varLoad(sym2);
  string reg3 = varLoad(val);
  toBuffer("t0 = " + reg2 + " + " + reg3);
  varSave(sym2);
  varSave(val);
  string reg1 = varLoad(sym1);
  toBuffer(reg1 + " = t0[0]");
  varSetDirty(sym1);
  varSave(sym1);
}

void binOpCal(string sym, string val1, string val2, string op) {
  string reg1 = varLoad(sym);
  string reg2 = varLoad(val1);
  string reg3 = varLoad(val2);
  toBuffer(reg1 + " = " + reg2 + " " + op + " " + reg3);
  varSetDirty(sym);
  varSave(sym);
  varSave(val1);
  varSave(val2);
}

deque<string> toTigger(const deque<string> &codes) {
  tigger_code.clear();
  init();
  for (auto &rawcode : codes) {
    if (rawcode.substr(0, 2) == "//") {
      continue;
    }
    auto code = split(rawcode, " ");
    assert(code.size() > 0);
    if (code[0] == "var") { // declararion
      if (isdigit(code[1][0])) {
        decl(code[2], code[1]);
      } else {
        decl(code[1]);
      }
    } else if (code[0].substr(0, 2) == "f_") { // func def header
      funcHeader(code[0], code[1].substr(1, code[1].size() - 2));
    } else if (code[0] == "end") { // func def end
      funcEnd(code[1]);
    } else if (code[0] == "goto") { // goto label
      toBuffer(code[0] + " " + code[1]);
    } else if (code[0][0] == 'l') { // label:
      toBuffer(code[0]);
    } else if (code[0] == "if") { // if a op b goto label
      ifStmt(code[1], code[2], code[3], code[5]);
    } else if (code[0] == "param") { // param val
      paramDec(code[1]);
    } else if (code[0] == "return") { // return (val)
      if (code.size() > 1) { // return val
        returnStmt(code[1]);
      } else { // return
        toBuffer("return");
      }
    } else if (code[0] == "call") { // call func
      callFunc(code[1]);
    } else if (code[2] == "call") { // sym = call func
      callFunc(code[3], code[0]);
    } else if (code[2] == "-" || code[2] == "!") {// sym = sinop val
      assignStmt(code[0], code[3], code[2]);
    } else if (code[0].back() == ']') { //sym[val] = val
      auto t = split(code[0], "[");
      arrayAssign(t[0], t[1].substr(0, t[1].size() - 1), code[2]);
    } else if (code[2].back() == ']') { // sym = sym[val]
      auto t = split(code[2], "[");
      assignArray(code[0], t[0], t[1].substr(0, t[1].size() - 1));
    } else if (code.size() == 3) { // sym = val
      assignStmt(code[0], code[2]);
    } else { // sym = val op val
      binOpCal(code[0], code[2], code[4], code[3]);
    }
  }
  return tigger_code;
}