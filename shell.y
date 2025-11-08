
/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%code requires 
{
#include <string>

#if __cplusplus > 199711L
#define register      // Deprecated in C++11 so remove the keyword
#endif
}

%union
{
  char        *string_val;
  // Example of using a c++ type in yacc
  std::string *cpp_string;
}

%type <cpp_string> pipe_list cmd_and_args 
%token <cpp_string> WORD 
%token NOTOKEN GREAT GREATGREAT LESS GREATAMPERSAND GREATGREATAMPERSAND TWOGREAT 
%token AMPERSAND PIPE NEWLINE IF FI THEN LBRACKET RBRACKET SEMI 
%token DO DONE WHILE FOR IN

%{
//#define yylex yylex
#include <cstdio>
#include "Shell.hh"

void yyerror(const char * s);
int yylex();

%}

%%

goal: command_list;

arg_list:
	arg_list WORD {
    Shell::TheShell->_simpleCommand->insertArgument( $2 );
	}
  | /*empty string*/
  ;

cmd_and_args:
	WORD {
		Shell::TheShell->_simpleCommand = new SimpleCommand(); 
		Shell::TheShell->_simpleCommand->insertArgument( $1 );
	} arg_list
	;

pipe_list:
	cmd_and_args {
		Shell::TheShell->_pipeCommand->insertSimpleCommand(Shell::TheShell->_simpleCommand); 
		Shell::TheShell->_simpleCommand = new SimpleCommand();
	}
	| pipe_list PIPE cmd_and_args {
    Shell::TheShell->_pipeCommand->insertSimpleCommand(Shell::TheShell->_simpleCommand); 
		Shell::TheShell->_simpleCommand = new SimpleCommand();
  }
	;

io_modifier:
	GREATGREAT WORD {
		Shell::TheShell->_pipeCommand->_outFile = $2;
    Shell::TheShell->_pipeCommand->_append = true;
    Shell::TheShell->_pipeCommand->_ioCount++;
  }
	| GREAT WORD {
		Shell::TheShell->_pipeCommand->_outFile = $2;
    Shell::TheShell->_pipeCommand->_ioCount++;
	}
	| GREATGREATAMPERSAND WORD {
		Shell::TheShell->_pipeCommand->_outFile = $2;
    Shell::TheShell->_pipeCommand->_errFile= Shell::TheShell->_pipeCommand->_outFile;
    Shell::TheShell->_pipeCommand->_append = true;
    Shell::TheShell->_pipeCommand->_ioCount++;
  } 
	| GREATAMPERSAND WORD {
		Shell::TheShell->_pipeCommand->_outFile = $2;
		Shell::TheShell->_pipeCommand->_errFile = Shell::TheShell->_pipeCommand->_outFile;
    Shell::TheShell->_pipeCommand->_ioCount++;
  }
	| TWOGREAT WORD {
    Shell::TheShell->_pipeCommand->_errFile = $2;
    Shell::TheShell->_pipeCommand->_ioCount++;
  }
  | LESS WORD {
		Shell::TheShell->_pipeCommand->_inFile = $2;
	}
	;

io_modifier_list:
	io_modifier_list io_modifier
	| /*empty*/
	;

background_optional: 
	AMPERSAND {
    Shell::TheShell->_pipeCommand->_background = true;
  }
	| /*empty*/
	;

SEPARATOR:
	NEWLINE
	| SEMI
	;

command_line:
  pipe_list io_modifier_list background_optional SEPARATOR { 
		Shell::TheShell->_listCommands->insertCommand(Shell::TheShell->_pipeCommand);
		Shell::TheShell->_pipeCommand = new PipeCommand(); 
  }
  | if_command SEPARATOR {
		Shell::TheShell->_listCommands->
		insertCommand(Shell::TheShell->_ifCommand);
  }
  | while_command SEPARATOR {
    Shell::TheShell->_listCommands->insertCommand(Shell::TheShell->_whileCommand);
	}
  | for_command SEPARATOR {
		Shell::TheShell->_listCommands->insertCommand(Shell::TheShell->_forCommand);
	}
  | SEPARATOR /*accept empty cmd line*/
  | error SEPARATOR {
		yyerrok; Shell::TheShell->clear();
	}
	; /*error recovery*/

