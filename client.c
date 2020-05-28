/*
    Client program to access the accounts in the bank
    This program connects to the server using sockets

    Raziel Nicolás Martínez Castillo A01410695

    File made by professor Gilberto Echeverria
    
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// Sockets libraries
#include <netdb.h>
#include <arpa/inet.h>
// Custom libraries
#include "codes.h"
#include "sockets.h"

#define BUFFER_SIZE 1024

///// FUNCTION DECLARATIONS
void usage(char * program);
void communicationLoop(int connection_fd);
void showResults( message_t * message);
void playerTurn( message_t * message, int connection_fd);

///// MAIN FUNCTION
int main(int argc, char * argv[])
{
    int connection_fd;

    printf("\n=== CLIENT PROGRAM ===\n");

    // Check the correct arguments
    if (argc != 3)
    {
        usage(argv[0]);
    }

    // Start the server
    connection_fd = connectSocket(argv[1], argv[2]);
	// // Use the bank operations available
    // bankOperations(connection_fd);

    // Establish the communication
    communicationLoop(connection_fd);

    // Close the socket
    close(connection_fd);

    return 0;
}

///// FUNCTION DEFINITIONS

/*
    Explanation to the user of the parameters required to run the program
*/
void usage(char * program)
{
    printf("Usage:\n");
    printf("\t%s {server_address} {port_number}\n", program);
    exit(EXIT_FAILURE);
}

void playerTurn( message_t * message, int connection_fd) { //Update status of the player to get more cards or stay

    int playerOption;

    //If the player or dealer got a natural just tell the player with out doing more updates
    if(message->playerStatus == NATURAL){
        printf("You got a Natural Blackjack with the cards [%s] and [%s]!\n", message->playerCards[0], message->playerCards[1]);
    } else { //After Initial deal
        printf("After the initial deal you have the cards [%s] and [%s], which sum a total of: %d.\n", message->playerCards[0],message->playerCards[1], message->totalPlayer);
    }

    if(message->dealerStatus == NATURAL){
        printf("The dealer got a Natural Blackjack with the cards [%s] and [%s]!\n", message->dealerCards[0], message->dealerCards[1]);
    } else { //After Initial deal
        printf("The dealer has a [%s] and a [faced-down] card.\n", message->dealerCards[0]);
    }

    if((message->playerStatus != NATURAL) && (message->dealerStatus != NATURAL)){
        while((message->playerStatus != STAND) && (message->playerStatus != BUST) && (message->playerStatus != TWENTYONE)){

            playerOption =  0;

            while((playerOption != 1) && (playerOption != 2)){
                printf("\nChoose one of the options:\n1: Stay\n2: Get another card\n");
                scanf("%d", &playerOption);
                switch(playerOption){
                    case 1:
                        message->playerStatus = STAND;
                        break;
                    case 2:
                        message->playerStatus = HIT;
                        break;
                    default:
                        printf("You should press either 1 or 2\n");
                }
            }

            // Send the status chosen by the player
            send(connection_fd, message, sizeof (*message), 0);
            
             if(message->playerStatus == HIT){
                // Gets the status calculated by the server
                if (!recvData(connection_fd, message, sizeof (*message)))
                {
                    return;
                }
                printf("You got the card: [%s]. Your new hand contains the cards:", message->playerCards[(message->numPlayerCards)-1]);
            } else if (message->playerStatus == STAND){
                printf("You decided to stay with the cards:");
            }
            for(int i = 0; i<message->numPlayerCards; i++){
                    printf(" [%s]", message->playerCards[i]);
                }
                printf(" which sum a total of: %d\n", message->totalPlayer);
            }
    }
}

