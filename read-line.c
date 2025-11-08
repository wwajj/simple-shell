/*
 * CS252: Systems Programming
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "tty-raw-mode.c"

#define MAX_BUFFER_LINE 2048

#define CTRL_A 1
#define CTRL_D 4
#define CTRL_E 5
#define BACKSPACE 8
#define TAB 9
#define ENTER 10
#define ESC 27
#define CTRL_QUESTION 31
#define CHAR_A 65
#define CHAR_B 66
#define CHAR_C 67
#define CHAR_D 68
#define L_BRACKET 91
#define CTRL_H 127

extern void tty_raw_mode(void);

// Buffer where line is stored
char line_buffer[MAX_BUFFER_LINE];

// Simple history array
// This history does not change. 
// Yours have to be updated.
int history_index = 0;
char ** history = NULL;
int history_length = 0;
int history_count = 0;

void read_line_print_usage()
{
  char * usage = "\n"
    " ctrl-?       Print usage\n"
    " Backspace    Deletes last character\n"
    " up arrow     See last command in the history\n";

  write(1, usage, strlen(usage));
}

char* my_strdup(const char* s) {
  size_t len = strlen(s);
  char* result = (char *) malloc(len + 1);
  if (!result) {
    return NULL; 
  }
  memcpy(result, s, len + 1);
  return result;
}

/* 
 * Input a line with some basic editing.
 */
