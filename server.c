/*
    Program for a simple bank server
    It uses sockets and threads
    The server will process simple transactions on a limited number of accounts

    Raziel Nicolás Martínez Castillo A01410695
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// Signals library
#include <errno.h>
#include <signal.h>
// Sockets libraries
#include <netdb.h>
#include <sys/poll.h>
// Posix threads library
#include <pthread.h>

// Custom libraries
#include "codes.h"
#include "sockets.h"

#define MAX_ACCOUNTS 5
#define BUFFER_SIZE 1024
#define MAX_QUEUE 5
#define MAX_PLAYERS 8

///// Structure definitions

// Data for a single bank account
typedef struct account_struct {
    int id;
    int pin;
    float balance;
} account_t;

// Data for the bank operations
typedef struct bank_struct {
    // Store the total number of operations performed
    int total_transactions;
    // An array of the accounts
    account_t * account_array;
} bank_t;

// Structure for the mutexes to keep the data consistent
typedef struct locks_struct {
    // Mutex for the number of transactions variable
    pthread_mutex_t transactions_mutex;
    // Mutex array for the operations on the accounts
    pthread_mutex_t * account_mutex;
} locks_t;

// // Data that will be sent to each thread
// typedef struct data_struct {
//     // The file descriptor for the socket
//     int connection_fd;
//     // A pointer to a bank data structure
//     bank_t * bank_data;
//     // A pointer to a locks structure
//     locks_t * data_locks;
// } thread_data_t;

typedef struct viuda_struct {

    int bet;
    int betAgreement;
    int lowestAmount;

} viuda_t;

// Data that will be sent to each thread
typedef struct data_struct {
    // The file descriptor for the socket
    message_t message;
    int connection_fd;
    int connectionNumber;
    int player;
    int agreeBet;
    int prize; //The amount to give to the winner
    // // A pointer to a bank data structure
    // bank_t * bank_data;
    viuda_t * viuda_data;
    // // A pointer to a locks structure
    // locks_t * data_locks;
} thread_data_t;


// Global variables for signal handlers
int interrupt_exit = 0;


///// FUNCTION DECLARATIONS
void usage(char * program);
void setupHandlers();
void detectInterruption(int signal);
void initBank(bank_t * bank_data, locks_t * data_locks);
void readBankFile(bank_t * bank_data);
// void waitForConnections(int server_fd, bank_t * bank_data, locks_t * data_locks);
void waitForConnections(int server_fd, viuda_t * viuda_data);
void * attentionThread(void * arg);
void closeBank(bank_t * bank_data, locks_t * data_locks);
int checkValidAccount(int account);
void storeChanges(bank_t * bank_data);
void completeFirstDeal(message_t * message, int connection_fd);
/*
    TODO: Add your function declarations here
*/
void calculateResults(message_t * message);
void dealerTurn(message_t * message, int connection_fd);
void playerTurn(message_t * message, int connection_fd);
int getRandomCard(message_t * message, char pord);


///// MAIN FUNCTION
int main(int argc, char * argv[])
{
    int server_fd;
    // bank_t bank_data;
    // locks_t data_locks;

    viuda_t viuda_data;

    printf("\n=== SERVER PROGRAM ===\n");

    // Check the correct arguments
    if (argc != 2)
    {
        usage(argv[0]);
    }

    // Configure the handler to catch SIGINT
    // setupHandlers();

    // Initialize the data structures
    // initBank(&bank_data, &data_locks);

	// Show the IPs assigned to this computer
	printLocalIPs();
    // Start the server
    server_fd = initServer(argv[1], MAX_QUEUE);
	// Listen for connections from the clients
    // waitForConnections(server_fd, &bank_data, &data_locks);
    waitForConnections(server_fd, &viuda_data);

    printf("Closing the server socket\n");
    // Close the socket
    close(server_fd);

    // Clean the memory used
    // closeBank(&bank_data, &data_locks);

    printf("byeeeeee\n");
    // Finish the main thread
    pthread_exit(NULL);

    printf("ADIOS\n");

    return 0;
}

///// FUNCTION DEFINITIONS

/*
    Explanation to the user of the parameters required to run the program
*/
void usage(char * program)
{
    printf("Usage:\n");
    printf("\t%s {port_number}\n", program);
    exit(EXIT_FAILURE);
}

void detectInterruption(int signal)
{
    printf("\nGot an interruption\n");
    interrupt_exit = 1;
}

