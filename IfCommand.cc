
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "Command.hh"
#include "SimpleCommand.hh"
#include "IfCommand.hh"

IfCommand::IfCommand() {
  _condition = NULL;
  _listCommands =  NULL;
}


// Run condition with command "test" and return the exit value.
int
IfCommand::runTest(SimpleCommand * condition) {
  int ret = fork();
  if (ret == 0) {
    int argc = condition->_arguments.size() + 2;
    char **argv = new char *[argc];
    argv[0] = strdup("test");

    for (unsigned int i = 0; i < condition->_arguments.size(); ++i) {
      argv[i + 1] = strdup(condition->_arguments[i]->c_str());
    }
    argv[argc - 1] = NULL;

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
  condition->print();
  return 1;
}

void 
IfCommand::insertCondition( SimpleCommand * condition ) {
  _condition = condition;
}

void 
IfCommand::insertListCommands( ListCommands * listCommands) {
  _listCommands = listCommands;
}

void 
IfCommand::clear() {
}

void 
IfCommand::print() {
  printf("IF [ \n"); 
  this->_condition->print();
  printf("   ]; then\n");
  this->_listCommands->print();
}
  
void 
IfCommand::execute() {
  // Run command if test is 0
  if (runTest(this->_condition) == 0) {
    _listCommands->execute();
  }
}

