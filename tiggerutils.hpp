#include <cassert>
#include <deque>
#include <iostream>
#include <string>
#include <unordered_map>
#include <cstring>
using namespace std;

struct Var {
  string name;
  string addr;
  string reg;
  bool is_global;
  bool is_array;
  bool is_num;
  bool dirty;
  Var(string _name, string _addr, bool _is_global, bool _is_array, bool _is_num,
      string _reg = "") {
    name = _name;
    addr = _addr;
    reg = _reg;
    is_global = _is_global;
    is_array = _is_array;
    is_num = _is_num;
    dirty = false;
  }
  Var(string _name, int _addr, bool _is_global, bool _is_array, bool _is_num,
      string _reg = "") {
    name = _name;
    addr = to_string(_addr);
    reg = _reg;
    is_global = _is_global;
    is_array = _is_array;
    is_num = _is_num;
    dirty = false;
  }
};

deque<string> toTigger(const deque<string> &codes);