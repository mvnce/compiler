%{
#include <iostream>
#include <string>
#include <fstream>
#include <stdio.h>

#include "symbol_table.h"
#include "ast.h"
#include "type_info.h"

extern int line_num;
extern int yylex();
extern std::string out_filename;
extern bool use_int_code;
 
symbolTable symbol_table;
int error_count = 0;

// Create an error function to call when the current line has an error
void yyerror(std::string err_string) {
  std::cout << "ERROR(line " << line_num << "): "
       << err_string << std::endl;
  error_count++;
}

// Create an alternate error function when a *different* line than being read in has an error.
void yyerror2(std::string err_string, int orig_line) {
  std::cout << "ERROR(line " << orig_line << "): "
       << err_string << std::endl;
  error_count++;
}

%}

%union {
  char * lexeme;
  int value;
  ASTNode * ast_node;
  tableEntry * symbol_table_entry;
  tableFunction * symbol_table_function;
}

%token CASSIGN_ADD CASSIGN_SUB CASSIGN_MULT CASSIGN_DIV COMP_EQU COMP_NEQU COMP_LESS COMP_LTE COMP_GTR COMP_GTE BOOL_AND BOOL_OR COMMAND_PRINT COMMAND_IF COMMAND_ELSE COMMAND_WHILE COMMAND_FOR COMMAND_BREAK COMMAND_CONTINUE COMMAND_RANDOM COMMAND_RETURN COMMAND_DEFINE COMMAND_DECLARE
%token <lexeme> VAL_LIT CHAR_LIT STRING_LIT TYPE META_TYPE ID

%right '=' CASSIGN_ADD CASSIGN_SUB CASSIGN_MULT CASSIGN_DIV
%right ':' '?'
%left BOOL_OR
%left BOOL_AND
%nonassoc COMP_EQU COMP_NEQU COMP_LESS COMP_LTE COMP_GTR COMP_GTE
%left '+' '-'
%left '*' '/'
%nonassoc UMINUS '!'
%left '.'

%nonassoc NOELSE
%nonassoc COMMAND_ELSE


%type <ast_node> expression declare_assign statement statement_list var_usage array_index lhs_ok command argument_list argument code_block for_init opt_expr if_start while_start for_start flow_command
%type <value> type_any
%type <symbol_table_entry> var_declare
%type <symbol_table_function> define_start declare_start
%%

program:      statement_list {
                // This rule should be the final one to trigger.

                IC_Array ic_array;  // Intermediate code container

                // Traverse the AST, filling ic_array with code
                $1->CompileTubeIC(symbol_table, ic_array);

                // Generate function code from the symbol table
                symbol_table.CompileTubeIC(ic_array);

                ic_array.AddBlock();
                ic_array.AddLocal();
                ic_array.AlgebraicOpt();
                ic_array.AlgebraicOpt();

                // Open the specified output file
                std::ofstream out_file(out_filename.c_str()); 

                // Determine the proper output format.
                if (use_int_code == true) {
                  ic_array.PrintIC(out_file);        // Write Intermediate Code
                } else {
                  ic_array.PrintTubeCode(out_file);  // Write TubeCode Assembly
                }
              }

statement_list:	 {
	           $$ = new ASTNode_Root;
                 }
	|        statement_list statement {
                   if ($2 != NULL) $1->AddChild($2);
                   $$ = $1;
		 }
	|        statement_list define_command {
                   $$ = $1;
		 }
  |        statement_list declare_command {
                   $$ = $1;
  }

statement:   var_declare ';'    {  $$ = NULL; /* Only update symbol table */ }
         |   declare_assign ';' {  $$ = $1;  }
	 |   expression ';'     {  $$ = $1;  }
	 |   command ';'        {  $$ = $1;  }
         |   flow_command       {  $$ = $1;  }
         |   code_block         {  $$ = $1;  }
         |   ';'                {  $$ = NULL; /* No statement to include */ }

