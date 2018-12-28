
/*
Elavator algorithm:

Standart Case:
1) The elevator in screen start from platform 0.
2) The user clicked on bottun '2'
3) The elevator in screen up to platform 1, and than to platform 2

Alternate Flow - Invalid button input:
1) The elevator in screen start from platform 0.
2) The user clicked on bottun 'A'
3) The elevator in screen doesn't move.
4) The LCD will write 'wrong input' (optianal)

Alternate Flow - Shabat ha-yom!:
1) The elevator in screen start from platform 0.
2) The user clicked on bottun '3'
3) The elevator in screen up to platform 1, and than to platform 2, and finally arrives to platform 3.
4) The user perform exception.
5) the elevator from platform 3 (in this case) up to platform 8, and down back to platform 0. In Each platform there is a timer of 500 ms (0.5 second)

Alternate Flow - Sunday came!:
1) The elevator in screen at platform 4, at Sutarday mode.
2) The user perform exception.
3) the elevator stops from move(and still in the screen)
4) click on button 2


*/




#include <plib.h>  // lib for the exception
#include <p32xxxx.h>
#pragma config FPLLMUL = MUL_20, FPLLIDIV = DIV_2, FPLLODIV = DIV_1, FWDTEN = OFF
#pragma config POSCMOD = HS, FNOSC = PRIPLL, FPBDIV = DIV_8
void delay(void);

#include <math.h>

#define INFINITY -1




void lightLeds(char num, int timer);
void initPortD_LEDS(void);
void initPortE_LEDS(void);
void initPortF_LEDS(void);

char getKeypadButtonPushed();
void initPortB_KEYPAD(void);
void initPortE_KEYPAD(void);
void initPortF_KEYPAD(void);
void initPortG_KEYPAD(void);
void delay(void);

// draw Elevator
void drawSutardayElevator(void);
void drawElevator(int floorNow, int floorToBe);
void drawElevatorUp(int from, int to, int height);
void drawElevatorDown(int from, int to, int height);
void drawInPlace(int in, int height);

// draw box
void drawBox(int buttomLeftX, int buttomLeftY, int heigth, int width)
void drawHorizonalLine(int startX, int startY, int length)
void drawVerticalLine(int startX, int startY, int length)
void drawPoint(int x, int y);



void initPortD_DAC(void);
void initPortE_DAC(void);
void initPortF_DAC(void);


char isSutarday = 0x01;


void __ISR(_EXTERNAL_1_IRQ, ipl1)INT1_interrupt(void) //_EXTERNAL_1_IRQ
{
	// switch from sutarday to normal, and the opposite
	isSutarday ^= 1;
}








int main()
{

	// define exception:
	//   TRISF&=0x40;//int0-RF6
	//   TRISD=0;
	//	TRISE&=0x300;//int12-RE8/9
	//   PORTF=4;   // LEDS

	IPC1 = 0x04000004;//??????(int1+timer1)
	IEC0bits.INT1IE = 1;
	INTCONbits.INT1EP = 0;
	IFS0bits.INT1IF = 0;

	INTCONbits.MVEC = 1;//????? ??????

	asm("ei");//?????? ????


	int startFloor = 0;
	int endFloor;
	while (1)
	{
		if (isSutarday & 1)  // if its sutarday
		{
			drawSutardayElevator();
			// TODO: set an timer
		}
		else  // during the week
		{
			char key = getKeypadButtonPushed();
			endFloor = (key - 0x30);

			if (endFloor > 7) // if illeagal floor given stay in place
			{
				endFloor = startFloor;
			}
			drawElevator(startFloor, endFloor);
			startFloor = endFloor;
		}
	}
	return 0;
}





/////////////// LED //////////////

// if timer is -1 -> infinity
void lightLeds(char num, int timer)
{
	initPortD_LEDS();  // for RD4
	initPortE_LEDS();
	initPortF_LEDS();

	PORTF = 0x04; // set the entry of decoder 3x8 to be LED
	PORTE = num; // the value of 'num' will set in port E
	PORTDbits.RD4 = 1; // light-up the LED

	if (timer != INFINITY)
	{
		int i;
		for (i = 0; i < timer * 1000; i++);
		PORTDbits.RD4 = 0; // light-down the LED
	}

}

void initPortD_LEDS(void)
{
	unsigned int portMap;

	portMap = TRISD;
	portMap &= 0xffffffcf;  // RD.4 and RD.5 are init for input
	TRISD = portMap;
}

void initPortE_LEDS(void)
{
	unsigned int portMap;

	portMap = TRISE;
	portMap &= 0xffffff00;  // RE.0 til 7 are init for input (the connection between mips32 to LCD, LEDS, and etc)
	TRISE = portMap;
}

