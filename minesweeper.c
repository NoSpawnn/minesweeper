#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define ROWS 10
#define COLS 10
#define BOMB_PERCENTAGE 25

enum State { OPEN, CLOSED, FLAGGED };
enum CellType { EMPTY, BOMB };

typedef struct {
  enum State state;
  enum CellType type;
} Cell;

typedef struct {
  int rows;
  int columns;
  int cursorRow;
  int cursorCol;
  Cell cells[ROWS][COLS];
} Field;

void fieldInit(Field *field, int rows, int cols) {
  field->rows = rows;
  field->columns = cols;
  field->cursorRow = 0;
  field->cursorCol = 0;

  for (int row = 0; row < field->rows; row++) {
    for (int col = 0; col < field->columns; col++) {
      field->cells[row][col].state = CLOSED;
      field->cells[row][col].type = EMPTY;
    }
  }
}

void fieldRandomizeBombs(Field *field, int bombPercentage) {
  int totalCells = field->columns * field->rows;
  int bombCount = totalCells * bombPercentage / 100;
  int setBombs = 0;

  do {
    int randRow = rand() % (field->rows);
    int randCol = rand() % (field->columns);

    if (field->cells[randRow][randCol].type != BOMB) {
      field->cells[randRow][randCol].type = BOMB;
      setBombs++;
    }

  } while (setBombs != bombCount);
}

int fieldCellGetNborBombsCount(Field *field, int row, int col) {
  int mineCount = 0;

  for (int rowDelta = -1; rowDelta <= 1; rowDelta++) {
    for (int colDelta = -1; colDelta <= 1; colDelta++) {
      if (row == 0 && rowDelta == -1 ||
          row == field->rows - 1 && rowDelta == 1 ||
          col == 0 && colDelta == -1 ||
          col == field->columns - 1 && colDelta == 1 ||
          rowDelta == 0 && colDelta == 0)
        continue;

      if (field->cells[row + rowDelta][col + colDelta].type == BOMB)
        mineCount++;
    }
  }

  return mineCount;
}

void fieldPrint(Field *field) {
  for (int row = 0; row < field->rows; row++) {
    for (int col = 0; col < field->columns; col++) {
      if (field->cursorRow == row && field->cursorCol == col)
        printf("[");
      else
        printf(" ");

      switch (field->cells[row][col].state) {
      case OPEN:
        if (field->cells[row][col].type == EMPTY) {
          int bombCount = fieldCellGetNborBombsCount(field, row, col);
          if (bombCount == 0)
            printf(" ");
          else
            printf("%d", bombCount);
        } else {
          printf("*");
        }
        break;
      case CLOSED:
        printf(".");
        break;
      case FLAGGED:
        printf("!");
        break;
      }

      if (field->cursorRow == row && field->cursorCol == col)
        printf("]");
      else
        printf(" ");
    }
    printf("\n");
  }
}

void fieldOpenCellAtCursor(Field *field) {
  int row = field->cursorRow;
  int col = field->cursorCol;

  if (field->cells[row][col].state != OPEN)
    field->cells[row][col].state = OPEN;
}

int main() {
  srand(time(NULL));
  char cmd;
  Field field;
  struct termios savedAttrs;
  struct termios tAttr;

  if (!isatty(STDIN_FILENO)) {
    fprintf(stderr, "Must be run in a terminal");
    exit(1);
  }

  tcgetattr(STDIN_FILENO, &savedAttrs);
  tcgetattr(STDIN_FILENO, &tAttr);

  // Non-canonical and disable echo
  tAttr.c_lflag &= ~(ICANON | ECHO);
  tAttr.c_cc[VMIN] = 1;
  tAttr.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSANOW, &tAttr);

  fieldInit(&field, ROWS, COLS);
  fieldRandomizeBombs(&field, BOMB_PERCENTAGE);
  fieldPrint(&field);

  while (1) {
    read(STDIN_FILENO, &cmd, 1);

    switch (cmd) {
    case 'w':
      if (field.cursorRow != 0)
        field.cursorRow -= 1;
      break;
    case 's':
      if (field.cursorRow != field.rows - 1)
        field.cursorRow += 1;
      break;
    case 'a':
      if (field.cursorCol != 0)
        field.cursorCol -= 1;
      break;
    case 'd':
      if (field.cursorCol != field.columns - 1)
        field.cursorCol += 1;
      break;
    case 'f':
      field.cells[field.cursorRow][field.cursorCol].state = FLAGGED;
      break;
    case ' ':
      fieldOpenCellAtCursor(&field);
      break;
    }

    // Reset cursor to original position
    // https://gist.github.com/ConnerWill/d4b6c776b509add763e17f9f113fd25b#cursor-controls
    printf("\e[%dA", field.rows);
    printf("\e[%dD", field.columns);

    fieldPrint(&field);
  }

  tcsetattr(STDIN_FILENO, TCSANOW, &savedAttrs);

  return 0;
}