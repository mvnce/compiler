#include <cstdlib>

#include "ast.h"
#include "tube8-parser.tab.hh"

extern void yyerror(std::string err_string);
extern void yyerror2(std::string err_string, int orig_line);


///////////////
//  ASTNode

void ASTNode::TransferChildren(ASTNode * in_node)
{
  for (int i = 0; i < in_node->GetNumChildren(); i++) {
    AddChild(in_node->GetChild(i));
  }
  in_node->children.resize(0);
}


/////////////////////
//  ASTNode_Root

tableEntry * ASTNode_Root::CompileTubeIC(symbolTable & table, IC_Array & ica)
{
  // Compile the code for each sub-tree below a root.
  for (int i = 0; i < (int) children.size(); i++) {
    tableEntry * cur_entry = children[i]->CompileTubeIC(table, ica);
    if (cur_entry != NULL && cur_entry->GetTemp() == true) {
      table.RemoveEntry( cur_entry );
    }
  }
  return NULL;
}


/////////////////////////
//  ASTNode_Variable

tableEntry * ASTNode_Variable::CompileTubeIC(symbolTable & table, IC_Array & ica)
{
  return var_entry;
}


////////////////////////
//  ASTNode_Literal

ASTNode_Literal::ASTNode_Literal(int in_type, std::string in_lex)
  : ASTNode(in_type), lexeme(in_lex)
{
}  

tableEntry * ASTNode_Literal::CompileTubeIC(symbolTable & table, IC_Array & ica)
{
  tableEntry * out_var = table.GetTempVar(type);
  if (type == Type::VALUE || type == Type::CHAR) {
    ica.Add("val_copy", lexeme, out_var);
  } else if (type == Type::STRING) {
    // Drop the beginning and ending quotes and process escape chars.
    std::string str_value = "";
    for (int i = 1; i < (int) lexeme.size() - 1; i++) {
      if (lexeme[i] == '\\') {
        switch (lexeme[++i]) {
        case '\\': str_value += '\\'; break;
        case '"':  str_value += '"';  break;
        case 't':  str_value += '\t'; break;
        case 'n':  str_value += '\n'; break;
        };
      } else {
        str_value += lexeme[i];
      }
    }        

    std::stringstream size_str;
    size_str << str_value.size();
    ica.Add("ar_set_siz", out_var, size_str.str());
    for (int i = 0; i < (int) str_value.size(); i++) {
      std::stringstream set_val;
      switch (str_value[i]) {
      case '\\': set_val << "'\\\\'"; break;
      case '\t': set_val << "'\\t'"; break;
      case '\n': set_val << "'\\n'"; break;
      case '\'': set_val << "'\\''"; break;
      default: set_val << "'" << str_value[i] << "'";
      };

      std::stringstream idx_str;
      idx_str << i;
      ica.Add("ar_set_idx", out_var, idx_str.str(), set_val.str());
    }
  } else {
    std::cerr << "INTERNAL ERROR: Unknown type!" << std::endl;
  }

  return out_var;
}


//////////////////////
// ASTNode_Assign

ASTNode_Assign::ASTNode_Assign(ASTNode * lhs, ASTNode * rhs)
  : ASTNode(lhs->GetType())
{ 
  if (lhs->GetType() != rhs->GetType()) {
    std::string err_message = "types do not match for assignment (lhs='";
    err_message += Type::AsString(lhs->GetType());
    err_message += "', rhs='";
    err_message += Type::AsString(rhs->GetType());
    err_message += "')";
    yyerror(err_message);
    exit(1);
  }
  children.push_back(lhs);
  children.push_back(rhs);
}

tableEntry * ASTNode_Assign::CompileTubeIC(symbolTable & table,
						IC_Array & ica)
{
  tableEntry * lhs_var = children[0]->CompileTubeIC(table, ica);
  tableEntry * rhs_var = children[1]->CompileTubeIC(table, ica);

  if (type == Type::VALUE || type == Type::CHAR) {
    // Determine if the lhs is part of an array
    if (lhs_var->GetArrayID() == -1) {    // NOT an array on the LHS!
      ica.Add("val_copy", rhs_var, lhs_var);
    } else {                              // An array on the LHS!
      // Since this assignment is to an array index, we need entries for each.
      tableEntry * array_entry = table.BuildTempEntry(Type::VALUE_ARRAY, lhs_var->GetArrayID());
      tableEntry * index_entry = table.BuildTempEntry(Type::VALUE, lhs_var->GetIndexID());
      
      //ica.Add("ar_set_idx", lhs_var->GetArrayID(), lhs_var->GetIndexID(), rhs_var);
      ica.Add("ar_set_idx", array_entry, index_entry, rhs_var);

      table.RemoveEntry(array_entry);
      table.RemoveEntry(index_entry);
    }
  } else if (type == Type::VALUE_ARRAY || type == Type::STRING) {
    ica.Add("ar_copy", rhs_var, lhs_var);
  } else {
    std::cerr << "Internal Compiler ERROR: Unknown type in Assign!" << std::endl;
    exit(1);
  }

  if (rhs_var->GetTemp() == true) table.RemoveEntry( rhs_var );

  return lhs_var;
}


