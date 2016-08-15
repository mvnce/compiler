%{
#include <iostream>
#include <fstream>
int line_count = 0;
%}
%option c++ noyywrap

type		val|char|string
print		print
random		random
id 			[a-zA-Z_][_a-zA-Z0-9]*
val			[0-9]+(\.[0-9]+)|(\.[0-9]+)|[0-9]+
char 		\'.\'
string		\"[^\"]*[^\n]\"
badstring 	\"[^\"\n]*
ascii		\+|\-|\*|\/|\(|\)|\=|\,|\{|\}|\[|\]|\.|\;
add			\+\=
sub			\-\=
mult		\*\=
div			\/\=
equ			\=\=
nequ		\!\=
less		\<
lte			\<\=
gtr			\>
gte 		\>\=
and 		\&\&	
or			\|\|
space		[\t \r]*
comment		\#(.*)

eol			\n
unknown 	.
%%

{type}		{ std::cout << "TYPE: " << yytext << std::endl; }
{print}		{ std::cout << "COMMAND_PRINT: " << yytext << std::endl; }
{random}	{ std::cout << "COMMAND_RANDOM: " << yytext << std::endl; }
{id}		{ std::cout << "ID: " << yytext << std::endl; }
{val}		{ std::cout << "VAL_LITERAL: " << yytext << std::endl; }
{char}		{ std::cout << "CHAR_LITERAL: " << yytext << std::endl; }
{string}	{ std::cout << "STRING_LITERAL: " << yytext << std::endl; }
{badstring}	{ std::cout << "ERROR(line " << line_count+1 << "): Unterminated string." << std::endl; exit(1);}
{ascii}		{ std::cout << "ASCII_CHAR: " << yytext << std::endl; }

{add}		{ std::cout << "ASSIGN_ADD: " << yytext << std::endl; }
{sub}		{ std::cout << "ASSIGN_SUB: " << yytext << std::endl; }
{mult}		{ std::cout << "ASSIGN_MULT: " << yytext << std::endl; }
{div}		{ std::cout << "ASSIGN_DIV: " << yytext << std::endl; }

{equ}		{ std::cout << "COMP_EQU: " << yytext << std::endl; }
{nequ}		{ std::cout << "COMP_NEQU: " << yytext << std::endl; }
{less}		{ std::cout << "COMP_LESS: " << yytext << std::endl; }
{lte}		{ std::cout << "COMP_LTE: " << yytext << std::endl; }
{gtr}		{ std::cout << "COMP_GTR: " << yytext << std::endl; }
{gte}		{ std::cout << "COMP_GTE: " << yytext << std::endl; }

{and}		{ std::cout << "BOOL_AND: " << yytext << std::endl; }
{or}		{ std::cout << "BOOL_OR: " << yytext << std::endl; }

{space}		{}
{comment}	{}
{unknown}	{ std::cout << "Unknown token on line " << line_count+1 << ": " << yytext << std::endl; exit(1); }
{eol}		{ line_count++; }
%%

int main (int argc, char* argv[]) {

	if (argc != 2) {
		std::cerr << "Format: " << argv[0] << " [source filename]" << std::endl;
    	exit(1);
    }

    std::ifstream sfile(argv[1]);

    if (sfile.fail()) {
	    std::cerr << "Error: Unable to open '" << argv[1] << "'.  Halting." << std::endl;
		exit(2); 
	}

	FlexLexer* lexer = new yyFlexLexer(&sfile);
	while (lexer->yylex());
	std::cout << "Line Count: " << line_count << std::endl;
	return 0; 
}