type_any:       TYPE {
		  std::string type_name = $1;
		  int type_id = 0;
		  if (type_name == "val") type_id = Type::VALUE;
		  else if (type_name == "char") type_id = Type::CHAR;
		  else if (type_name == "string") type_id = Type::STRING;
		  else {
		    std::string err_string = "unknown type '";
		    err_string += $1;
                    err_string += "'";
		    yyerror(err_string);
		  }
                  $$ = type_id;
                }
        |       META_TYPE '(' TYPE ')' {
		  std::string type_name = $3;
		  int type_id = 0;
		  if (type_name == "val") type_id = Type::VALUE_ARRAY;
		  else if (type_name == "char") type_id = Type::STRING;
		  else {
		    std::string err_string = "unknown type 'array(";
		    err_string += $3;
                    err_string += ")'";
		    yyerror(err_string);
		  }
                  $$ = type_id;
                }

var_declare:	type_any ID {
	          if (symbol_table.InCurScope($2) == true) {
		    std::string err_string = "redeclaration of variable '";
		    err_string += $2;
                    err_string += "'";
                    yyerror(err_string);
		    exit(1);
                  }

                  $$ = symbol_table.AddEntry($1, $2);
	        }

declare_assign:  var_declare '=' expression {
                   ASTNode_Variable * var_node = new ASTNode_Variable($1);
                   var_node->SetLineNum(line_num);

	           $$ = new ASTNode_Assign(var_node, $3);
                   $$->SetLineNum(line_num);
	         }

var_usage:   ID {
	       tableEntry * cur_entry = symbol_table.Lookup($1);
               if (cur_entry == NULL) {
		 std::string err_string = "unknown variable '";
		 err_string += $1;
                 err_string += "'";
		 yyerror(err_string);
                 exit(1);
               }
	       $$ = new ASTNode_Variable(cur_entry);
               $$->SetLineNum(line_num);
             }

array_index: var_usage '[' expression ']' {
               $$ = new ASTNode_ArrayAccess($1, $3);
               $$->SetLineNum(line_num);
             }

lhs_ok:  var_usage { $$ = $1; }
      |  array_index { $$ = $1; }

