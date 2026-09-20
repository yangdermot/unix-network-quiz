#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "QuizDB.h"

#define BUFSIZE 676

// remove newline character from end of string as the client uses enter 
void remove_newline(char *s)
{
    s[strcspn(s, "\n")] = '\0';
    s[strcspn(s, "\r")] = '\0';
}

int main(int argc, char *argv[])
{
    int listenSocket, clientSocket;
    struct sockaddr_in serverAddr;
    char message[BUFSIZE];
    int totalQs, randomQs[5];
    int i;

    // makes sure the user just runs the required arguements the IPv4 address and port number
    // there should be 3 argurments
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <IPv4-address> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // initialise random number generator
    srand(time(NULL));

    // create the IPv4 TCP socket for the server
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // we are buildling the server's IP address and port to match the sockets format
    // clear the struct making ecerything 0 
    memset(&serverAddr, 0, sizeof(serverAddr));
    // setting the address to IPv4 address
    serverAddr.sin_family = AF_INET;
    // making the port number an integer and then putting it into network formatt for the bind() function
    serverAddr.sin_port = htons(atoi(argv[2]));

    // Convert the IP address from text form to binary form
    // and store it in the sin_addr field of the struct
    // also error checks it
    if (inet_pton(AF_INET, argv[1], &serverAddr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(listenSocket);
        exit(EXIT_FAILURE);
    }

    // connect the socket to an address that the client will use
    // error checks
    if (bind(listenSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        perror("bind");
        close(listenSocket);
        exit(EXIT_FAILURE);
    }

    // ready the server for clients, waiting for the accept(), let the waiting queue be up to 5 
    // error checks 
    if (listen(listenSocket, 5) == -1)
    {
        perror("listen");
        close(listenSocket);
        exit(EXIT_FAILURE);
    }

    // print given status 
    printf("<Listening on %s:%s>\n", argv[1], argv[2]);
    printf("<Press ctrl-C to terminate>\n");

    // get total number of questions
    totalQs = sizeof(QuizQ) / sizeof(QuizQ[0]);

    // flow through all the clients with the quiz
    while (1)
    {
        int score = 0;
        int response;

        // accept the client connection
        clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == -1)
        {
            perror("accept");
            continue;
        }

        // place given intro in buffer
        snprintf(message, sizeof(message),
                 "Welcome to Unix Programming Quiz!\n"
                 "The quiz comprises five questions posed to you one after the other.\n"
                 "You have only one attempt to answer a question.\n"
                 "Your final score will be sent to you after conclusion of the quiz.\n"
                 "To start the quiz, press Y and <enter>.\n"
                 "To quit the quiz, press q and <enter>.\n");
        
        // send welcome text thru the client socket
        // error checks
        if (write(clientSocket, message, strlen(message)) == -1)
        {
            perror("write");
            close(clientSocket);
            continue;
        }

        // read the clients response 
        // error checks
        response = read(clientSocket, message, sizeof(message) - 1);
        if (response <= 0)
        {
            close(clientSocket);
            continue;
        }
        // give the string a null terminator and remove newline 
        message[response] = '\0';
        remove_newline(message);

        // quit the connection if client inputs q
        // both lowercase uppercase work
        if (strcasecmp(message, "q") == 0)
        {
            close(clientSocket);
            continue;
        }

        // if the clients input is not y we close the connection
        if (strcasecmp(message, "Y") != 0)
        {
            close(clientSocket);
            continue;
        }

        // my random question chooser 
        {
            int count = 0;

            // get 5 questions
            while (count < 5)
            {
                // get random question index, confirm if used or not
                int r = rand() % totalQs;
                int used = 0;

                // check if question is used or not 
                for (i = 0; i < count; i++)
                {
                    if (randomQs[i] == r)
                    {
                        used = 1;
                        break;
                    }
                }

                // if not used we add it to the questions for the client and increase the count till its 5 questions
                if (used == 0)
                {
                    randomQs[count] = r;
                    count++;
                }
            }
        }

        // go thru the 5 chosen questions
        for (i = 0; i < 5; i++)
        {
            int qindex = randomQs[i];

            // gets current question in the buffer 
            snprintf(message, sizeof(message), "%s\n", QuizQ[qindex]);

            // send it to client through the clients socket 
            if (write(clientSocket, message, strlen(message)) == -1)
            {
                perror("write");
                break;
            }

            // read the clients response 
            response = read(clientSocket, message, sizeof(message) - 1);
            if (response <= 0)
                break;

            // apply null terminator to string and remove the newline
            message[response] = '\0';
            remove_newline(message);

            // compare the clients answer with the quiz result
            // if they got it right increase score, otherwise tell them the right answer
            // write the feedback into the buffer
            if (strcasecmp(message, QuizA[qindex]) == 0)
            {
                score++;
                snprintf(message, sizeof(message), "Right Answer.\n");
            }
            else
            {
                snprintf(message, sizeof(message),
                         "Wrong Answer. Right answer is %s.\n",
                         QuizA[qindex]);
            }
            
            // print the feedback thru the clients socket
            if (write(clientSocket, message, strlen(message)) == -1)
            {
                perror("write");
                break;
            }
        }
        
        // buffer message, clients quiz score 
        snprintf(message, sizeof(message),
                 "Your quiz score is %d/5. Goodbye!\n", score);

        // write to client
        if (write(clientSocket, message, strlen(message)) == -1)
            perror("write");

        // close the client and get next
        close(clientSocket);
    }
    // listen socket has to stay alive whole server run time so we close it last
    close(listenSocket);
    return 0;
}