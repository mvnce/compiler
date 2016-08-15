#include "symbol_table.h"

#include "ast.h"
#include "tc.h"
#include "tube6-parser.tab.hh"

extern void yyerror(std::string err_string);
extern void yyerror2(std::string err_string, int orig_line);
