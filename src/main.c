#include <stm32f031x6.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "display.h"
#include "sound.h"
#include "musical_notes.h"
#include "font5x7.h"

typedef struct 
{
    uint16_t score;
    uint16_t fails;
} Player;

void initClock(void);
void initSysTick(void);
void SysTick_Handler(void);

void initGameBoard();
void gameMusic();
void displayMenu(int selection, Player *currentplayer);

//replay or quit
//void sendSerialPrompt();
//void checkSerialInput();

void delay(volatile uint32_t dly);
void setupIO();

void playTurn(Player *currentPlayer, uint16_t lane);
void resetGame(Player *player1, Player *player2);
int checkInput(GPIO_TypeDef *Port, uint32_t BitNumber, uint16_t lane);
void startGame(int totalTurns, Player *player1, Player *player2);
void sendGameResults(Player *player1, Player *player2);

int isInside(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h, uint16_t px, uint16_t py);
void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber);
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode);
void target(uint16_t lane, Player *currentPlayer);


bool gameOver(Player *player1, Player *player2, int totalTurns);

volatile uint32_t milliseconds;
//uint16_t score=0;
//uint16_t fails=0;


int main()
{

	Player currentPlayer;
    currentPlayer.score = 0;  
    currentPlayer.fails = 0;

	Player player1 = {0, 0};
    Player player2 = {0, 0};

    int totalTurns = 6; 
	int score = 0;
	int i;

	initClock();
	initSysTick();
	setupIO();
	initSound();

	displayMenu(1, &currentPlayer);

	//For the games border and finishline
	initGameBoard();

	startGame(totalTurns, &player1, &player2);

	//main game loop
	while(1)
	{
		startGame(totalTurns, &player1, &player2);
		


		//Display scores and updates
		printText("Score:", 5, 124, RGBToWord(0xff,0xff,0), 0);
		printNumber(score,47,124,RGBToWord(0xff,0xff,0), 0);

		gameMusic();

		if(gameOver(&player1,&player2,totalTurns))
		{
			//send game results to serial port
			sendGameResults(&player1,&player2);
			//sendSerialPrompt();
			//checkSerialInput();
		}
		delay(500);
	}

	for(i=0;i<3;i++)
	{
		printTextX2("X",5+(i*15),140,RGBToWord(128,128,128),0);
	}

	return 0;
}

void initGameBoard()
{
	int x;
	int y;

	for(x=0;x<16;x++) //initilizing the grey borders
	{
		for(y=0;y<5;y++)
		{	
			fillRectangle(x*8,y*28,8,8,61307);	
		}
	}
	
	for(y=0;y<4;y++) //initilizing the finish line
	{
		fillRectangle(112,(y*16)+(y*12+8),16,16,RGBToWord(0,128,0));
		fillRectangle(112,(y*16)+(y*12+12),16,16,RGBToWord(0,128,0));
	}
}

void gameMusic()
{
	int i;
	for(i=350;i<500;i++)
		{
			playNote(850-i);
			delay(5);
		}
		delay(1000);
}
void displayMenu(int selcetion, Player *currentPlayer)
{
	Player player1 = *currentPlayer;
	Player player2;
	int totalTurns=10;
	
	while(1)
	{
		fillRectangle(0, 0, 128, 160, RGBToWord(0, 0, 0));
		printTextX2("MENU",5,20, RGBToWord(255,255,0),0);
		printText("1. start", 10, 50, RGBToWord(128, 128, 128), 0);
    	printText("2. Instructions", 10, 90, RGBToWord(128, 128, 128), 0);
    	printText("3. Exit", 10, 110, RGBToWord(128, 128, 128), 0);

		if ((GPIOA->IDR & (1 << 4)) == 0) 
		{  
			startGame(totalTurns, &player1, &player2);
			break;
		}
		else if ((GPIOA->IDR & (1 << 5)) == 0)
		{
			
    		fillRectangle(0, 0, 128, 160, RGBToWord(0, 0, 0));
    		printTextX2("Instructions:", 5, 20, RGBToWord(255, 255, 0), 0);
    		printText("1. Hit targets in lane", 10, 50, RGBToWord(128, 128, 128), 0);
    		printText("2. Avoid missing", 10, 70, RGBToWord(128, 128, 128), 0);
    		delay(1000);  // Give time to read
    		break;  // Return to the main menu
		}
		
		else if((GPIOA->IDR & (1 << 8)) == 0) 
		{
		    resetGame(&player1, &player2);
            break;
		}	
	}	
}

