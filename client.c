#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFSIZE 676

int main(int argc, char *argv[])
{
    int clientSocket, serverMessage, i;
    struct sockaddr_in serverAddr;
    char message[BUFSIZE];
    char answer[BUFSIZE];

    // check if client inputs the right 3 arguements
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <IPv4-address> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // creates the client IPv4 TCP socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // using the struct to formatt the sockets IP address and port number
    memset(&serverAddr, 0, sizeof(serverAddr));
    // makes it an IPv4 address
    serverAddr.sin_family = AF_INET;
    // making the port number an integer and then putting it into network formatt
    serverAddr.sin_port = htons(atoi(argv[2]));

    // Convert the IP address from text form to binary form
    // and store it in the sin_addr field of the struct
    if (inet_pton(AF_INET, argv[1], &serverAddr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    // we are connecting the clients socket to the server address in serverAddr
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        perror("connect");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    // getting the welcome message from server and displaying it 
    serverMessage = read(clientSocket, message, sizeof(message) - 1);
    if (serverMessage <= 0)
    {
        close(clientSocket);
        exit(EXIT_FAILURE);
    }
    // apply null terminator and remove newline
    message[serverMessage] = '\0';
    printf("%s", message);

    // get the clients inputted response 
    if (fgets(answer, sizeof(answer), stdin) == NULL)
    {
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    // send the response thru the socket to the server
    if (write(clientSocket, answer, strlen(answer)) == -1)
    {
        perror("write");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    // if the user inputs q or Q we close the connection
    // only need to check the first incase of newline
    if (answer[0] == 'q' || answer[0] == 'Q')
    {
        close(clientSocket);
        exit(EXIT_SUCCESS);
    }

    // the 5 question loop
    for (i = 0; i < 5; i++)
    {
        // read the question from the serve, printing it 
        serverMessage = read(clientSocket, message, sizeof(message) - 1);
        if (serverMessage <= 0)
            break;
        message[serverMessage] = '\0';
        printf("%s", message);

        // get users typed response
        if (fgets(answer, sizeof(answer), stdin) == NULL)
            break;

        // write the response back to the server
        if (write(clientSocket, answer, strlen(answer)) == -1)
        {
            perror("write");
            close(clientSocket);
            exit(EXIT_FAILURE);
        }

        // client recieves the feedback if they are right or wrong, printing it
        serverMessage = read(clientSocket, message, sizeof(message) - 1);
        if (serverMessage <= 0)
            break;
        message[serverMessage] = '\0';
        printf("%s", message);
    }

    // recieve the final score for the client 
    serverMessage = read(clientSocket, message, sizeof(message) - 1);
    if (serverMessage > 0)
    {
        message[serverMessage] = '\0';
        printf("%s", message);
    }

    // close the connection beteen serve and client after 5 questions
    close(clientSocket);
    return 0;
}