/*
    Modify the signal handlers for specific events
*/
void setupHandlers()
{
    struct sigaction new_action;

    // Clear the structure
    bzero(&new_action, sizeof new_action);
    // Indicate the handler function to use
    new_action.sa_handler = detectInterruption;
    // Set a mask to block signals
    sigfillset(&new_action.sa_mask);

    // Establish the handler in my program
    sigaction(SIGINT, &new_action, NULL);

}



/*
    Function to initialize all the information necessary
    This will allocate memory for the accounts, and for the mutexes
*/
void initBank(bank_t * bank_data, locks_t * data_locks)
{
    // Set the number of transactions
    bank_data->total_transactions = 0;

    // Allocate the arrays in the structures
    bank_data->account_array = malloc(MAX_ACCOUNTS * sizeof (account_t));
    // Allocate the arrays for the mutexes
    data_locks->account_mutex = malloc(MAX_ACCOUNTS * sizeof (pthread_mutex_t));

    // Initialize the mutexes, using a different method for dynamically created ones
    //data_locks->transactions_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_init(&data_locks->transactions_mutex, NULL);
    for (int i=0; i<MAX_ACCOUNTS; i++)
    {
        //data_locks->account_mutex[i] = PTHREAD_MUTEX_INITIALIZER;
        pthread_mutex_init(&data_locks->account_mutex[i], NULL);
        // Initialize the account balances too
        bank_data->account_array[i].balance = 0.0;
    }

    // Read the data from the file
    readBankFile(bank_data);
}


/*
    Get the data from the file to initialize the accounts
*/
void readBankFile(bank_t * bank_data)
{
    FILE * file_ptr = NULL;
    char buffer[BUFFER_SIZE];
    int account = 0;
    char * filename = "accounts.txt";

    file_ptr = fopen(filename, "r");
    if (!file_ptr)
    {
        perror("ERROR: fopen");
    }

    // Ignore the first line with the headers
    fgets(buffer, BUFFER_SIZE, file_ptr);
    // Read the rest of the account data
    while( fgets(buffer, BUFFER_SIZE, file_ptr) )
    {
        sscanf(buffer, "%d %d %f", &bank_data->account_array[account].id, &bank_data->account_array[account].pin, &bank_data->account_array[account].balance); 
        account++;
    }
    
    fclose(file_ptr);
}


/*
    Main loop to wait for incomming connections
*/
// void waitForConnections(int server_fd, bank_t * bank_data, locks_t * data_locks)
void waitForConnections(int server_fd, viuda_t * viuda_data)
{
    struct sockaddr_in client_address;
    socklen_t client_address_size;
    char client_presentation[INET_ADDRSTRLEN];
    int client_fd;
    // pthread_t new_tid;
    // int poll_response;
	// int timeout = 500;		// Time in milliseconds (0.5 seconds)
    int connectionsNum = 0;
    int status;

    // Create a structure array to hold the file descriptors to poll
    // struct pollfd test_fds[1];
    // // Fill in the structure
    // test_fds[0].fd = server_fd;
    // test_fds[0].events = POLLIN;    // Check for incomming data

    // Get the size of the structure to store client information
    // connection_data = malloc(MAX_PLAYERS * sizeof(thread_data_t));

    while (1)
    {
        // if(interrupt_exit) {
        //     printf("Interrupted\n");
        //     break;
        // } 

        // poll_response = poll(test_fds, 1, timeout);
        // if (poll_response == 0)
        // {
        //     printf(". ");
        //     fflush(stdout);
        // }
        // else if (poll_response > 0) //if something was received
        // {
            client_address_size = sizeof client_address;

            // ACCEPT
            // Wait for a client connection
            client_fd = accept(server_fd, (struct sockaddr *)&client_address, &client_address_size);
            if (client_fd == -1)
            {
                perror("ERROR: accept");
                close(client_fd);
            }
            
            // Get the data from the client
            inet_ntop(client_address.sin_family, &client_address.sin_addr, client_presentation, sizeof client_presentation);
            printf("Received incomming connection from %s on port %d\n", client_presentation, client_address.sin_port);

            printf("CLIENT_FD: %d\n", client_fd);
            pthread_t tid;
            thread_data_t * connection_data = NULL;
            connection_data = malloc (sizeof (thread_data_t));
            // Prepare the structure to send to the thread
            connection_data->connection_fd = client_fd;
            connection_data->connectionNumber = connectionsNum;
            connection_data->viuda_data = viuda_data;
            connection_data->player = connectionsNum;
            
            // CREATE A THREAD
            status = pthread_create(&tid, NULL, attentionThread, connection_data);

            if (status == -1)
            {
                perror("ERROR: pthread_create");
                close(client_fd);
            }    
            connectionsNum++;  
        // }
        // else
        // {
        //     if (errno == EINTR) // if the poll gets interrupted, break while
        //     {
        //         printf("Error Interrupt\n");
        //         break;
                
        //     }
        //     else
        //     {
        //         perror("ERROR: poll");
        //         exit(EXIT_FAILURE);
        //     }
        // }
    }
    // Show the number of total transactions
    // printf("Transactions made: %d\n", bank_data->total_transactions);

    // free(connection_data);

    // Store any changes in the file
    // storeChanges(bank_data);
}