/////////////////////
// ASTNode_Math1

ASTNode_Math1::ASTNode_Math1(ASTNode * in_child, int op)
  : ASTNode(ChooseType(in_child)), math_op(op)
{
  if (in_child->GetType() != Type::VALUE) {
    std::string err_message = "cannot use type '";
    err_message += Type::AsString(in_child->GetType());
    err_message += "' in mathematical expressions";
    yyerror(err_message);
    exit(1);
  }
  children.push_back(in_child);
}

int ASTNode_Math1::ChooseType(ASTNode * child)
{
  // For the moment, lets always convert to a number.
  return Type::VALUE;
}

tableEntry * ASTNode_Math1::CompileTubeIC(symbolTable & table, IC_Array & ica)
{
  tableEntry * in_var = children[0]->CompileTubeIC(table, ica);
  tableEntry * out_var = table.GetTempVar(type);

  switch (math_op) {
  case '-':
    ica.Add("mult", in_var, "-1", out_var);
    break;
  case '!':
    ica.Add("test_equ", in_var, "0", out_var);
    break;
  default:
    std::cerr << "Internal compiler error: unknown Math1 operation '" << math_op << "'." << std::endl;
    exit(1);
  };

  if (in_var->GetTemp() == true) table.RemoveEntry( in_var );

  return out_var;
}



/////////////////////
// ASTNode_Math2

ASTNode_Math2::ASTNode_Math2(ASTNode * in1, ASTNode * in2, int op)
  : ASTNode(ChooseType(in1, in2)), math_op(op)
{
  bool rel_op = (op==COMP_EQU) || (op==COMP_NEQU) || (op==COMP_LESS) || (op==COMP_LTE) ||
    (op==COMP_GTR) || (op==COMP_GTE);

  ASTNode * in_test = (in1->GetType() != Type::VALUE) ? in1 : in2;
  if (in_test->GetType() != Type::VALUE) {
    if (!rel_op || in_test->GetType() != Type::CHAR) {
      std::string err_message = "cannot use type '";
      err_message += Type::AsString(in_test->GetType());
      err_message += "' in mathematical expressions";
      yyerror(err_message);
      exit(1);
    } else if (rel_op && (in1->GetType() != Type::CHAR || in2->GetType() != Type::CHAR)) {
      std::string err_message = "types do not match for relationship operator (lhs='";
      err_message += Type::AsString(in1->GetType());
      err_message += "', rhs='";
      err_message += Type::AsString(in2->GetType());
      err_message += "')";
      yyerror(err_message);
      exit(1);
    }
  }

  children.push_back(in1);
  children.push_back(in2);
}


int ASTNode_Math2::ChooseType(ASTNode * child1, ASTNode * child2)
{
  // At the moment, both sides need to be of the same type, so return the type of either child.
  // @CAO Always return VALUE, regardless of inputs...
  return Type::VALUE; // child1->GetType();
}


tableEntry * ASTNode_Math2::CompileTubeIC(symbolTable & table, IC_Array & ica)
{
  tableEntry * i1 = children[0]->CompileTubeIC(table, ica);
  tableEntry * i2 = children[1]->CompileTubeIC(table, ica);
  tableEntry * o3 = table.GetTempVar(type);

  // Determine the correct operation...  
  if (math_op == '+') { ica.Add("add", i1, i2, o3); }
  else if (math_op == '-') { ica.Add("sub",  i1, i2, o3); }
  else if (math_op == '*') { ica.Add("mult", i1, i2, o3); }
  else if (math_op == '/') { ica.Add("div",  i1, i2, o3); }
  else if (math_op == COMP_EQU)  { ica.Add("test_equ",  i1, i2, o3); }
  else if (math_op == COMP_NEQU) { ica.Add("test_nequ", i1, i2, o3); }
  else if (math_op == COMP_GTR)  { ica.Add("test_gtr",  i1, i2, o3); }
  else if (math_op == COMP_GTE)  { ica.Add("test_gte",  i1, i2, o3); }
  else if (math_op == COMP_LESS) { ica.Add("test_less", i1, i2, o3); }
  else if (math_op == COMP_LTE)  { ica.Add("test_lte",  i1, i2, o3); }
  else {
    std::cerr << "INTERNAL ERROR: Unknown Math2 type '" << math_op << "'" << std::endl;
  }

  // Cleanup symbol table.
  if (i1->GetTemp() == true) table.RemoveEntry( i1 );
  if (i2->GetTemp() == true) table.RemoveEntry( i2 );

  return o3;
}