command_list :
	command_line { 
		Shell::TheShell->execute();
	}
	| command_list command_line {
		Shell::TheShell->execute();
	}
     ;  /* command loop*/

if_command:
	IF LBRACKET { 
		Shell::TheShell->_level++; 
		Shell::TheShell->_ifCommand = new IfCommand();
	} arg_list RBRACKET SEMI THEN {
		Shell::TheShell->_ifCommand->insertCondition(Shell::TheShell->_simpleCommand);
		Shell::TheShell->_simpleCommand = new SimpleCommand();
	}
	command_list FI { 
		Shell::TheShell->_level--; 
		Shell::TheShell->_ifCommand->insertListCommands(Shell::TheShell->_listCommands);
		Shell::TheShell->_listCommands = new ListCommands();
	}
    ;

while_command:
    WHILE LBRACKET {
      Shell::TheShell->_whileCommand = new WhileCommand();
    } arg_list RBRACKET SEMI DO {
      Shell::TheShell->_whileCommand->insertCondition(Shell::TheShell->_simpleCommand);

      if (Shell::TheShell->_level == 0) {
        Shell::TheShell->wStack1.push(Shell::TheShell->_whileCommand);
      } else {
        WhileCommand *outerLoop = Shell::TheShell->wStack1.top();
        Shell::TheShell->wStack1.pop();
        outerLoop->insertListCommands(Shell::TheShell->_listCommands);
        Shell::TheShell->wStack1.push(outerLoop);
        Shell::TheShell->wStack1.push(Shell::TheShell->_whileCommand);
      }

      Shell::TheShell->_level++;
      Shell::TheShell->_simpleCommand = new SimpleCommand();
      Shell::TheShell->_listCommands = new ListCommands();
    } command_list DONE {
      Shell::TheShell->_level--;
      WhileCommand *outerLoop = Shell::TheShell->wStack1.top();
      Shell::TheShell->wStack1.pop();
      
      if (Shell::TheShell->wStack2.empty()) {
        outerLoop->_listCommands = Shell::TheShell->_listCommands;
        Shell::TheShell->_listCommands = new ListCommands();
      }
      
      Shell::TheShell->wStack2.push(outerLoop);
      Shell::TheShell->_listCommands = new ListCommands();
    }
    ;

for_command:
    FOR WORD IN {
      Shell::TheShell->_forCommand = new ForCommand();
      Shell::TheShell->_forCommand->insertLoopCounter($2);
    } arg_list SEMI DO {
      Shell::TheShell->_forCommand->insertCondition(Shell::TheShell->_simpleCommand);

      if (Shell::TheShell->_level == 0) {
        Shell::TheShell->fStack1.push(Shell::TheShell->_forCommand);
      } else {
        ForCommand *outerLoop = Shell::TheShell->fStack1.top();
        Shell::TheShell->fStack1.pop();
        outerLoop->insertListCommands(Shell::TheShell->_listCommands);
        Shell::TheShell->fStack1.push(outerLoop);
        Shell::TheShell->fStack1.push(Shell::TheShell->_forCommand);
      }

      Shell::TheShell->_level++;
      Shell::TheShell->_simpleCommand = new SimpleCommand();
      Shell::TheShell->_listCommands = new ListCommands();
    } command_list DONE {
      Shell::TheShell->_level--;
      ForCommand *outerLoop = Shell::TheShell->fStack1.top();
      Shell::TheShell->fStack1.pop();

      if (Shell::TheShell->fStack2.empty()) {
          outerLoop->_listCommands = Shell::TheShell->_listCommands;
          Shell::TheShell->_listCommands = new ListCommands();
      }

      Shell::TheShell->fStack2.push(outerLoop);
      Shell::TheShell->_listCommands = new ListCommands();
    }
   ;


%%

void
yyerror(const char * s)
{
  fprintf(stderr,"%s", s);
}

#if 0
main()
{
  yyparse();
}
#endif