bool gameOver(Player *player1, Player *player2, int totalTurns)
{
	if(player1->fails == 3 || player2->fails == 3 || totalTurns == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void sendGameResults(Player *player1, Player *player2)
{
	char result[100];

	if (player1->score > player2->score)
	{
		printText("Player 1 wins", 5, 20, RGBToWord(255, 255, 0), 0);
    	snprintf(result, sizeof(result), "Score: %d", player1->score);
    	printText(result, 10, 40, RGBToWord(128, 128, 0), 0);
	}
	else if(player2->score >player1->score)
	{
		printText("Player 2 wins", 5, 20, RGBToWord(255, 255, 0), 0);
    	snprintf(result, sizeof(result), "Score: %d", player2->score);
    	printText(result, 10, 40, RGBToWord(128, 128, 0), 0);
	}
	else
	{
		printText("It a Tie", 5, 20, RGBToWord(255, 255, 0), 0);
    	//snprintf(result, sizeof(result), "Score: %d", player1->score);
    	printText(result, 10, 40, RGBToWord(128, 128, 0), 0);
	}
}

void startGame(int totalTurns, Player *player1, Player *player2)
{
    int currentTurn = 0;
    int lane;

	// Game loop for total turns or until a player reaches the max fails
    while (currentTurn < totalTurns && player1->fails < 3 && player2->fails < 3) 
	{

        if (currentTurn % 2 == 0)//if  player 1 is alive do this else do player  
		{
            printText("Player 1's Turn", 5, 20, RGBToWord(255, 255, 0), 0);
            target();
        } 
		else 
		{
            printText("Player 2's Turn", 5, 20, RGBToWord(255, 0, 0), 0);
            Tar
        }

        currentTurn++;
    }

	// determine the winner
    if (player1->score > player2->score) 
	{
        printText("Player 1 Wins!", 5, 60, RGBToWord(255, 255, 0), 0);
    } 
	else if (player2->score > player1->score) 
	{
        printText("Player 2 Wins!", 5, 60, RGBToWord(255, 0, 0), 0);
    } 
	else 
	{
        printText("It's a Tie!", 5, 60, RGBToWord(128, 128, 128), 0);
    }

	//game replay 
	while(1) 
	{
    	printText("Replay: 'Up' | Quit: 'Down'", 5, 80, RGBToWord(255, 255, 255), 0);

		if ((GPIOA->IDR & (1 << 8)) == 0) // Up pressed
		{  
			resetGame(&player1, &player2);
			return;  // Restart game
		}
		if ((GPIOA->IDR & (1 << 11)) == 0)// Down pressed 
		{  
			printText("Game Over", 5, 100, RGBToWord(255, 0, 0), 0);
			break; 
		}
		delay(100);  
	}
}


void playTurn(Player *currentPlayer, uint16_t lane)
{
    int success = 0;

    // Wait for player to pres the button in the correct lane
    if (checkInput(GPIOB, 4, lane)) {
        currentPlayer->score++;
        success = 1;
    } else {
        currentPlayer->fails++;
    }

    //feedback based on success
    if (success) {
        printText("Hit!", 5, 40, RGBToWord(0, 255, 0), 0);
    } else {
        printText("Miss!", 5, 40, RGBToWord(255, 0, 0), 0);
    }

    delay(500); // Short delay to allow player to see result
}

int checkInput(GPIO_TypeDef *Port, uint32_t BitNumber, uint16_t lane)
{
    // Check if the specific button for the current lane was pressed
    if ((Port->IDR & (1 << BitNumber)) == 0) 
    {
        // Match BitNumber to the lane and return 1 if it’s correct
        if ((lane == 1 && BitNumber == 4) || 
            (lane == 2 && BitNumber == 5) || 
            (lane == 3 && BitNumber == 11) || 
            (lane == 4 && BitNumber == 8)) {
            return 1;
        }
    }
    return 0;
}

void resetGame(Player *player1, Player *player2)
{
    player1->score = 0;
    player1->fails = 0;
    player2->score = 0;
    player2->fails = 0;
}

void initSysTick(void)
{
	SysTick->LOAD = 48000;
	SysTick->CTRL = 7;
	SysTick->VAL = 10;
	__asm(" cpsie i "); // enable interrupts
}
void SysTick_Handler(void)
{
	milliseconds++;
}
void initClock(void)
{
// This is potentially a dangerous function as it could
// result in a system with an invalid clock signal - result: a stuck system
        // Set the PLL up
        // First ensure PLL is disabled
        RCC->CR &= ~(1u<<24);
        while( (RCC->CR & (1 <<25))); // wait for PLL ready to be cleared
        
// Warning here: if sys/tem clock is greater than 24MHz then wait-state(s) need to be
// inserted into Flash memory interface
				
        FLASH->ACR |= (1 << 0);
        FLASH->ACR &=~((1u << 2) | (1u<<1));
        // Turn on FLASH prefetch buffer
        FLASH->ACR |= (1 << 4);
        // set PLL multiplier to 12 (yielding 48MHz)
        RCC->CFGR &= ~((1u<<21) | (1u<<20) | (1u<<19) | (1u<<18));
        RCC->CFGR |= ((1<<21) | (1<<19) ); 

        // Need to limit ADC clock to below 14MHz so will change ADC prescaler to 4
        RCC->CFGR |= (1<<14);

        // and turn the PLL back on again
        RCC->CR |= (1<<24);        
        // set PLL as system clock source 
        RCC->CFGR |= (1<<1);
}

void delay(volatile uint32_t dly)
{
	uint32_t end_time = dly + milliseconds;
	while(milliseconds != end_time)
		__asm(" wfi "); // sleep
}
void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber)
{
	Port->PUPDR = Port->PUPDR &~(3u << BitNumber*2); // clear pull-up resistor bits
	Port->PUPDR = Port->PUPDR | (1u << BitNumber*2); // set pull-up bit
}
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode)
{
	/*
	*/
	uint32_t mode_value = Port->MODER;
	Mode = Mode << (2 * BitNumber);
	mode_value = mode_value & ~(3u << (BitNumber * 2));
	mode_value = mode_value | Mode;
	Port->MODER = mode_value;
}
void setupIO()
{
	RCC->AHBENR |= (1 << 18) + (1 << 17); // enable Ports A and B
	display_begin();
	pinMode(GPIOB,4,0);
	pinMode(GPIOB,5,0);
	pinMode(GPIOA,8,0);
	pinMode(GPIOA,11,0);
	enablePullUp(GPIOB,4);
	enablePullUp(GPIOB,5);
	enablePullUp(GPIOA,11);
	enablePullUp(GPIOA,8);
}

