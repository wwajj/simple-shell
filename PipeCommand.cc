/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 * DO NOT PUT THIS PROJECT IN A PUBLIC REPOSITORY LIKE GIT. IF YOU WANT 
 * TO MAKE IT PUBLICALLY AVAILABLE YOU NEED TO REMOVE ANY SKELETON CODE 
 * AND REWRITE YOUR PROJECT SO IT IMPLEMENTS FUNCTIONALITY DIFFERENT THAN
 * WHAT IS SPECIFIED IN THE HANDOUT. WE OFTEN REUSE PART OF THE PROJECTS FROM  
 * SEMESTER TO SEMESTER AND PUTTING YOUR CODE IN A PUBLIC REPOSITORY
 * MAY FACILITATE ACADEMIC DISHONESTY.
 */

#include <dirent.h>
#include <fcntl.h>
#include <regex.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "PipeCommand.hh"
#include "Shell.hh"

extern int yylex_destroy();

PipeCommand::PipeCommand() {
    // Initialize a new vector of Simple PipeCommands
    _simpleCommands = std::vector<SimpleCommand *>();

    _outFile = NULL;
    _inFile = NULL;
    _errFile = NULL;
    _background = false;
    _append = false;
    _subshell = false;
    _ioCount = 0;
}

void PipeCommand::insertSimpleCommand( SimpleCommand * simplePipeCommand ) {
    // add the simple command to the vector
    _simpleCommands.push_back(simplePipeCommand);
}

void PipeCommand::clear() {
    // deallocate all the simple commands in the command vector
    for (auto simplePipeCommand : _simpleCommands) {
        delete simplePipeCommand;
    }

    // remove all references to the simple commands we've deallocated
    // (basically just sets the size to 0)
    _simpleCommands.clear();

    if ( _outFile && _errFile ) {
        delete _outFile;
    } else  if ( _outFile ) {
        delete _outFile;
    } else if ( _errFile ){
        delete _errFile;
    }
    _outFile = NULL;

    if ( _inFile ) {
        delete _inFile;
    }
    _inFile = NULL;

    _errFile = NULL;

    _background = false;

    _append = false;

    _ioCount = 0;
}

void PipeCommand::print() {
    printf("\n\n");
    //printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple PipeCommands\n");
    printf("  --- ----------------------------------------------------------\n");

    int i = 0;
    // iterate over the simple commands and print them nicely
    for ( auto & simplePipeCommand : _simpleCommands ) {
        printf("  %-3d ", i++ );
        simplePipeCommand->print();
    }

    printf( "\n\n" );
    printf( "  Output       Input        Error        Background\n" );
    printf( "  ------------ ------------ ------------ ------------\n" );
    printf( "  %-12s %-12s %-12s %-12s\n",
            _outFile?_outFile->c_str():"default",
            _inFile?_inFile->c_str():"default",
            _errFile?_errFile->c_str():"default",
            _background?"YES":"NO");
    printf( "\n\n" );
}