void showResults( message_t * message) { //Show the results and messages depending on the calculations of the server

     //Show player and dealer cards after initial Deal
    printf("Your final hand is:");
    for(int i = 0; i<message->numPlayerCards; i++){
        printf(" [%s]", message->playerCards[i]);
    }
    printf(" accumulating %d.\n", message->totalPlayer);

    printf("The dealer's final hand is:");
    for(int i = 0; i<message->numDealerCards; i++){
        printf(" [%s]", message->dealerCards[i]);
    }
    printf(" accumulating %d.\n\n", message->totalDealer);
    if (message->playerStatus == BUST){
        printf("You are over 21. Sorry, the dealer takes your bet (%d).\n", message->playerBet);
    } else if((message->dealerStatus == BUST) && ((message->playerStatus == STAND) || (message->playerStatus == TWENTYONE))){
        printf("The dealer busted and you didn't. The dealer returns your bet (%d) plus the amount of your bet (%d).\n", message->playerBet, message->playerBet);
    } else if (((message->dealerStatus == STAND) || (message->dealerStatus == TWENTYONE)) && ((message->playerStatus == STAND) || (message->playerStatus == TWENTYONE))){
        if(message->totalPlayer > message->totalDealer){
            printf("Your hand is better than the dealer's hand. The dealer returns your bet (%d) plus the amount of your bet (%d).\n", message->playerBet, message->playerBet);
        }else if (message->totalPlayer < message->totalDealer) {
            printf("The dealer's hand is better. The dealer gets your bet (%d).\n", message->playerBet);
        } else if (message->totalPlayer == message->totalDealer){
            printf("This is stand-off. You get your bet (%d) back.\n", message->playerBet);
        }
    } else if ((message->dealerStatus == NATURAL) && (message->playerStatus != NATURAL)){
        printf("Sorry, the dealer got a Natural Blackjack but you didn't. The dealer takes your bet (%d).\n", message->playerBet);
    } else if ((message->dealerStatus != NATURAL) && (message->playerStatus == NATURAL)){
        printf("You got a Natural Blackjack and the dealer didn't. The dealer returns your bet (%d) plus 1.5x times the amount of your bet (%.0f\n).\n", message->playerBet, message->playerBet * 1.5);
    } else if ((message->dealerStatus == NATURAL) && (message->playerStatus == NATURAL)){
        printf("Both, the dealer and you, got a Natural Blackjack. This is a PUSH. You get your bet (%d) back.\n", message->playerBet);
    }

    printf("Your new amount of chips is: %d\n", message->playerAmount);

}

// Do the actual receiving and sending of data
void communicationLoop(int connection_fd)
{
    message_t message; //message with the information that will be updated between server and client
    int round = 0;
    int askBet;

    // Handshake
    message.msg_code = PLAY;
    send(connection_fd, &message, sizeof message, 0);

    //Check reply, receive AMOUNT
    if (!recvData(connection_fd, &message, sizeof message)) //The final results are received from the server
    {
        printf("Connection refused by the server");
        return;
    }
    if (message.msg_code != AMOUNT)
    {
        printf("Invalid server\n");
        return;
    }

    // Ask user for his total amount of chips to play
    printf("Enter the amount of chips that you have to play: ");
    scanf("%d", &message.playerAmount);
    send(connection_fd, &message, sizeof message, 0);

    // Get the OK to start the game loop, receive STARTs
    recvData(connection_fd, &message, sizeof message);

    while(message.playerAmount >= 2) //While the player has enough money to bet
    {
        round++;
        askBet = 1;

        printf("\n|||||||||||||||ROUND %d|||||||||||||||\n", round);

        printf("\n/////ASKING FOR THE BET/////\n");

        while(askBet){ //Keep asking for a valid bet
            printf("Enter a bet between 2 and 500 chips: ");
            scanf("%d", &message.playerBet);
            if(message.playerBet > message.playerAmount){
                printf("You don't have that amount of chips to bet. You have %d. Provide a smaller bet.\n", message.playerAmount);
            }else if ((message.playerBet > 500) || (message.playerBet < 2)){
                printf("The amount that you want to bet is out of range. Your bet should be from 2 to 500 chips.\n");
            }else {
                askBet = 0;
            }
        }
        
        message.msg_code = BET; //Send bet amount
        send(connection_fd, &message, sizeof message, 0);

        //Receives the total hand accumulated by the player
        if (!recvData(connection_fd, &message, sizeof message))
        {
            return;
        }

        printf("\n/////PLAYER'S TURN/////\n\n");
        playerTurn(&message, connection_fd); //Here is where the player decides to stay or get more cards
        
        //Receive results made by the dealer
        if (!recvData(connection_fd, &message, sizeof message)) //The final results are received from the server
        {
            return;
        }

        printf("\n/////SHOWING FINAL RESULTS CALCULATED BY THE SERVER/////\n\n");
        showResults(&message); //Show a message depending on the calculations of the server

        if(message.playerAmount < 2) { //Check if the player can keep playing, if cannot, disconnect.
            printf("You don't have enough money to keep playing, goodbye!\n");
            return;
        } else {
            //Reset status of dealer and player for next round
            message.playerStatus = START;
            message.dealerStatus = START;
        }
    }

    // Finish the communication
    message.msg_code = BYE;
    send(connection_fd, &message, sizeof message, 0);
    recvData(connection_fd, &message, sizeof message);
}
