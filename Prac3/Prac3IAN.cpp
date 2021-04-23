//==============================================================================
// Copyright (C) John-Philip Taylor
// tyljoh010@myuct.ac.za
//
// This file is part of the EEE4084F Course
//
// This file is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>
//
// This is an adaptition of The "Hello World" example avaiable from
// https://en.wikipedia.org/wiki/Message_Passing_Interface#Example_program
//==============================================================================


/** \mainpage Prac3 Main Page
 *
 * \section intro_sec Introduction
 *
 * The purpose of Prac3 is to learn some basics of MPI coding.
 *
 * Look under the Files tab above to see documentation for particular files
 * in this project that have Doxygen comments.
 */



//---------- STUDENT NUMBERS --------------------------------------------------
//
// EDWIAN004
// AGXSPRK001
//-----------------------------------------------------------------------------

/* Note that Doxygen comments are used in this file. */
/** \file Prac3
 *  Prac3 - MPI Main Module
 *  The purpose of this prac is to get a basic introduction to using
 *  the MPI libraries for prallel or cluster-based programming.
 */

// Includes needed for the program
#include "Prac3.h"
#include <iostream>

unsigned char* getMedian(unsigned char *){
 return 0;
};

/** This is the master node function, describing the operations
    that the master will be doing */
void Master () {
 //! <h3>Local vars</h3>
 // The above outputs a heading to doxygen function entry
 int  j;             //! j: Loop counter
 char buff[BUFSIZE]; //! buff: Buffer for transferring message data
 MPI_Status stat;    //! stat: Status of the MPI application

 printf("0: We have %d processors\n", numprocs);


 // Read the input image
 if(!Input.Read("Data/greatwall.jpg")){
  printf("Cannot read image\n");
  return;
 }

 int chunkSize = Input.Height*Input.Width*Input.Components/(numprocs-1);
 int chunkHeight = Input.Height/(numprocs-1);
 int totalSize = Input.Height*Input.Width*Input.Components;
 int chunkWidth = Input.Width * Input.Components;
 int windowHeight = 10;
 int windowWidth = 10;
 int startRow = 0;
 unsigned char linearChunk[chunkSize];


printf("Sending times...\n");
for(int j = 1; j < numprocs; j++){
    tic();

     //flatten 2D array chunk:
    int counter = 0;
    for (int y = startRow; y < startRow + chunkHeight; y++){
        for (int x = 0; x < Input.Width*Input.Components; x++, counter++){
            linearChunk[counter] = Input.Rows[y][x];
        }
    }
    MPI_Send(&chunkSize, 32, MPI_BYTE, j, TAG, MPI_COMM_WORLD);
    MPI_Send(&chunkHeight, 32, MPI_BYTE, j, TAG, MPI_COMM_WORLD);
    MPI_Send(&chunkWidth, 32, MPI_BYTE, j, TAG, MPI_COMM_WORLD);
    MPI_Send(&windowHeight, 32, MPI_BYTE, j, TAG, MPI_COMM_WORLD);
    MPI_Send(&windowWidth, 32, MPI_BYTE, j, TAG, MPI_COMM_WORLD);
    MPI_Send(linearChunk, chunkSize, MPI_CHAR, j, TAG, MPI_COMM_WORLD);
    startRow += chunkHeight;

    printf("Time = %lg ms\n", (double)toc()/1e-3);
}

 // Allocated RAM for the output image
 if(!Output.Allocate(Input.Width, Input.Height, Input.Components)) return;

printf("Receiving times...\n");
for (int j=1; j < numprocs; j++){
    tic();

    MPI_Recv(linearChunk, chunkSize, MPI_CHAR, j, TAG, MPI_COMM_WORLD, &stat);
    int counter = 0;
    for (int y = (j-1)*chunkHeight; y < j*chunkHeight; y++){
        for (int x = 0; x< Input.Width*Input.Components; x++, counter++){
            Output.Rows[y][x] = linearChunk[counter];
        }
    }

    printf("Time = %lg ms\n", (double)toc()/1e-3);
}

 // Write the output image
 if(!Output.Write("Data/Output.jpg")){
  printf("Cannot write image\n");
  return;
 }


printf("End of Experiment code code...\n\n");

 //! <h3>Output</h3> The file Output.jpg will be created on success to save
 //! the processed output.
}
//------------------------------------------------------------------------------