/////////////////////
// ASTNode_Bool2

ASTNode_Bool2::ASTNode_Bool2(ASTNode * in1, ASTNode * in2, int op)
  : ASTNode(ChooseType(in1, in2)), bool_op(op)
{
  ASTNode * in_test = (in1->GetType() != Type::VALUE) ? in1 : in2;
  if (in_test->GetType() != Type::VALUE) {
    std::string err_message = "cannot use type '";
    err_message += Type::AsString(in_test->GetType());
    err_message += "' in mathematical expressions";
    yyerror(err_message);
    exit(1);
  }

  children.push_back(in1);
  children.push_back(in2);
}


int ASTNode_Bool2::ChooseType(ASTNode * child1, ASTNode * child2)
{
  return Type::VALUE;
}


tableEntry * ASTNode_Bool2::CompileTubeIC(symbolTable & table, IC_Array & ica)
{
  tableEntry * in_var1 = children[0]->CompileTubeIC(table, ica);
  tableEntry * out_var = table.GetTempVar(type);
  std::string end_label = table.NextLabelID("end_bool_");

  // Convert the first answer to a 0 or 1 and put it in out_var.
  ica.Add("test_nequ", in_var1, "0", out_var);

  // Determine the correct operation for short-circuiting...  
  if (bool_op == '&') {
    ica.Add("jump_if_0", out_var, end_label, "", "AND!");
  }
  else if (bool_op == '|') {
    ica.Add("jump_if_n0", out_var, end_label, "", "OR!");
  }
  else { std::cerr << "INTERNAL ERROR: Unknown Bool2 type '" << bool_op << "'" << std::endl; }

  // The output code should only get here if the first part didn't short-circuit...
  tableEntry * in_var2 = children[1]->CompileTubeIC(table, ica);

  // Convert the second answer to a 0 or 1 and put it in out_var.
  ica.Add("test_nequ", in_var2, "0", out_var);

  // Leave the output label to jump to.
  ica.AddLabel(end_label);

  // Cleanup symbol table.
  if (in_var1->GetTemp() == true) table.RemoveEntry( in_var1 );
  if (in_var2->GetTemp() == true) table.RemoveEntry( in_var2 );

  return out_var;
}


///////////////////////////
// ASTNode_ArrayAccess

ASTNode_ArrayAccess::ASTNode_ArrayAccess(ASTNode * in1, ASTNode * in2)
  : ASTNode(Type::InternalType(in1->GetType()))
{
  if (Type::IsArray(in1->GetType()) == false) {
    std::string err_message = "cannot index into a non-array type '";
    err_message += Type::AsString(in1->GetType());
    err_message += "'.";
    yyerror(err_message);
    exit(1);
  }

  if (in2->GetType() != Type::VALUE) {
    yyerror ("array indices must be of type val");
    exit(1);
  }

  children.push_back(in1);
  children.push_back(in2);
}


tableEntry * ASTNode_ArrayAccess::CompileTubeIC(symbolTable & table, IC_Array & ica)
{
  tableEntry * in_var0 = children[0]->CompileTubeIC(table, ica);
  tableEntry * in_var1 = children[1]->CompileTubeIC(table, ica);
  tableEntry * out_var = table.GetTempVar(type);
    
  out_var->SetArrayIndex(in_var0->GetVarID(), in_var0, in_var1->GetVarID());
  ica.Add("ar_get_idx", in_var0, in_var1, out_var);

  if (in_var0->GetTemp() == true) table.RemoveEntry( in_var0 );
  if (in_var1->GetTemp() == true) table.RemoveEntry( in_var1 );

  return out_var;
}


/////////////////////////
// ASTNode_FunctionCall

ASTNode_FunctionCall::ASTNode_FunctionCall(tableFunction * in_fun, symbolTable & table)
    : ASTNode(in_fun->GetReturnType()), fun_entry(in_fun)
{
  // If we are currently in a function definition, track all internal variables
  // to backup in case of recursion.

  if (table.GetCurFunction() != NULL) {
    const int cur_scope = table.GetCurScope();
    backup_vars.clear();
    for (int i = 1; i <= cur_scope; i++) {
      backup_vars.insert(backup_vars.end(),
                         table.GetScopeVars(i).begin(),
                         table.GetScopeVars(i).end());
    }
  }
}

