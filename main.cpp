#include <iostream>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <unistd.h>
#include <algorithm>
#include <cmath>

struct Card {
    std::string value;
    int row; // top left row position
    int col; //top left col position
    bool revealed;
    bool isRed;
};

// weird function that sets up terminal for raw input, dont really know how it works but it does
void setRawMode(bool enable) {
    static struct termios oldt, newt;
    if (enable) {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO); 
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }
}

std::string getRandomCardValue() {
    const std::string cardValues[] = {"A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"};
    int randomIndex = rand() % 13;
    return cardValues[randomIndex];
}

std::pair<int, int> calculateHandValue(const std::vector<Card>& hand) {
    int total = 0;
    int aceCount = 0;

    for (const auto& card : hand) {
        if(card.revealed){
            if (card.value == "A") {
                aceCount++;
                total += 1; // add 1 for now and adjust for 11 later
            } else if (card.value == "J" || card.value == "Q" || card.value == "K") {
                total += 10;
            } else {
                total += std::stoi(card.value);
            }
        }
    }

    // aces
    int altTotal = total;
    while (aceCount > 0 && altTotal + 10 <= 21) {
        altTotal += 10;
        aceCount--;
    }

    return (altTotal > total) ? std::make_pair(total, altTotal) : std::make_pair(total, total);
}

// calculates centered positions for cards
void calculateCardPositions(std::vector<Card>& cards, int startRow, int terminalWidth) {
    int cardWidth = 12;
    int totalWidth = cards.size() * cardWidth + (cards.size() - 1) * 2; // total width of cards with gaps
    int startCol = (terminalWidth - totalWidth) / 2; // centered starting col

    for (size_t i = 0; i < cards.size(); i++) {
        cards[i].row = startRow;
        cards[i].col = startCol + i * (cardWidth + 2);
    }
}

double roundToTwoDecimals(double value) {
    return std::round(value * 100.0) / 100.0;
}

