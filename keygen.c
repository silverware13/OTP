// keygen:
// This program creates a key file of specified length.
// The characters in the file generated will be any of the
// 27 allowed characters, generated using the standard UNIX 
// randomization methods.
// 
// Input:
// The length of the key file.
//
// Return:
// A random string of characters that ends with a newline.
//
// Zachary Thomas
// Assignment 4
// 3/3/2018

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main (int argc, char *argv[]) {

	srand(time(0)); // Seed our RNG.
	
	// Array of valid ranodom chararacters.
	char ranArray[28] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char outStr[1000];
	int charCount = 20;	

	// Get the requested amount of random numbers.
	int i, rnd;
	for(i = 0; i < charCount; i++) {

		rnd = rand() % 27 // Returns a number from 0 to 26.
		printf("%c", ranArray[rnd]); // Print a random char from ranArray. 	
		
	}

	printf("\n"); // End with a newline.
	
	return 0;	
}