tableEntry * ASTNode_FunctionCall::CompileTubeIC(symbolTable & table, IC_Array & ica)
{
  // Collect all of the arguments, but don't yet transfer them into place.
  const std::vector<tableEntry *> & fun_args = fun_entry->GetArgs();
  std::vector<tableEntry *> arg_result_vars(children.size());
  for (int i = 0; i < (int) children.size(); i++) {
    arg_result_vars[i] = children[i]->CompileTubeIC(table, ica);
  }

  // Determine the names of the labels that we will be using.
  std::string call_label = fun_entry->GetCallLabel();
  std::string return_label = table.NextLabelID("function_return_");

  // Determine which temporary variables we need to backup before making the call.
  std::vector<int> backup_temp_scalars;
  backup_temp_scalars.insert(backup_temp_scalars.begin(),
                             table.GetTempScalars().begin(),
                             table.GetTempScalars().end());
  
  std::vector<int> backup_temp_arrays;
  backup_temp_arrays.insert(backup_temp_arrays.begin(),
                            table.GetTempArrays().begin(),
                            table.GetTempArrays().end());

  // Backup all of the local variables.
  for (tableEntry * cur_var : backup_vars) {
    if (Type::IsArray(cur_var->GetType())) {
      ica.Add("ar_push", cur_var);
    } else {
      ica.Add("push", cur_var);
    }
  }

  // Backup all of the temporary variables.
  for (int i = 0; i < (int) backup_temp_scalars.size(); i++) {
    tableEntry * temp_var = table.BuildTempEntry(Type::VALUE, backup_temp_scalars[i]);
    ica.Add("push", temp_var);
    table.FreeTempVar(temp_var);
  }
  for (int i = 0; i < (int) backup_temp_arrays.size(); i++) {
    tableEntry * temp_var = table.BuildTempEntry(Type::VALUE_ARRAY, backup_temp_arrays[i]);
    ica.Add("ar_push", temp_var);
    table.FreeTempVar(temp_var);
  }

  // Put the new function arguments into place (any old ones are backed up).
  for (int i = 0; i < (int) arg_result_vars.size(); i++) {
    tableEntry * cur_var = arg_result_vars[i];
    if (Type::IsArray(cur_var->GetType())) {  // Array!
      ica.Add("ar_copy", cur_var, fun_args[i]);
    } else {                                  // Regular variable.
      ica.Add("val_copy", cur_var, fun_args[i]);
    }
    if (cur_var->GetTemp() == true) table.RemoveEntry( cur_var );
  }
  
  // Setup the jumpback at the end of the function.
  ica.Add("push", return_label, "", "", "Save return position on the execution stack.");
  ica.Add("jump", call_label, "", "", "Call the function.");
  ica.AddLabel(return_label);

  // Setup an output variale, copy the result into it, and return
  tableEntry * out_var = table.GetTempVar(type);
  if (Type::IsArray(type)) {  // Array!
    ica.Add("ar_copy", fun_entry, out_var, "", "Copy over return value.");
  } else {                    // Regular variable
    ica.Add("val_copy", fun_entry, out_var, "", "Copy over return value.");
  }

  // Restore all of the local variables after the call in the reverse order
  for (int i = backup_temp_arrays.size()-1; i >= 0; i--) {
    tableEntry * temp_var = table.BuildTempEntry(Type::VALUE_ARRAY, backup_temp_arrays[i]);
    ica.Add("ar_pop", temp_var);
    table.FreeTempVar(temp_var);
  }
  for (int i = backup_temp_scalars.size()-1; i >= 0; i--) {
    tableEntry * temp_var = table.BuildTempEntry(Type::VALUE, backup_temp_scalars[i]);
    ica.Add("pop", temp_var);
    table.FreeTempVar(temp_var);
  }
  for (int i = backup_vars.size()-1; i >= 0; i--) {
    tableEntry * cur_var = backup_vars[i];
    if (Type::IsArray(cur_var->GetType())) {
      ica.Add("ar_pop", cur_var);
    } else {
      ica.Add("pop", cur_var);
    }
  }

  return out_var;
}


