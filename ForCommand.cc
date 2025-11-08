#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "Command.hh"
#include "SimpleCommand.hh"
#include "ForCommand.hh"
#include "PipeCommand.hh"
#include "Shell.hh"

ForCommand::ForCommand() {
  _condition = NULL;
  _listCommands = NULL;
  _loopCounter = NULL;
}

// Run condition with command "test" and return the exit value.
std::vector<std::string* >
ForCommand::runTest(SimpleCommand * condition) {
  std::vector<std::string *>newArgs;
  const char** args = (const char**) PipeCommand::expandEnvVarsAndWildcards(condition);
  int size = 0;
  while (args[size] != NULL) {
    ++size;
  }

  for (int i = 0; i < size; i++) {
    newArgs.push_back(new std::string(args[i]));
  }
  return newArgs;
}

void 
ForCommand::insertCondition( SimpleCommand * condition ) {
  _condition = condition;
}

void 
ForCommand::insertListCommands( ListCommands * listCommands) {
  _listCommands = listCommands;
}

void 
ForCommand::clear() {
}

void
ForCommand::insertLoopCounter( std::string * loopCounter ) {
  _loopCounter = loopCounter;
}

void 
ForCommand::print() {
  printf("FOR [ \n"); 
  this->_condition->print();
  printf("   ]; do\n");
  this->_listCommands->print();
}
  
void 
ForCommand::execute() {
  ForCommand *f1;
  ForCommand *f2;

  if (Shell::TheShell->fStack2.empty()) {
    f1 = this;
  } else {
    f1 = Shell::TheShell->fStack2.top();
    Shell::TheShell->fStack2.pop();
    f2 = f1;
  }

  ForCommand *temp;
  while (!Shell::TheShell->fStack2.empty()) {
    temp = Shell::TheShell->fStack2.top();
    Shell::TheShell->fStack2.pop();
    f2->_listCommands->_commands.push_back(temp);
    f2 = temp;
  }

  std::vector<std::string* > expandedConditions = runTest(f1->_condition);
  const char* A = f1->_loopCounter->c_str();
  const char* B;
  for (unsigned int i = 0; i < expandedConditions.size(); i++) {
    B = expandedConditions[i]->c_str();
    setenv(A, B, 1);

    f1->_listCommands->execute();
  }

  unsetenv(A);
  //while (runTest(f1->_condition) == 0) {
    //f1->_listCommands->execute();
  //}
}