void displayTable(int selectedButton, const std::vector<Card>& dealerCards, const std::vector<Card>& playerCards, double balance, double bet) {
    system("clear");
    
    struct winsize w;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &w);
    int rows = w.ws_row;
    int cols = w.ws_col;

    const std::string greenBackground = "\033[48;5;22m";
    const std::string brownBackground = "\033[48;5;94m";
    const std::string whiteBackground = "\033[48;5;15m";
    const std::string redBackground = "\033[48;5;196m";
    const std::string blackText = "\033[30m";
    const std::string redText = "\033[31m";
    const std::string yellowText = "\033[33m";
    const std::string buttonBackground = "\033[48;5;237m";
    const std::string highlightedBackground = "\033[48;5;240m";
    const std::string reset = "\033[0m";

    const int borderThickness = 2;
    const int buttonHeight = 6;

    const std::string buttons[] = {"HIT", "STAND", "DOUBLE"};
    const int numButtons = 3;
    const int buttonWidth = 10;
    const int buttonPadding = 5;
    int buttonStartRow = rows - buttonHeight + 2;
    int buttonStartCol = (cols - (numButtons * buttonWidth + (numButtons - 1) * buttonPadding)) / 2;

    auto drawCard = [&](const Card& card) {
        int cardHeight = 8;
        int cardWidth = 12;
        for (int i = 0; i < cardHeight; i++) {
            for (int j = 0; j < cardWidth; j++) {
                if (card.revealed) {
                    if (i == 0 && j == 0) {
                        std::cout << whiteBackground << (card.isRed ? redText : blackText) << card.value << whiteBackground;
                        j += card.value.size() - 1;
                    } else if (i == cardHeight - 1 && j == 12 - (int)card.value.size()) {
                        std::cout << whiteBackground << (card.isRed ? redText : blackText) << card.value << whiteBackground;
                        j += card.value.size() - 1;
                    } else {
                        std::cout << whiteBackground << " ";
                    }
                } else {
                    std::cout << redBackground << " ";
                }
            }
            std::cout << reset << "\n";
            if (i < cardHeight - 1) {
                std::cout << "\033[" << card.row + i + 1 << ";" << card.col << "H";
            }
        }
    };

    auto drawDeck = [&](int row, int col) {
        int cardHeight = 9;
        int cardWidth = 12;
        for (int i = 0; i < cardHeight; i++) {
            std::cout << "\033[" << row + i << ";" << col << "H";
            for (int j = 0; j < cardWidth; j++) {
                if (i == cardHeight - 1) {
                    std::cout << whiteBackground << " ";
                } else {
                    std::cout << redBackground << " ";
                }
            }
            std::cout << reset;
        }
    };

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (i >= rows - buttonHeight) {
                int buttonIndex = -1;
                for (int b = 0; b < numButtons; ++b) {
                    int adjustedButtonWidth = buttons[b].size() + 4;
                    int startCol = buttonStartCol + b * (adjustedButtonWidth + buttonPadding);
                    if (i >= buttonStartRow && i < buttonStartRow + 3 && j >= startCol && j < startCol + adjustedButtonWidth) {
                        buttonIndex = b;
                        break;
                    }
                }
                if (buttonIndex != -1) {
                    if (buttonIndex == selectedButton) {
                        std::cout << highlightedBackground;
                    } else {
                        std::cout << buttonBackground;
                    }

                    int adjustedButtonWidth = buttons[buttonIndex].size() + 4;
                    int textStart = buttonStartCol + buttonIndex * (adjustedButtonWidth + buttonPadding) + 2;

                    if (i == buttonStartRow + 1 && j >= textStart && j < (int)(textStart + buttons[buttonIndex].size())) { // cast to int to get rid of dumb warning
                        std::cout << yellowText << buttons[buttonIndex][j - textStart] << reset;
                    } else {
                        std::cout << " ";
                    }
                } else {
                    std::cout << buttonBackground << " ";
                }
            } else if (i < borderThickness || i >= rows - buttonHeight - borderThickness ||  j < borderThickness || j >= cols - borderThickness) {
                std::cout << brownBackground << " ";
            } else {
                std::cout << greenBackground << " ";
            }
        }
        std::cout << reset << "\n";
    }

    for (const auto& card : dealerCards) {
        std::cout << "\033[" << card.row << ";" << card.col << "H";
        drawCard(card);
    }

    if (!dealerCards.empty()) {
        int deckRow = dealerCards.back().row - 1;
        int deckCol = dealerCards.back().col + 16;
        drawDeck(deckRow, deckCol);
    }

    for (const auto& card : playerCards) {
        std::cout << "\033[" << card.row << ";" << card.col << "H";
        drawCard(card);
    }

    std::pair<int, int> dealerTotal = calculateHandValue(dealerCards);
    std::pair<int, int> playerTotal = calculateHandValue(playerCards);

    std::stringstream dealerValueText;
    std::stringstream playerValueText;
    dealerValueText << dealerTotal.first;
    if (dealerTotal.first != dealerTotal.second) dealerValueText << ", " << dealerTotal.second;
    playerValueText << playerTotal.first;
    if (playerTotal.first != playerTotal.second) playerValueText << ", " << playerTotal.second;

    if (!dealerCards.empty()) {
        std::cout << "\033[" << dealerCards[0].row + 9 << ";" << dealerCards[0].col + 2 << "H" << buttonBackground << yellowText << "Dealer: " << dealerValueText.str() << reset;
    }

    if (!playerCards.empty()) {
        std::cout << "\033[" << playerCards[0].row + 9 << ";" << playerCards[0].col + 2 << "H" << buttonBackground << yellowText << "Player: " << playerValueText.str() << reset;
    }

    std::cout << "\033[" << rows - buttonHeight + 1 << ";5H" << buttonBackground << yellowText << "BALANCE: $" << balance << reset;
    std::cout << "\033[" << rows - buttonHeight + 2 << ";5H" << buttonBackground << yellowText << "BET: $" << bet << reset;
}

