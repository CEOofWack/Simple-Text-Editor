#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <stdbool.h>
#include <ncurses.h>
#include <string.h>

/**
 * Simple Text Editor
 * 
 * (Currently) a half functional text editor.
 * Manual character changing (for now), and no concept of lines of text (for now)
 * 
 * Supports buffer indexing using left and right arrow-keys, deletion with the backspace key. 
 * 
 * TO DO: 
 * -SEGFAULT in print_buffer when dereferncing start_ptr on linux systems, investigate!
 * -Issue with print buffer when inserting newline inbetween characters
 * -Issue with pressing ESC, look into that.
 * -Cursor underline not working for null character and ' '
 * -num_chars does not update properly since it counts overwritten characters as well
 * -Clean up the code
 * 
 * FIXED: 
 * Problem: Prints whole buffer, probably not good since there could be garbage. Handle that by fixing the prev thing.
 * Fix: used calloc to zero memory with null characters (\0). Continue to investigate potential issues for now.
 * 
 * Problem: Test increment and decrement logic for left and right arrow keys, not sure if fixed.
 * Fix: moving left and right through the buffer appears to work fine, cursor stops moving at both ends, continue testing.
 * 
 * Problem: Termination at end of buffer is not ideal, add logic to handle dynamically moving the "end" in the buffer (sum like dat?).
 * Fix: End buffer with EOF, expands to -1 at compile time but I'm told it's good practice? Calloc should have taken care of any issues with garbage
 * 
 * Problem: Issue with print buffer when inserting newline inbetween characters
 * Fix: clear terminal screen after before displaying buffer, should have done this before lmao
 */

int num_chars = 0;

// Flush input buffer. Unused for now
void flush_input_buffer(void) {
  int c;
  while ((c = getchar()) != '\n' && c != EOF);
}

// Function that enables "raw mode" by configuring the terminal enviroment using functions from the terimios.h header file
void raw_mode() {
  struct termios current_mode;
  tcgetattr(STDIN_FILENO, & current_mode);
  current_mode.c_lflag &= ~(ECHO);
  current_mode.c_lflag &= ~(ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &current_mode);
}

// Function that disables "raw mode" by re-configuring the terminal enviroment using functions from the terimios.h header file
void disable_raw_mode() {
  struct termios current_mode;
  tcgetattr(STDIN_FILENO, & current_mode);
  current_mode.c_lflag |= (ECHO);
  current_mode.c_lflag |= (ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &current_mode);
}

// Prints the entire buffer 
void print_buffer(char* start_ptr, char* cursor){
    while (*start_ptr != EOF) { //SEGFAULT HERE: only on linux, mac is too lenient. Possible getting lucky
      if (*start_ptr == ' ' || *start_ptr == '\0') {
        printf("%c", ' ');
        start_ptr++;
        continue;
      }
      if (start_ptr == cursor){
        // Not working
        if(*start_ptr == '\0' || *start_ptr == ' ' || *start_ptr == '\x1B'){
            printf("_"); // cursor is represented by an underline for now
            start_ptr++;
            continue;
        }
        printf("\033[4m%c\033[0m", *start_ptr);
        start_ptr++;
        continue;
      }
      printf("%c", *start_ptr); // Problem with inserting newline in between characters
      start_ptr++;
    }
}

// Function that handles user input and buffer writing
int add_char(char * start_ptr, char * end_ptr, char * cursor) {
  char c, z;
  z = getchar();

  if (z == '`') {
    print_buffer(start_ptr, cursor);
    printf("\n");
    printf("Num chracters in buffer: %d\n", num_chars);
    return 0;
  } 
  // ESC character received, begin check for escape sequence
  else if (z == '\x1B') {
    // seems these two getchar calls move the stdin cursor past [ and the next letter, they both get ignored on the next read call run
    getchar(); // ignore [
    c = getchar();
    switch (c) {
    case 'A':
      // Does nothing for now
      return 1;
    case 'B':
      // Does nothing for now
      return 1;
    case 'C':
      // Moves cursor right
      return 3;
    case 'D':
      // Moves cursor left
      return 2;

    default:
      return 1;
    }
  } 
  // Backspace received, delete character
  else if(z == '\x7F'){
    return 4;
  }
  else {
    *cursor = z;
    num_chars++;
    return 1;

  }
}

int main() {
  int cond = 0;
  char *buff = calloc(1000, sizeof(char));
  char *start_ptr = buff;
  char *end_ptr = buff + 1000;
  *end_ptr = EOF;
  char *cursor = buff;

  raw_mode();

  printf("\033[H\033[J"); //move cursor to home and clear screen
  printf("\033[?25l"); //hide cursor
  while (true) {
    printf("\033[J"); // clear screen
    print_buffer(start_ptr, cursor);
    printf("\033[H");
  
    cond = add_char(start_ptr, end_ptr, cursor);
    if (cond == 1) {
      cursor++;
      continue;
    } 
    else if (cond == 2) {
      if (cursor == start_ptr) {
        continue;
      }
      cursor--;
    } 
    else if (cond == 3) {
      if (cursor == end_ptr) {
        continue;
      }
      cursor++;
    } 
    else if (cond == 4) {
        if (cursor == start_ptr){
            *cursor = '\0';
            num_chars--;
            continue;
        }
        *cursor = '\0';
        num_chars--;
    }
    else {
      break;
    }
  }

  disable_raw_mode();
  
  free(buff);
  printf("\033[H"); // send cursor home
  printf("\033[J"); // clear screen
  printf("\033[?25h"); // show cursor

  return 0;
}
