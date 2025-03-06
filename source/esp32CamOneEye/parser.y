%debug
%{
    #include <stdio.h>
    #include "commander.h"
    #include <Arduino.h>
    
    void yyerror(const char *s);
    int yylex(void);
    extern void handleLedOn();
    extern void handleLedOff();
    extern void handleHelp();
    extern void handleSelect(int servoKind);
    extern void handleSet(int servoValue);
    extern void handleMove(int servoValue);
    extern void handleReset();
    extern void handleSweep();
    extern void handleCenter();
    extern void handlePose(char *string);
%}

%union {
    char *string;
    unsigned int value;
}

%token <string> HEXNUMBER
%token <value> NUMBER 
%token LED ON OFF HELP NEWLINE
%token SELECT SERVO 
%token LU LD EH EV
%token SET MOVE RESET SWEEP CENTER

%type <value> servo_kind

%%
program:
      command NEWLINE    { }
    | command           { }  /* Allow commands without newline */
    ;

command:
      led_command    
    | select_command 
    | set_command    
    | move_command   
    | help_command   
    | reset_command  
    | sweep_command  
    | center_command 
    | pose_command   
    ;

led_command:
    LED ON     { handleLedOn(); }
    | LED OFF  { handleLedOff(); }
    ;

select_command:
    SELECT servo_kind { handleSelect($2); }
    ;

set_command:
    SET NUMBER { 
        if ($2 >= 0 && $2 <= 180) /* Assuming servo range */
            handleSet($2); 
        else
            yyerror("Servo value out of range (0-180)");
    }
    ;

move_command:
    MOVE NUMBER { 
        if ($2 >= 0 && $2 <= 255)
            handleMove($2); 
        else
            yyerror("Move value out of range (0-255)");
    }
    ;

servo_kind:
      EH { $$ = 0; }
    | EV { $$ = 1; }
    | LU { $$ = 2; } 
    | LD { $$ = 3; }
    ;

help_command:
    HELP    { handleHelp(); }
    ;

reset_command:
    RESET   { handleReset(); }
    ;

sweep_command:
    SWEEP   { handleSweep(); }
    ;

center_command:
    CENTER  { handleCenter(); }
    ;

pose_command:
    HEXNUMBER    { 
        handlePose($1); 
        free($1);  /* Free the strdup'd string */
    }
    ;

%%

static const char *parseError = NULL;

void yyerror(const char *s) {
    parseError = s;
}

const char *getParseError() {
    return parseError;
}

void clearParseError() {
    parseError = NULL;  /* Use NULL instead of empty string */
}