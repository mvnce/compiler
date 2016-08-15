%{
#include <iostream>
#include <stdio.h>

#include "symbol_table.h"
#include "ast.h"
#include "tube3.tab.hh"

int line_num = 1;
bool do_interpret = false;
%}

id		[a-zA-Z_][a-zA-Z0-9_]*
comment		#.*
eol		\n
whitespace	[ \t\r]
operator	[+\-*/=(),[\]{}.;]
types           val|char|string
val_lit		[0-9]+|[0-9]*\.[0-9]+
char_lit        '(.|(\\[\\'nt]))'
string_lit      \"(\\[nt"\\]|[^\\"])*\"
string_err      \"(\\.|[^\\"])*\"
string_err2     \"(\\.|[^\\"])*
                 
%%

{types} {
         yylval.lexeme = strdup(yytext); 
         return TYPE;
       }

print { 
         yylval.lexeme = strdup(yytext); 
         return COMMAND_PRINT;
       }

random { 
         yylval.lexeme = strdup(yytext); 
         return COMMAND_RANDOM;
       }

{id}   { 
         yylval.lexeme = strdup(yytext); 
	 return ID;
       }

{val_lit} { 
         yylval.lexeme = strdup(yytext); 
         return VALUE;
       }

{operator} { 
         yylval.lexeme = strdup(yytext); 
         return (int) yytext[0];
       }

"+=" { return CASSIGN_ADD; }
"-=" { return CASSIGN_SUB; }
"*=" { return CASSIGN_MULT; }
"/=" { return CASSIGN_DIV; }

"==" { return COMP_EQU; }
"!=" { return COMP_NEQU; }
"<"  { return COMP_LESS; }
"<=" { return COMP_LTE; }
">"  { return COMP_GTR; }
">=" { return COMP_GTE; }

"&&" { return BOOL_AND; }
"||" { return BOOL_OR; }


{comment} { ; }
{eol}  { line_num++; }

{whitespace} { ; }
.      { std::cout << "ERROR(line " << line_num << "): Unknown Token '"
                   << yytext << "'." << std::endl; exit(1);
       }

%%

// No rules needed in flex.
