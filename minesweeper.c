#include "minesweeper.h"
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

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

  field->cells = (Cell **)malloc(rows * sizeof(Cell *));

  for (int row = 0; row < field->rows; row++) {
    field->cells[row] = (Cell *)malloc(cols * sizeof(Cell *));
    for (int col = 0; col < field->cols; col++) {
      field->cells[row][col].state = CLOSED;
      field->cells[row][col].type = EMPTY;
    }
  }
}

void fieldFree(Field *field) {
  for (int i = 0; i < field->rows; i++)
    free(field->cells[i]);

  free(field->cells);
}

void fieldRandomizeBombs(Field *field, int bombPercentage) {
  int totalCells = field->cols * field->rows;
  totalBombs = totalCells * bombPercentage / 100;
  int setBombs = 0;

  do {
    int randRow = rand() % (field->rows);
    int randCol = rand() % (field->cols);

    if (field->cells[randRow][randCol].type != BOMB &&
        randRow != field->cursorRow && randCol != field->cursorCol) {
      field->cells[randRow][randCol].type = BOMB;
      setBombs++;
    }

  } while (setBombs != totalBombs);
}

bool isInField(Field *field, int row, int col) {
  return (row >= 0) && (row < field->rows) && (col >= 0) && (col < field->cols);
}

int fieldCellGetNborBombsCount(Field *field, int row, int col) {
  int mineCount = 0;

  for (int rowDelta = -1; rowDelta <= 1; rowDelta++) {
    for (int colDelta = -1; colDelta <= 1; colDelta++) {
      if (!isInField(field, row + rowDelta, col + colDelta))
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
        printf(ANSI_RED "!" ANSI_RESET);
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

void fieldOpenCell(Field *field, int row, int col) {
  Cell *cell = &field->cells[row][col];

  if (cell->type == BOMB) {
    fieldShowAllBombs(field);
    printf("\nThat was a bomb! Game over.\n");
    exit(0);
  } else if (cell->state != OPEN) {
    cell->state = OPEN;
    if (fieldCellGetNborBombsCount(field, row, col) == 0)
      fieldOpenAdjacentCells(field, row, col);
  }
}

void fieldOpenCellAtCursor(Field *field) {
  fieldOpenCell(field, field->cursorRow, field->cursorCol);
}

void fieldOpenAdjacentCells(Field *field, int row, int col) {
  for (int rowDelta = -1; rowDelta <= 1; rowDelta++) {
    for (int colDelta = -1; colDelta <= 1; colDelta++) {
      int currentCol = col + colDelta;
      int currentRow = row + rowDelta;

      if (!isInField(field, currentRow, currentCol))
        continue;

      Cell *cell = &field->cells[currentRow][currentCol];
      if (cell->type == EMPTY && cell->state == CLOSED &&
          fieldCellGetNborBombsCount(field, currentRow, currentCol) == 0) {
        cell->state = OPEN;
        fieldOpenAdjacentCells(field, currentRow, currentCol);
      } else if (cell->type != BOMB && cell->state != FLAGGED &&
                 fieldCellGetNborBombsCount(field, currentRow, currentCol) != 0)
        fieldOpenCell(field, currentRow, currentCol);
    }
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

int main(int argc, char *argv[]) {
  srand(time(NULL));
  char cmd;
  Field field;
  struct termios tAttr;
  bool first = true;
  int optFlag;
  int rowsFromFlag = 0;
  int colsFromFlag = 0;

  if (!isatty(STDIN_FILENO)) {
    fprintf(stderr, "Must be run in a terminal");
    exit(1);
  }

  while ((optFlag = getopt(argc, argv, "r:c:")) != -1) {
    switch (optFlag) {
    case 'r':
      rowsFromFlag = atoi(optarg);
      if (rowsFromFlag <= 0) {
        fprintf(stderr, "'%s' is an invalid value for -r\n", optarg);
        exit(1);
      }
      break;
    case 'c':
      colsFromFlag = atoi(optarg);
      if (colsFromFlag <= 0) {
        fprintf(stderr, "'%s' is an invalid value for -c\n", optarg);
        exit(1);
      }
      break;
    }
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

  if (colsFromFlag > 0 && rowsFromFlag > 0)
    fieldInit(&field, rowsFromFlag, colsFromFlag); // Both
  else if (colsFromFlag > 0 && rowsFromFlag == 0)
    fieldInit(&field, DEFAULT_ROWS, colsFromFlag); // Cols only
  else if (colsFromFlag == 0 && rowsFromFlag > 0)
    fieldInit(&field, rowsFromFlag, DEFAULT_COLS); // Rows only
  else
    fieldInit(&field, DEFAULT_ROWS, DEFAULT_COLS); // Neither

  fieldPrint(&field);

  while (true) {
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
      if (first) {
        fieldRandomizeBombs(&field, BOMB_PERCENTAGE);
        first = false;
      }
      fieldOpenCellAtCursor(&field);
      break;
    case 'q':
      fieldFree(&field);
      exit(0);
    }

    if (!first && correctlyFlagged == totalBombs) {
      printf("You won!\n");
      exit(0);
    }

    fieldRePrint(&field);
  }

  fieldFree(&field);
  return 0;
}