// helper to handle dealer autoplay
int handleDealerTurn(std::vector<Card>& dealerCards, int dealerRow, int selectedButton, const std::vector<Card>& playerCards, double balance, double bet, int cols) {
    std::pair<int, int> dealerHandPair = calculateHandValue(dealerCards);
    int dealerHandValue = std::max(dealerHandPair.first, dealerHandPair.second);

    while (dealerHandValue < 17) {
        sleep(1);
        dealerCards.push_back({getRandomCardValue(), 0, 0, true, static_cast<bool>(rand() % 2)});
        calculateCardPositions(dealerCards, dealerRow, cols);
        displayTable(selectedButton, dealerCards, playerCards, balance, bet);
        std::cout << std::flush;

        dealerHandPair = calculateHandValue(dealerCards);
        dealerHandValue = std::max(dealerHandPair.first, dealerHandPair.second);
    }
    return dealerHandValue;
}

void determineOutcome(int dealerHandValue, int playerHandValue, double& balance, double bet) {
    if (dealerHandValue <= 21) {
        std::cout << "\nDealer stands at " << dealerHandValue << ".\n" << std::flush;
        if (dealerHandValue == playerHandValue) {
            std::cout << "\033[33mIt is a push.\033[0m" << std::flush;
            balance += bet;
        } else if (dealerHandValue > playerHandValue) {
            std::cout << "\033[31mDealer wins.\033[0m" << std::flush;
        } else {
            std::cout << "\033[32mYou win!!!\033[0m" << std::flush;
            balance += roundToTwoDecimals(bet * 2);
        }
    } else {
        std::cout << "\033[32mYou win!!!\033[0m" << std::flush;
        balance += roundToTwoDecimals(bet * 2);
    }
    sleep(2);
}

std::string club = R"(
     .-~~-.
    {      }
 .-~-.    .-~-.
{              }
 `.__.'||`.__.'
       ||
      '--`
)" ;

std::string diamond = "\033[31m" R"(
     /\
   .'  `.
  '      `.
<          >
 `.      .'
   `.  .'
     \/
)" "\033[0m"; 

std::string spade =  R"(
       /\
     .'  `.
    '      `.
 .'          `.
{              }
 ~-...-||-...-~
       ||
      '--`
)" ; 

