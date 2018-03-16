// otp_enc_d:
// This program listens for messages from clients.
// When it recives a valid connection it takes in a message and encodes it.
// Returning the encoded message to the client.
//
// Zachary Thomas
// Assignment 4
// 3/3/2018

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

// The child connection function handles all message sending between client and server.
// INPUT: The number for the established connection.
void childConnection(int establishedConnectionFD) {

	// Setup char arrays to hold messages.
	int charsRead;
        int charsWritten;
	char buffer[1100000];
	char textBuffer[500001];
	char keyBuffer[500001];
	char encText[1100000];
	
	memset(buffer, '\0', sizeof(buffer)); // Clear our buffer
	memset(textBuffer, '\0', sizeof(textBuffer)); // Clear our textBuffer
	memset(keyBuffer, '\0', sizeof(keyBuffer)); // Clear our keyBuffer
	memset(encText, '\0', sizeof(encText)); // Clear our encText
	
	// Get the message from the client and display it.
	int bufLen; // Holds the buffer length.
	int bufSum = 0; // The number of chars we have writen to our buffer
	do {
		charsRead = recv(establishedConnectionFD, &buffer[bufSum], 1000, 0); // Read the client's message from the socket
		bufSum += charsRead; // Get the number of chars read and add it to our sum.
		bufLen = strlen(buffer); // Get the current length of the string in the buffer.
		if (charsRead < 0) error("ERROR reading from socket");
	} while (buffer[bufLen-1] != '@'); // Keep reading until we find the @ terminating char.
	
	// See if otp_enc is trying to connect, otherwise refuse connection.
	if (buffer[0] != 'e' || buffer[1] != '#') {
		charsWritten = send(establishedConnectionFD, "!@", strlen("!@"), 0);
		if (charsWritten < 0) error("ERROR writing to socket");
		close(establishedConnectionFD); // Close the existing socket which is connected to the client
		exit(1);
	}
	
	// Split buffer up based on special chars.
	int i = 0, ii = 0, iii = 0, saveMode = 0;
	char tempChar = '\0';
	for(i = 0; buffer[i] != '@'; i++) {
	 	
		// Set text save mode if we find '#'.
		if(buffer[i] == '#') {
			saveMode = 1;
			i++;
		}
		
		// Set key save mode if we find '%'.
		if(buffer[i] == '%') {
			saveMode = 2;
			i++;
		}

		// If we are looking at the text buffer start saving to textBuffer.
		if(saveMode == 1) {
			textBuffer[ii]  = buffer[i]; // Save the current char.
			ii++;
		}

		// If we are looking at the key buffer start saving to keyBuffer.
		if(saveMode == 2) {
			keyBuffer[iii]  = buffer[i]; // Save the current char.
			iii++;
		}

	} 	
		
	// Encrypt plain text with key.
	int valText = 0, valKey = 0, valSum = 0;
	char charArray[29] = "!ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
	for(i = 0; textBuffer[i] != '\0'; i++) {

		// Find the value of plain text char.
		for(ii = 1; charArray[ii] != '\0'; ii++) {
			if(textBuffer[i] == charArray[ii]) valText = ii;
		}
		
		// Find the value of key char.
		for(ii = 1; charArray[ii] != '\0'; ii++) {
			if(keyBuffer[i] == charArray[ii]) valKey = ii;
		}
 
		// Get the sum of our text and key chars.
		valSum = valText + valKey; // Get the sum of our two chars.
		while(valSum > 27) valSum += -27; // Never let our sum be greater than 27.
			
		// Find the encrypted char.
		for(ii = 1; charArray[ii] != '\0'; ii++) {
			if(valSum == ii) encText[i] = charArray[ii]; // Save the current encrypted char.
		}

	}

	// Send a the encrypted text back to the client
	int encLen = strlen(encText);
	do {
		charsWritten = send(establishedConnectionFD, encText, encLen, 0); // Write to client.
		if (charsWritten < 0) error("Error: writing to socket");
	} while (charsWritten < encLen);
	
	do {
		charsWritten = send(establishedConnectionFD, "@", strlen("@"), 0); // Write '@' at the end of our message to client.
		if (charsWritten < 0) error("Error: writing to socket");
	} while (charsWritten < strlen("@"));
	
	close(establishedConnectionFD); // Close the existing socket which is connected to the client
	
}

int main(int argc, char *argv[])
{
	
	// Set up variables.
	int listenSocketFD, establishedConnectionFD, portNumber, childSum = 0;
	int childExitMethod = -5;
	socklen_t sizeOfClientInfo;
	pid_t childPID;
	struct sockaddr_in serverAddress, clientAddress;
	
	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process
	
	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket");
			
	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) error("ERROR on binding"); // Connect socket to port.
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections
		
	// Keep listening until stopped manually
	while(1) {

		// Check if any children have died, adjust sum.
		while((childPID = waitpid(-1, &childExitMethod, WNOHANG)) > 0) childSum += -1; // Lower the sum of children for each one we reap. 
	
		// Don't allow more than five children to exist. 
		if(childSum < 5) {	

			// Accept a connection, blocking if one is not available until one connects
			sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
			establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
			if (establishedConnectionFD < 0) error("ERROR on accept");
			
			// Fork the accepted connection.
			pid_t spawnPID = -5;
			spawnPID = fork(); 
			
			// See if we are the parent or child.
			switch(spawnPID) {
		
				// Error could not fork.
				case -1: {
					perror("Fork failed.\n");
					exit(1);
					break;
				}	
		
				// This is the child.
				case 0: {
					childConnection(establishedConnectionFD); // The child handles messages from client to server.	
				}

				// This is the parent.
				default: {
					childSum += 1; // Increase the child count.
				}
			}
		} else {
			if((childPID = waitpid(-1, &childExitMethod, 0)) > 0) childSum += -1; // If we already have five children just wait here for a child to finish and reap it.
		}
	}
	
	return 0; 

}
