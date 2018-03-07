// otp_enc:
// This program connects to otp_enc_d, and asks it to
// perform a one-time pad style encryption. By itself, 
// otp_enc doesn’t do the encryption - otp_end_d does.
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

void error(const char *msg, int errVal) { perror(msg); exit(errVal); } // Error function used for reporting issues

int main(int argc, char *argv[])
{

	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[500000];
	char keyBuffer[500000];
 
	if (argc < 3) { fprintf(stderr,"USAGE: %s plaintext key port\n", argv[0]); exit(0); } // Check usage & args

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("CLIENT: ERROR opening socket", 1);
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		error("CLIENT: ERROR connecting", 1);
	
	// Get input from plain text file.
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
	FILE *plainText = fopen(argv[1], "r"); // Open the plain text file.
	if(plainText == 0) error("CLIENT: ERROR could not open plain text file", 1);
	while(fgets(buffer, sizeof(buffer)-1, plainText)); // Get input from plain text file, trunc to buffer -1 chars, leaving \0.
	fclose(plainText); // Close the plain text file.
	
	// Get input from key file.
	memset(keyBuffer, '\0', sizeof(keyBuffer)); // Clear out the buffer array
	FILE *keyText = fopen(argv[2], "r"); // Open the key file.
	if(keyText == 0) error("CLIENT: ERROR could not open key file", 1);
	while(fgets(keyBuffer, sizeof(keyBuffer)-1, keyText)); // Get input from key file, trunc to buffer -1 chars, leaving \0.
	keyBuffer[strcspn(keyBuffer, "\n")] = '\0'; // Replace the newline at the end of file with NULL.
	fclose(keyText); // Close the key file.
	
	// Make sure that our key file is as large if not larger than our plain text file.
	int keyLen = strlen(keyBuffer);
	int plnLen = strlen(buffer);
	if (plnLen > keyLen) error("Error: key is too short", 1);
	
	// Send plain text to server.
	charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket", 1);
	if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");

	// Send key to server.
	charsWritten = send(socketFD, keyBuffer, strlen(keyBuffer), 0); // Write to the server
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket", 1);
	if (charsWritten < strlen(keyBuffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");
	
	// Get return message from server
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
	if (charsRead < 0) error("CLIENT: ERROR reading from socket", 1);
	if(buffer[0] == 'B' && buffer[1] == 'A' && buffer[2] == 'D') error("CLIENT: Connection rejected on port %d", 2);
	printf("%s\n", buffer);

	close(socketFD); // Close the socket
	return 0;

}
