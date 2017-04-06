////////////////////////////////////////////////////////////////////////////////////
// ECE 2534:        Lab 02, Hakeem Bisyir
//
// File name:       main.c
//
// Description:     This program simulates a side-channel attack on either a 4-bit
//					or 8-bit password. The program takes input from the UART and
//                  outputs a hamming distance that compares the guessed numbers
//                  to the key. Once the correct number is found, the time is recorded
//                  and can be viewed at the statistics menu.
//
// Efficiency:		The program works as described.
//
// My Experience:	Designing the framework for the states and menus was fairly simple,
//					however there were many other parts of this project that were very
//					difficult.  Specifically, understanding the different functions of
//					and how to initialize the timers differently for different tasks.
//                  It was also very challenging to display the leaderboard and timer
//                  on the Oled as there were a few difficult type conversions involving
//                  strings/characters and ints. The UART implementation was fairly challenging
//                  until I started to read more of the header files and gain a complete
//                  understanding of the concept. Overall this project was difficult
//                  but greatly increased my understanding for concepts that I was
//                  weak in before, specifically timers and UART.
//
// Date:   10/06/2016

#include <stdio.h>                      // for sprintf()
#include <plib.h>                       // Peripheral Library
#include <stdbool.h>                    // for data type bool
#include "PmodOLED.h"
#include "OledChar.h"
#include "OledGrph.h"
#include "delay.h"
#include "myUART.h"

// Digilent board configuration
#pragma config ICESEL       = ICS_PGx1  // ICE/ICD Comm Channel Select
#pragma config DEBUG        = OFF       // Debugger Disabled for Starter Kit
#pragma config FNOSC        = PRIPLL	// Oscillator selection
#pragma config POSCMOD      = XT	    // Primary oscillator mode
#pragma config FPLLIDIV     = DIV_2	    // PLL input divider
#pragma config FPLLMUL      = MUL_20	// PLL multiplier
#pragma config FPLLODIV     = DIV_1	    // PLL output divider
#pragma config FPBDIV       = DIV_8	    // Peripheral bus clock divider
#pragma config FSOSCEN      = OFF	    // Secondary oscillator enable

//Function Declarations
void Initialize(); //Initialize clock and OLED
bool getInput1();  //Detects 0-1 transition for BTN1
bool getInput2();  //Detects 0-1 transition for BTN2
void Timer2InitMessage(); //Initial message timer
void Timer3Init(); //Debouncing timer
void Timer2InitGame(); //Seconds counter for HD

int i = 0;
int j = 0;
int k = 0;
int messageTime = 1;
int inputWait = 1;
char buf[17];
int hamTime;
int bitLeaderboard4[3] = {0,0,0};
int bitLeaderboard8[3] = {0,0,0};
char yourTime[6];
char leader14[6] = "  ---";
char leader24[6] = "  ---";
char leader34[6] = "  ---";
char leader18[6] = "  ---";
char leader28[6] = "  ---";
char leader38[6] = "  ---";
int ham = 0;
char bitInput[] = "****";
char key[] = "0000";
char UARTInput = 'a';
unsigned int timeCount = 0;
unsigned int timer2_current=0, timer2_previous=0;
int randomNum4, randomNum8;
bool sameGame = false;


// Initialize Timer2 so that it rolls over 1,000 times per second
void Timer2InitMessage() 
{
    // The period of Timer 2 is (256 * 39062)/(10 MHz) = .1 s (freq = 10 Hz)
    OpenTimer2(T2_ON | T2_IDLE_CON | T2_SOURCE_INT | T2_PS_1_256 | T2_GATE_OFF, 39061);
    return;
}

void Timer3Init() 
{
    // The period of Timer 2 is (62 * 15625)/(10 MHz) = .1 s (freq = 10 Hz)
    OpenTimer3(T3_ON | T3_IDLE_CON | T3_SOURCE_INT | T3_PS_1_64 | T3_GATE_OFF, 15624);
    return;
}