/** This is the Slave function, the workers of this MPI application. */
void Slave(int ID){

    MPI_Status stat;
    
    int chunkSize = 0;
    int chunkHeight = 0;
    int chunkWidth = 0;
    int windowHeight = 0;
    int windowWidth = 0;
    
    MPI_Recv(&chunkSize, 32, MPI_BYTE, 0, TAG, MPI_COMM_WORLD, &stat);
    MPI_Recv(&chunkHeight, 32, MPI_BYTE, 0, TAG, MPI_COMM_WORLD, &stat);
    MPI_Recv(&chunkWidth, 32, MPI_BYTE, 0, TAG, MPI_COMM_WORLD, &stat);
    MPI_Recv(&windowHeight, 32, MPI_BYTE, 0, TAG, MPI_COMM_WORLD, &stat);
    MPI_Recv(&windowWidth, 32, MPI_BYTE, 0, TAG, MPI_COMM_WORLD, &stat);
    unsigned char linearChunk[chunkSize];
    MPI_Recv(linearChunk, chunkSize, MPI_BYTE, 0, TAG, MPI_COMM_WORLD, &stat);

    unsigned char ** image = new unsigned char*[chunkHeight];
    unsigned char ** output = new unsigned char*[chunkHeight];
    unsigned char window[windowHeight*windowWidth];
    int edgex = windowHeight/2;
    int edgey = windowWidth/2;
    
    int counter = 0;
    int pos = 0;
    for (int y = 0; y < chunkHeight; y++){
        image[y] = new unsigned char[chunkWidth];
        output[y] = new unsigned char[chunkWidth];
        for (int x = 0; x< chunkWidth; x++, counter++){
            image[y][x] = linearChunk[counter];
            output[y][x] = linearChunk[counter];
        }
    }

    for (int x = edgex; x < chunkHeight - edgex; x++){ //start at half the window size across and down so we can get median values around it.
        for (int y = edgey; y < chunkWidth - edgey; y++){
            counter = 0;

            for (int k = -windowHeight/2; k <= windowHeight/2; k++){ //get all values in a box around a central point - the value in question.
                for (int j =-windowWidth/2*3; j<=windowWidth/2*3; j+=3){ //move 3 along each time to get corresponding R,G and B values.
                    window[counter] = image[x+k][y+j];
                    ++counter;
                }
            }

            for (int i = 0; i<windowWidth*windowHeight; i++){
                for (int j=i+1; j<windowWidth*windowHeight; j++){
                    if (window[i]>window[j]){
                        unsigned char temp = window[i];
                        window[i] = window[j];
                        window[j] = temp;
                    }
                }
            }
            output[x][y] = window[windowHeight*windowWidth/2];
            //linearChunk[pos] = window[windowHeight*windowWidth/2];
            //++pos;
        }
    }

    counter = 0;
    for (int y = 0; y < chunkHeight; y++){
        for (int x = 0; x< chunkWidth; x++, counter++){
            linearChunk[counter] = output[y][x];
        }
    }

    
    MPI_Send(linearChunk, chunkSize, MPI_CHAR, 0, TAG, MPI_COMM_WORLD);
}
//------------------------------------------------------------------------------

/** This is the entry point to the program. */
int main(int argc, char** argv){
 int myid;

 // MPI programs start with MPI_Init
 MPI_Init(&argc, &argv);

 // find out how big the world is
 MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

 // and this processes' rank is
 MPI_Comm_rank(MPI_COMM_WORLD, &myid);

 // At this point, all programs are running equivalently, the rank
 // distinguishes the roles of the programs, with
 // rank 0 often used as the "master".
 if(myid == 0) Master();
 else Slave (myid);

 // MPI programs end with MPI_Finalize
 MPI_Finalize();
 return 0;
}
//------------------------------------------------------------------------------