void PipeCommand::execute() {
    // Don't do anything if there are no simple commands
    if ( _simpleCommands.size() == 0 ) {
        Shell::TheShell->prompt();
        return;
    }

    if (_ioCount > 1) {
      printf("Ambiguous output redirect.\n");
      return;
    }

    // Print contents of PipeCommand data structure
    // print();

    // Add execution here
    // For every simple command fork a new process:
    // Setup i/o redirection
    // and call exec

    const char* cmd; 
    const char ** args;
    int tmpin = dup(0);
    int tmpout = dup(1);
    int tmperr = dup(2);
    int fdin;
    int fdout;
    int fderr;

    if (_inFile) {
      fdin = open(_inFile->c_str(), O_RDONLY);
    } else {
      fdin = dup(tmpin);
    }

    int ret, status, num_args;
    char buffer[64];
    for (unsigned long i = 0; i < _simpleCommands.size(); i++) {
      num_args = _simpleCommands[i]->_arguments.size();
      setenv("_new", _simpleCommands[i]->_arguments[num_args - 1]->c_str(), 1);
      
      dup2(fdin, 0);
      close(fdin);

      if (i == (_simpleCommands.size() - 1)) {
        if (_outFile) {
          if (_append) {
            fdout = open(_outFile->c_str(), O_CREAT | O_WRONLY | O_APPEND,  0666);
          } else {
            fdout = open(_outFile->c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666);
          }
        } else {
          fdout = dup(tmpout);
        }
        if (_errFile) {
          if (_append) {
            fderr = open(_errFile->c_str(), O_CREAT | O_WRONLY | O_APPEND, 0666);
          } else {
            fderr = open(_errFile->c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666);
          }
        } else {
          fderr = dup(tmperr);
        }
        dup2(fderr, 2);
        close(fderr);
      } else {
        int fdpipe[2];
        pipe(fdpipe);
        fdout = fdpipe[1];
        fdin = fdpipe[0];
      } 
      dup2(fdout, 1);
      close(fdout); 

      SimpleCommand *s = _simpleCommands[i];
      args = (const char **) expandEnvVarsAndWildcards(s);
      int size = 0;
      while (args[size] != NULL) {
        size++;
      }
      cmd = args[0];
      if (strcmp(cmd, "exit") == 0) {
        printf("Good bye!!\n");
        
        free(args);
        dup2(tmpin, 0);
        dup2(tmpout, 1);
        dup2(tmperr, 2);
        close(tmpin);
        close(tmpout);
        close(tmperr);

        yylex_destroy();
        exit(0);
      }
      if (strcmp(cmd, "setenv") == 0) {
        const char *A = args[1]; 
        const char *B = args[2];
        setenv(A, B, 1);
        free(args);
        return;
      }
      if (strcmp(cmd, "unsetenv") == 0) {
        const char *A = args[1];
        unsetenv(A);
        free(args);
        return;
      }
      if (strcmp(cmd, "cd") == 0) {
        if (args[1] == NULL) {
            chdir(getenv("HOME"));
        } else {
            if (chdir(args[1]) != 0) {
              fprintf(stderr, "cd: can't cd to %s\n", args[1]);
            }
        }
        free(args);
        dup2(tmpin, 0);
        dup2(tmpout, 1);
        dup2(tmperr, 2);
        close(tmpin);
        close(tmpout);
        close(tmperr);
        return;
      }

      ret = fork();
      if (ret == 0) {
        if (strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "printenv") == 0) {
          char **p = environ;
          while (*p != NULL) {
            fprintf(stdout, "%s\n", *p);
            p++;
          }
          free(args);
          yylex_destroy();
          exit(0);
        } else {
          execvp(args[0], (char* const*) args);
          perror("execvp");
          free(args);
          yylex_destroy();
          exit(1);
        }
      }
    }

    free(args);
    dup2(tmpin, 0);
    dup2(tmpout, 1);
    dup2(tmperr, 2);
    close(tmpin);
    close(tmpout);
    close(tmperr);

    char* errMessage;
    if (!_background) {
      waitpid(ret, &status, 0);
      if (WIFEXITED(status)) {
        snprintf(buffer, sizeof(buffer), "%d", WEXITSTATUS(status));
        setenv("question", buffer, 1);
        if (WEXITSTATUS(status) != 0) {
          errMessage = getenv("ON_ERROR");
          if (errMessage) {
            printf("%s\n", errMessage);
          }
        }
      }
    } else {
      sprintf(buffer, "%d", ret);
      setenv("bang", buffer, 1);
    }

    // Clear to prepare for next command
    // clear();
    // Print new prompt
    //Shell::TheShell->prompt();
}

bool mycomp(std::string *a, std::string *b) {
  return *a < *b;
}

