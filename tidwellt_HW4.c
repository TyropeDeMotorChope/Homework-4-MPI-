/*
 Name: Trenton Tidwell
 BlazerId: tidwellt
 Course Section: CS 432
 Homework #: 4
*/

#include <sys/time.h> //Clock
#include <time.h>
#include <stdlib.h>   //Rand
#include <stdio.h>
#include "mpi.h"

#define MASTER           0
#define ASSEMBLER        1
#define FROM_MASTER_PREV 2
#define FROM_MASTER_CURR 3
#define FROM_MASTER_NEXT 4
#define FROM_ASSEMBLER   5
#define FROM_MASTER_rnum 6
#define FROM_WORKER      7

int** InitializeBoard(int** board, int Rows, int Cols, int fill);
void copy_array_2d(int** read_arr, int** write_arr, int rows, int cols);
void fill_ghost(int** board, int rows, int cols);
void delete_arr(int** arr, int rows);
void game_of_life();
void run_game(int ROWS, int COLS, int max_cycles, int numtasks);
void print_board(int** board, int Rows, int Cols);


int main(int argc, char* argv[]){
  //COMMAND LINE ARGUMENT
  int  ROWS,
       COLS,
       max_cycles,
       numworkers,
       source,
       numtasks,
       taskid,
       dest,
       mtype,
       i, j, k , r, c;


  if(argc == 4){
    ROWS = strtol(argv[1], NULL, 10);
    COLS = strtol(argv[1], NULL, 10);
    max_cycles = strtol(argv[2], NULL, 10);
    numtasks = strtol(argv[3], NULL, 10);
  } else if(argc == 5){
    ROWS = strtol(argv[1], NULL, 10);
    COLS = strtol(argv[2], NULL, 10);
    max_cycles = strtol(argv[3], NULL, 10);
    numtasks = strtol(argv[4], NULL, 10);
  }

  struct timeval t1, t2;
  double elapsedTime;
  gettimeofday(&t1, NULL);

  MPI_Status status;
  MPI_Init(NULL, NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &taskid);
  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  numworkers = numtasks - 2;


  /*--------------------------------MASTER---------------------------------*/
  if(taskid == MASTER){
     int **GameBoard = (int **)malloc((ROWS+2)*sizeof(int *));
     int **NextBoard = (int **)malloc((ROWS+2)*sizeof(int *));
     GameBoard = InitializeBoard(GameBoard, ROWS, COLS, 1);

     MPI_Bcast(&ROWS, 1, MPI_INT, MASTER, MPI_COMM_WORLD);

     for(int cycle_count = 0; cycle_count < max_cycles; cycle_count++){
       dest = 1;
       for(r=1; r <= ROWS; r++){

	 MPI_Send(&r, 1, MPI_INT, dest, FROM_MASTER_rnum, MPI_COMM_WORLD);
	 MPI_Send(&GameBoard[r-1][0], COLS+2, MPI_INT, dest, FROM_MASTER_PREV, MPI_COMM_WORLD);
	 MPI_Send(&GameBoard[r][0], COLS+2, MPI_INT, dest, FROM_MASTER_CURR, MPI_COMM_WORLD);
	 MPI_Send(&GameBoard[r+1][0], COLS+2, MPI_INT, dest, FROM_MASTER_NEXT, MPI_COMM_WORLD);
	 game_of_life();

	 if(dest == numworkers){
	   dest = 1;
	 } else {dest += 1;}

       }

       MPI_Recv(&GameBoard, ROWS*COLS, MPI_INT, ASSEMBLER, FROM_ASSEMBLER, MPI_COMM_WORLD, &status);

       // fill_ghost(NextBoard, ROWS+2, COLS+2);
       print_board(GameBoard, ROWS, COLS);

       // int** temp;
       // temp = **GameBoard;
       // **GameBoard = **NextBoard;
       // **NextBoard = **temp;
       printf("\n\nCYCLE: %d\n\n", cycle_count);
     }

     delete_arr(GameBoard, ROWS+2);
  }


     //Clock and print time required
     gettimeofday(&t2, NULL);
     double elapsed_secs = (t2.tv_sec - t1.tv_sec) * 1000.0;
     printf("\nTime required for program to run: %10.2f seconds\n\n", elapsed_secs);
     MPI_Finalize();


  return 0;

}