/*
    Hear the request from the client and send an answer
*/
void * attentionThread(void * arg)
{  
    // Receive the data for the bank, mutexes and socket file descriptor
    thread_data_t * info = arg;
    int round = 0;
    printf("\nSTARTED THREAD WITH CONNECTION: %d\n", info->connection_fd);
    // response_t response;
    // // Create a structure array to hold the file descriptors to poll
    // struct pollfd test_fds[1];
    // // Fill in the structure
    // test_fds[0].fd = info->connection_fd;
    // test_fds[0].events = POLLIN;    // Check for incomming data
    // int poll_response;
    // int timeout = 500;

    recvData(info->connection_fd, &info->message, sizeof info->message); //receive PLAY
    // Validate that the client corresponds to this server
    if (info->message.msg_code != PLAY)
    {
        printf("Error: unrecognized client\n");
        send(info->connection_fd, &info->message, sizeof info->message, 0);
        pthread_exit(NULL);
    }

    // Prepare a reply
    info->message.msg_code = AMOUNT;
    send(info->connection_fd, &info->message, sizeof info->message, 0);

    // Get the reply and validate again receives AMOUNT
    recvData(info->connection_fd, &info->message, sizeof info->message);
    if (info->message.msg_code != AMOUNT)
    {
        printf("Error: unrecognized client\n");
        close(info->connection_fd);
        pthread_exit(NULL);
    }

    printf("The starting amount of the player is: %d\n", info->message.playerAmount);

    if(info->viuda_data->lowestAmount > info->message.playerAmount) {
        info->viuda_data->lowestAmount = info->message.playerAmount;
    }

    printf("The players can bet at most %d.\n", info->viuda_data->lowestAmount);

    // Prepare a reply
    info->message.playerStatus = START;
    info->message.dealerStatus = START;
    send(info->connection_fd, &info->message, sizeof info->message, 0);

    // printf("\tRunning thread with connection_fd: %d\n", info->connection_fd);

    // Loop to listen for messages from the client
    while(info->message.playerAmount >= 2){

        round++;

        printf("\n|||||||||||||||ROUND %d|||||||||||||||\n", round);

        //Get the bet and player status from the client
        printf("\n/////Getting player's bet/////\n");

        // if(interrupt_exit) {
        //     printf("Interrupted\n");
        //     break;
        // } 

        // poll_response = poll(test_fds, 1, timeout);
        // if (poll_response == 0)
        // {
        //     printf(". ");
        //     fflush(stdout);
        // }
        // else if (poll_response > 0) //if something was received
        // {
            // Receive the request
            if (!recvData(info->connection_fd, &info->message, sizeof info->message)){
                pthread_exit(NULL);
                // return;
            }

            printf("The bet of the player is: %d\n", info->message.playerBet);

            completeFirstDeal(&info->message, info->connection_fd); //Generates the first 2 cards of the Player and Dealer

            if((info->message.dealerStatus != NATURAL) && (info->message.playerStatus != NATURAL)){ // if no one got a natural blackjack
                printf("\n/////PLAYER'S TURN/////\n");
                playerTurn(&info->message, info->connection_fd); //Update player info based on the options chosen by the user

                //Calculate dealers hands and total accumulated
                printf("\n/////DEALER'S TURN/////\n");
                dealerTurn(&info->message, info->connection_fd); //Update dealer's info when player's turn is over
            }

            //Take decision based on the status 
            printf("\n/////TAKING DECISION BASED ON THE STATUS/////\n");
            calculateResults(&info->message);

            if(info->message.playerAmount < 2){ //The client disconnects when the player doesn't have enough chips
                printf("The player doesn't have enough money to keep playing. The player will exit now.\n");
            } 

            send(info->connection_fd, &info->message, sizeof info->message, 0);


    //         // Update the number of transactions
    //         if(response == OK){
    //             pthread_mutex_lock(&info->data_locks->transactions_mutex);
    //             info->bank_data->total_transactions++;
    //             pthread_mutex_unlock(&info->data_locks->transactions_mutex);
    //         } else if(response == NO_ACCOUNT){
    //             printf("Invalid acount number received.\n");
    //         }

    //         // Prepare the message to the client
    //         sprintf(buffer, "%d %f", response, balance);

    //         // Send a reply
    //         sendData(info->connection_fd, buffer, strlen(buffer)+1);
    //     }
    //     else
    //     {
    //         if (errno == EINTR) // if the poll gets interrupted, break while
    //         {
    //             printf("Error Interrupt\n");
    //             break;
                
    //         }
    //         else
    //         {
    //             perror("ERROR: poll");
    //             exit(EXIT_FAILURE);
    //         }
    //     }
    }

    printf("SALIOO\n");

    printf("\nENDING THREAD WITH CONNECTION: %d\n", info->connection_fd);

    // Finish the connection
    if (!recvData(info->connection_fd, &info->message, sizeof info->message))
    {
        printf("DENTRO DE FUNCION\n");
        printf("Client disconnected\n");
    }
    info->message.msg_code = BYE;
    send(info->connection_fd, &info->message, sizeof info->message, 0);
    close(info->connection_fd);

    pthread_exit(NULL);
}