void ASTNode_FunctionCall::TypeCheckArgs()
{
  const std::vector<tableEntry *>  & def_args = fun_entry->GetArgs();

  // Make sure we have the correct number of arguments.
  if (children.size() != def_args.size()) {
    std::stringstream msg;
    msg << "Incorrect number of arguments provided for function '" << fun_entry->GetName()
        << "' (expected " << def_args.size() << ", received " << children.size() << ").";
    yyerror(msg.str());
    exit(1);
  }

  // Type check each specific argument.
  for (int i = 0; i < (int) def_args.size(); i++) {
    if (def_args[i]->GetType() != children[i]->GetType()) {
      std::string err_message = "Incorrect argument type provided for function '";
      err_message += fun_entry->GetName();
      err_message += "'";
      yyerror(err_message);
      exit(1);
    }
  }
}

//////////////////////
// ASTNode_MethodCall

ASTNode_MethodCall::ASTNode_MethodCall(ASTNode * in_base, std::string in_name)
  : ASTNode(Type::VOID), name(in_name)
{
  children.push_back(in_base);

  if (name == "size") SetType(Type::VALUE);
  else if (name == "resize") SetType(Type::VOID);
  else if (name == "push") SetType(Type::VOID);
  else if (name == "pop") SetType(Type::InternalType(in_base->GetType()));
  else {
    std::string err_message = "unknown method '";
    err_message += name;
    err_message += "'";
    yyerror(err_message);
    exit(1);
  }

  // @CAO For the moment, require the base to be an array.
  if (!Type::IsArray(in_base->GetType())) {
    std::string err_message = "array methods cannot be run on type '";
    err_message += Type::AsString(in_base->GetType());
    err_message += "'";
    yyerror(err_message);
    exit(1);
  }
}


tableEntry * ASTNode_MethodCall::CompileTubeIC(symbolTable & table, IC_Array & ica)
{
  tableEntry * array_var = children[0]->CompileTubeIC(table, ica);
  tableEntry * out_var = table.GetTempVar(type);

  if (name == "size") {
    ica.Add("ar_get_siz", array_var, out_var);
  }
  else if (name == "resize") {
    tableEntry * size_var = children[1]->CompileTubeIC(table, ica);
    ica.Add("ar_set_siz", array_var, size_var);
    if (size_var->GetTemp() == true) table.RemoveEntry( size_var );
  }
  else if (name == "push") {
    tableEntry * arg_var = children[1]->CompileTubeIC(table, ica);
    tableEntry * old_size_var = table.GetTempVar(Type::VALUE);
    tableEntry * new_size_var = table.GetTempVar(Type::VALUE);
    ica.Add("ar_get_siz", array_var, old_size_var);
    ica.Add("add", "1", old_size_var, new_size_var);
    ica.Add("ar_set_siz", array_var, new_size_var);
    ica.Add("ar_set_idx", array_var, old_size_var, arg_var);
    if (arg_var->GetTemp() == true) table.RemoveEntry( arg_var );
    table.RemoveEntry( old_size_var );
    table.RemoveEntry( new_size_var );
  }
  else if (name == "pop") {
    tableEntry * size_var = table.GetTempVar(Type::VALUE);
    ica.Add("ar_get_siz", array_var, size_var);
    ica.Add("sub", size_var, "1", size_var);
    ica.Add("ar_get_idx", array_var, size_var, out_var);
    ica.Add("ar_set_siz", array_var, size_var);
    table.RemoveEntry( size_var );
  }
  
  else {
    std::cerr << "Internal Compiler ERROR: Unknown method in ASTNode_MethodCall" << std::endl;
    exit(1);
  }

  // Cleanup symbol table.
  if (array_var->GetTemp() == true) table.RemoveEntry( array_var );

  return out_var;
}


void ASTNode_MethodCall::TypeCheckArgs()
{
  if (name == "size") {
    if (children.size() != 1) {
      yyerror("array size() method does not take any arguments.");
      exit(1);
    }
  }
  else if (name == "resize") {
    if (children.size() != 2) {
      yyerror("array resize() method takes exactly one (val) argument.");
      exit(1);
    }
    if (children[1]->GetType() != Type::VALUE) {
      yyerror("array resize() method argument must be of type val.");
      exit(1);
    }
  }
  else if (name == "push") {
    if (children.size() != 2) {
      yyerror("array push() method takes exactly one argument.");
      exit(1);
    }
    if (children[1]->GetType() != Type::InternalType(children[0]->GetType())) {
      yyerror("array push() method argument must be consistent with array.");
      exit(1);
    }
  }
  else if (name == "pop") {
    if (children.size() != 1) {
      yyerror("array pop() method does not take any arguments.");
      exit(1);
    }
  }
}


/////////////////////
// ASTNode_If

