%{
#include "symbol_table.h"
#include "type_info.h"
#include "ast.h"
#include "tube6-parser.tab.hh"

#include <iostream>
#include <stdio.h>
#include <string>

// Two global variables (not clean, but works...)
int line_num = 1;
std::string out_filename = "";
%}

%option nounput
%option noyywrap

type		val|char|string
meta_type       array
id	        _*[a-zA-Z][a-zA-Z0-9_]*
val_lit         [0-9]+|[0-9]*[.][0-9]+
char_lit        '(.|(\\[\\'nt]))'
string_lit      \"(\\[nt"\\]|[^\\"])*\"
string_err      \"(\\.|[^\\"])*\"
string_err2     \"(\\.|[^\\"])*
comment		#[^\n]*
eol		\n
whitespace	[ \t\r]
operator	[+\-*/=(),!{}[\].:?;]

%%

"break"    { return COMMAND_BREAK; }
"continue" { return COMMAND_CONTINUE; }
"else"     { return COMMAND_ELSE; }
"for"      { return COMMAND_FOR; }
"if"       { return COMMAND_IF; }
"print"    { return COMMAND_PRINT; }
"random"   { return COMMAND_RANDOM; }
"while"    { return COMMAND_WHILE; }

{type}        { yylval.lexeme = strdup(yytext);  return TYPE; }
{meta_type}   { yylval.lexeme = strdup(yytext);  return META_TYPE; }
{id}          { yylval.lexeme = strdup(yytext);  return ID; }
{val_lit}     { yylval.lexeme = strdup(yytext);  return VAL_LIT; }
{char_lit}    { yylval.lexeme = strdup(yytext);  return CHAR_LIT; }
{string_lit}  { yylval.lexeme = strdup(yytext);   return STRING_LIT; }

{string_err} {
           std::cout << "ERROR(line " << line_num << "): Unknown escape char in string." << std::endl;
           exit(1);
         }

{string_err2} {
           std::cout << "ERROR(line " << line_num << "): Unterminated string." << std::endl;
           exit(1);
         }

{operator}  { yylval.lexeme = strdup(yytext);  return (int) yytext[0]; }

"+=" { return CASSIGN_ADD; }
"-=" { return CASSIGN_SUB; }
"*=" { return CASSIGN_MULT; }
"/=" { return CASSIGN_DIV; }

"==" { return COMP_EQU; }
"!=" { return COMP_NEQU; }
"<" { return COMP_LESS; }
"<=" { return COMP_LTE; }
">" { return COMP_GTR; }
">=" { return COMP_GTE; }

"&&" { return BOOL_AND; }
"||" { return BOOL_OR; }

{comment} { ; }
{whitespace} { ; }
{eol}  { line_num++; }

.      { std::cout << "ERROR(line " << line_num << "): Unknown Token '" << yytext << "'." << std::endl; exit(1); }

%%

void LexMain(int argc, char * argv[])
{
  FILE * file = NULL;
  bool input_found = false;

  // Loop through all of the command-line arguments.
  for (int arg_id = 1; arg_id < argc; arg_id++) {
    std::string cur_arg(argv[arg_id]);

    if (cur_arg == "-h") {
      std::cout << "Tubular Compiler v. 0.6.0 (Project 6 release 1)"  << std::endl
           << "Format: " << argv[0] << "[flags] [filename]" << std::endl
           << std::endl
           << "Available Flags:" << std::endl
           << "  -h  :  Help (this information)" << std::endl
        ;
      exit(0);
    }

    // PROCESS OTHER ARGUMENTS HERE IF YOU ADD THEM

    // If the next argument begins with a dash, assume it's an unknown flag...
    if (cur_arg[0] == '-') {
      std::cerr << "ERROR: Unknown command-line flag: " << cur_arg << std::endl;
      exit(1);
    }

    // Assume the current argument is a filename (first input, then output)
    if (!input_found) {
      file = fopen(argv[arg_id], "r");
      if (!file) {
        std::cerr << "Error opening " << cur_arg << std::endl;
        exit(1);
      }
      yyin = file;
      input_found = true;
      continue;
    } else if (out_filename == "") {
      out_filename = cur_arg;
    }

    // Both files already loaded!
    else {
      std::cout << "ERROR: Unknown argument '" << cur_arg << "'" << std::endl;
      exit(1);
    }
  }

  // Make sure we've loaded input and output filenames before we finish...
  if (input_found == false || out_filename == "") {
    std::cerr << "Format: " << argv[0] << "[flags] [input filename] [output filename]" << std::endl;
    std::cerr << "Type '" << argv[0] << " -h' for help." << std::endl;
    exit(1);
  }

  return;
}