void Timer2InitGame() 
{
    // The period of Timer 2 is (16 * 62500)/(10 MHz) = 100 ms (freq = 10 Hz)
    OpenTimer2(T2_ON | T2_IDLE_CON | T2_SOURCE_INT | T2_PS_1_16 | T2_GATE_OFF, 62499);
    return;
}

int main() 
{
    Initialize();
    LATGCLR = (1 << 15) | (1 << 14) | (1 << 13) | (1 << 12);
    OledClearBuffer();
	OledSetCursor(0, 0);
	OledPutString("ECE 2534");
	OledSetCursor(0, 1);
	OledPutString("Lab 2");
    OledSetCursor(0, 2);
	OledPutString("Security Sim");
	OledUpdate();
    UARTInit(UART1, 10000000, 9600);
    i = 0;
    TMR2 = 0x0;
	Timer2InitMessage();
	while (messageTime) {
		if (INTGetFlag(INT_T2)) {
			INTClearFlag(INT_T2);
			i++;
		};
		if (i >= 6) {
            //WHYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY
			messageTime = 0;
		};
	};
	OledClearBuffer();
	OledUpdate();
    
    enum States {mainHD, HDMenu4Bit, HDMenu8Bit, HDMenuBack, HDGame4Bit, ham4Bit,
    HDGame8Bit, ham8Bit, mainStats, statsMenu4Bit, statsMenu8Bit, statsMenuBack, 
    statistics4Bit, statistics8Bit}; // Possible states of state machine
    enum States systemState = mainHD; 	 // Initialize system state
    
	TMR2 = 0x0;
    Timer2InitGame();
    
	while (1)
	{
        if (INTGetFlag(INT_T3)) {
            INTClearFlag(INT_T3);
            switch (systemState)
            {
            case mainHD:
                inputWait = 1;
                OledClearBuffer();
                OledSetCursor(0, 0);
                OledPutString("Security Sim");
                OledSetCursor(0, 1);
                OledPutString("-> HD");
                OledSetCursor(0, 2);
                OledPutString("   Stats");
                OledUpdate();
                while (inputWait) {
                    if (getInput1())
                    {
                		systemState = mainStats;
                        inputWait = 0;
                    }
                   	else if (getInput2())
                	{
                       	systemState = HDMenu4Bit;
                        inputWait = 0;
                    }
                    if (UARTReceivedDataIsAvailable(UART1)) {
                        UARTInput = UARTGetDataByte(UART1);
                    }
                }
                break;
            case HDMenu4Bit:
                inputWait = 1;
                OledClearBuffer();
                OledSetCursor(0, 0);
                OledPutString("HD");
                OledSetCursor(0, 1);
                OledPutString("-> 4-Bit");
                OledSetCursor(0, 2);
                OledPutString("   8-Bit");
                OledSetCursor(0, 3);
                OledPutString("   Back");
                OledUpdate();
                while (inputWait) {
                    if (getInput1())
                    {
                		systemState = HDMenu8Bit;
                        inputWait = 0;
                    }
                   	else if (getInput2())
                	{
                       	systemState = HDGame4Bit;
                        inputWait = 0;
                    }
                    if (UARTReceivedDataIsAvailable(UART1)) {
                        UARTInput = UARTGetDataByte(UART1);
                    }
                }
                break;
            case HDMenu8Bit:
                inputWait = 1;
                OledClearBuffer();
                OledSetCursor(0, 0);
                OledPutString("HD");
                OledSetCursor(0, 1);
                OledPutString("   4-Bit");
                OledSetCursor(0, 2);
                OledPutString("-> 8-Bit");
                OledSetCursor(0, 3);
                OledPutString("   Back");
                OledUpdate();
                while (inputWait) {
                    if (getInput1())
                    {
                		systemState = HDMenuBack;
                        inputWait = 0;
                    }
                   	else if (getInput2())
                	{
                       	systemState = HDGame8Bit;
                        inputWait = 0;
                    }
                    if (UARTReceivedDataIsAvailable(UART1)) {
                        UARTInput = UARTGetDataByte(UART1);
                    }
                }
                break;
            case HDMenuBack:
                inputWait = 1;
                OledClearBuffer();
                OledSetCursor(0, 0);
                OledPutString("HD");
                OledSetCursor(0, 1);
                OledPutString("   4-Bit");
                OledSetCursor(0, 2);
                OledPutString("   8-Bit");
                OledSetCursor(0, 3);
                OledPutString("-> Back");
                OledUpdate();
                while (inputWait) {
                    if (getInput1())
                    {
                		systemState = HDMenu4Bit;
                        inputWait = 0;
                    }
                   	else if (getInput2())
                	{
                       	systemState = mainHD;
                        inputWait = 0;
                    }
                    if (UARTReceivedDataIsAvailable(UART1)) {
                        UARTInput = UARTGetDataByte(UART1);
                    }
                }
                break;
            case HDGame4Bit:
                inputWait = 1;
                ham = 0;
                if (!sameGame) {
                    //New Random Number
                    //Initialize all timers
                    randomNum4 = ReadTimer2();
                    randomNum4 = randomNum4 & (0xf);
                    timeCount = 0;
                    timer2_current=0; 
                    timer2_previous=0;
                }
                //Keeps old timer value and random number is it is the same game
                char bitInput4[] = "****";
                char key4[5];
                UARTInput = 'a';
                OledClearBuffer();
                OledSetCursor(0, 0);
                OledPutString("4-Bit HD");
                OledSetCursor(0, 1);
                OledPutString(bitInput4);
                OledUpdate();
                Timer2InitGame();
                j = 0;
                while (inputWait) {
                    timer2_current = ReadTimer2();
                    if (timer2_previous > timer2_current)
                    {
                        // Displays counter while finding the key
                        timeCount++;
                        // Display elapsed time in units of seconds, with decimal point
                        sprintf(buf, "%13d:%2d", (timeCount/10)/60, (timeCount/10)%60);
                        OledSetCursor(0, 3);
                        OledPutString(buf);
                        OledUpdate();
                    }
                    if (UARTReceivedDataIsAvailable(UART1)) {
                        UARTInput = UARTGetDataByte(UART1);
                        OledSetCursor(0, 2);
                        OledPutChar(UARTInput);
                        OledUpdate();
                    }
                    if ((UARTInput == 0x30 || UARTInput == 0x31) && (j < 4)) {
                        bitInput4[j] = UARTInput;
                        j++;
                        OledSetCursor(0, 1);
                        OledPutString(bitInput4);
                        OledUpdate();
                        UARTInput = 'a';
                    }
                   	if (UARTInput == 0x0D) {
                        inputWait = 0;
                        hamTime = timeCount;
                    }
                    timer2_previous = timer2_current;
                }
                j = 0;
                for (k = 3; k >= 0; k--) {
                    if (randomNum4 & (1 << j)) {
                        key4[k] = '1';
                    }
                    else {
                        key4[k] = '0';
                    }
                    j++;
                }
                j = 0;
                while (j < 4) {
                    if (bitInput4[j] != key4[j]) {
                        ham++;
                    }
                    j++;
                }
                if (ham == 0) {
                    if (hamTime < bitLeaderboard4[0] || bitLeaderboard4[0] == 0) {
                            bitLeaderboard4[2] = bitLeaderboard4[1];
                            bitLeaderboard4[1] = bitLeaderboard4[0];
                            bitLeaderboard4[0] = hamTime;
                    }
                    else if (hamTime < bitLeaderboard4[1] || bitLeaderboard4[1] == 0) {
                        bitLeaderboard4[2] = bitLeaderboard4[1];
                        bitLeaderboard4[1] = hamTime;
                    }
                    else if (hamTime < bitLeaderboard4[2] || bitLeaderboard4[2] == 0) {
                        bitLeaderboard4[2] = hamTime;
                    }
                }
                LATGCLR = (1 << 15) | (1 << 14) | (1 << 13) | (1 << 12);
                if (ham & (1 << 0)) {
                    LATGSET = (1 << 12);
                }
                if (ham & (1 << 1)) {
                    LATGSET = (1 << 13);
                }
                if (ham & (1 << 2)) {
                    LATGSET = (1 << 14);
                }
                if (ham & (1 << 3)) {
                    LATGSET = (1 << 15);
                }
                if (ham == 0) {
                    systemState = ham4Bit;
                    sameGame = false;
                    LATGCLR = (1 << 15) | (1 << 14) | (1 << 13) | (1 << 12);
                }
                else {
                    sameGame = true;
                }
                break;
            case ham4Bit:
                inputWait = 1;
                sprintf(yourTime, "%3d:%d", (hamTime/10)/60, (hamTime/10)%60);
                OledClearBuffer();
                OledSetCursor(0, 0);
                OledPutString("CORRECT!");
                OledSetCursor(0, 1);
                OledPutString("Your time: ");
                OledSetCursor(10, 1);
                OledPutString(yourTime);
                OledSetCursor(0, 3);
                OledPutString("-> Main Menu");
                OledUpdate();
                while (inputWait) {
                    if (getInput1())
                    {
                    }
                   	else if (getInput2())
                	{
                        inputWait = 0;
                        systemState = mainHD;
                    }
                    if (UARTReceivedDataIsAvailable(UART1)) {
                        UARTInput = UARTGetDataByte(UART1);
                    }
                }
                LATGCLR = (1 << 15) | (1 << 14) | (1 << 13) | (1 << 12);
                break;
            case HDGame8Bit:
                inputWait = 1;
                if (!sameGame) {
                    //New Random Number
                    //Initialize all timers
                    randomNum8 = ReadTimer2();
                    randomNum8 = randomNum8 & 0xff;
                    timeCount = 0;
                    timer2_current=0; 
                    timer2_previous=0;
                }
                //Keeps old timer value and random number if it is the same game
                ham = 0;
                char bitInput8[] = "********";
                char key8[9];
                UARTInput = 'a';
                OledClearBuffer();
                OledSetCursor(0, 0);
                OledPutString("8-Bit HD");
                OledSetCursor(0, 1);
                OledPutString(bitInput8);
                OledUpdate();
                Timer2InitGame();
                j = 0;
                while (inputWait) {
                    timer2_current = ReadTimer2();
                    if (timer2_previous > timer2_current)
                    {
                        // Timer counts while finding key
                        timeCount++;
                        // Display elapsed time in units of seconds, with decimal point
                        sprintf(buf, "%13d:%2d", (timeCount/10)/60, (timeCount/10)%60);
                        OledSetCursor(0, 3);
                        OledPutString(buf);
                        OledUpdate();
                    }
                    if (UARTReceivedDataIsAvailable(UART1)) {
                        UARTInput = UARTGetDataByte(UART1);
                        OledSetCursor(0, 2);
                        OledPutChar(UARTInput);
                        OledUpdate();
                    }
                    if ((UARTInput == 0x30 || UARTInput == 0x31) && (j < 8)) {
                        bitInput8[j] = UARTInput;
                        j++;
                        OledSetCursor(0, 1);
                        OledPutString(bitInput8);
                        OledUpdate();
                        UARTInput = 'a';
                    }
                   	if (UARTInput == 0x0D) {
                        inputWait = 0;
                        hamTime = timeCount;
                    }
                    timer2_previous = timer2_current;
                }
                j = 0;
                for (k = 7; k >= 0; k--) {
                    if (randomNum8 & (1 << j)) {
                        key8[k] = '1';
                    }
                    else {
                        key8[k] = '0';
                    }
                    j++;
                }
                j = 0;
                while (j < 8) {
                    if (bitInput8[j] != key8[j]) {
                        ham++;
                    }
                    j++;
                }
                if (ham == 0) {
                    if (hamTime < bitLeaderboard8[0] || bitLeaderboard8[0] == 0) {
                        bitLeaderboard8[2] = bitLeaderboard8[1];
                        bitLeaderboard8[1] = bitLeaderboard8[0];
                        bitLeaderboard8[0] = hamTime;
                    }
                    else if (hamTime < bitLeaderboard8[1] || bitLeaderboard8[1] == 0) {
                        bitLeaderboard8[2] = bitLeaderboard8[1];
                        bitLeaderboard8[1] = hamTime;
                    }
                    else if (hamTime < bitLeaderboard8[2] || bitLeaderboard8[2] == 0) {
                        bitLeaderboard8[2] = hamTime;
                    }
                }
                LATGCLR = (1 << 15) | (1 << 14) | (1 << 13) | (1 << 12);
                if (ham & (1 << 0)) {
                    LATGSET = (1 << 12);
                }
                if (ham & (1 << 1)) {
                    LATGSET = (1 << 13);
                }
                if (ham & (1 << 2)) {
                    LATGSET = (1 << 14);
                }
                if (ham & (1 << 3)) {
                    LATGSET = (1 << 15);
                }
                if (ham == 0) {
                    systemState = ham8Bit;
                    sameGame = false;
                    LATGCLR = (1 << 15) | (1 << 14) | (1 << 13) | (1 << 12);
                }
                else {
                    sameGame = true;
                }
                break;
            case ham8Bit:
                inputWait = 1;
                sprintf(yourTime, "%3d:%d", (hamTime/10)/60, (hamTime/10)%60);
                OledClearBuffer();
                OledSetCursor(0, 0);
                OledPutString("CORRECT!");
                OledSetCursor(0, 1);
                OledPutString("Your time: ");
                OledSetCursor(10, 1);
                OledPutString(yourTime);
                OledSetCursor(0, 3);
                OledPutString("-> Main Menu");
                OledUpdate();
                while (inputWait) {
                    if (getInput1())
                    {
                    }
                   	else if (getInput2())
                	{
                        inputWait = 0;
                        systemState = mainHD;
                    }
                    if (UARTReceivedDataIsAvailable(UART1)) {
                        UARTInput = UARTGetDataByte(UART1);
                    }
                }
                LATGCLR = (1 << 15) | (1 << 14) | (1 << 13) | (1 << 12);
                break;
            case mainStats:
                    inputWait = 1;
                	OledClearBuffer();
                    OledSetCursor(0, 0);
                    OledPutString("Security Sim");
                    OledSetCursor(0, 1);
                    OledPutString("   HD");
                    OledSetCursor(0, 2);
                    OledPutString("-> Stats");
                    OledUpdate();
                	while (inputWait) {
                    if (getInput1())
                        {
                            systemState = mainHD;
                            inputWait = 0;
                        }
                       	else if (getInput2())
                    	{
                           	systemState = statsMenu4Bit;
                            inputWait = 0;
                        }
                        if (UARTReceivedDataIsAvailable(UART1)) {
                        UARTInput = UARTGetDataByte(UART1);
                        }
                    }
        			break;
            	case statsMenu4Bit:
                    inputWait = 1;
                	OledClearBuffer();
                    OledSetCursor(0, 0);
                    OledPutString("Statistics");
                    OledSetCursor(0, 1);
                    OledPutString("-> 4-Bit");
                    OledSetCursor(0, 2);
                    OledPutString("   8-Bit");
                    OledSetCursor(0, 3);
                    OledPutString("   Back");
                    OledUpdate();
                    while (inputWait) {
            		if (getInput1())
                       {
                            systemState = statsMenu8Bit;
                            inputWait = 0;
                        }
                       	else if (getInput2())
                    	{
                           	systemState = statistics4Bit;
                            inputWait = 0;
                        }
                        if (UARTReceivedDataIsAvailable(UART1)) {
                        UARTInput = UARTGetDataByte(UART1);
                        }
                    }
    				break;
        		case statsMenu8Bit:
                	inputWait = 1;
                	OledClearBuffer();
                    OledSetCursor(0, 0);
                    OledPutString("Statistics");
                    OledSetCursor(0, 1);
                    OledPutString("   4-Bit");
                    OledSetCursor(0, 2);
                    OledPutString("-> 8-Bit");
                    OledSetCursor(0, 3);
                    OledPutString("   Back");
                    OledUpdate();
                    while (inputWait) {
        			if (getInput1())
                       {
                            systemState = statsMenuBack;
                            inputWait = 0;
                        }
                       	else if (getInput2())
                    	{
                           	systemState = statistics8Bit;
                            inputWait = 0;
                        }
                        if (UARTReceivedDataIsAvailable(UART1)) {
                        UARTInput = UARTGetDataByte(UART1);
                        }
                    }
                    break;
                case statsMenuBack:
                	inputWait = 1;
                	OledClearBuffer();
                    OledSetCursor(0, 0);
                    OledPutString("Statistics");
                    OledSetCursor(0, 1);
                    OledPutString("   4-Bit");
                    OledSetCursor(0, 2);
                    OledPutString("   8-Bit");
                    OledSetCursor(0, 3);
                    OledPutString("-> Back");
                    OledUpdate();
                    while (inputWait) {
        			if (getInput1())
                       {
                            systemState = statsMenu4Bit;
                            inputWait = 0;
                        }
                       	else if (getInput2())
                    	{
                           	systemState = mainStats;
                            inputWait = 0;
                        }
                        if (UARTReceivedDataIsAvailable(UART1)) {
                            UARTInput = UARTGetDataByte(UART1);
                        }
                    }
                    break;
                case statistics4Bit:
                	inputWait = 1;
                    if (bitLeaderboard4[0] != 0) {
                        sprintf(leader14, "%3d:%2d", (bitLeaderboard4[0]/10)/60, (bitLeaderboard4[0]/10)%60);
                    }
                    if (bitLeaderboard4[1] != 0) {
                        sprintf(leader24, "%3d:%2d", (bitLeaderboard4[1]/10)/60, (bitLeaderboard4[1]/10)%60);
                    }
                    if (bitLeaderboard4[2] != 0) {
                        sprintf(leader34, "%3d:%2d", (bitLeaderboard4[2]/10)/60, (bitLeaderboard4[2]/10)%60);
                    }
                	OledClearBuffer();
                    OledSetCursor(0, 0);
                    OledPutString("4-Bit Statistics");
                    OledSetCursor(0, 1);
                    OledPutString("1.");
                    OledSetCursor(3, 1);
                    OledPutString(leader14);
                    OledSetCursor(0, 2);
                    OledPutString("2.");
                    OledSetCursor(3, 2);
                    OledPutString(leader24);
                    OledSetCursor(0, 3);
                    OledPutString("3.");
                    OledSetCursor(3, 3);
                    OledPutString(leader34);
                    OledUpdate();
                    while (inputWait) {
                        if (getInput1())
                            {
                            }
                        else if (getInput2())
                            {
                                systemState = statsMenu4Bit;
                                inputWait = 0;
                            }
                        if (UARTReceivedDataIsAvailable(UART1)) {
                            UARTInput = UARTGetDataByte(UART1);
                        }
                    }
                    break;
                case statistics8Bit:
                	inputWait = 1;
                    if (bitLeaderboard8[0] != 0) {
                        sprintf(leader18, "%3d:%2d", (bitLeaderboard8[0]/10)/60, (bitLeaderboard8[0]/10)%60);
                    }
                    if (bitLeaderboard8[1] != 0) {
                        sprintf(leader28, "%3d:%2d", (bitLeaderboard8[1]/10)/60, (bitLeaderboard8[1]/10)%60);
                    }
                    if (bitLeaderboard8[2] != 0) {
                        sprintf(leader38, "%3d:%2d", (bitLeaderboard8[2]/10)/60, (bitLeaderboard8[2]/10)%60);
                    }
                	OledClearBuffer();
                    OledSetCursor(0, 0);
                    OledPutString("8-Bit Statistics");
                    OledSetCursor(0, 1);
                    OledPutString("1.");
                    OledSetCursor(3, 1);
                    OledPutString(leader18);
                    OledSetCursor(0, 2);
                    OledPutString("2.");
                    OledSetCursor(3, 2);
                    OledPutString(leader28);
                    OledSetCursor(0, 3);
                    OledPutString("3.");
                    OledSetCursor(3, 3);
                    OledPutString(leader38);
                    OledUpdate();
                    while (inputWait) {
                        if (getInput1())
                            {
                            }
                        else if (getInput2())
                            {
                                systemState = statsMenu8Bit;
                                inputWait = 0;
                            }
                        if (UARTReceivedDataIsAvailable(UART1)) {
                            UARTInput = UARTGetDataByte(UART1);
                        }
                    }
                    break;
    			default:
        			// Should never happen; set all LEDs and trap CPU
            		LATGSET = (0xf << 12);
                	while(1)
                    { // Do nothing; infinite loop to help with debugging     
    				}
        			break;
            } // end switch
        }
    }
}