ASTNode_If::ASTNode_If(ASTNode * in1, ASTNode * in2, ASTNode * in3)
  : ASTNode(Type::VOID)
{
  if (in1->GetType() != Type::VALUE) {
    yyerror("condition for if statements must evaluate to type val.");
    exit(1);
  }

  children.push_back(in1);
  children.push_back(in2);
  children.push_back(in3);
}


tableEntry * ASTNode_If::CompileTubeIC(symbolTable & table, IC_Array & ica)
{
  std::string else_label = table.NextLabelID("if_else_");
  std::string end_label = table.NextLabelID("if_end_");

  tableEntry * in_var0 = children[0]->CompileTubeIC(table, ica);

  // If the condition is false, jump to else.  Otherwise continue through if.
  ica.Add("jump_if_0", in_var0, else_label);
  if (in_var0->GetTemp() == true) table.RemoveEntry( in_var0 );

  if (children[1]) {
    tableEntry * in_var1 = children[1]->CompileTubeIC(table, ica);
    if (in_var1 && in_var1->GetTemp() == true) table.RemoveEntry( in_var1 );
  }

  // Now that we are done with "if", jump to the end; also start the else here.
  ica.Add("jump", end_label);
  ica.AddLabel(else_label);

  if (children[2]) {
    tableEntry * in_var2 = children[2]->CompileTubeIC(table, ica);
    if (in_var2 && in_var2->GetTemp() == true) table.RemoveEntry( in_var2 );
  }

  // Close off the code with the end label.
  ica.AddLabel(end_label);

  return NULL;
}


/////////////////////
// ASTNode_While

ASTNode_While::ASTNode_While(ASTNode * in1, ASTNode * in2)
  : ASTNode(Type::VOID)
{
  children.push_back(in1);
  children.push_back(in2);

  if (in1->GetType() != Type::VALUE) {
    yyerror("condition for while statements must evaluate to type val.");
    exit(1);
  }
}


tableEntry * ASTNode_While::CompileTubeIC(symbolTable & table, IC_Array & ica)
{
  std::string start_label = table.NextLabelID("while_start_");
  std::string end_label = table.NextLabelID("while_end_");

  table.PushWhileStartLabel(start_label);
  table.PushWhileEndLabel(end_label);

  ica.AddLabel(start_label);

  tableEntry * in_var0 = children[0]->CompileTubeIC(table, ica);

  // If the condition is false, jump to end.  Otherwise continue through body.
  ica.Add("jump_if_0", in_var0, end_label);
  if (in_var0->GetTemp() == true) table.RemoveEntry( in_var0 );

  if (children[1]) {
    tableEntry * in_var1 = children[1]->CompileTubeIC(table, ica);
    if (in_var1 && in_var1->GetTemp() == true) table.RemoveEntry( in_var1 );
  }

  // Now that we are done with the while body, jump back to the start.
  ica.Add("jump", start_label);
  ica.AddLabel(end_label);

  table.PopWhileStartLabel();
  table.PopWhileEndLabel();

  return NULL;
}


/////////////////////
// ASTNode_For

ASTNode_For::ASTNode_For(ASTNode * node_init, ASTNode * node_test, ASTNode * node_inc, ASTNode * node_body)
  : ASTNode(Type::VOID)
{
  children.push_back(node_init);
  children.push_back(node_test);
  children.push_back(node_inc);
  children.push_back(node_body);

  if (node_test && node_test->GetType() != Type::VALUE) {
    yyerror("condition in 'for' statements must evaluate to type val");
    exit(1);
  }
}


tableEntry * ASTNode_For::CompileTubeIC(symbolTable & table, IC_Array & ica)
{
  ASTNode * node_init = children[0];
  ASTNode * node_test = children[1];
  ASTNode * node_inc  = children[2];
  ASTNode * node_body = children[3];

  // Start by doing the initialization code (part 1 of for loop)
  if (node_init) {    
    tableEntry * in_var_init = node_init->CompileTubeIC(table, ica);
    if (in_var_init && in_var_init->GetTemp() == true) table.RemoveEntry( in_var_init );
  }

  // Start the loop; both start and end labels need to be tracked to allow for 'break' commands.
  std::string start_label = table.NextLabelID("for_start_");
  std::string end_label = table.NextLabelID("for_end_");
  table.PushWhileStartLabel(start_label);
  table.PushWhileEndLabel(end_label);
  ica.AddLabel(start_label);

  // Test the run condition if we have one.  Otherwise ALWAYS run the loop.
  if (node_test) {
    tableEntry * in_var_test = node_test->CompileTubeIC(table, ica);

    // If the condition is false, jump to end.  Otherwise continue through body.
    ica.Add("jump_if_0", in_var_test, end_label);
    if (in_var_test->GetTemp() == true) table.RemoveEntry( in_var_test );
  }

  // If we have a body, run it!
  if (node_body) {
    tableEntry * in_var_body = node_body->CompileTubeIC(table, ica);
    if (in_var_body && in_var_body->GetTemp() == true) table.RemoveEntry( in_var_body );
  }

  // Finally, run the increment code.
  if (node_inc) {
    tableEntry * in_var_inc = node_inc->CompileTubeIC(table, ica);
    if (in_var_inc && in_var_inc->GetTemp() == true) table.RemoveEntry( in_var_inc );
  }

  // Now that we are done with the 'for' body, jump back to the start.
  ica.Add("jump", start_label);
  ica.AddLabel(end_label);

  table.PopWhileStartLabel();
  table.PopWhileEndLabel();

  return NULL;
}


