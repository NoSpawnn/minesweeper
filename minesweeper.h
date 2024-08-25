#include <stdbool.h>

#define DEFAULT_ROWS 10
#define DEFAULT_COLS 10
#define BOMB_PERCENTAGE 25
#define ANSI_RED "\x1b[31m"
#define ANSI_RESET "\x1b[0m"

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
  Cell **cells;
} Field;

void resetTermState();
void fieldInit(Field *field, int rows, int cols);
void fieldFree(Field *field);
// Check if the given position is within the bounds of the field
bool isInField(Field *field, int row, int col);
// Place n random bombs on the field when `n = totalCells * bombPercentage / 100`
void fieldRandomizeBombs(Field *field, int bombPercentage);
// Get the number of bombs neighbouring a given cell on the field
int fieldCellGetNborBombsCount(Field *field, int row, int col);
void fieldPrint(Field *field);
void fieldRePrint(Field *field);
// Reveal all bombs on the field and `RePrint` the field
void fieldShowAllBombs(Field *field);
void fieldOpenCell(Field *field, int row, int col);
void fieldOpenCellAtCursor(Field *field);
void fieldOpenAdjacentCells(Field *field, int row, int col);
void fieldFlagCellAtCursor(Field *field);
void fieldMoveCursor(Field *field, Direction direction);