void initPortF_LEDS(void)
{
	unsigned int portMap;

	portMap = TRISF;
	portMap &= 0xfffffef8;  // RF.0 til 2 are init for input for decoder 3x8
	// RF.8 init for input for decoder 3x8 Enable/Disable
	TRISF = portMap;

	PORTFbits.RF8 = 1;  // the decoder is Enable
}






/////////////// KEYPAD //////////////

char getKeypadButtonPushed()
{
	int RUN_ZERO[4] = { 0xee, 0xdd, 0xbb, 0x77 };
	char keypad_button_ascii[16][2] = { { 0xee, '1' }, { 0xde, '2' }, { 0xbe, '3' }, { 0x7e, 'A' },
	{ 0xed, '4' }, { 0xdd, '5' }, { 0xbd, '6' }, { 0x7d, 'B' },
	{ 0xeb, '7' }, { 0xdb, '8' }, { 0xbb, '9' }, { 0x7b, 'C' },
	{ 0xe7, '*' }, { 0xd7, '0' }, { 0xb7, '#' }, { 0x77, 'D' } };
	char isButtonPushed = 0;
	int column = 0;
	int keyval_col;
	int keyval_row;
	char num_led;

	initPortB_KEYPAD();
	initPortE_KEYPAD();
	initPortF_KEYPAD();
	initPortG_KEYPAD();

	PORTG = 0x00; // set entry of decoder 2x4 be Keypad
	PORTF = 0x07; // set entry of decoder 3x8 be decoder 2x4

	int delayIteration = 0;
	while (!isButtonPushed && delayIteration < 3) // 3 as this project doesnt require the 4'th column only numbers 0-7
	{
		// TODO: maybe delete it
		PORTG = 0x00;
		PORTF = 0x07;

		PORTE = RUN_ZERO[column];

		delay(); // time to capture the button pushing

		keyval_col = PORTB & 0x0f;  // get the value of pushing (if there was one)

		if (keyval_col != 0x0f)  // if there was 
			isButtonPushed = 1;
		else
		if ((++column) == 4)
			column = 0;

		delayIteration++;
	}

	if (!isButtonPushed)
	{
		return keypad_button_ascii[15][1];
	}

	keyval_row = RUN_ZERO[column] & 0xf0;
	num_led = keyval_row | keyval_col;

	int i;
	for (i = 0; i < 16; i++)
	{
		if (num_led == keypad_button_ascii[i][0])
			break;
	}

	return keypad_button_ascii[i][1];
}

void initPortB_KEYPAD(void)
{
	unsigned int portMap;

	portMap = TRISB;
	portMap &= 0xffff7fff;  // init RB.15 be input (must be in each init of Port B)
	portMap |= 0xF;  // init RB.0 til 3 be output (for getting the pushed-button signal from keypad, which means optianal in init Port B)
	TRISB = portMap;

	AD1PCFG |= 0x800f;  // RB.0 til 3, and 15, are init to be digital port

	// need in each init Port B
	CNCONbits.ON = 0;
	CNEN = 0x3c;
	CNPUE = 0x3c;
	CNCONbits.ON = 1;
}

void initPortE_KEYPAD(void)
{
	unsigned int portMap;

	portMap = TRISE;
	portMap &= 0xffffff00;  // RE.0 til 7 are init for input (the connection between mips32 to LCD, LEDS, and etc)
	TRISE = portMap;
}

void initPortF_KEYPAD(void)
{
	unsigned int portMap;

	portMap = TRISF;
	portMap &= 0xfffffef8;  // RF.0 til 2 are init for input for decoder 3x8
	// RF.8 init for input for decoder 3x8 Enable/Disable
	TRISF = portMap;

	PORTFbits.RF8 = 1;  // the decoder is Enable
}

void initPortG_KEYPAD(void)
{
	unsigned int portMap;

	portMap = TRISG;
	portMap &= 0xfffffffc;  // RG.0 til 1 are init for input for decoder 2x4
	TRISG = portMap;

	PORTG = 0x00;
}


void delay(void)
{
	int i;
	for (i = 0; i < 6400; i++);
}


//////////////////////////// DAC //////////////////////////////////


