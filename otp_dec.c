// otp_dec:
// This program will connect to otp_dec_d and will
// ask it to decrypt ciphertext using a passed-in 
// ciphertext and key, and otherwise performs exactly 
// like otp_enc. 
//
// Zachary Thomas
// Assignment 4
// 3/3/2018

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

void error(const char *msg, int errVal) { fprintf(stderr, "%s\n", msg); exit(errVal); } // Error function used for reporting issues

int main(int argc, char *argv[])
{
	// Setup variables.	
	int socketFD, portNumber, charsWritten, charsRead, i, ii;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[500000];
	char keyBuffer[500000];
 
	if (argc < 3) { fprintf(stderr,"USAGE: %s encryptedtext key port\n", argv[0]); exit(1); } // Check usage & args

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL)  error("Error: no such host\n", 1); 
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("Error: opening socket", 1);
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		error("Error: connecting", 1);
	
	// Get input from encrypted text file.
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
	FILE *encText = fopen(argv[1], "r"); // Open the encrypted text file.
	if(encText == 0) error("Error: could not open encrypted text file", 1);
	while(fgets(buffer, sizeof(buffer)-1, encText)); // Get input from encrypted text file, trunc to buffer -1 chars, leaving \0.
	fclose(encText); // Close the encrypted text file.

	// Make sure encrypted text file does not have any invalid chars. 
	char charArray[29] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ \n";
	int validChar;
	for(i = 0; buffer[i] != '\0'; i++) {

		validChar = 0;

		// See if the current char is valid.
		for(ii = 0; charArray[ii] != '\0'; ii++) {
			if(buffer[i] == charArray[ii]) validChar = 1;
		}
		
		// If we found an invalid char we throw an error here.
		if(!validChar) error("Error: input contains bad characters", 1);
		
	}
	if(i == 0) error("Error: encrypted text file is empty", 1);
	buffer[strcspn(buffer, "\n")] = '%'; // Replace the newline at the end of file with '%'.
	
	// Get input from key file.
	memset(keyBuffer, '\0', sizeof(keyBuffer)); // Clear out the buffer array
	FILE *keyText = fopen(argv[2], "r"); // Open the key file.
	if(keyText == 0) error("Error: could not open key file", 1);
	while(fgets(keyBuffer, sizeof(keyBuffer)-1, keyText)); // Get input from key file, trunc to buffer -1 chars, leaving \0.
	fclose(keyText); // Close the key file.
	
	// Make sure key file does not have any invalid chars. 
	for(i = 0; keyBuffer[i] != '\0'; i++) {

		validChar = 0;

		// See if the current char is valid.
		for(ii = 0; charArray[ii] != '\0'; ii++) {
			if(keyBuffer[i] == charArray[ii]) validChar = 1;
		}
		
		// If we found an invalid char we throw an error here.
		if(!validChar) error("Error: input contains bad characters", 1);
		
	}
	keyBuffer[strcspn(keyBuffer, "\n")] = '@'; // Replace the newline at the end of file with '@'.
	
	// Make sure that our key file is as large if not larger than our encrypted text file.
	int keyLen = strlen(keyBuffer);
	int plnLen = strlen(buffer);
	if (plnLen > keyLen) error("Error: key is too short", 1);

	// Send program id to server.
	do {
		charsWritten = send(socketFD, "d#", strlen("d#"), 0); // Write to the server
		if (charsWritten < 0) error("Error: writing to socket", 1);
	} while (charsWritten < strlen("d#"));
	
	// Send encrypted text to server.
	do {
		charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server
		if (charsWritten < 0) error("Error: writing to socket", 1);
	} while (charsWritten < strlen(buffer));

	// Send key to server.
	do {
		charsWritten = send(socketFD, keyBuffer, strlen(keyBuffer), 0); // Write to the server
		if (charsWritten < 0) error("Error: writing to socket", 1);
	} while (charsWritten < strlen(keyBuffer));
	
	// Get the message from the server and display it.
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	int bufLen; // Holds the buffer length.
	int bufSum = 0; // The number of chars we have writen to our buffer
	do {
		charsRead = recv(socketFD, &buffer[bufSum], 100, 0); // Read the client's message from the socket
		bufSum += charsRead; // Get the number of chars read and add it to our sum.
		bufLen = strlen(buffer); // Get the current length of the string in the buffer.
		if (charsRead < 0) error("ERROR reading from socket", 1);
	} while (buffer[bufLen-1] != '@'); // Keep reading until we find the @ terminating char.
	
	buffer[bufLen-1] = '\0'; // Replace @ at the end of our message with NULL.
	
	// Server has rejected this connection. Most likely we sent this to the wrong server.
	if(buffer[0] == '!') { fprintf(stderr, "Error: could not contact otp_dec_d on port %d\n", portNumber); exit(2); } 
	printf("%s\n", buffer); // Print the plain text.

	close(socketFD); // Close the socket
	return 0;

}