// Expands environment vars and wildcards of a SimpleCommand and
// returns the arguments to pass to execvp.
char ** PipeCommand::expandEnvVarsAndWildcards(SimpleCommand *simpleCommandNumber) {
  std::vector<std::string *> tempArgs;
  for (unsigned long i = 0; i < simpleCommandNumber->_arguments.size(); i++) {
    tempArgs.push_back(new std::string(simpleCommandNumber->_arguments[i]->c_str()));
  }

  char* arg;
  std::vector<std::string *> newArgs;
  for (unsigned long i = 0; i < tempArgs.size(); i++) {
    arg = (char *) tempArgs[i]->c_str();

    if (((arg[0] == '$') && (arg[1] == '(')) || (arg[0] == '`')) {
      tempArgs = expandSubshell(tempArgs, arg);
    } 
    else if (strstr(arg, "${")) {
      tempArgs[i] = expandEnvVars(arg);
    }
    else if (strstr(arg, "$") || strstr(arg, "*")) {
      std::vector<std::string *> args;
      std::string prefix = "";
      expandWildcard(prefix, *(tempArgs[i]), &args);
      std::sort(args.begin(), args.end(), mycomp);
      
      newArgs = tempArgs;
      int temp = i;
      for (int j = 0; j < args.size(); j++) {
        newArgs.insert(newArgs.begin() + temp, args[j]);
        temp++;
      }
      newArgs.erase(newArgs.begin() + (newArgs.size() - 1));
      tempArgs = newArgs;
    }
  }

  const char ** args = (const char **) malloc((tempArgs.size() + 1) * sizeof(char *));
  for (unsigned long i = 0; i < tempArgs.size(); i++) {
    args[i] = tempArgs[i]->c_str();
  }
  args[tempArgs.size()] = NULL;

  return (char **) args;
}

void PipeCommand::expandWildcard(std::string prefix, std::string suffix, std::vector<std::string *> *args) {
  std::string *argPrefix;
  if (suffix.empty()) {
    argPrefix = new std::string(prefix);
    args->push_back(argPrefix);
    return;
  }

  int s = suffix.find('/', 1);
  std::string nextComponent;
  if (s != std::string::npos) {
    nextComponent = suffix.substr(0, s);
    suffix = suffix.substr(s, suffix.length());
  } else {
    nextComponent = suffix;
    suffix = "";
  }

  std::string newPrefix;
  if ((nextComponent.find('*') == std::string::npos) && ((nextComponent.find('?') == std::string::npos))) {
    newPrefix = prefix + nextComponent;
    expandWildcard(newPrefix, suffix, args);
    return;
  }

  std::string *reg = new std::string();
  *reg = "^";
  for (int i = 0; i < nextComponent.length(); i++) {
    if ((i == 0) && (nextComponent[0] == '/')) {
      continue;
    }
    if (nextComponent.c_str()[i] == '*') {
      *reg += '.';
      *reg += '*';
    } else if (nextComponent.c_str()[i] == '?') {
      *reg += '.';
    } else if (nextComponent.c_str()[i] == '.') {
      *reg += '\\';
      *reg += '.';
    } else {
      *reg += nextComponent.c_str()[i];
    }
  }
  *reg += '$';
  
  regex_t regex;
  int ret = regcomp(&regex, reg->c_str(), 0);
  
  std::string d;
  DIR *dir;
  if ((prefix.empty()) && (nextComponent[0] == '/')) {
    d = "/";
  } else if (prefix.empty()) {
    d = ".";
  } else {
    d = prefix;
  }
  
  dir = opendir(d.c_str());
  if (dir == NULL) {
    return;
  }

  std::string *ent;
  struct dirent *entry;
  regmatch_t pmatch;
  while ((entry = readdir(dir)) != NULL) {
    if (regexec(&regex, entry->d_name, 1, &pmatch, 0) == 0) {
      ent = new std::string(entry->d_name);
      if (ent->find(".") != 0) {
        if ((prefix.length() != 0) || (nextComponent[0] == '/')) {
          newPrefix = prefix + "/" + entry->d_name; 
        } else {
          newPrefix = prefix + entry->d_name;
        }
        expandWildcard(newPrefix, suffix, args);
      } else {
        if (reg->find(".") == 2) {
          if ((prefix.length() != 0) || (nextComponent[0] == '/')) {
            newPrefix = prefix + "/" + entry->d_name; 
          } else {
            newPrefix = prefix + entry->d_name;
          }
          expandWildcard(newPrefix, suffix, args);
        }
      }
      delete(ent);
    }
  }
  regfree(&regex);
  closedir(dir);
}