expression:  expression '+' expression { 
	       $$ = new ASTNode_Math2($1, $3, '+');
               $$->SetLineNum(line_num);
             }
	|    expression '-' expression {
	       $$ = new ASTNode_Math2($1, $3, '-');
               $$->SetLineNum(line_num);
             }
	|    expression '*' expression {
	       $$ = new ASTNode_Math2($1, $3, '*');
               $$->SetLineNum(line_num);
             }
	|    expression '/' expression {
	       $$ = new ASTNode_Math2($1, $3, '/');
               $$->SetLineNum(line_num);
             }
	|    expression COMP_EQU expression {
               $$ = new ASTNode_Math2($1, $3, COMP_EQU);
               $$->SetLineNum(line_num);
             }
	|    expression COMP_NEQU expression {
               $$ = new ASTNode_Math2($1, $3, COMP_NEQU);
               $$->SetLineNum(line_num);
             }
	|    expression COMP_LESS expression {
               $$ = new ASTNode_Math2($1, $3, COMP_LESS);
               $$->SetLineNum(line_num);
             }
	|    expression COMP_LTE expression {
               $$ = new ASTNode_Math2($1, $3, COMP_LTE);
               $$->SetLineNum(line_num);
             }
	|    expression COMP_GTR expression {
               $$ = new ASTNode_Math2($1, $3, COMP_GTR);
               $$->SetLineNum(line_num);
             }
	|    expression COMP_GTE expression {
               $$ = new ASTNode_Math2($1, $3, COMP_GTE);
               $$->SetLineNum(line_num);
             }
	|    expression BOOL_AND expression {
               $$ = new ASTNode_Bool2($1, $3, '&');
               $$->SetLineNum(line_num);
             }
	|    expression BOOL_OR expression {
               $$ = new ASTNode_Bool2($1, $3, '|');
               $$->SetLineNum(line_num);
             }
	|    lhs_ok '=' expression {
               $$ = new ASTNode_Assign($1, $3);
               $$->SetLineNum(line_num);
             }
	|    lhs_ok CASSIGN_ADD expression {
               $$ = new ASTNode_Assign($1, new ASTNode_Math2($1, $3, '+') );
               $$->SetLineNum(line_num);
             }
	|    lhs_ok CASSIGN_SUB expression {
               $$ = new ASTNode_Assign($1, new ASTNode_Math2($1, $3, '-') );
               $$->SetLineNum(line_num);
             }
	|    lhs_ok CASSIGN_MULT expression {
               $$ = new ASTNode_Assign($1, new ASTNode_Math2($1, $3, '*') );
               $$->SetLineNum(line_num);
             }
	|    lhs_ok CASSIGN_DIV expression {
               $$ = new ASTNode_Assign($1, new ASTNode_Math2($1, $3, '/') );
               $$->SetLineNum(line_num);
             }
	|    '-' expression %prec UMINUS {
               $$ = new ASTNode_Math1($2, '-');
               $$->SetLineNum(line_num);
             }
	|    '!' expression %prec UMINUS {
               $$ = new ASTNode_Math1($2, '!');
               $$->SetLineNum(line_num);
             }
	|    '(' expression ')' { $$ = $2; } // Ignore parens; used for order
	|    VAL_LIT {
               $$ = new ASTNode_Literal(Type::VALUE, $1);
               $$->SetLineNum(line_num);
             }
	|    CHAR_LIT {
               $$ = new ASTNode_Literal(Type::CHAR, $1);
               $$->SetLineNum(line_num);
             }
        |    STRING_LIT {
               $$ = new ASTNode_Literal(Type::STRING, $1);
               $$->SetLineNum(line_num);
             }
	|    var_usage { $$ = $1; }
	|    array_index { $$ = $1; }
        |    ID '(' ')' {
	       tableFunction * cur_fun = symbol_table.LookupFunction($1);
               if (cur_fun == NULL) {
		 std::string err_string = "unknown function '";
		 err_string += $1;
                 err_string += "'";
		 yyerror(err_string);
                 exit(1);
               }
               ASTNode_FunctionCall * node = new ASTNode_FunctionCall(cur_fun, symbol_table);
               node->TypeCheckArgs();
               $$ = node;
               $$->SetLineNum(line_num);
             }
        |    ID '(' argument_list ')' {
	       tableFunction * cur_fun = symbol_table.LookupFunction($1);
               if (cur_fun == NULL) {
		 std::string err_string = "unknown function '";
		 err_string += $1;
                 err_string += "'";
		 yyerror(err_string);
                 exit(1);
               }
               ASTNode_FunctionCall * node = new ASTNode_FunctionCall(cur_fun, symbol_table);
	       node->TransferChildren($3);
	       delete $3;
               node->TypeCheckArgs();
               $$ = node;
               $$->SetLineNum(line_num);
             }
        |    expression '.' ID '(' ')' {
               ASTNode_MethodCall * node = new ASTNode_MethodCall($1, $3);
               node->TypeCheckArgs();
               $$ = node;
               $$->SetLineNum(line_num);
             }
        |    expression '.' ID '(' argument_list ')' {
               ASTNode_MethodCall * node = new ASTNode_MethodCall($1, $3);
	       node->TransferChildren($5);
	       delete $5;
               node->TypeCheckArgs();
               $$ = node;
               $$->SetLineNum(line_num);
             }
        |    COMMAND_RANDOM '(' expression ')' {
               $$ = new ASTNode_Random($3);
               $$->SetLineNum(line_num);
             }
        |    expression '?' expression ':' expression {
               $$ = new ASTNode_Ternary($1, $3, $5);
               $$->SetLineNum(line_num);
             }

argument_list:	argument_list ',' argument {
		  ASTNode * node = $1; // Grab the node used for arg list.
		  node->AddChild($3);    // Save this argument in the node.
		  $$ = node;
		}
	|	argument {
		  // Create a temporary AST node to hold the arg list.
		  $$ = new ASTNode_TempNode(Type::VOID);
		  $$->AddChild($1);   // Save this argument in the temp node.
                  $$->SetLineNum(line_num);
		}

argument:	expression { $$ = $1; }


command:   COMMAND_PRINT '(' argument_list ')' {
	     $$ = new ASTNode_Print(NULL);
	     $$->TransferChildren($3);
             $$->SetLineNum(line_num);
	     delete $3;
           }
        |  COMMAND_BREAK {
             $$ = new ASTNode_Break();
             $$->SetLineNum(line_num);
           }
        |  COMMAND_CONTINUE {
             $$ = new ASTNode_Continue();
             $$->SetLineNum(line_num);
           }
        |  COMMAND_RETURN expression {
             $$ = new ASTNode_Return($2, symbol_table);
             $$->SetLineNum(line_num);
           }

