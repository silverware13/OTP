// otp_enc_d:
// This program will run in the background as a daemon. 
// Upon execution, otp_enc_d must output an error if it 
// cannot be run due to a network error, such as the
// ports being unavailable. Its function is to perform
// the actual encoding.
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

int main(int argc, char *argv[])
{
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead, badConnection = 0;
	socklen_t sizeOfClientInfo;
	char buffer[500000];
	char keyBuffer[500000];
	char encText[500000];
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
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		error("ERROR on binding");
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

	// Accept a connection, blocking if one is not available until one connects
	sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
	establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
	if (establishedConnectionFD < 0) error("ERROR on accept");

	// Get the message from the client and display it
	memset(buffer, '\0', sizeof(buffer));
	memset(keyBuffer, '\0', sizeof(keyBuffer));
	memset(encText, '\0', sizeof(encText));
	charsRead = recv(establishedConnectionFD, buffer, 255, 0); // Read the client's message from the socket
	if (charsRead < 0) error("ERROR reading from socket");
	
	// Split buffer up based on newline.
	int i = 0, ii = 0, keySave = 0;
	char tempChar = '\0';
	for(i = 0; buffer[i] != '\0'; i++) {
	 	
		// Newline means we are starting into the key buffer.
		if(buffer[i] == '\n') {
			keySave = 1;
			buffer[i] = '\0';
			i++;
		}

		// If we are looking at the key buffer save it.
		if(keySave == 1) {
			keyBuffer[ii]  = buffer[i]; // Save the current char.
			ii++;
			buffer[i] = '\0';	
		}

	} 	
	
	//if (buffer[255] != 'e') {badConnection = 1;} // See if this is a valid connection.
	if (badConnection) {
		charsRead = send(establishedConnectionFD, "BAD", 3,0);
		if (charsRead < 0) error("ERROR writing to socket");
		close(establishedConnectionFD); // Close the existing socket which is connected to the client
		close(listenSocketFD); // Close the listening socket
		return 0;
	}
	
	if (!badConnection) { 
		
		printf("SERVER: Plain text from client: \"%s\"\n", buffer); // Only send message if valid connection.
		printf("SERVER: key from client: \"%s\"\n", keyBuffer); // Only send message if valid connection.

		// Encrypt plain text with key.
		int valText = 0, valKey = 0, valSum = 0;
		char charArray[29] = "!ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
		for(i = 0; buffer[i] != '\0'; i++) {

			// Find the value of plain text char.
			for(ii = 1; charArray[ii] != '\0'; ii++) {
				if(buffer[i] == charArray[ii]) valText = ii;
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

		// Send a Success message back to the client
		int encLen = strlen(encText);
		charsRead = send(establishedConnectionFD, encText, encLen, 0); // Send encrypted message back
		if (charsRead < 0) error("ERROR writing to socket");
		close(establishedConnectionFD); // Close the existing socket which is connected to the client
		close(listenSocketFD); // Close the listening socket
	
	}
	
	return 0; 

}