void game_of_life(){
  //Do not include GhostCells in array diminsions passed to this function
  //this method takes in 3 row parameters to decide what to write to 1 row in the next board

  int  rows,
       cols,
       row_num,
       source,
       taskid,
       i, j, r, c;
  MPI_Status status;

  /*-----------------------------WORKER TASKS------------------------------*/
  if(taskid > ASSEMBLER){
    int *next;
    int *R0;
    int *R1;
    int *R2;

    next = (int *) malloc(cols*sizeof(int));
    R0 = (int *) malloc(cols*sizeof(int));
    R1 = (int *) malloc(cols*sizeof(int));
    R2 = (int *) malloc(cols*sizeof(int));

    MPI_Bcast(&cols, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
    MPI_Recv(&row_num, 1, MPI_INT, MASTER, FROM_MASTER_rnum, MPI_COMM_WORLD, &status);
    MPI_Recv(&R0, cols, MPI_INT, MASTER, FROM_MASTER_PREV, MPI_COMM_WORLD, &status);
    MPI_Recv(&R1, cols, MPI_INT, MASTER, FROM_MASTER_CURR, MPI_COMM_WORLD, &status);
    MPI_Recv(&R2, cols, MPI_INT, MASTER, FROM_MASTER_NEXT, MPI_COMM_WORLD, &status);


    int live_count;

    for(int c = 1; c<=cols; c++){
      live_count = 0;

      for(int j = -1; j <= 1; j++){
        if(R0[c+j]==1){live_count+=1;}
        if(R2[c+j]==1){live_count+=1;}
      }
      if(R1[c-1]==1){live_count+=1;}
      if(R1[c+1]==1){live_count+=1;}

      //Conway's algorithm
      if(live_count < 2 || live_count > 3 ){next[c] = 0;}
      else {next[c] = 1;}
    }

    next[0] = next[cols];
    next[cols+1] = next[1];

    MPI_Send(&row_num, 1, MPI_INT, ASSEMBLER, FROM_WORKER, MPI_COMM_WORLD);
    MPI_Send(&next, cols+2, MPI_INT, ASSEMBLER, FROM_WORKER, MPI_COMM_WORLD);
    free(next);

  }

  return;
}

void fill_ghost(int** board, int rows, int cols){
  //Sizes including GhostCells should be passed to this function
  //Coppies full array including ghost cells
  for(int i = 1; i < rows; i++){                       //Rows
    board[i][0] = board[i][cols-2];
    board[i][cols-1] = board[i][1];
  }

  for(int j = 1; j < cols; j++){                        //Cols
    board[0][j] = board[rows-2][j];
    board[rows-1][j] = board[1][j];
  }

  /***** copy board corner data to ghost cells******/
  board[0][0] = board[rows-2][cols-2];                       //Corners
  board[0][cols-1] = board[rows-2][1];
  board[rows-1][0] = board[1][cols-2];
  board[rows-1][cols-1] = board[1][1];

}

void Assemble(){

  int  rows,
       cols,
       row_num,
       source,
       taskid,
       r, c;
  MPI_Status status;


  if(taskid == ASSEMBLER){
    MPI_Bcast(&rows, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
    int **newboard = (int **) malloc(rows*sizeof(int *));
    int *new_row = (int* ) malloc(rows*sizeof(int));

    for(r=0; r<=rows; r++){
      newboard[r] = (int *)malloc(rows * sizeof(int));
    }
    for(r=0; r<=rows; r++){
      MPI_Recv(&row_num, 1, MPI_INT,MPI_ANY_SOURCE, FROM_WORKER, MPI_COMM_WORLD, &status);
      MPI_Recv(&new_row, rows, MPI_INT,status.MPI_SOURCE, FROM_WORKER, MPI_COMM_WORLD, &status);
      newboard[row_num] = new_row;
    }

    for(r=0; r<rows; r++){
      newboard[0][r] = newboard[rows][r];
      newboard[rows+1][r] = newboard[1][r];
    }



    MPI_Send(&newboard, rows*rows, MPI_INT, MASTER, FROM_ASSEMBLER, MPI_COMM_WORLD);
    delete_arr(newboard, rows);
  }

}

int** InitializeBoard(int** board, int Rows, int Cols, int fill){
  //Do not include GhostCells in array diminsions passed to this function

  int max_row = Rows+1;
  int max_col = Cols+1;

  //Create collumn array to hold pointers to each row array
  for(int row = 0; row < Rows+2; row++){
    board[row] = (int *)malloc((Cols+2)*sizeof(int));
  }

  //During init, one board is filled and the other is copied
  //This is for the case that this object should be filled
  if(fill == 1) {
    srand(time(NULL));

    // Iterate through Board
    for(int row = 1; row < Rows; row++){
      for(int col = 1; col <= Cols; col++){
        if (0==(rand()%5)){
          board[row][col] = 1;
        }
        else{
          board[row][col] = 0;
        }
      }
    }
    fill_ghost(board, Rows+2, Cols+2);
  }


  else{     //if fill variable is false, fill array with all false
    for(int row = 0; row <= Rows; row++){
      for(int col = 0; col <= Cols; col++){
        board[row][col] = 0; // all cells are dead
      }
    }
  }

  return board;
}

void copy_array_2d(int** read_arr, int** write_arr, int rows, int cols){
  //Sizes including GhostCells should be passed to this function
  //Coppies full array including ghost cells

  for(int row = 0; row < rows; row++){
    for(int col = 0; col < cols; col++){
      write_arr[row][col] = read_arr[row][col];
    }
  }
}

void print_board(int** board, int Rows, int Cols){
  //Sizes of dimensions excluding ghost cells should be passed here
  for(int i = 1; i<=Rows; i++){
    for(int j = 1; j<=Cols; j++){
      if (board[i][j] == 0){printf("O ");}
      else{printf( ". ");}
    }
    printf("\n");
  }
  printf("\n\n");
}

void delete_arr(int** arr, int rows){
  //Full array dimensions (including Ghost Cells) Should be passed here

  int i;
  for(i = 0; i<rows; i++){
    free(arr[i]);        //Delete each row pointer
    arr[i] = 0;
  }
  free(arr);            //Delete the pointer to the collumn which
  arr = 0;                //Holds the row pointers
}