int isInside(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h, uint16_t px, uint16_t py)
{
	// checks to see if point px,py is within the rectange defined by x,y,w,h
	uint16_t x2,y2;
	x2 = x1+w;
	y2 = y1+h;
	int rvalue = 0;
	if ( (px >= x1) && (px <= x2))
	{
		// ok, x constraint met
		if ( (py >= y1) && (py <= y2))
			rvalue = 1;
	}
	return rvalue;
}

void target(uint16_t lane, Player *currentPlayer)
{
	int colour=RGBToWord(128,128,128);
	int touch=0;//to stop people from getting two point on the green
	int grace=0;//you get three runs of grace where you cant get another fail
	int scored=0;//if you dont score a point you will get a fail. 
	int x=0;
	int y;
	int i;
	if(lane==5)
	{
		lane= (rand()%4)+1;//rands the lane
	}

	switch (lane)//assigns the right y and colour for the lane
	{
	case 1:
		y=10;
		colour=RGBToWord(204,0,204);
		break;
	case 2:
		y=38;
		colour =RGBToWord(255,255,0);
		break;
	case 3:
		y=66;
		colour=RGBToWord(0,0,255);
		break;
	case 4:
		y=94;
		colour=RGBToWord(153,0,0);
		break;
	default:
		y=10;
		break;
	}

	for(x=0;x<16;x++)
	{	
		fillRectangle(x*8,y,8,16,colour);	//the target
		delay(100);
		if(x<14)//15 and 16 are the finish line
		{
			fillRectangle(x*8,y,8,16,0);	//filling in black after the target
			touch=0;				
			if(grace>0)
			{grace-=1;}
		}
		else
		{
			fillRectangle(x*8,y,8,16,RGBToWord(0,128,0)); //filling in the grren after the target
		}
		
		if((GPIOB->IDR & (1 << 4))==0) // right pressed
		{
			if(lane==3 && isInside(112,y,16,16,x*8,y) && touch==0)
			{
				currentPlayer->score+=1;//adds one score
				touch=1;
				scored=1;
				for(i=550;i<800;i++)
				{playNote(i);
				delay(1);}
				playNote(0);
			}
			else if(touch==0 && grace==0)
			{
				currentPlayer->fails+=1;
				grace=3;
				for(i=350;i<500;i++)
				{playNote(850-i);
				delay(5);}
				playNote(0);
				break;
			}
		}
		if((GPIOB->IDR & (1 << 5))==0) // left pressed
		{
			if(lane==2 && isInside(112,y,16,16,x*8,y) && touch==0)
			{
				currentPlayer->score+=1;//adds one score
				touch=1;
				scored=1;
				for(i=550;i<800;i++)
				{playNote(i);
				delay(1);}
				playNote(0);
			}
			else if(touch==0 && grace==0)
			{
				currentPlayer->fails+=1;
				grace=3;
				for(i=350;i<500;i++)
				{playNote(850-i);
				delay(5);}
				playNote(0);
				break;
			}
		}
		if((GPIOA->IDR & (1 << 11)) == 0) // down pressed
		{
			if(lane==4 && isInside(112,y,16,16,x*8,y) && touch==0)
			{
				currentPlayer->score+=1;//adds one score
				touch=1;
				scored=1;
				for(i=550;i<800;i++)
				{
					playNote(i);
					delay(1);
				}
				playNote(0);
			}
			else if(touch==0 && grace==0)
			{
				currentPlayer->fails+=1;
				grace=3;
				for(i=350;i<500;i++)
				{
					playNote(850-i);
					delay(5);
				}
				playNote(0);
				break;
			}
		}
		if((GPIOA->IDR & (1 << 8)) == 0) // up pressed
		{
			if(lane==1 && isInside(112,y,16,16,x*8,y) && touch==0)
			{
				currentPlayer->score+=1;//adds one score
				touch=1;
				scored=1;
				for(i=550;i<800;i++)
				{
					playNote(i);
					delay(1);
				}
				playNote(0);
			}
			else if(touch==0 && grace==0)
			{
				currentPlayer->fails+=1;
				grace=3;
				for(i=350;i<500;i++)
				{
					playNote(850-i);
					delay(5);
				}
				playNote(0);
				break;
			}	//you get three runs of grace where you cant get another fail
		}
		printText("Score:", 5, 124, RGBToWord(0xff,0xff,0), 0);
		printNumber(currentPlayer->score,47,124,RGBToWord(0xff,0xff,0), 0);
		for(i=0; i< currentPlayer->fails && i<3 ;i++)
		{
			printTextX2("X",5+(i*15),140,RGBToWord(204,0,0),0);
			
		}
		if(currentPlayer->fails>2)
		{
			break;
		}
		
	}
	if(scored==0 && grace==0)//the grace is here to stop people from getting to fails if they press on the space before
	{
		currentPlayer->fails+=1;
		for(i=350;i<500;i++)
		{playNote(850-i);
		delay(5);}
		playNote(0);
	}
	
	 for (i = 0; i < currentPlayer->fails && i < 3; i++)
	{
		printTextX2("X",5+(i*15),140,RGBToWord(204,0,0),0);
	}
}





//isInside(112,y,16,16,x*8,y)

//printText("Score:", 2, 124, RGBToWord(0xff,0xff,0), 0);
//printNumber(score,42,124,RGBToWord(0xff,0xff,0), 0);

//((GPIOB->IDR & (1 << buttonNeed))==0)


/*
else if((GPIOB->IDR & (1 << buttonNeed))==0 && (isInside(112,y,16,16,x*8,y)==0))
{
	touch=1;
	if(score>0)
		//score-=1;
	printText("Score:", 2, 124, RGBToWord(0xff,0xff,0), 0);
	printNumber(score,42,124,RGBToWord(0xff,0xff,0), 0);			
}


((GPIOB->IDR & (1 << 4))==0) // right pressed
((GPIOB->IDR & (1 << 5))==0) // left pressed
((GPIOA->IDR & (1 << 11)) == 0) // down pressed
((GPIOA->IDR & (1 << 8)) == 0) // up pressed
*/