char * read_line() {

  // save old terminal state
  struct termios dflt;
  tcgetattr(0, &dflt);

  // Set terminal in raw mode
  tty_raw_mode();

  int line_length = 0;
  line_buffer[0] = '\0';
  int cursor = 0;

  // Read one line until enter is typed
  while (1) {

    // Read one character in raw mode.
    char ch;
    read(0, &ch, 1);

    if ((ch >= 32) && (ch < CTRL_H)) {
      // It is a printable character. 

      // Do echo
      write(1,&ch,1);

      // If max number of character reached return.
      if (line_length==MAX_BUFFER_LINE-2) break; 

      if (cursor >= line_length) {
        line_buffer[line_length] = ch;
        line_length++;
        line_buffer[line_length] = '\0';
      } else {
        line_buffer[cursor] = ch;
      }
      cursor++;
    }
    else if (ch == TAB) {
      if ((!line_length) || (line_buffer[line_length - 1] == ' ') || (cursor < line_length)) {
        continue;
      }

      int i = line_length - 1;
      while ((i > -1) && (line_buffer[i] != ' ')) {
        i--;
      }
      i++;

      DIR *dir = opendir(".");
      struct dirent *ent;
      char *suffix = NULL;
      while (ent == readdir(dir)) {
        if (!strncmp(line_buffer + i, ent->d_name, strlen(line_buffer + i))) {
          if (!suffix) {
            suffix = my_strdup(ent->d_name);
          }
          else {
            int j = 0;
            while ((suffix[j]) && (ent->d_name[j])) {
              if (suffix[j] != ent->d_name[j]) {
                break;
              }
              j++;
            }
            suffix[j] = '\0';

            char *new_suffix = my_strdup(suffix);
            free(suffix);
            suffix = new_suffix;
          }
        }
      }

      closedir(dir);
      if (!suffix) {
        continue;
      }

      for (int k = 0; k < strlen(suffix); k++) {
        line_buffer[i + k] = suffix[k];
        if ((i + k) >= cursor) {
          ch = line_buffer[i + k];
          write(1, &ch, 1);
        }
      }

      line_buffer[i + strlen(suffix)] = '\0';
      line_length = strlen(line_buffer);
      cursor = line_length;
    }
    else if (ch == ENTER) {
      // <Enter> was typed. Return line
      if (!strlen(line_buffer)) {
        write(1, &ch, 1);
        continue;
      }
      if (history_count >= history_length) {
        history_length += 10;
        history = (char **) realloc(history, history_length);
      }

      history[history_count] = my_strdup(line_buffer);
      history_count++;

      write(1, &ch, 1);
      history_index = history_count;

      break;
    }
    else if (ch == CTRL_QUESTION) {
      // ctrl-?
      read_line_print_usage();
      line_buffer[0]=0;
      break;
    }
    else if (ch == CTRL_A) {
      for (int i = 0; i < cursor; i++) {
        ch = BACKSPACE;
        write(1, &ch, 1);
      }
      cursor = 0;
    }
    else if (ch == CTRL_E) {
      for (; cursor < line_length; cursor++) {
        ch = line_buffer[cursor];
        write(1, &ch, 1);
      }
    }
    else if (ch == CTRL_D) {
      if (cursor >= line_length) {
        continue;
      }

      for (int i = cursor; i < line_length; i++) {
        ch = line_buffer[i + 1];
        write(1, &ch, 1);
        line_buffer[i] = line_buffer[i + 1];
      }
      ch = ' ';
      write(1, &ch, 1);
    
      for (int i = line_length; i > cursor; i--) {
        ch = BACKSPACE;
        write(1, &ch, 1);
      }
      line_length--;
    }
    else if ((ch == CTRL_H) || (ch == BACKSPACE)) {
      // <backspace> was typed. Remove previous character read.
      if (!cursor) {
        continue;
      }
      cursor--;
      ch = BACKSPACE;
      write(1, &ch, 1);

      for (int i = cursor; i < line_length; i++) {
        ch = line_buffer[i + 1]; 
        write(1, &ch, 1);
        line_buffer[i] = line_buffer[i + 1];
      }
      ch = ' '; 
      write(1, &ch, 1);
      
      // Go back one character
      for (int i = line_length; i > cursor; i--) {
        ch = BACKSPACE;
        write(1, &ch, 1);
      }
 
      // Remove one character from buffer
      line_length--;
    }
    else if (ch == ESC) {
      // Escape sequence. Read two chars more
      //
      // HINT: Use the program "keyboard-example" to
      // see the ascii code for the different chars typed.
      //
      char ch1; 
      char ch2;
      read(0, &ch1, 1);
      read(0, &ch2, 1);
      if (ch1 == L_BRACKET) {
        if ((ch2 == CHAR_A)) {
          if ((!history_count) || (!history_index)) {
            continue;
          }

          // Erase old line
          // Print backspaces
          int i = 0;
          for (i =0; i < line_length; i++) {
            ch = 8;
            write(1,&ch,1);
          }

          // Print spaces on top
          for (i =0; i < line_length; i++) {
            ch = ' ';
            write(1,&ch,1);
          }

          // Print backspaces
          for (i =0; i < line_length; i++) {
            ch = 8;
            write(1,&ch,1);
          }	

          // Copy line from history
          strcpy(line_buffer, history[--history_index]);
          line_length = strlen(line_buffer);
          
          // echo line
          write(1, line_buffer, line_length);
          cursor = line_length;
        }
        else if (ch2 == CHAR_B) {
          if ((!history_count) || (history_index >= history_count)) {
            continue;
          }

          for (int i = 0; i < line_length; i++) {
            ch = BACKSPACE;
            write(1, &ch, 1);
          }
          for (int i = 0; i < line_length; i++) {
            ch = ' ';
            write(1, &ch, 1);
          }

          for (int i = 0; i < line_length; i++) {
            ch = BACKSPACE;
            write(1, &ch, 1);
          }

          if (history_index == (history_count - 1)) {
            line_buffer[0] = '\0';
            line_length = 0;
            history_index++;
          }
          else {
            strcpy(line_buffer, history[++history_index]);
            line_length = strlen(line_buffer);
          }

          write(1, line_buffer, line_length);
          cursor = line_length;
        }
        else if (ch2 == CHAR_C) {
          if (cursor >= line_length) {
            continue;
          }
          ch = line_buffer[cursor];
          write(1, &ch, 1);
          cursor++;
        }
        else if (ch2 == CHAR_D) {
          if (!cursor) {
            continue;
          }

          ch = BACKSPACE;
          write(1, &ch, 1);
          cursor--;
        }
      }
    }
  }

  // Add eol and null char at the end of string
  line_buffer[line_length]=10;
  line_length++;
  line_buffer[line_length]=0;

  tcsetattr(0, TCSAFLUSH, &dflt);

  return line_buffer;
}

