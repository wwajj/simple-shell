#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <vector>

#include "Command.hh"
#include "Shell.hh"

int yyparse(void);

Shell * Shell::TheShell;

Shell::Shell() {
    this->_level = 0;
    this->_enablePrompt = true;
    this->_listCommands = new ListCommands(); 
    this->_simpleCommand = new SimpleCommand();
    this->_pipeCommand = new PipeCommand();
    this->_currentCommand = this->_pipeCommand;
    this->_whileCommand = new WhileCommand();
    this->_forCommand = new ForCommand();
    if ( !isatty(0)) {
	    this->_enablePrompt = false;
    }
}

void Shell::prompt() {
  char* customPrompt;
  if (_enablePrompt) {
    customPrompt = getenv("PROMPT");
    if (customPrompt) {
       printf("%s", customPrompt);
    } else {
	    printf("myshell>");
	  }
    fflush(stdout);
  }
}

void Shell::print() {
  printf("\n--------------- Command Table ---------------\n");
  this->_listCommands->print();
}

void Shell::clear() {
  this->_listCommands->clear();
  this->_simpleCommand->clear();
  this->_pipeCommand->clear();
  this->_currentCommand->clear();
  this->_level = 0;
}

void Shell::execute() {
  if (this->_level == 0 ) {
    //this->print();
    this->_listCommands->execute();
    this->_listCommands->clear();
    this->prompt();
  }
}

void yyset_in (FILE *  in_str );

extern "C" void ctrlC(int sig) {
  printf("\n");
  Shell::TheShell->prompt();
}

extern "C" void zombie(int sig) {
  pid_t pid;
  while ((pid = waitpid(-1, NULL, WNOHANG)) > 1) {}
}


int main(int argc, char **argv) {
  struct sigaction saCtrlC;
  saCtrlC.sa_handler = ctrlC;
  sigemptyset(&saCtrlC.sa_mask);
  saCtrlC.sa_flags = SA_RESTART;

  struct sigaction saZombie;
  saZombie.sa_handler = zombie;
  sigemptyset(&saZombie.sa_mask);
  saZombie.sa_flags = SA_RESTART; 

  if (sigaction(SIGINT, &saCtrlC, NULL)) {
    perror("sigaction");
    exit(2);
  }
  if (sigaction(SIGCHLD, &saZombie, NULL)) {
    perror("sigaction");
    exit(2);
  }

  char * input_file = NULL;
  if ( argc > 1 ) {
    input_file = argv[1];
    FILE * f = fopen(input_file, "r");
    if (f==NULL) {
	fprintf(stderr, "Cannot open file %s\n", input_file);
        perror("fopen");
        exit(1);
    }
    yyset_in(f);
  }  

  Shell::TheShell = new Shell();
 
  if (argc > 2) {
    if (strstr(argv[1], ".sh")) {
      setenv("#", std::to_string(argc - 2).c_str(), 1);
      setenv("0", argv[1], 1);
    
      std::string allArgs;
      for (int i = 2; i < argc; i++) {
        setenv(std::to_string(i - 1).c_str(), argv[i], 1);
        allArgs += (std::string(argv[i])) + " ";
      }
      allArgs.erase(allArgs.size() - 1);
      setenv("*", allArgs.c_str(), 1);
    }
  }
  if (input_file != NULL) {
    // No prompt if running a script
    Shell::TheShell->_enablePrompt = false;
  }
  else {
    Shell::TheShell->prompt();
  }
  yyparse();
  Shell::TheShell->clear();
}


