#ifndef forcommand_hh
#define forcommand_hh

#include "Command.hh"
#include "SimpleCommand.hh"
#include "ListCommands.hh"

// Command Data Structure

class ForCommand : public Command {
public:
  SimpleCommand * _condition;
  ListCommands * _listCommands; 
  std::string *_loopCounter;

  ForCommand();
  void insertCondition( SimpleCommand * condition );
  void insertListCommands( ListCommands * listCommands);
  void insertLoopCounter(std::string * loopCounter);
  std::vector<std::string *> runTest(SimpleCommand * condition);

  void clear();
  void print();
  void execute();

};

#endif
