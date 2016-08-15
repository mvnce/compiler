%{
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cstdlib>
#include <string>
#include <vector>
extern int yylex();
extern int line_count;
extern void yyerror(char*);
void yyerror(const char *p) {
	line_count++;
	std::cout << "ERROR(line "<< line_count <<"): "<< p << std::endl; exit(1);
}

void perror(std::string s) {
    line_count++;
    std::cout << "ERROR(line "<< line_count <<"): "<< s << std::endl; exit(1);
}

std::vector<std::string> data;

int find(std::string s) {
    for (int i = 0; i < data.size(); i++ ) {
        if (s == data[i]) return 1;
    }
    return 0;
}

%}

%union {
  char * lexeme;
  int val;
}
%token<lexeme>  ID TYPE	VAL_LITERAL  // Default type is int
%token 		COMMAND_PRINT
%token 		COMMAND_RANDOM
%left 		COMP BOOL
%left 		'+' '-'
%left 		'*' '/'
%left		ASSIGN
%right '='
%nonassoc 	UMINUS

%type <val> EXPR
%%

START: START DECLARATION { } | {}

DECLARATION: EXPR ';' {  }
| TYPE ID '=' EXPR ';' { if(find($2) == 1) {std::cout << "ERROR(line "<< ++line_count <<"): " << "redeclaration of variable \'" << std::string($2) << "\'" << std::endl; exit(1);} else {data.push_back( strdup($2) );} }
| TYPE ID ';' { if(find($2) == 1) {std::cout << "ERROR(line "<< ++line_count <<"): " << "redeclaration of variable \'" << std::string($2) << "\'" << std::endl; exit(1);}  else {data.push_back( strdup($2) );} }
| ID TYPE { if( find($1) == 0 ) {std::cout << "ERROR(line "<< ++line_count <<"): " << "unknown variable \'" << std::string($1) << "\'" << std::endl; exit(1);} } 
| print ';'{  }

EXPR: EXPR '+' EXPR { $$ = $1 + $3; }
| EXPR '-' EXPR { $$ = $1 - $3; }
| EXPR '*' EXPR { $$ = $1 * $3; }
| EXPR '/' EXPR {
  if ($3 == 0) yyerror("div by zero");
  else $$ = $1 / $3; }
| '-' EXPR %prec UMINUS { $$ = -$2; }
| '(' EXPR ')' { $$ = $2; }
| VAL_LITERAL {  }
| ID '=' EXPR { if( find($1) == 0 ) {std::cout << "ERROR(line "<< ++line_count <<"): " << "unknown variable \'" << std::string($1) << "\'" << std::endl; exit(1); } } 
| COMMAND_RANDOM '(' EXPR ')' {  }
| ID { if( find($1) == 0 ) {std::cout << "ERROR(line "<< ++line_count <<"): " << "unknown variable \'" << std::string($1) << "\'" << std::endl; exit(1); } } 
| EXPR COMP EXPR
| EXPR BOOL EXPR
| EXPR ASSIGN EXPR

print : COMMAND_PRINT '(' print ')' {}
	|	EXPR
	|	EXPR ',' EXPR {  }
	|	print ','print {  };

%%
