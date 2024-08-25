#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define ROWS 10
#define COLS 10
#define BOMB_PERCENTAGE 25

typedef enum { OPEN, CLOSED, FLAGGED } State;
typedef enum { EMPTY, BOMB } CellType;
typedef enum { UP, DOWN, LEFT, RIGHT } Direction;

typedef struct {
  State state;
  CellType type;
} Cell;

typedef struct {
  int rows;
  int cols;
  int cursorRow;
  int cursorCol;
  Cell cells[ROWS][COLS];
} Field;

struct termios savedAttrs;
int totalFlagged = 0, correctlyFlagged = 0;
int totalBombs;

void resetTermState() {
  tcsetattr(STDIN_FILENO, TCSANOW, &savedAttrs);
  printf("\033[?25h");
}

void fieldInit(Field *field, int rows, int cols) {
  field->rows = rows;
  field->cols = cols;
  field->cursorRow = 0;
  field->cursorCol = 0;

  for (int row = 0; row < field->rows; row++) {
    for (int col = 0; col < field->cols; col++) {
      field->cells[row][col].state = CLOSED;
      field->cells[row][col].type = EMPTY;
    }
  }
}

void fieldRandomizeBombs(Field *field, int bombPercentage) {
  int totalCells = field->cols * field->rows;
  totalBombs = totalCells * bombPercentage / 100;
  int setBombs = 0;

  do {
    int randRow = rand() % (field->rows);
    int randCol = rand() % (field->cols);

    if (field->cells[randRow][randCol].type != BOMB) {
      field->cells[randRow][randCol].type = BOMB;
      setBombs++;
    }

  } while (setBombs != totalBombs);
}

int fieldCellGetNborBombsCount(Field *field, int row, int col) {
  int mineCount = 0;

  for (int rowDelta = -1; rowDelta <= 1; rowDelta++) {
    for (int colDelta = -1; colDelta <= 1; colDelta++) {
      if (row == 0 && rowDelta == -1 ||
          row == field->rows - 1 && rowDelta == 1 ||
          col == 0 && colDelta == -1 ||
          col == field->cols - 1 && colDelta == 1 ||
          rowDelta == 0 && colDelta == 0)
        continue;

      if (field->cells[row + rowDelta][col + colDelta].type == BOMB)
        mineCount++;
    }
  }

  return mineCount;
}

void fieldPrint(Field *field) {
  printf("Marked: %d  Total: %d\n", totalFlagged, totalBombs);
  for (int row = 0; row < field->rows; row++) {
    for (int col = 0; col < field->cols; col++) {
      if (field->cursorRow == row && field->cursorCol == col)
        printf("[");
      else
        printf(" ");

      Cell cell = field->cells[row][col];
      switch (cell.state) {
      case OPEN:
        if (cell.type == EMPTY) {
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

void fieldRePrint(Field *field) {
  // Reset cursor to original position
  // https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797#cursor-controls
  printf("\033[%dA", field->rows + 1);
  printf("\033[%dD", field->cols);

  fieldPrint(field);
}

void fieldShowAllBombs(Field *field) {
  for (int row = 0; row < field->rows; row++) {
    for (int col = 0; col < field->cols; col++) {
      if (field->cells[row][col].type == BOMB)
        field->cells[row][col].state = OPEN;
    }
  }
  fieldRePrint(field);
}

void fieldOpenCellAtCursor(Field *field) {
  int row = field->cursorRow;
  int col = field->cursorCol;
  Cell cell = field->cells[row][col];

  if (cell.type == BOMB) {
    fieldShowAllBombs(field);
    printf("\nThat was a bomb! Game over.\n");
    exit(0);
  } else if (cell.state != OPEN) {
    field->cells[row][col].state = OPEN;
  }
}

void fieldFlagCellAtCursor(Field *field) {
  Cell *cell = &field->cells[field->cursorRow][field->cursorCol];

  if (cell->state == OPEN)
    return;

  if (cell->state == FLAGGED) {
    cell->state = CLOSED;
    totalFlagged--;

    if (cell->type == BOMB)
      correctlyFlagged--;

    return;
  }

  cell->state = FLAGGED;
  totalFlagged++;

  if (cell->type == BOMB)
    correctlyFlagged++;
}

void fieldMoveCursor(Field *field, Direction direction) {
  switch (direction) {
  case UP:
    if (field->cursorRow != 0)
      field->cursorRow -= 1;
    break;
  case DOWN:
    if (field->cursorRow != field->rows - 1)
      field->cursorRow += 1;
    break;
  case LEFT:
    if (field->cursorCol != 0)
      field->cursorCol -= 1;
    break;
  case RIGHT:
    if (field->cursorCol != field->cols - 1)
      field->cursorCol += 1;
    break;
  }
}

int main() {
  srand(time(NULL));
  char cmd;
  Field field;
  struct termios tAttr;

  if (!isatty(STDIN_FILENO)) {
    fprintf(stderr, "Must be run in a terminal");
    exit(1);
  }

  tcgetattr(STDIN_FILENO, &savedAttrs);
  tcgetattr(STDIN_FILENO, &tAttr);

  // Non-canonical and disable echo
  // https://www.gnu.org/software/libc/manual/html_node/Noncanon-Example.html
  tAttr.c_lflag &= ~(ICANON | ECHO);
  tAttr.c_cc[VMIN] = 1;
  tAttr.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSANOW, &tAttr);
  printf("\033[?25l"); // Hide cursor
  atexit(resetTermState);

  fieldInit(&field, ROWS, COLS);
  fieldRandomizeBombs(&field, BOMB_PERCENTAGE);
  fieldPrint(&field);

  while (1) {
    read(STDIN_FILENO, &cmd, 1);

    switch (cmd) {
    case 'w':
      fieldMoveCursor(&field, UP);
      break;
    case 's':
      fieldMoveCursor(&field, DOWN);
      break;
    case 'a':
      fieldMoveCursor(&field, LEFT);
      break;
    case 'd':
      fieldMoveCursor(&field, RIGHT);
      break;
    case 'f':
      fieldFlagCellAtCursor(&field);
      break;
    case ' ':
      fieldOpenCellAtCursor(&field);
      break;
    case 'q':
    // TODO: Catch ctrl+c
    case '\x003':
      exit(0);
    }

    if (correctlyFlagged == totalBombs) {
      printf("You won!\n");
      exit(0);
    }

    fieldRePrint(&field);
  }

  return 0;
}