/////////////////////
// ASTNode_Break

ASTNode_Break::ASTNode_Break()
  : ASTNode(Type::VOID)
{
}


tableEntry * ASTNode_Break::CompileTubeIC(symbolTable & table, IC_Array & ica)
{
  if (table.GetWhileDepth() == 0) {
    yyerror2("'break' command used outside of any loop", line_num);
    exit(1);
  }

  ica.Add("jump", table.GetWhileEndLabel());

  return NULL;
}


/////////////////////
// ASTNode_Continue

ASTNode_Continue::ASTNode_Continue()
  : ASTNode(Type::VOID)
{
}


tableEntry * ASTNode_Continue::CompileTubeIC(symbolTable & table, IC_Array & ica)
{
  if (table.GetWhileDepth() == 0) {
    yyerror2("'continue' command used outside of any loop", line_num);
    exit(1);
  }

  ica.Add("jump", table.GetWhileStartLabel());

  return NULL;
}


/////////////////////
// ASTNode_Print

ASTNode_Print::ASTNode_Print(ASTNode * out_child)
  : ASTNode(Type::VOID)
{
  // Save the child...
  if (out_child != NULL) AddChild(out_child);
}


tableEntry * ASTNode_Print::CompileTubeIC(symbolTable & table, IC_Array & ica)
{
  // Collect the output arguments as they are calculated...
  for (int i = 0; i < (int) children.size(); i++) {
    tableEntry * cur_var = children[i]->CompileTubeIC(table, ica);
    switch (cur_var->GetType()) {
    case Type::VALUE:
      ica.Add("out_val", cur_var);
      break;
    case Type::CHAR:
      ica.Add("out_char", cur_var);
      break;
    case Type::STRING:
    case Type::VALUE_ARRAY:
      {
        tableEntry * size_var = table.GetTempVar(false);
        tableEntry * index_var = table.GetTempVar(false);
        tableEntry * entry_var = table.GetTempVar(false);        
        std::string start_label = table.NextLabelID("print_array_start_");
        std::string end_label = table.NextLabelID("print_array_end_");
        
        ica.Add("val_copy", "0", index_var, "", "Init loop variable for printing array.");
        ica.Add("ar_get_siz", cur_var, size_var, "", "Save size of array into variable.");
        ica.AddLabel(start_label);

        ica.Add("test_gte", index_var, size_var, entry_var, "Test if we are finished yet...");
        ica.Add("jump_if_n0", entry_var, end_label, "", " ...and jump to end if so.");

        ica.Add("ar_get_idx", cur_var, index_var, entry_var,
                "Collect the value at the next index.");

        if (cur_var->GetType() == Type::STRING) ica.Add("out_char", entry_var, "", "",
                                                        "Print this entry!");
        else if (cur_var->GetType() == Type::VALUE_ARRAY) ica.Add("out_val", entry_var, "", "",
                                                                   "Print this entry!");

        ica.Add("add", index_var, "1", index_var, "Increment to the next index.");
        
        ica.Add("jump", start_label);
        ica.AddLabel(end_label);

        table.FreeTempVar(size_var);
        table.FreeTempVar(index_var);
        table.FreeTempVar(entry_var);
      }
      break;
    default:
      std::cerr << "Internal Compiler ERROR: Unknown Type in Write::CompilerTubeIC" << std::endl;
      exit(1);
    };

    if (cur_var->GetTemp() == true) table.RemoveEntry( cur_var );
  }
  ica.Add("out_char", "'\\n'", "", "", "End print statements with a newline.");
  
  return NULL;
}


//////////////////////
//  ASTNode_Random