if_start:  COMMAND_IF '(' expression ')' {
             $$ = new ASTNode_If($3, NULL, NULL);
             $$->SetLineNum(line_num);
           }

while_start:  COMMAND_WHILE '(' expression ')' {
                $$ = new ASTNode_While($3, NULL);
                $$->SetLineNum(line_num);
              }

for_init:   var_declare { $$ = NULL; }
        |   declare_assign { $$ = $1; }
        |   expression { $$ = $1; }
        |   { $$ = NULL; }

opt_expr:   expression { $$ = $1; }
        |   { $$ = NULL; }

for_start:  COMMAND_FOR '(' for_init ';' opt_expr ';' opt_expr ')' {
                $$ = new ASTNode_For($3, $5, $7, NULL);
                $$->SetLineNum(line_num);
            }

flow_command:  if_start statement COMMAND_ELSE statement {
                 $$->SetChild(1, $2);
                 $$->SetChild(2, $4);
                 $$ = $1;
               }
            |  if_start statement %prec NOELSE {
                 $$ = $1;
                 $$->SetChild(1, $2);
               }
            |  while_start statement {
                 $$ = $1;
                 $$->SetChild(1, $2);
               }
            |  for_start statement {
                 $$ = $1;
                 $$->SetChild(3, $2);
               }

declare_start: COMMAND_DECLARE type_any ID {
                 if (symbol_table.GetCurScope() != 0) {
       std::string err_string = "Attempting to define function '";
       err_string += $3;
                   err_string += "' outside of global scope!";
       yyerror(err_string);
                   exit(1);
                 }
                 $$ = symbol_table.StartFunction($2, $3);
                 if ($$ == NULL) { // Return types do not match!
       std::string err_string = "Attempting to define function '";
       err_string += $3;
                   err_string += "', with return type '";
                   err_string += Type::AsString($2);
                   err_string += ", but previously declared as type '";
                   err_string += Type::AsString(symbol_table.LookupFunction($3)->GetReturnType());
                   err_string += "'.";
       yyerror(err_string);
                   exit(1);
                 } else if ($$->GetAST() != NULL) {
       std::string err_string = "Attempting to re-define function '";
       err_string += $3;
                   err_string += "'.";
       yyerror(err_string);
                   exit(1);
                 }
}

define_start:  COMMAND_DEFINE type_any ID {
                 if (symbol_table.GetCurScope() != 0) {
		   std::string err_string = "Attempting to define function '";
		   err_string += $3;
                   err_string += "' outside of global scope!";
		   yyerror(err_string);
                   exit(1);
                 }
                 $$ = symbol_table.StartFunction($2, $3);
                 if ($$ == NULL) { // Return types do not match!
		   std::string err_string = "Attempting to define function '";
		   err_string += $3;
                   err_string += "', with return type '";
                   err_string += Type::AsString($2);
                   err_string += ", but previously declared as type '";
                   err_string += Type::AsString(symbol_table.LookupFunction($3)->GetReturnType());
                   err_string += "'.";
		   yyerror(err_string);
                   exit(1);
                 } else if ($$->GetAST() != NULL) {
		   std::string err_string = "Attempting to re-define function '";
		   err_string += $3;
                   err_string += "'.";
		   yyerror(err_string);
                   exit(1);
                 }
               }

fun_arg_list:  var_declare { ; }
            |  fun_arg_list ',' var_declare { ; }

fun_arg_test:  fun_arg_list { ; }
            |  { ; }

define_command:  define_start '(' fun_arg_test ')' {
                     $1->SetArgs(symbol_table.GetScopeVars(1));
                 } statement {
                     $1->SetAST($6);
                     symbol_table.EndFunction();
                 }

declare_command: declare_start '(' fun_arg_test ')' {
                     $1->SetArgs(symbol_table.GetScopeVars(1));
                     symbol_table.EndFunction();
                 }

block_start: '{' { symbol_table.IncScope(); }
block_end:   '}' { symbol_table.DecScope(); }
code_block:  block_start statement_list block_end { $$ = $2; }

%%
void LexMain(int argc, char * argv[]);

int main(int argc, char * argv[])
{
  error_count = 0;
  LexMain(argc, argv);

  yyparse();

  if (error_count == 0) std::cout << "Parse Successful!" << std::endl;

  return error_count;
}