std::string* PipeCommand::expandEnvVars(char* arg) {
  std::string str(arg);
  int start, end;
  int num_vars = 0;
  for (unsigned long i = 0; i < str.size(); i++) {
    if (str[i] == '$') {
      num_vars++;
    }
  }

  unsigned long i = 0;
  for (int j = 0; j < num_vars; j++) {
    for (; i < str.size(); i++) {
      if (str[i] == '$') {
        start = i;
      } else if (str[i] == '}') {
        end = i;
        break;
      }
    }

    std::string var = str.substr(start + 2, end - (start + 2));
    if (getenv(var.c_str())) {
      var = getenv(var.c_str());
    } 

    std::string prev = str.substr(0, start);
    std::string after = str.substr(end + 1);
    str = prev + var + after;
    i = start;
  }

  std::string* strPtr = new std::string(str);
  return strPtr;
}

std::vector<std::string*> PipeCommand::expandSubshell(std::vector<std::string *>tempArgs, char * arg) { 
  auto it = std::find(tempArgs.begin(), tempArgs.end(), new std::string(arg));
  int subshellIndex = std::distance(tempArgs.begin(), it);

  char *subshell_arg = arg;
  char new_arg[64];
  if (subshell_arg[0] == '$') {
    strncpy(new_arg, subshell_arg + 2, strlen(subshell_arg) - 3);
    new_arg[strlen(subshell_arg) - 3] = '\0';
  } else {
    strncpy(new_arg, subshell_arg + 1, strlen(subshell_arg) - 2);
    new_arg[strlen(subshell_arg) - 2] = '\0';
  }

	int tempin = dup(0);
	int tempout = dup(1);

	int fdpipein[2];
  pipe(fdpipein);

	int fdpipeout[2];
	pipe(fdpipeout);

	write(fdpipein[1], new_arg, strlen(new_arg));
	write(fdpipein[1], "\n", 1);
	close(fdpipein[1]);

	dup2(fdpipein[0], 0);
	close(fdpipein[0]);

  dup2(fdpipeout[1], 1);	
	close(fdpipeout[1]);

	int ret = fork();
  if (ret == 0) {
    const char ** args = {NULL};
    execvp("/proc/self/exe", (char* const*) args);
		perror("execvp");
    exit(1);
	}

  dup2(tempin, 0);
	dup2(tempout, 1);
	close(tempin);
	close(tempout);

	char c;
	char *buffer = (char *) malloc(1024);
	int i = 0;

	while (read(fdpipeout[0], &c, 1)) {
		if (c == '\n') {
			buffer[i] = ' ';
		}
		else {
			buffer[i] = c;
		}
	  i++;
  }
	buffer[i] = '\0';

  std::vector<std::string *> newArgs;
  char* token;
  token = strtok(const_cast<char *>(buffer), " ");
  while (token != NULL) {
    std::string* newArg = new std::string(token);
    newArgs.push_back(newArg);
    token = strtok(nullptr, " ");
  }

  free(buffer);
  buffer = NULL;

  std::vector<std::string *> finalArgs;
  for (unsigned long i = 0; i < tempArgs.size(); i++) {
    if (i == (subshellIndex - 1)) {
      for (unsigned long j = 0; j < newArgs.size(); j++) {
        finalArgs.push_back(newArgs[j]);
      }
      continue;
    }
    finalArgs.push_back(tempArgs[i]);
  }

  return finalArgs;
}
