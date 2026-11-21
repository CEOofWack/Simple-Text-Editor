#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <stdbool.h>

/**
 * Simple Text Editor
 * 
 * (Currently) a half functional text editor.
 * Manual character changing (for now), and no concept of lines of text (for now)
 * 
 * Supports buffer indexing using left and right arrow-keys, deletion in place with the backspace key. 
 * 
 * TODO: 
 * -STATUS UNKNOWN: SEGFAULT in print_buffer when dereferncing start_ptr on linux systems, investigate!
 * -num_chars does not update properly since it counts overwritten characters as well
 * -PARTIALLY DONE: add shifting buffer to make deletion more seemless
 * -add logic for tab characters so they appear as multiple spaces and improve insertion between chars
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
 * 
 * Problem: Bus error when pressing tab continuously, investigate
 * Partial fix: The problem wasn't exactly this. The buffer ends with EOF at the last character, however I wasn't taking 
 * into account that that last character can get overwritten after cursor gets to the end, then the end 
 * check would fail at the next run and if the user increments the cursor the program attempts to access inaccessible memory. 
 * At that, the buffer was holding X chars and at tge X-1th idx a EOF, so in reality it would actually hold X-1 chars 
 * which also isn't ideal. The fix is simply adding space for one more char to hold EOF and to run the check for the 
 * final char at the X-2th idx.
 */

int num_chars = 0;

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

// Shifts buffer after backspace pressed. 
void shift_buffer(char* cursor){
  char* temp = cursor;
  while (true){
    if(*(temp + 1) == EOF){
      *temp = '\0';
      return;
    }
    *(temp) = *(temp + 1);
    temp++;
  }

}

// Prints the entire buffer 
void print_buffer(char* start_ptr, char* cursor){
    
    while (*start_ptr != EOF) { 
      if (start_ptr == cursor){
        if(*start_ptr == '\0' || *start_ptr == ' ' || *start_ptr == '\x1B'){
            printf("█"); // cursor is represented by inverted char
            start_ptr++;
            continue;
        }
        printf("\033[7m%c\033[m", *start_ptr); // cursor is represented by inverted char
        start_ptr++;
        continue;
      }
      if (*start_ptr == ' ' || *start_ptr == '\0') {
        printf("%c", ' ');
        start_ptr++;
        continue;
      }
      printf("%c", *start_ptr); 
      start_ptr++;
    }
}


// Function that handles user input and buffer writing
int add_char(char *start_ptr, char *end_ptr, char *cursor) {
  char c, z;
  z = getchar();

  if (z == '`') {
    print_buffer(start_ptr, cursor);
    return 0;
  } 
  // ESC character received, begin check for escape sequence
  else if (z == '\x1B') {
    // seems these two getchar calls move the stdin cursor past [ and the next letter, they both get ignored on the next read call run
    getchar(); // ignore [
    c = getchar();
    switch (c) {
    // A and B (up and down) do nothing for now
    case 'C':
      return 3; // Moves cursor right
    case 'D':
      return 2; // Moves cursor left

    default:
      return 10;
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
  // Not dynamicaly growing buffer for now, only 1000 chars
  char *buff = calloc(1001, sizeof(char)); // Buffer where all characters are written/read
  if (buff == NULL){printf("Error allocating buffer\n"); return -1;}
  char *start_ptr = buff; // Pointer to the start of the buffer
  char *end_ptr = buff + 1000; // Pointer to the end of the buffer
  *end_ptr = EOF;
  char *cursor = buff; // Cursor in buffer

  raw_mode();

  printf("\033[J"); // clear screen
  printf("\033[H"); // send cursor top left (terminal home)
  printf("\033[J"); // clear screen
  printf("\033[7mSIMPLE TEXT EDITOR - V1                                                                                                                              Press '`' to exit \033[m");
  printf("\033[3H"); // send cursor to program output home (Starts 3rd row)
  printf("\033[?25l"); // hide terminal cursor
  while (true) {
    printf("\033[J"); // clear screen
    print_buffer(start_ptr, cursor);
    printf("\033[3H"); // send cursor to program output home (Starts 3rd row)
    start_ptr = buff;
    cond = add_char(start_ptr, end_ptr, cursor);

    if (cond == 10){
      continue;
    }
    else if (cond == 1) {
      if ((cursor + 1) == end_ptr) {
        continue;
      }
      cursor++;
    } 
    else if (cond == 2) {
      if (cursor == start_ptr) {
        continue;
      }
      cursor--;
    } 
    else if (cond == 3) {
      if ((cursor + 1) == end_ptr) {
        continue;
      }
      cursor++;
    } 
    else if (cond == 4) {
        *cursor = '\0';
        shift_buffer(cursor);
        if (cursor == start_ptr) {
          num_chars--;
          continue;
        }
        cursor--;
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

  //printf("%d\n", num_chars);

  return 0;
}
