
/*
 *
 * CS-252 
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <iostream>
#include <cstring>
#include <dirent.h>
#include <fnmatch.h>
#include <limits.h>
#include <pwd.h>
#include <unistd.h>

#include "Shell.hh"
#include "y.tab.hh"
#include "read-line.c"

extern char * read_line();

int mygetc(FILE * f) {
  static char *p;
  char ch;

     if (!isatty(0)) {
  // stdin is not a tty. Call real getc
    return getc(f);
  }

  // stdin is a tty. Call our read_line.
  if (p==NULL || *p == 0) {
    char * s = read_line();
    p = s;
  }

  ch = *p;
  p++;

  return ch;
}

#undef getc
#define getc(f) mygetc(f)

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
  unput(c);
}

%}

%option noyywrap

%%

"#".*\n {
  /* Discard comments starting with # */
}

\n {
  return NEWLINE;
}

[ \t] {
  /* Discard spaces and tabs */
}

">" {
  return GREAT;
}

"<" {
  return LESS;
}

"|" {
  return PIPE;
}

">>" {
  return GREATGREAT;
}

">&" {
  return GREATAMPERSAND;
}

"2>" {
  return TWOGREAT;
}

">>&" {
  return GREATGREATAMPERSAND;
}

"&" {
  return AMPERSAND;
}

"[" {
  return LBRACKET;
}

"]" {
  return RBRACKET;
}

"if" {
  return IF;
}

"then" {
  return THEN;
}

"fi" {
  return FI;
}

"while" {
  return WHILE;
}

"do" {
  return DO;
}

"done" {
  return DONE;
}

"for" {
  return FOR;
}

"in" {
  return IN;
}

";" {
  return SEMI;
}

"source "([^ \t\n]+) {
  Shell::TheShell->_enablePrompt = false;
  char *filename = (char *) malloc(yyleng - 6);
  if (!filename) {
    perror("malloc");
    exit(1);
  }
  strncpy(filename, yytext + 7, yyleng - 7);
  filename[yyleng - 7] = '\0';

  FILE *fp = fopen(filename, "r+");
  fputc('\n', fp);
  yypush_buffer_state(yy_create_buffer(fp, YY_BUF_SIZE));
  Shell::TheShell->_enablePrompt = false;
  BEGIN(INITIAL);
  yyparse();
  yypop_buffer_state();
  
  fclose(fp);
  fp = NULL;
  if (isatty(0)) {
    Shell::TheShell->_enablePrompt = true;
  }
}

["][^"]*["] {
  // removing quotes
  yylval.cpp_string = new std::string(yytext + 1, yyleng - 2);
  return WORD;
} 

"${$}" { 
  int pid = getpid();
  std::string str = std::to_string(pid);
  yylval.cpp_string = new std::string(str);
  return WORD;
}

"${?}" {
  std::string str = getenv("question");
  yylval.cpp_string = new std::string(str);
  return WORD;
}

"${!}" {
  std::string str = getenv("bang");
  yylval.cpp_string = new std::string(str);
  return WORD;
}

"${_}" {
  std::string str = getenv("_new");
  yylval.cpp_string = new std::string(str);
  return WORD;
}

"${SHELL}" {
  char absolutePath[PATH_MAX];
  if (realpath("/proc/self/exe", absolutePath) != NULL) {
      std::string path(absolutePath);
      yylval.cpp_string = new std::string(path);
  } else {
      perror("realpath");
  }
  return WORD;
}

"~" {
  std::string str = getenv("HOME");
  yylval.cpp_string = new std::string(str);
  return WORD;
}

"~/" {
  std::string str = getenv("HOME");
  str += '/';
  yylval.cpp_string = new std::string(str);
  return WORD;
}

\~[^ \t\n]*\/[^ \t\n]* {
  std::string str = yytext + 1;
  size_t index= str.find("/");
  std::string username = str.substr(0, index);
  std::string dir = str.substr(index);
  struct passwd *pw = getpwnam(username.c_str());
  if (pw) {
    username = pw->pw_dir;
    username += dir;
    yylval.cpp_string = new std::string(username);
    return WORD;
  }
  yylval.cpp_string = new std::string(yytext);
  return WORD;
}

\~[^ \t\n]* {
  char *username = strdup(yytext + 1);
  struct passwd *pw = getpwnam(username);
  if (pw) {
    std::string str = pw->pw_dir;
    yylval.cpp_string = new std::string(str);
    return WORD;
  }
  yylval.cpp_string = new std::string(yytext);
  return WORD;
}

[^ \t\n]*[?<!\\][<>] {
  // escape w < or >
  std::string str(yytext);
  std::string parsed;

  for (int i = 0; i < str.size(); i++) {
    if (str[i] == '\\') {
      if (str[i + 1] == '\\') {
        parsed += '\\';
        i++;
      } else {
        continue;
      }
    } else {
      parsed += str[i];
    }
  }

  yylval.cpp_string = new std::string(parsed.c_str());
  return WORD;
}

$\([^\t\n]*\) {
  // keep subshell as one argument
  Shell::TheShell->_pipeCommand->_subshell = true;
  yylval.cpp_string = new std::string(yytext);
  return WORD;
}

`[^\t\n]*` {
  // keep subshell as one argument
  Shell::TheShell->_pipeCommand->_subshell = true;
  yylval.cpp_string = new std::string(yytext);
  return WORD;
}

[^ \t\n<>|;][^ \t\n<>|;]*  {
  // Assume that file names have only alpha chars
  // escape w/out < or >
  std::string str(yytext);
  std::string parsed;

  for (int i = 0; i < str.size(); i++) {
    if (str[i] == '\\') {
      if (str[i + 1] == '\\') {
        parsed += '\\';
        i++;
      } else {
        continue;
      }
    } else {
      parsed += str[i];
    }
  }

  yylval.cpp_string = new std::string(parsed.c_str());
  return WORD;
}
