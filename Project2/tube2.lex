%{
#include <iostream>
#include <fstream>
#include <string.h>
#include "tube2.tab.hh"
using namespace std;

void yyerror(char*);
int yyparse(void);
int line_count = 0;

%}

type		val|char|string
val			[0-9]+(\.[0-9]+)|(\.[0-9]+)|[0-9]+
ascii		\+|\-|\*|\/|\(|\)|\=|\,|\{|\}|\[|\]|\.|\;
space		[\t \r]*
comment		\#(.*)
eol			\n
unknown 	.

%%
val|char|string  { return TYPE; }
print	{ return COMMAND_PRINT; }
random	{ return COMMAND_RANDOM; }

[a-zA-Z\_][0-9a-zA-Z\_]* { yylval.lexeme = strdup(yytext); return ID; }

[0-9]*(\.[0-9]+)?	{ yylval.lexeme = strdup(yytext); return VAL_LITERAL; }
['].[']	{ cout << "CHAR_LITERAL: " << yytext << endl; }
["][^"\n]*["]	{ cout << "STRING_LITERAL: " << yytext << endl; }
[+\-\*\/\(\)\=\,\{\}\[\]\.\;]	{ return yytext[0]; }

"+="	{ return ASSIGN; }
"-="	{ return ASSIGN; }
"*="	{ return ASSIGN; }
"/="	{ return ASSIGN; }

"=="	{ return COMP; }
"!="	{ return COMP; }
"<"	{ return COMP; }
"<="	{ return COMP; }
">"	{ return COMP; }
">="	{ return COMP; }

"&&"	{ return BOOL; }
"||"	{ return BOOL; }

[/][*](.|[\n\t \r])*[*][/]		{
				string str(yytext);
				for(string::size_type i = 0; i < str.size(); ++i) {
					if (str[i] == '\n'){
						line_count++;
					}
				}	
			}

[\n]	{ line_count++; }
[\t \r]         /* Ignore Whitespace */;

#.*	;

. 	{
		string text(yytext);
		if (text == "\"") {
			line_count++; cout << "ERROR(line " << line_count << "): Unterminated string." << endl; exit(1);
		}
		else
			line_count++; cout << "Unknown token on line " << line_count << ": " << yytext << endl; exit(1); 
	}
%%

void yyerror(char* str) { cout << "parse error" << endl; }
int yywrap(void) { }

int main(int num_args, char* argv[]) {

	if (num_args != 2) {
		cerr << "Format: " << argv[0] << " [source filename]" << endl;
    	exit(1);
    }

	FILE* file = fopen(argv[1],"r");

	if(file == NULL) {
		cerr << "Error: Unable to open '" << argv[1] << "'.  Halting." << endl;
		exit(2); 
	}
	yyin = file;
	yyparse();
	cout << "Parse Successful!" << endl;
	fclose(file);
	return 0;
}