void drawSutardayElevator(void)
{
	//Go Up
	int startFloor = 0;
	int endFloor = 1;
	do{
		int keyValue = pow(2, endFloor);
		lightLeds(keyValue, 64);

		drawElevator(startFloor, endFloor);
		drawElevator(endFloor, endFloor);
		startFloor++;
		endFloor++;
	} while (endFloor <= 7);

	//Go Down
	startFloor = 7;
	endFloor = 6;
	do{
		int keyValue = pow(2, endFloor);
		lightLeds(keyValue, 64);

		drawElevator(startFloor, endFloor);
		drawElevator(endFloor, endFloor); // elevator waits;
		startFloor--;
		endFloor--;

	} while (endFloor >= 0);
}

void drawElevator(int floorNow, int floorToBe)
{
	// TODO: chage locations of init to drawBox?? 
	initPortD_DAC();
	initPortE_DAC();
	initPortF_DAC();

	int floorNowValue = floorNow * 20;
	int floorToBeValue = floorToBe * 20;
	int height = 255 - 7 * 20; // 20 pixel height, 7 heighest floor, 255 max height possible

	int range = floorToBe - floorNow;
	if (range == 0)
	{
		drawInPlace(floorNowValue, height);  // TODO: add 'if' statement. 
											// if isSutarday var is 1 -> go to 'drawInPlace_Sutarday' (not written yet)
											// else -> go to drawInPlace
											// TODO: in 'drawInPlace_Sutarday' add timer
	}
	else if (range > 0)
	{
		drawElevatorUp(floorNowValue, floorToBeValue, height);
	}
	else
	{
		drawElevatorDown(floorNowValue, floorToBeValue, height);
	}

}

void drawElevatorUp(int from, int to, int height)
{
	int i;
	int startY;
	int startX = 0x00;
	for (startY = from; startY < to; startY++)
	{
		drawBox(startX, startY, height, 0x33);
	}
}

void drawElevatorDown(int from, int to, int height)
{
	int i;
	int startY;
	int startX = 0x00;
	for (startY = from; startY > to; startY--)
	{
		drawBox(startX, startY, height, 0x33);
	}
}

void drawInPlace(int in, int height)
{
	int i;
	int startY;
	int startX = 0x00;
	for (startY = 0; startY < 100; startY++)
	{
		drawBox(startX, startY, height, 0x33);
		if (isSutarday & 1)
			break;
	}

}


void drawBox(int buttomLeftX, int buttomLeftY, int heigth, int width)
{
	int i;
	int x, y;
	for (i = 0; i < 3000; i++)
	{
		PORTF = 0x05;

		// draw base
		drawHorizonalLine(buttomLeftX, buttomLeftY, width);

		// draw Up-base
		drawHorizonalLine(buttomLeftX, buttomLeftY + heigth, width);

		// draw Left-base
		drawVerticalLine(buttomLeftX, buttomLeftY, heigth);

		// draw Right-base
		drawVerticalLine(buttomLeftX + width, buttomLeftY, heigth);
	}
}

void drawHorizonalLine(int startX, int startY, int length)
{
	int x = startX;
	int y = startY;

	while (x < length + 1)
	{
		drawPoint(x, y);
		y++;
	}
}

void drawVerticalLine(int startX, int startY, int length)
{
	int x = startX;
	int y = startY;

	while (y < length + 1)
	{
		drawPoint(x, y);
		x++;
	}
}


void drawPoint(int x, int y)
{
	PORTF = 0x05;  // Select DAC Channel A
	PORTE = x;
	PORTDbits.RD4 = 1;
	PORTDbits.RD4 = 0;

	PORTF = 0x06;  // Select DAC Channel B
	PORTE = y;
	PORTDbits.RD4 = 1;
	PORTDbits.RD4 = 0;
}

// inits for DAC
void initPortD_DAC(void)
{
	unsigned int portMap;

	portMap = TRISD;
	portMap &= 0xffffff4f;  // RD.4 and RD.5 are init for input. As well as RD.7 (don't know why)
	// TODO: ask why RD.7 is input? 
	TRISD = portMap;

	PORTDbits.RD4 = 0;  // DAC is disabled
	PORTDbits.RD5 = 0;  // DAC is synchomized
}

void initPortE_DAC(void)
{
	unsigned int portMap;

	portMap = TRISE;
	portMap &= 0xffffff00;  // RE.0 til 7 are init for input (the connection between mips32 to LCD, LEDS, and etc)
	TRISE = portMap;

	PORTE = 0x00;  // inject zero do DAC (for init)
}

void initPortF_DAC(void)
{
	unsigned int portMap;

	portMap = TRISF;
	portMap &= 0xfffffef8;  // RF.0 til 2 are init for input for decoder 3x8
	// RF.8 init for input for decoder 3x8 Enable/Disable
	TRISF = portMap;

	PORTFbits.RF8 = 1;  // the decoder is Enable
}
















