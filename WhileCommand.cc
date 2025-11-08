#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "Command.hh"
#include "SimpleCommand.hh"
#include "WhileCommand.hh"
#include "PipeCommand.hh"
#include "Shell.hh"

WhileCommand::WhileCommand() {
  _condition = NULL;
  _listCommands = NULL;
}

// Run condition with command "test" and return the exit value.
int
WhileCommand::runTest(SimpleCommand * condition) {
  char *argv[condition->_arguments.size() + 2];
  argv[0] = strdup("test");
  char **args= PipeCommand::expandEnvVarsAndWildcards(condition);
  for (unsigned int i = 1; i < condition->_arguments.size() + 2; i++) {
    argv[i] = args[i - 1];
  }
  argv[condition->_arguments.size() + 1] = NULL;

  int ret = fork();
  if (ret == 0) {
    execvp(argv[0], argv);
    perror("execvp");
    exit(1);
  } else {
    int status;
    waitpid(ret, &status, 0);

    if (WIFEXITED(status)) {
      status = WEXITSTATUS(status);
      return status;
    }
  }
  return 1;
}

void 
WhileCommand::insertCondition( SimpleCommand * condition ) {
  _condition = condition;
}

void 
WhileCommand::insertListCommands( ListCommands * listCommands) {
  _listCommands = listCommands;
}

void 
WhileCommand::clear() {
}

void 
WhileCommand::print() {
  printf("WHILE [ \n"); 
  this->_condition->print();
  printf("   ]; do\n");
  this->_listCommands->print();
}
  
void 
WhileCommand::execute() {
  WhileCommand *w1;
  WhileCommand *w2;
  
  if (Shell::TheShell->wStack2.empty()) {
    w1 = this;
  } else {
    w1 = Shell::TheShell->wStack2.top();
    Shell::TheShell->wStack2.pop();
    w2 = w1;
  }

  WhileCommand *temp;
  while (!Shell::TheShell->wStack2.empty()) {
    temp = Shell::TheShell->wStack2.top();
    Shell::TheShell->wStack2.pop();
    w2->_listCommands->_commands.push_back(temp);
    w2 = temp;
  }
  
  while (runTest(w1->_condition) == 0) {
    w1->_listCommands->execute();
  }
}
