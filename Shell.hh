#ifndef shell_hh
#define shell_hh

#include "ListCommands.hh"
#include "PipeCommand.hh"
#include "IfCommand.hh"
#include "WhileCommand.hh"
#include "ForCommand.hh"

#include <stack>

class Shell {

public:
  int _level; // Only outer level executes.
  bool _enablePrompt;
  ListCommands * _listCommands; 
  SimpleCommand *_simpleCommand;
  PipeCommand * _pipeCommand;
  IfCommand * _ifCommand;
  WhileCommand * _whileCommand;
  ForCommand * _forCommand;
  Command * _currentCommand;
  static Shell * TheShell;

  std::stack<WhileCommand *> wStack1;
  std::stack<WhileCommand *> wStack2;

  std::stack<ForCommand *> fStack1;
  std::stack<ForCommand *> fStack2;
  
  Shell();
  void execute();
  void print();
  void clear();
  void prompt();

};

#endif
