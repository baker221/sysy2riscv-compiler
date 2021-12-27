#include "riscvutils.hpp"

deque<string> riscv_codes;
void emit(const string &code) { riscv_codes.push_back(code); }

deque<string> split2(const string &str, const string &sep = " ") {
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

bool isInt12(const int &num) { return (num >= -2048 && num <= 2047); }

bool isInt12(const string &num) { return isInt12(stoi(num)); }

bool global;
int stk;

void rinit() {
  riscv_codes.clear();
  global = true;
}

void functionHeader(string func, string int1, string int2) {
  global = false;
  emit(".text");
  emit(".align 2");
  emit(".global " + func);
  emit(".type " + func + ", @function");
  emit(func + ":");
  stk = (stoi(int2) / 4 + 1) * 16;
  emit("sw ra, -4(sp)");
  if (isInt12(-stk)) {
    emit("addi sp, sp, " + to_string(-stk));
  } else {
    emit("li t0, " + to_string(-stk));
    emit("add sp, sp, t0");
  }
}

void returnStmt() {
  if (isInt12(stk)) {
    emit("addi sp, sp, " + to_string(stk));
  } else {
    emit("li t0, " + to_string(stk));
    emit("add sp, sp, t0");
  }
  emit("lw ra, -4(sp)");
  emit("ret");
}

void functionEnd(string func) {
  global = true;
  emit(".size " + func + ", .-" + func);
}

void globalVarDecl(string name, string val) {
  emit(".global " + name);
  emit(".section .sdata");
  emit(".align 2");
  emit(".type " + name + ", @object");
  emit(".size " + name + ", 4");
  emit(name + ":");
  emit(".word " + val);
}

void storeStmt(string reg, string num) {
  int offset = stoi(num) * 4;
  if (isInt12(offset)) {
    emit("sw " + reg + ", " + to_string(offset) + "(sp)");
  } else {
    emit("li t0, " + to_string(offset));
    emit("add t0, t0, sp");
    emit("sw " + reg + ", 0(t0)");
  }
}

void loadNum(string num, string reg) {
  int offset = stoi(num) * 4;
  if (isInt12(offset)) {
    emit("lw " + reg + ", " + to_string(offset) + "(sp)");
  } else {
    emit("li t0, " + to_string(offset));
    emit("add t0, t0, sp");
    emit("lw " + reg + ", 0(t0)");
  }
}

void loadAddrNum(string num, string reg) {
  int offset = stoi(num) * 4;
  if (isInt12(offset)) {
    emit("addi " + reg + ", sp, " + to_string(offset));
  } else {
    emit("li t0, " + to_string(offset));
    emit("add " + reg + ", sp, t0");
  }
}

void ifGoto(string reg1, string op, string reg2, string label) {
  if (op == "<") {
    emit("blt " + reg1 + ", " + reg2 + ", ." + label);
  } else if (op == ">") {
    emit("bgt " + reg1 + ", " + reg2 + ", ." + label);
  } else if (op == "<=") {
    emit("ble " + reg1 + ", " + reg2 + ", ." + label);
  } else if (op == ">=") {
    emit("bge " + reg1 + ", " + reg2 + ", ." + label);
  } else if (op == "==") {
    emit("beq " + reg1 + ", " + reg2 + ", ." + label);
  } else if (op == "!=") {
    emit("bne " + reg1 + ", " + reg2 + ", ." + label);
  }
}

void binOp(string reg1, string reg2, string reg3, string op) {
  if (op == "+") {
    emit("add " + reg1 + ", " + reg2 + ", " + reg3);
  } else if (op == "-") {
    emit("sub " + reg1 + ", " + reg2 + ", " + reg3);
  } else if (op == "*") {
    emit("mul " + reg1 + ", " + reg2 + ", " + reg3);
  } else if (op == "/") {
    emit("div " + reg1 + ", " + reg2 + ", " + reg3);
  } else if (op == "%") {
    emit("rem " + reg1 + ", " + reg2 + ", " + reg3);
  } else if (op == "<") {
    emit("slt " + reg1 + ", " + reg2 + ", " + reg3);
  } else if (op == ">") {
    emit("sgt " + reg1 + ", " + reg2 + ", " + reg3);
  } else if (op == "<=") {
    emit("sgt " + reg1 + ", " + reg2 + ", " + reg3);
    emit("seqz " + reg1 + ", " + reg1);
  } else if (op == ">=") {
    emit("slt " + reg1 + ", " + reg2 + ", " + reg3);
    emit("seqz " + reg1 + ", " + reg1);
  } else if (op == "!=") {
    emit("xor " + reg1 + ", " + reg2 + ", " + reg3);
    emit("snez " + reg1 + ", " + reg1);
  } else if (op == "==") {
    emit("xor " + reg1 + ", " + reg2 + ", " + reg3);
    emit("seqz " + reg1 + ", " + reg1);
  } else if (op == "&&") {
    emit("snez " + reg1 + ", " + reg2);
    emit("snez t0, " + reg3);
    emit("and " + reg1 + ", " + reg1 + ", t0");
  } else if (op == "||") {
    emit("or " + reg1 + ", " + reg2 + ", " + reg3);
    emit("snez " + reg1 + ", " + reg1);
  }
}

deque<string> toRiscv(const deque<string> &codes) {
  rinit();
  for (auto &rawcode : codes) {
    auto code = split2(rawcode);
    if (code[0] == "return") { // return
      returnStmt();
    } else if (code.size() == 1) { // label:
      emit("." + code[0]);
    } else if (code[0] == "goto") { // goto label
      emit("j ." + code[1]);
    } else if (code[0].substr(0, 2) == "f_") { // f_func [int1] [int2]
      functionHeader(code[0].substr(2), code[1].substr(1, code[1].size() - 2),
                 code[2].substr(1, code[2].size() - 2));
    } else if (code[0] == "end") { // end f_func
      functionEnd(code[1].substr(2));
    } else if (code[1] == "=" && global) { // global vars
      if (code[2] == "malloc") {           // globalvars = malloc int
        emit(".comm " + code[0] + ", " + code[3] + ", 4");
      } else { // globalvar = num
        globalVarDecl(code[0], code[2]);
      }
    } else if (code[0] == "call") { // call f_func
      emit("call " + code[1].substr(2));
    } else if (code[0] == "store") { // store reg int
      storeStmt(code[1], code[2]);
    } else if (code[0] == "load") {
      if (isdigit(code[1][0]) || code[1][0] == '-') { // load int reg
        loadNum(code[1], code[2]);
      } else { // load globalvar reg
        string gvar = code[1], reg = code[2];
        emit("lui " + reg + ", %hi(" + gvar + ")");
        emit("lw " + reg + ", %lo(" + gvar + ")(" + reg + ")");
      }
    } else if (code[0] == "loadaddr") {
      if (isdigit(code[1][0]) || code[1][0] == '-') { // loadaddr int reg
        loadAddrNum(code[1], code[2]);
      } else { // loadaddr globalvar reg
        emit("la " + code[2] + ", " + code[1]);
      }
    } else if (code[0] == "if") { // if reg1 op reg2 goto label
      ifGoto(code[1], code[2], code[3], code[5]);
    } else if (code[2] == "!") { // reg1 = ! reg2
      emit("seqz " + code[0] + ", " + code[3]);
    } else if (code[2] == "-") { // reg1 = - reg2
      emit("neg " + code[0] + ", " + code[3]);
    } else if (code[0].back() == ']') { // reg1[int] = reg2
      auto t = split2(code[0], "[");
      string reg1 = t[0];
      string num = t[1].substr(0, t[1].size() - 1);
      string reg2 = code[2];
      emit("sw " + reg2 + ", " + num + "(" + reg1 + ")");
    } else if (code[2].back() == ']') { // reg1 = reg2[int]
      auto t = split2(code[2], "[");
      string reg1 = code[0];
      string reg2 = t[0];
      string num = t[1].substr(0, t[1].size() - 1);
      emit("lw " + reg1 + ", " + num + "(" + reg2 + ")");
    } else if (code.size() == 3) {
      if (isdigit(code[2][0]) || code[2][0] == '-') { // reg = num
        emit("li " + code[0] + ", " + code[2]);
      } else { // reg1 = reg2
        emit("mv " + code[0] + ", " + code[2]);
      }
    } else { // reg1 = reg2 op reg3
      binOp(code[0], code[2], code[4], code[3]);
    }
  }
  return riscv_codes;
}