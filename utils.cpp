#include "utils.hpp"

Variable::Variable(bool is_const) : seq_no(count++) {
  if (is_const) {
    this->type = var_type::v_const;
  } else {
    this->type = var_type::v_var;
  }
}
Variable::Variable(const int _val) {
  this->type = var_type::v_value;
  this->value = _val;
}
bool Variable::checkConst() {
  return (this->type == var_type::v_const || this->type == var_type::v_value);
}
void Environment::putVar(string name, Variable *var) {
  assert(var != NULL);
  if (variables.count(name)) {
    yyerror("variable redefined");
  }
  variables[name] = var;
}
