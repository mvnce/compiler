#include "symbol_table.h"

#include "ast.h"
#include "ic.h"
#include "tube8-parser.tab.hh"

extern void yyerror(std::string err_string);
extern void yyerror2(std::string err_string, int orig_line);

void tableFunction::SetArgs(const std::vector<tableEntry *> & in_args)
{
  // If the args for this function have not been set already, do so.
  if (args_set == false) {
    args = in_args;
    args_set = true;
  }

  // Otherwise make sure the new args are consistent with the old ones.
  else {
    if (args.size() != in_args.size()) {
      std::stringstream msg;
      msg << "Expected " << args.size() << " arguments in definition for function '" << name
          << "' but received " << in_args.size();
      yyerror(msg.str());
      exit(1);                       
    }
    for (int i = 0; i < (int) args.size(); i++) {
      if (args[i]->GetType() != in_args[i]->GetType()) {
        std::stringstream msg;
        msg << "Expected argument " << i << " in definition for function '" << name
            << "' to be of type " << Type::AsString(args[i]->GetType()) << ", but received type "
            << Type::AsString(in_args[i]->GetType()) << ".";
        yyerror(msg.str());
        exit(1);                       
      }
      in_args[i]->SetVarID( args[i]->GetVarID() );  // Make sure var lines up with original!
    }
  }
}

void tableFunction::CompileTubeIC(symbolTable & table, IC_Array & ica)
{
  std::string fun_comment = "FUNCTION: ";
  fun_comment += name;
  ica.Add("nop", "", "", "", fun_comment);

  // Drop a label to mark the beginning on this function.
  ica.AddLabel(call_label);

  // Process the AST for this function
  if (ast == NULL) {
    std::string err_message = "function '";
    err_message += name;
    err_message += "' never fully defined!";
    yyerror2(err_message, dec_line);
    exit(1);
  }
  ast->CompileTubeIC(table, ica);
}

void symbolTable::CompileTubeIC(IC_Array & ica)
{
  if (function_map.size() > 0) {
    std::string end_label = "define_functions_end";
    
    ica.Add("nop");
    ica.Add("nop");
    ica.Add("nop", "", "", "", "============ FUNCTIONS ============");
    ica.Add("jump", end_label, "", "", "Skip over function defs during normal execution");
    ica.Add("nop");
    
    for (std::map<std::string, tableFunction *>::iterator it = function_map.begin();
         it != function_map.end();
         ++it ) {
      tableFunction * cur_fun = it->second;
      cur_fun->CompileTubeIC(*this, ica);
    }
    
    ica.AddLabel(end_label);
  }
}


