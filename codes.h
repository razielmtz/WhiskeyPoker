#define MAXCARDS 21 //A player can get at most 21 cards (21 A's)
#define MAXLENGTH 3 // Needed to save strings in the array, '10' having the most characters

// The different types of operations available
typedef enum valid_operations {CHECK, DEPOSIT, WITHDRAW, TRANSFER, EXIT} operation_t;

// The types of responses available
// typedef enum valid_responses {OK, INSUFFICIENT, NO_ACCOUNT, BYE, ERROR} response_t;

// Define constants for the messages in the protocol
typedef enum {PLAY, START, AMOUNT, BET, BYE, BUST, NATURAL, HIT, STAND, TWENTYONE, HI} code_t;

// Structure to be sent between client and server
typedef struct {
    code_t msg_code;
    code_t playerStatus;
    code_t dealerStatus;
    int playerAmount;
    int playerBet;
    int numPlayerCards;
    char playerCards[MAXCARDS][MAXLENGTH];
    int totalPlayer;
    int numDealerCards;
    char dealerCards[MAXCARDS][MAXLENGTH]; 
    int totalDealer;
} message_t;