ASTNode_Random::ASTNode_Random(ASTNode * in_child)
  : ASTNode(Type::VALUE)
{
  if (in_child->GetType() != Type::VALUE) {
    std::string err_message = "cannot use type '";
    err_message += Type::AsString(in_child->GetType());
    err_message += "' as an argument to random";
    yyerror(err_message);
    exit(1);
  }
  children.push_back(in_child);
}

tableEntry * ASTNode_Random::CompileTubeIC(symbolTable & table, IC_Array & ica)
{
  tableEntry * in_var = children[0]->CompileTubeIC(table, ica);
  tableEntry * out_var = table.GetTempVar(type);

  ica.Add("random", in_var, out_var);

  if (in_var->GetTemp() == true) table.RemoveEntry( in_var );

  return out_var;
}


//////////////////////
//  ASTNode_Return

ASTNode_Return::ASTNode_Return(ASTNode * in_child, symbolTable & table)
  : ASTNode(Type::VOID)
{
  fun_entry = table.GetCurFunction();
  if (fun_entry == NULL) {
    yyerror("'return' run outside of any function.");
    exit(1);
  }
  if (fun_entry->GetReturnType() != in_child->GetType()) {
    std::string err_message = "incorrect return type for function '";
    err_message += fun_entry->GetName();
    err_message += "'.  Expected: '";
    err_message += Type::AsString(fun_entry->GetReturnType());
    err_message += "', but found '";
    err_message += Type::AsString(in_child->GetType());
    err_message += "')";
    yyerror(err_message);
    exit(1);
  }
  children.push_back(in_child);
}

tableEntry * ASTNode_Return::CompileTubeIC(symbolTable & table, IC_Array & ica)
{
  // Calculate the return value
  tableEntry * in_var = children[0]->CompileTubeIC(table, ica);

  // Save this value as the function return value.
  int rtype = fun_entry->GetReturnType();
  if (Type::IsArray(rtype)) { // Return type is an array.
    ica.Add("ar_copy", in_var, fun_entry);
  } else {                    // Return type is not an array.
    ica.Add("val_copy", in_var, fun_entry);
  }
  if (in_var->GetTemp() == true) table.RemoveEntry( in_var );

  // Now jump back!
  tableEntry * jump_var = table.GetTempVar(type);
  ica.Add("pop", jump_var);
  ica.Add("jump", jump_var);
  table.RemoveEntry( jump_var );

  // Cannot be part of another expression, so nothing to return.
  return NULL;
}


//////////////////////
//  ASTNode_Ternary

ASTNode_Ternary::ASTNode_Ternary(ASTNode * child_test, ASTNode * child_true, ASTNode * child_false)
  : ASTNode(child_true->GetType())
{
  if (child_test->GetType() != Type::VALUE) {
    yyerror("condition for ternary operators must evaluate to type val");
    exit(1);
  }
  if (child_true->GetType() != child_false->GetType()) {
    std::string err_message = "ternary operator must have matching types; '";
    err_message += Type::AsString(child_true->GetType());
    err_message += "' != '";
    err_message += Type::AsString(child_false->GetType());
    err_message += "'.";
    yyerror(err_message);
    exit(1);
  }
  children.push_back(child_test);
  children.push_back(child_true);
  children.push_back(child_false);
}

tableEntry * ASTNode_Ternary::CompileTubeIC(symbolTable & table, IC_Array & ica)
{
  tableEntry * test_var = children[0]->CompileTubeIC(table, ica);
  tableEntry * out_var = table.GetTempVar(type);
  std::string false_label = table.NextLabelID("ternary_false_");
  std::string end_label = table.NextLabelID("ternary_end_");

  // Execute either the true or false code and retun the appropriate value.
  ica.Add("jump_if_0", test_var, false_label);
  tableEntry * true_var = children[1]->CompileTubeIC(table, ica);
  if (type == Type::VALUE || type == Type::CHAR) {
    ica.Add("val_copy", true_var, out_var);
  } else {
    ica.Add("ar_copy", true_var, out_var);
  }
  ica.Add("jump", end_label);

  ica.AddLabel(false_label);
  tableEntry * false_var = children[2]->CompileTubeIC(table, ica);
  if (type == Type::VALUE || type == Type::CHAR) {
    ica.Add("val_copy", false_var, out_var);
  } else {
    ica.Add("ar_copy", false_var, out_var);
  }

  ica.AddLabel(end_label);

  if (test_var->GetTemp() == true) table.RemoveEntry( test_var );
  if (true_var->GetTemp() == true) table.RemoveEntry( true_var );
  if (false_var->GetTemp() == true) table.RemoveEntry( false_var );

  return out_var;
}
