#ifndef pipecommand_hh
#define pipecommand_hh

#include "Command.hh"
#include "SimpleCommand.hh"

// Command Data Structure

class PipeCommand : public Command {
public:
  std::vector<SimpleCommand *> _simpleCommands;
  std::string * _outFile;
  std::string * _inFile;
  std::string * _errFile;
  bool _background;
  bool _append;
  bool _subshell;
  int _ioCount;

  PipeCommand();
  void insertSimpleCommand( SimpleCommand * simpleCommand );

  void clear();
  void print();
  void execute();

  // Expands environment vars and wildcards of a SimpleCommand and
  // returns the arguments to pass to execvp.
  static char ** expandEnvVarsAndWildcards(SimpleCommand * simpleCommandNumber);
  static void expandWildcard(std::string prefix, std::string suffix, std::vector<std::string *> *args);
  static std::string* expandEnvVars(char * arg);
  static std::vector<std::string*> expandSubshell(std::vector<std::string*> tempArgs, char * arg);
};

#endif
