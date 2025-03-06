%{
#include "y.tab.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern YYSTYPE yylval;

extern char *getParseError();
extern void clearParseError();

%}

%%

"led"       { return LED; }
"on"        { return ON; }
"off"       { return OFF; }
"help"      { return HELP; }
"select"    { return SELECT; }
"set"       { return SET; }
"move"      { return MOVE; }
"servo"     { return SERVO; }
"reset"     { return RESET; }
"sweep"     { return SWEEP; }
"lu"        { return LU; }
"ld"        { return LD; }
"eh"        { return EH; }
"ev"        { return EV; }
"center"    { return CENTER; }
\%[A-F0-9]{8}   { 
                    yylval.string = strdup(yytext+1);
                    return HEXNUMBER; 
                }
[-+]?[0-9]+ {
               yylval.value = atoi(yytext);  // Convert string to integer 
               return NUMBER;
            }
[ \t]       ; /* Skip whitespace */
\n          { return NEWLINE; }
.           { /* ignore */ }

%%

/* Prevents Flex from expecting file input */
int yywrap() {
    return 1;
}

/* Function to set the input string */
char *set_input_string(const char *str) {
    clearParseError();
    YY_BUFFER_STATE buffer = yy_scan_string(str);
    yyparse();  // Call Bison parser
    yy_delete_buffer(buffer);
    char *parseError = getParseError();
    return parseError;
}
