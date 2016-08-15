%{
#include <iostream>
#include <string>
#include <fstream>
#include "symbol_table.h"
#include "ast.h"
#include <stdio.h>
#include <cstdlib>
#include <vector>

using namespace std;

extern int line_num;
extern int while_id;
extern symbolTable symbol_table;
extern int yylex();
extern bool do_interpret;
extern FILE * yyin;
FILE * outfile;
 
void yyerror(std::string err_string) {
  std::cout << "ERROR(line " << line_num << "): "
            << err_string << std::endl;
  exit(1);
}
//symbolTable symbol_table;
%}
%union {
  char * lexeme;
  ASTNode_Base * ast_node;
}
%token CASSIGN_ADD CASSIGN_SUB CASSIGN_MULT CASSIGN_DIV CASSIGN_MOD
%token COMP_EQU COMP_NEQU COMP_LESS COMP_LTE COMP_GTR COMP_GTE BOOL_AND BOOL_OR
%token CTRL_IF CTRL_ELSE CTRL_WHILE CTRL_BREAK CTRL_CONTINUE

%token <lexeme> CHAR VALUE TYPE ID COMMAND_PRINT COMMAND_RANDOM
%left BOOL_OR
%left BOOL_AND

%right '=' CASSIGN_ADD CASSIGN_SUB CASSIGN_MULT CASSIGN_DIV CASSIGN_MOD
%left COMP_EQU COMP_NEQU COMP_LESS COMP_LTE COMP_GTR COMP_GTE
%left '+' '-'
%left '*' '/'
%nonassoc UMINUS NOT
%type <ast_node> expression declaration statement statement_list var_usage var_declare command argument_list scope ctrl_flow
%%
program:      statement_list {
                // NOTE: Any code here is run LAST after parsing is finished.
                // Uncomment next line to print out status of AST.
                // $1->Debug();
                $1->ProcessIC(outfile);
              }
        ;
statement_list: {
                   $$ = new ASTNode_Root;
                   // NOTE: Any code here is run FIRST as parsing begins.
                }
        | statement_list statement {
                   // NOTE: Code here is run as each full statement finishes being parsed.
                   $1->AddChild($2);
                }

// NOTE: Different statement structures should be defined within the statement rule.
statement:   declaration ';' {  $$ = $1;  }
  |    expression ';'  {  $$ = $1;  }
  |    command ';'    {  $$ = $1;  }
  |    scope   {  $$ = $1;  }
  |    ctrl_flow    {  $$ = $1;  }

// Declarations include with and without assignments
declaration:  var_declare '=' expression {
         $$ = new ASTNode_Assign($1, $3);
       }
        |    var_declare {
               $$ = $1;
             }
// var_usage represents any time a previous declared variable is USED.
var_usage:   ID {
         tableEntry * cur_entry = symbol_table.Lookup($1);
         if (cur_entry == NULL) {
           cur_entry = symbol_table.DeepLookup($1);
         }

               if (cur_entry == NULL) {
     std::string err_string = "unknown variable '";
     err_string += $1;
                 err_string += "'";
     yyerror(err_string);
                 exit(1);
               }
         $$ = new ASTNode_Variable(cur_entry);
             }
// var_declare represents any time a new variable is being CREATED.
var_declare:  TYPE ID {
            if (symbol_table.Lookup($2) != NULL) {
        std::string err_string = "redeclaration of variable '";
        err_string += $2;
                    err_string += "'";
                    yyerror(err_string);
        exit(1);
                  }
      std::string type_name = $1;
      int type_id = 0;
      if (type_name == "val") type_id = Type::VAL;
                  // NOTE: additional type indentification would go here. (Project 4)
      else if (type_name == "char") type_id = Type::CHAR;

      else {
        std::string err_string = "unknown type '";
        err_string += $1;
                    err_string += "'";
        yyerror(err_string);
      }
            tableEntry * cur_entry = symbol_table.Insert(type_id, $2);
                  // Return new variable node in case this is the LHS of an expresssion.
            $$ = new ASTNode_Variable(cur_entry);
          }
