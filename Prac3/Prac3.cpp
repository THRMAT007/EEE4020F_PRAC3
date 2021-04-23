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
// THRMAT007 & LKYROS001
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
 if(!Input.Read("Data/fly.jpg")){
  printf("Cannot read image\n");
  return;
 }

 // declaring all constants
 int numcols = Input.Width * Input.Components;
 int stop;
 int start = 0; // inital start constant
tic();
// splitting and sending data to threads.
for(int j = 1; j < numprocs; j++){
    //tic();
    stop = Input.Height*j/(numprocs-1); // getting end condition
    int numrows = stop - start; // getting width of section
    int split = numrows*numcols; // getting total number of elements
    unsigned char subsection[split]; // create buffer to store all elements 
    int datavals[3] = {start,stop, numcols }; //data values to be passed to mpi thread

     //convert to 1D array:
    int offset = 0; // offset
    for (int y = start; y < stop; y++){
        for (int x = 0; x < numcols; x++, offset++){
            subsection[offset] = Input.Rows[y][x]; // populate 1d array
        }
    }

    MPI_Send(datavals, 3, MPI_INT, j, TAG, MPI_COMM_WORLD); // using MPI_Int so i dont need to worry about sizes
    MPI_Send(subsection, split, MPI_CHAR, j, TAG, MPI_COMM_WORLD);

    //printf("Node %d Time = %lg ms\n",j, (double)toc()/1e-3);
    start = stop-1; // over overlap since row = stop1 wont be included, as edges are exculuded, so include missed row in start2 to ensure there arent missing rows in middle of picture.
}

 // Allocated RAM for the output image
 if(!Output.Allocate(Input.Width, Input.Height, Input.Components)) return;

// receiving and compiling picture from threads.
for (int j=1; j < numprocs; j++){
    //tic();
    int datavals[3]; // for return array of data values
    MPI_Recv(datavals, 3, MPI_INT, j, TAG, MPI_COMM_WORLD, &stat);

    // pull variables out of array to make life easy with resusing names of variables, for continuity .
    start = datavals[0];
    stop = datavals[1];
    int numrows = stop - start;
    int split = numrows*numcols;
    unsigned char subsection[split];

    MPI_Recv(subsection, split, MPI_CHAR, j, TAG, MPI_COMM_WORLD, &stat);

    int offset = numcols; // offset used to not include the empty "row 0" each subsection has.
    for (int x = start+1; x < stop; x++){
        for (int y = 0; y< numcols; y++, offset++){
            Output.Rows[x][y] = subsection[offset];
        }
    }
}

 // Write the output image
 if(!Output.Write("Data/Output.jpg")){
  printf("Cannot write image\n");
  return;
 }

printf("Node %d Time = %lg ms\n",j, (double)toc()/1e-3);
printf("Done");

 //! <h3>Output</h3> The file Output.jpg will be created on success to save
 //! the processed output.
}

unsigned char medianfilter( unsigned char* filter){ //calculate median of values
for (int i = 0; i<9; i++){ //sorting arrray
                for (int j = i+1; j<9; j++){
                    if (filter[i]>filter[j]){
                        unsigned char temp = filter[i];
                        filter[i] = filter[j];
                        filter[j] = temp;
                    }
                }
            }
return(filter[4]); // return middle element, ie the median
}


/** This is the Slave function, the workers of this MPI application. */
void Slave(int ID){

    MPI_Status stat;
    int datavals[3]; // array to recieving data values

    MPI_Recv(datavals, 3, MPI_INT, 0, TAG, MPI_COMM_WORLD, &stat); // using MPI_Int so i dont need to worry about sizes
    
    int start = datavals[0]; // taking variables out of array for ease of association.  can copy past code from Master without to much editing
    int stop = datavals[1];
    int numcols = datavals[2];
    int numrows = stop - start;
    int split = numrows*numcols;
    unsigned char subsection[split];

    MPI_Recv(subsection, split, MPI_BYTE, 0, TAG, MPI_COMM_WORLD, &stat);
    unsigned char ** image = new unsigned char*[numrows];
    unsigned char ** output = new unsigned char*[numrows];
    unsigned char filter[9]; // array to be send to median filter

    int offset = 0;
    int pos = 0;
    for (int x = 0; x < numrows; x++){
        image[x] = new unsigned char[numcols];
        output[x] = new unsigned char[numcols];
        for (int y = 0; y < numcols; y++, offset++){
            image[x][y] = subsection[offset]; // converting from 1D back to 2D
        }
    }
    //starting at pos 3, since the previous 3 values make up datapoint 1, so starting at datapoint 2 to avoid edge issues
    for (int x = 1; x < numrows-1 ; x++){ 
        for (int y = 3; y < numcols - 3; y++){
            offset = 0;

            for (int k = -1; k <= 1; k++){ //getting the data values next to the datapoint, ie above,below,left and right ie
                for (int j =-3; j<=3; j+=3){ // single datapoint of image is split into rgb values (R,G,B), thus moving 3 over to compare R to R, and like wise for G and B
                    filter[offset] = image[x+k][y+j];
                    ++offset;
                }
            }
            output[x][y] = medianfilter(filter); // getting median
        }
    }
    offset = 0;
    for (int x = 0; x < numrows-1; x++){
        for (int y = 0; y< numcols; y++, offset++){
            subsection[offset] = output[x][y]; //converting again from 2D to 1D
        }
    }

    MPI_Send(datavals, 3, MPI_INT, 0, TAG, MPI_COMM_WORLD);
    MPI_Send(subsection, split, MPI_CHAR, 0, TAG, MPI_COMM_WORLD);
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