int getRandomCard(message_t * message, char pord){ //pord stands for player or dealer

    int newCard;
    char randomCard[3]; 

    const char cardsArray[13][3]= {"A","2","3","4","5","6","7","8","9","10","J","Q","K"};

    strcpy(randomCard, cardsArray[rand() % 13]); //Pick a random card from the array

    if(strcmp(randomCard, "A") == 0){
        newCard = 11; //Ace is equal to 11
    } else if ((strcmp(randomCard, "10") == 0) || (strcmp(randomCard, "J") == 0) || (strcmp(randomCard, "Q") == 0) || (strcmp(randomCard, "K") == 0)) {
        newCard = 10; //10, J, Q, and K are equal to 10
    } else {
        newCard = atol(randomCard); //Convert string to integer
    }

    if(pord == 'p'){ //If the player called the function
        strcpy((message->playerCards)[message->numPlayerCards], randomCard);
        if(message->numPlayerCards >= 2){
            printf("New card is: [%s].", randomCard);
        }
        (message->numPlayerCards)++;
        if((newCard == 11) && (message->totalPlayer + 11 > 21)){ //If ace + current amount > 21, then ace value is 1
            newCard = 1;
        }
    } else if(pord == 'd'){ //If the dealer called the function
        if(message->numDealerCards >= 2){
            printf("Dealer gets new card: [%s].", randomCard);
        }
        strcpy((message->dealerCards)[message->numDealerCards], randomCard);
        (message->numDealerCards)++;
        if((newCard == 11) && (message->totalDealer + 11 > 21)){ //If ace + current amount > 21, then ace value is 1
            newCard = 1;
        }
    }

    return newCard;
}

//Generates the first 2 cards of the Player and Dealer and tells if someone got a Natural Blackjack
void completeFirstDeal(message_t * message, int connection_fd){

    //Reset number of cards of the player and dealer from the previous round
    message->numPlayerCards = 0;
    message->numDealerCards = 0;
 
    message->totalPlayer = 0;
    message->totalDealer = 0;

    srand(time(NULL));

    for(int i = 0; i<2; i++){
        message->totalPlayer += getRandomCard(message, 'p');
        message->totalDealer += getRandomCard(message, 'd');
    }

    //Check for Natural blackjacks
    if(message->totalPlayer == 21) {
        printf("The player got a Natural Blackjack with the cards [%s] and [%s]!\n", message->playerCards[0], message->playerCards[1]);
        message->playerStatus = NATURAL;
    }

    if(message->totalDealer == 21) {
        message->dealerStatus = NATURAL;
        printf("The dealer got a Natural Blackjack with the cards [%s] and [%s]!\n", message->dealerCards[0], message->dealerCards[1]);
    }

    //If there is send the status to the client
    if((message->totalPlayer == 21) || (message->totalDealer == 21)) {
        send(connection_fd, message, sizeof (*message), 0);
    }

}