std::string heart = "\033[31m" R"(
 .-~~~-__-~~~-.
{              }
 `.          .'
   `.      .'
     `.  .'
       \/
)" "\033[0m"; 

int main() {
    srand(static_cast<unsigned>(time(0))); // seed for the rng
    double balance = 1000.0;
    double bet = 0.0;      

    system("clear");
    std::cout << "\033[32mWelcome to my Blackjack table!\033[0m\n";
    std::cout << "At the table, use the arrow keys/A-D keys to select an option, and then press ENTER to select it.\n";
   
    while(true){
        int selectedButton = 1; // start on stand button
        if(balance == 0){
            std::cout << "Sorry but you're all out of money...\n";
            sleep(2);
            break;
        }
        while (true) {
            std::cout << "\033[32m===============================\033[0m\n" << std::flush;
            std::cout << "Current Balance: $\033[33m" << balance << "\033[0m\n";
            std::cout << "Enter your bet: "; // TODO: Add checks for better valid input
            if (std::cin >> bet && bet > 0 && bet <= balance) {
                bet = roundToTwoDecimals(bet);
                balance -= bet; //good bet value
                balance = roundToTwoDecimals(balance);
                break;
            } else {
                std::cin.clear(); // if they input a string and need to clear bad input
                std::cin.ignore(1000, '\n');
                system("clear");
                std::cout << "Invalid bet. Please try again.\n";
            }
        }
        std::cin.ignore();

        setRawMode(true); // raw input mode
        char input;

        system("clear");
        std::cout << "Dealing cards" << std::flush;
        std::cout << ".\n" << std::flush;
        std::cout << spade << std::flush;
        usleep(500000); // 0.5 seconds
        system("clear");

        std::cout << "Dealing cards." << std::flush;
        std::cout << ".\n" << std::flush;
        std::cout << diamond << std::flush;
        usleep(500000); 
        system("clear");

        std::cout << "Dealing cards.." << std::flush;
        std::cout << ".\n" << std::flush;
        std::cout << club << std::flush;
        usleep(500000); 
        system("clear");

        std::cout << "Dealing cards..." << std::flush;
        std::cout << ".\n" << std::flush;
        std::cout << heart << std::flush;
        usleep(500000);
        system("clear");
        tcflush(STDIN_FILENO, TCIFLUSH);

        std::vector<Card> dealerCards;
        std::vector<Card> playerCards;

        // starting hands
        playerCards.push_back({getRandomCardValue(), 0, 0, true, static_cast<bool>(rand() % 2)});
        dealerCards.push_back({getRandomCardValue(), 0, 0, true, static_cast<bool>(rand() % 2)});
        playerCards.push_back({getRandomCardValue(), 0, 0, true, static_cast<bool>(rand() % 2)});
        dealerCards.push_back({getRandomCardValue(), 0, 0, false, static_cast<bool>(rand() % 2)});

        std::pair<int, int> playerTotal = calculateHandValue(playerCards);
        bool alreadyHit = false;
        while (true) {
            // Gets window terminal size
            struct winsize w;
            ioctl(STDIN_FILENO, TIOCGWINSZ, &w);
            int rows = w.ws_row;
            int cols = w.ws_col;

            int greenBackgroundHeight = rows - 6 - 4; // Height of the green area (total rows - button space - borders)
            int greenBackgroundStart = 3; 
            int adjustedCenter = greenBackgroundStart + (greenBackgroundHeight / 3);
            int gapBetweenRows = greenBackgroundHeight / 6;

            int dealerRow = adjustedCenter - gapBetweenRows;
            int playerRow = adjustedCenter + gapBetweenRows;


            calculateCardPositions(dealerCards, dealerRow, cols);
            calculateCardPositions(playerCards, playerRow, cols);

            if (playerTotal.second == 21) { //if blackjack
                displayTable(selectedButton, dealerCards, playerCards, balance, bet);
                std::cout << std::flush;
                sleep(1);
                std::cout << "\033[32mBLACKJACK! You win!!!\033[0m" << std::flush;
                balance += roundToTwoDecimals(bet * 2.5);
                sleep(2);
                break;
            }

            displayTable(selectedButton, dealerCards, playerCards, balance, bet);
            
            input = getchar(); 
            if (input == '\033') { // arrow keys start with this escape sequence
                getchar(); // Skip the '[' character
                char arrowKey = getchar();
                if (arrowKey == 'C') { // Right arrow
                    selectedButton = (selectedButton + 1) % 3;
                } else if (arrowKey == 'D') { // Left arrow
                    selectedButton = (selectedButton - 1 + 3) % 3;
                }
            } else if (input == 'A' || input == 'a') {  //A
                selectedButton = (selectedButton - 1 + 3) % 3;
            } else if (input == 'D' || input == 'd') {  //D
                selectedButton = (selectedButton + 1) % 3;
            } else if (input == '\n') { // Enter
                if (selectedButton == 1) { // STAND
                    std::pair<int, int> playerHandPair = calculateHandValue(playerCards);
                    int playerHandValue = std::max(playerHandPair.first, playerHandPair.second);

                    dealerCards[1].revealed = true; // Reveal the dealer's hidden card
                    calculateCardPositions(dealerCards, dealerRow, cols);
                    displayTable(selectedButton, dealerCards, playerCards, balance, bet);
                    std::cout << std::flush;

                    int dealerHandValue = handleDealerTurn(dealerCards, dealerRow, selectedButton, playerCards, balance, bet, cols);
                    determineOutcome(dealerHandValue, playerHandValue, balance, bet);
                    break;
                }
                else if (selectedButton == 2) { // DOUBLE
                    if(alreadyHit){
                        std::cout << "\n\033[33mYou have already hit!\033[0m\n";
                        sleep(1);
                        tcflush(STDIN_FILENO, TCIFLUSH);
                    }
                    else if (bet <= balance) {
                        balance -= bet;
                        balance = roundToTwoDecimals(balance);
                        bet *= 2;

                        playerCards.push_back({getRandomCardValue(), 0, 0, true, static_cast<bool>(rand() % 2)});
                        calculateCardPositions(playerCards, playerRow, cols);
                        displayTable(selectedButton, dealerCards, playerCards, balance, bet);
                        std::cout << std::flush;
                        sleep(1);

                        std::pair<int, int> playerTotal = calculateHandValue(playerCards);
                        if (playerTotal.first > 21) {
                            std::cout << "\n\033[31mYou busted! \nDealer wins.\033[0m" << std::flush;
                            sleep(2);
                            break;
                        }

                        dealerCards[1].revealed = true; // Reveal the dealer's hidden card
                        calculateCardPositions(dealerCards, dealerRow, cols);
                        displayTable(selectedButton, dealerCards, playerCards, balance, bet);
                        std::cout << std::flush;
                        sleep(1);

                        int dealerHandValue = handleDealerTurn(dealerCards, dealerRow, selectedButton, playerCards, balance, bet, cols);
                        int playerHandValue = std::max(playerTotal.first, playerTotal.second);
                        determineOutcome(dealerHandValue, playerHandValue, balance, bet);
                        break;
                    } else {
                        std::cout << "\n\033[33mNot enough balance to double.\033[0m\n";
                        sleep(1);
                        tcflush(STDIN_FILENO, TCIFLUSH);
                    }
                } else if (selectedButton == 0) { // HIT
                    alreadyHit = true;
                    playerCards.push_back({getRandomCardValue(), 0, 0, true, static_cast<bool>(rand() % 2)});
                    calculateCardPositions(playerCards, playerRow, cols);
                    displayTable(selectedButton, dealerCards, playerCards, balance, bet);
                    std::cout << std::flush;
                    sleep(1);

                    std::pair<int, int> playerTotal = calculateHandValue(playerCards);
                    if (playerTotal.first > 21) {
                        std::cout << "\n\033[31mYou busted! \nDealer wins.\033[0m" << std::flush;
                        sleep(2);
                        break;
                    } else if (playerTotal.second == 21) { // Auto stand if hit to a 21
                        sleep(1);
                        dealerCards[1].revealed = true; // Reveal dealers hidden card
                        calculateCardPositions(dealerCards, dealerRow, cols);
                        displayTable(selectedButton, dealerCards, playerCards, balance, bet);
                        std::cout << std::flush;

                        int dealerHandValue = handleDealerTurn(dealerCards, dealerRow, selectedButton, playerCards, balance, bet, cols);
                        int playerHandValue = std::max(playerTotal.first, playerTotal.second);
                        determineOutcome(dealerHandValue, playerHandValue, balance, bet);
                        break;
                    }
                }
            }
        }
        setRawMode(false); //back to normal input
        std::cout << "\nPlay again (y/n)?: ";
        char exiting;
        std::cin >> exiting;
        if(exiting == 'n' || exiting == 'N'){
            break;
        }
        system("clear");
    }

    setRawMode(false);
    system("clear");
    std::cout << "Thanks for playing!" << std::endl;
    std::cout << "Your final balance is: \033[32m$" << balance << "\033[0m" << std::endl;
    return 0;
}