/////////////////////////////////////////////////////////////////
// Function:    getInput1
// Description: Perform a nonblocking check to see if BTN1 has been pressed
// Inputs:      None
// Returns:     TRUE if 0-to-1 transition of BTN1 is detected;
//                otherwise return FALSE
//
bool getInput1()
{
    enum Button1Position {UP, DOWN}; // Possible states of BTN1
    
    static enum Button1Position button1CurrentPosition = UP;  // BTN1 current state
    static enum Button1Position button1PreviousPosition = UP; // BTN1 previous state
    // Reminder - "static" variables retain their values from one call to the next.
    
    button1PreviousPosition = button1CurrentPosition;

    // Read BTN1
    if(PORTG & 0x40)                                
    {
        button1CurrentPosition = DOWN;
    } 
	else
    {
        button1CurrentPosition = UP;
    } 
    
    if((button1CurrentPosition == DOWN) && (button1PreviousPosition == UP))
    {
        return TRUE; // 0-to-1 transition has been detected
    }
    return FALSE;    // 0-to-1 transition not detected
}



/////////////////////////////////////////////////////////////////
// Function:    getInput2
// Description: Perform a nonblocking check to see if BTN2 has been pressed
// Inputs:      None
// Returns:     TRUE if 0-to-1 transition of BTN1 is detected;
//                otherwise return FALSE
//
bool getInput2()
{
    enum Button2Position {UP, DOWN}; // Possible states of BTN2
    
    static enum Button2Position button2CurrentPosition = UP;  // BTN2 current state
    static enum Button2Position button2PreviousPosition = UP; // BTN2 previous state
    // Reminder - "static" variables retain their values from one call to the next.
    
    button2PreviousPosition = button2CurrentPosition;

    // Read BTN2
    if(PORTG & 0x80)                                
    {
        button2CurrentPosition = DOWN;
    } 
	else
    {
        button2CurrentPosition = UP;
    } 
    
    if((button2CurrentPosition == DOWN) && (button2PreviousPosition == UP))
    {
        return TRUE; // 0-to-1 transition has been detected
    }
    return FALSE;    // 0-to-1 transition not detected
}



/////////////////////////////////////////////////////////////////
// Function:     Initialize
// Description:  Initialize the system
// Inputs:       None
// Return value: None

void Initialize()
{
   // Initialize GPIO for all LEDs
   TRISGSET = 0xc0;     // For BTN1 & BTN2: configure PortG bit for input
   TRISGCLR = 0xf000;   // For LEDs 1-4: configure PortG pins for output
   ODCGCLR  = 0xf000;   // For LEDs 1-4: configure as normal output (not open drain)

   // Initialize Timer1, Timer2, and OLED
   DelayInit();
   Timer3Init();
   OledInit();
   
   return;
}