void playerTurn(message_t * message, int connection_fd){

    int newCard = 0;

    //Initial deal
    printf("Initial hand of the player: [%s] [%s] ", message->playerCards[0], message->playerCards[1]);
    printf("summing a total of: %d\n", message->totalPlayer);

    //Sends the total hand accumulated by the player
    send(connection_fd, message, sizeof (*message), 0);
    //Gets the status chosen by the player
    recvData(connection_fd, message, sizeof (* message));
    // Send the corresponding reply
    while (message->playerStatus == HIT)
    {
        printf("The player with a total of %d chose to get a card.\n", message->totalPlayer);
        newCard = getRandomCard(message, 'p');
        message->totalPlayer += newCard;
        printf(" The new total of this player is: %d.\n", message->totalPlayer);

        if (message->totalPlayer == 21) {
            message->playerStatus = TWENTYONE;
            printf("The current player got 21!\n");
            send(connection_fd, message, sizeof (*message), 0);
            break;
        } else if (message->totalPlayer > 21) {
            message->playerStatus = BUST;
            printf("The current player busted, he is over 21.\n");
            send(connection_fd, message, sizeof (*message), 0);
            break;
        }

        //Sends the status calculated by the server
        send(connection_fd, message, sizeof (*message), 0);

        //Gets the status chosen by the player
        recvData(connection_fd, message, sizeof (*message));

        if(message->playerStatus == STAND) {
            printf("Player chose to stay with %d.\n", message->totalPlayer);
        }
    }

    printf("After completing his/her turn the player accumulated the cards:");
    for(int i = 0; i<message->numPlayerCards; i++){
        printf(" [%s]", message->playerCards[i]);
    }
    printf(" which sum a total of: %d\n", message->totalPlayer);
}

void dealerTurn(message_t * message, int connection_fd){ //Automatic deicisions based on Blackjack rules

    int newCard = 0;

    //if the player turn is over
    if((message->playerStatus == STAND) || (message->playerStatus == NATURAL) || (message->playerStatus == TWENTYONE)) {

        printf("Initial dealer's hand: [%s] [%s] making a total of: %d\n", message->dealerCards[0], message->dealerCards[1], message->totalDealer);

        message->dealerStatus = HIT;
        while((message->dealerStatus != STAND) && (message->dealerStatus != BUST) && (message->dealerStatus != TWENTYONE)){
            if((message->totalDealer >= 17) && (message->totalDealer < 21)){
                printf("The dealer stays with a total of: %d\n", message->totalDealer);
                message->dealerStatus = STAND;
            } else if (message->totalDealer > 21) {
                printf("The dealer exceeds 21 with %d and busts.\n", message->totalDealer);
                message->dealerStatus = BUST;
            } else if(message->totalDealer == 21) {
                printf("The dealer got %d!\n", message->totalDealer);
                message->dealerStatus = TWENTYONE;
            } else {
                newCard = getRandomCard(message, 'd'); //If dealer status == HIT
                message->totalDealer += newCard;
                printf(" New dealer's total: %d\n", message->totalDealer);
            }
        }
    }

    printf("After completing the turn dealer accumulated the cards:");
    for(int i = 0; i<message->numDealerCards; i++){
        printf(" [%s]", message->dealerCards[i]);
    }
    printf(" which sum a total of: %d\n", message->totalDealer);
}

void calculateResults(message_t * message){

    if (message->playerStatus == BUST){
        printf("The dealer gets the player's bet.\n");
        message->playerAmount -= message->playerBet;
    } else if((message->dealerStatus == BUST) && (message->playerStatus == STAND)){
        printf("The dealer gives the player his bet plus the amount of his bet.\n");
        message->playerAmount += message->playerBet;
    } else if (((message->dealerStatus == STAND) || (message->dealerStatus == TWENTYONE)) && ((message->playerStatus == STAND) || (message->playerStatus == TWENTYONE))){
        if(message->totalPlayer > message->totalDealer){
            printf("The dealer gives the player his bet plus the amount of his bet.\n");
            message->playerAmount += message->playerBet;
        }else if (message->totalPlayer < message->totalDealer) {
            printf("The dealer gets the player's bet.\n");
            message->playerAmount -= message->playerBet;
        } else if (message->totalPlayer == message->totalDealer){
            printf("This is stand-off. The player gets his bet back.\n");
        }
    } else if ((message->dealerStatus == NATURAL) && (message->playerStatus != NATURAL)){
        printf("The dealer gets the player's bet.\n");
        message->playerAmount -= message->playerBet;
    } else if ((message->dealerStatus != NATURAL) && (message->playerStatus == NATURAL)){
        printf("The dealer gives the player his bet plus 1.5x the amount of his bet.\n");
        message->playerAmount += (message->playerBet * 1.5);
    } else if ((message->dealerStatus == NATURAL) && (message->playerStatus == NATURAL)){
        printf("This is PUSH. The player gets his bet back.\n");
    }
}

/*
    Free all the memory used for the bank data
*/
void closeBank(bank_t * bank_data, locks_t * data_locks)
{
    printf("DEBUG: Clearing the memory for the thread\n");
    free(bank_data->account_array);
    free(data_locks->account_mutex);
}


/*
    Return true if the account provided is within the valid range,
    return false otherwise
*/
int checkValidAccount(int account)
{
    return (account >= 0 && account < MAX_ACCOUNTS);
}