expression:  expression '+' expression { 
         $$ = new ASTNode_Math2($1, $3, '+');
             }
  |    expression '-' expression {
         $$ = new ASTNode_Math2($1, $3, '-');
             }
  |    expression '*' expression {
         $$ = new ASTNode_Math2($1, $3, '*');
             }
  |    expression '/' expression {
         $$ = new ASTNode_Math2($1, $3, '/');
             }
  |    var_usage '=' expression {
               $$ = new ASTNode_Assign($1, $3);
             }
  |    var_usage CASSIGN_ADD expression {
               // Rather than make a new type of node for compound assignements,
               // we combine existing ones.
               $$ = new ASTNode_Assign($1, new ASTNode_Math2($1, $3, '+') );
             }
  |    var_usage CASSIGN_SUB expression {
               $$ = new ASTNode_Assign($1, new ASTNode_Math2($1, $3, '-') );
             }
  |    var_usage CASSIGN_MULT expression {
               $$ = new ASTNode_Assign($1, new ASTNode_Math2($1, $3, '*') );
             }
  |    var_usage CASSIGN_DIV expression {
               $$ = new ASTNode_Assign($1, new ASTNode_Math2($1, $3, '/') );
             }
  |    expression COMP_EQU expression {
               // NOTE: Not enough info to know which operation from here on down!
               $$ = new ASTNode_Math2($1, $3, 'Q');
             }
  |    expression COMP_NEQU expression {
         $$ = new ASTNode_Math2($1, $3, 'U');
             }
  |    expression COMP_LESS expression {
         $$ = new ASTNode_Math2($1, $3, 'L');
             }
  |    expression COMP_LTE expression {
         $$ = new ASTNode_Math2($1, $3, 'T');
             }
  |    expression COMP_GTR expression {
         $$ = new ASTNode_Math2($1, $3, 'R');
             }
  |    expression COMP_GTE expression {
         $$ = new ASTNode_Math2($1, $3, 'E');
             }
  |    expression BOOL_AND expression {
         $$ = new ASTNode_Math3($1, $3, 'A');
             }
  |    expression BOOL_OR expression {
         $$ = new ASTNode_Math3($1, $3, 'O');
             }
  |    '-' expression %prec UMINUS {
         $$ = new ASTNode_Math1_Minus($2);
             }
  |    '!' expression %prec NOT {
         $$ = new ASTNode_Math0_Not($2);
             }
  |    '(' expression ')' { $$ = $2; } // Ignore parens; used for order
  |    VALUE {
               $$ = new ASTNode_Literal(Type::VAL, $1);
             }
  |    CHAR {
               $$ = new ASTNode_Literal(Type::CHAR, $1);
             }
  |    var_usage { $$ = $1; }
        |    COMMAND_RANDOM '(' expression ')' {
               $$ = new ASTNode_Random($3);
             }
// argument_list requires at least one argument, so no empty rule.
argument_list:  argument_list ',' expression {
      $1->AddChild($3);    // Save this argument in the node.
      $$ = $1;
    }
  | expression {
      // Create a temporary parse node to hold the arg list.
                  $$ = new ASTNode_Temp(Type::VOID);
      $$->AddChild($1);   // Save this argument in the temp node.
    }
// Currently the only command is 'print', but others may be used in the future.
command:   COMMAND_PRINT '(' argument_list ')' {
       $$ = new ASTNode_Print();
       $$->TransferChildren($3);
       delete $3;
           }

scope:  brace_s statement_list brace_e {
          $$ = $2;
        }

brace_s: '{'  {
                symbol_table.IncreaseScope(); 
              }

brace_e: '}'  {
                symbol_table.DecreaseScope();
              }

ctrl_flow:  CTRL_IF '(' expression ')' ';' {
              $$ = new ASTNode_If($3, NULL, NULL);
            }

          | CTRL_IF '(' expression ')' ';' CTRL_ELSE statement {
              $$ = new ASTNode_If($3, NULL, $7);
            }

          | CTRL_IF '(' expression ')' statement {
              $$ = new ASTNode_If($3, $5, NULL);
            }

          | CTRL_IF '(' expression ')' statement CTRL_ELSE statement {
              $$ = new ASTNode_If($3, $5, $7);
          }

          | CTRL_WHILE '(' expression ')' statement {
              $$ = new ASTNode_While($3, $5, while_id);
              while_id++;
              symbol_table.PopWhile();
          }

          | CTRL_BREAK ';' {
              if (symbol_table.GetWhileSize() <= 0) {
                yyerror("'break' command used outside of any loop");
                exit(1);
              }
              $$ = new ASTNode_Break(symbol_table.PopWhile());
          }

          | CTRL_CONTINUE ';' {
            if (symbol_table.GetWhileSize() <= 0) {
                yyerror("'continue' command used outside of any loop");
                exit(1);
              }
              $$ = new ASTNode_CONTINUE(symbol_table.GetWhileId()-1);
          }

%%
int main(int argc, char * argv[]) {

  // Make sure we have exactly two argument.
  if (argc != 3) {
    std::cerr << "Format: " << argv[0] << " [filename1]"  << "[filename2]" << std::endl;
    exit(1);
  }
  
  // Assume the argument is the filename.
  yyin = fopen(argv[1], "r");
  if (!yyin) {
    std::cerr << "Error opening " << argv[1] << std::endl;
    exit(1);
  }
  // Parse the file!  If it doesn't exit, parse must have been successful.
  outfile = fopen(argv[2], "w");
  if (!outfile) {
    std::cerr << "Error opening " << argv[2] << std::endl;
    exit(1);
  }
  yyparse();
  fclose(outfile);
  
  std::cout << "Parse Successful!" << std::endl;
  
  return 0;
}
