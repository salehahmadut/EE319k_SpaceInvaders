// Lab10.c
// Runs on TM4C123
// Jonathan Valvano and Daniel Valvano
// This is a starter project for the EE319K Lab 10

// Last Modified: 1/16/2021 
// http://www.spaceinvaders.de/
// sounds at http://www.classicgaming.cc/classics/spaceinvaders/sounds.php
// http://www.classicgaming.cc/classics/spaceinvaders/playguide.php
/* 
 Copyright 2021 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */
// ******* Possible Hardware I/O connections*******************
// Slide pot pin 1 connected to ground
// Slide pot pin 2 connected to PD2/AIN5
// Slide pot pin 3 connected to +3.3V 
// fire button connected to PE0
// special weapon fire button connected to PE1
// 8*R resistor DAC bit 0 on PB0 (least significant bit)
// 4*R resistor DAC bit 1 on PB1
// 2*R resistor DAC bit 2 on PB2
// 1*R resistor DAC bit 3 on PB3 (most significant bit)
// LED on PB4
// LED on PB5

// VCC   3.3V power to OLED
// GND   ground
// SCL   PD0 I2C clock (add 1.5k resistor from SCL to 3.3V)
// SDA   PD1 I2C data

//************WARNING***********
// The LaunchPad has PB7 connected to PD1, PB6 connected to PD0
// Option 1) do not use PB7 and PB6
// Option 2) remove 0-ohm resistors R9 R10 on LaunchPad
//******************************

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/CortexM.h"
#include "SSD1306.h"
#include "Print.h"
#include "Random.h"
#include "ADC.h"
#include "Images.h"
#include "Sound.h"
#include "Timer0.h"
#include "Timer1.h"
#include "TExaS.h"
#include "Switch.h"
#include <stdlib.h>
//********************************************************************************
// debuging profile, pick up to 7 unused bits and send to Logic Analyzer
#define PB54                  (*((volatile uint32_t *)0x400050C0)) // bits 5-4
#define PF321                 (*((volatile uint32_t *)0x40025038)) // bits 3-1
// use for debugging profile
#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))
#define PB5       (*((volatile uint32_t *)0x40005080)) 
#define PB4       (*((volatile uint32_t *)0x40005040)) 
// TExaSdisplay logic analyzer shows 7 bits 0,PB5,PB4,PF3,PF2,PF1,0 
// edit this to output which pins you use for profiling
// you can output up to 7 pins
void LogicAnalyzerTask(void){
  UART0_DR_R = 0x80|PF321|PB54; // sends at 10kHz
}
void ScopeTask(void){  // called 10k/sec
  UART0_DR_R = (ADC1_SSFIFO3_R>>4); // send ADC to TExaSdisplay
}
// edit this to initialize which pins you use for profiling
void Profile_Init(void){
  SYSCTL_RCGCGPIO_R |= 0x22;      // activate port B,F
  while((SYSCTL_PRGPIO_R&0x20) != 0x20){};
  GPIO_PORTF_DIR_R |=  0x0E;   // output on PF3,2,1 
  GPIO_PORTF_DEN_R |=  0x0E;   // enable digital I/O on PF3,2,1
  GPIO_PORTB_DIR_R |=  0x30;   // output on PB4 PB5
  GPIO_PORTB_DEN_R |=  0x30;   // enable on PB4 PB5  
}
//********************************************************************************
 
void Delay100ms(uint32_t count); // time delay in 0.1 seconds

typedef enum {dead,alive} status_t;

struct sprite {
	int32_t x;
	int32_t y;
	int32_t vx,vy;
	const uint8_t *image1;
	status_t status;
};

struct player {
	int32_t x;
	int32_t y;
	int32_t vx,vy;
	const uint8_t *image;
};

typedef struct player player_t;
typedef struct sprite sprite_t;

#define MaxAliens 8
#define MaxMissiles 10
int NeedToWrite = 0;
int adcData = 0;

sprite_t aliens[MaxAliens];
sprite_t aliensrow2[MaxAliens];
sprite_t aliensrow3[MaxAliens];
player_t spaceship;
sprite_t missiles[MaxMissiles];

void Init(void) {
	for(int i = 0; i<MaxAliens; i++)
	{
		aliens[i].x = 12*i + 10;
		aliens[i].y = 10;
		aliens[i].vx = 1;
		aliens[i].vy = 0;
		aliens[i].image1 = Alien10pointA;
		aliens[i].status = alive;
		aliensrow2[i].x = 12*i + 10;
		aliensrow2[i].y = 20;
		aliensrow2[i].vx = 1;
		aliensrow2[i].vy = 0;
		aliensrow2[i].image1 = Alien10pointA;
		aliensrow2[i].status = alive;
		aliensrow3[i].x = 12*i + 10;
		aliensrow3[i].y = 30;
		aliensrow3[i].vx = 1;
		aliensrow3[i].vy = 0;
		aliensrow3[i].image1 = Alien10pointA;
		aliensrow3[i].status = alive;
	}
	spaceship.x = 63;
	spaceship.y = 55;
	spaceship.vx = 0;
	spaceship.vy = 0;
	spaceship.image = PlayerShip1;
	for(int j = 0; j<MaxMissiles; j++)
	{
		missiles[j].status = dead;
	}
}

void Systick_Init(){
	NVIC_ST_CTRL_R = 0;
	NVIC_ST_RELOAD_R = 80000000/20 -1;
	NVIC_ST_CURRENT_R = 0;
	NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x40000000;
	NVIC_ST_CTRL_R = 7;
}

void AlienMove(sprite_t aliens[]) {
	if(((aliens[MaxAliens-1].x + aliens[MaxAliens-1].vx) > 115) || ((aliens[0].x + aliens[0].vx) < 0))
	{
		for(int i = 0; i<MaxAliens; i++)
		{
			aliens[i].vx = -aliens[i].vx;
		}
	}
	for(int j = 0; j<MaxAliens; j++)
	{
		aliens[j].x += aliens[j].vx;
		aliens[j].y += aliens[j].vy;
	}
}

void PlayerMove() {
	adcData = ADC_In();
	spaceship.x = (110*adcData)/4096;
	spaceship.y = 55;
}

void Fire(int32_t vx, int32_t vy, const uint8_t *img)
{
  int i = 0;
	while(missiles[i].status == alive)
	{
		i++;
		if(i==MaxMissiles) return;
	}
	missiles[i].x = spaceship.x+7;
	missiles[i].y = spaceship.y+4;
	missiles[i].vx = vx;
	missiles[i].vy = vy;
	missiles[i].image1 = Missile0;
	missiles[i].status = alive;
	return;
}

void Collisions(void){
  for(int i = 0; i<MaxMissiles; i++)
	{
		for(int j = 0; j<MaxAliens; j++)
		{
			if(missiles[i].status == alive && aliens[j].status == alive)
			{
				if(abs(aliens[j].x - missiles[i].x)<12 && abs(aliens[j].y - missiles[i].y) <8)
				{
					missiles[i].status = dead;
					aliens[j].status = dead;
				}
			}
		}
		for(int k = 0; k<MaxAliens; k++)
		{
			if(missiles[i].status == alive && aliensrow2[k].status == alive)
			{
				if(abs(aliensrow2[k].x - missiles[i].x)<12 && abs(aliensrow2[k].y - missiles[i].y) <8)
				{
					missiles[i].status = dead;
					aliensrow2[k].status = dead;
				}
			}
		}
		for(int l = 0; l<MaxAliens; l++)
		{
			if(missiles[i].status == alive && aliensrow3[l].status == alive)
			{
				if(abs(aliensrow3[l].x - missiles[i].x)<12 && abs(aliensrow3[l].y - missiles[i].y) <8)
				{
					missiles[i].status = dead;
					aliensrow3[l].status = dead;
				}
			}
		}
	}
		
}
void MissileMove(void){
	for(int i = 0; i<MaxMissiles; i++)
	{
		if(missiles[i].y < 0)
		{
			missiles[i].status = dead;
		}
		if(missiles[i].status == alive)
		{
			missiles[i].x += missiles[i].vx;
			missiles[i].y += missiles[i].vy;
		}
	}
}


void SysTick_Handler(void) {
	uint32_t btnInput = Switch_In();
	//static uint32_t unpressed = 0;
	if((btnInput&0x01)==1)
	{
		Fire(0, -1, Missile0);
	}
	AlienMove(aliens);
	AlienMove(aliensrow2);
	AlienMove(aliensrow3);
	PlayerMove();
	MissileMove();
	Collisions();
	NeedToWrite = 1;
}

void Draw(void) {
	SSD1306_ClearBuffer();
	for(int i = 0; i<MaxAliens; i++)
	{
		if(aliens[i].status == alive)
		{
			SSD1306_DrawBMP(aliens[i].x, aliens[i].y, aliens[i].image1, 0, SSD1306_INVERSE);
		}
		if(aliensrow2[i].status == alive)
		{			
			SSD1306_DrawBMP(aliensrow2[i].x, aliensrow2[i].y, aliensrow2[i].image1, 0, SSD1306_INVERSE);
		}
		if(aliensrow3[i].status == alive)
		{
			SSD1306_DrawBMP(aliensrow3[i].x, aliensrow3[i].y, aliensrow3[i].image1, 0, SSD1306_INVERSE);
		}
	}
	for(int j = 0; j<MaxMissiles; j++)
	{
		if(missiles[j].status == alive)
		{
			SSD1306_DrawBMP(missiles[j].x, missiles[j].y, missiles[j].image1, 0, SSD1306_INVERSE);
		}
	}
	SSD1306_DrawBMP(spaceship.x, spaceship.y, spaceship.image, 0, SSD1306_INVERSE);
	SSD1306_OutBuffer();
	
}

int main(void){
	DisableInterrupts();
	PLL_Init();
	SSD1306_Init(SSD1306_SWITCHCAPVCC);
  SSD1306_OutClear();
	Init();
	Systick_Init();
	ADC_Init(5);
	Switch_Init();
	EnableInterrupts();
	while(1)
	{
		if(NeedToWrite)
		{
			Draw();
			NeedToWrite = 0;
		}
	}
}
	

int main1(void){
	uint32_t time=0;
  DisableInterrupts();
  // pick one of the following three lines, all three set to 80 MHz
  //PLL_Init();                   // 1) call to have no TExaS debugging
  TExaS_Init(&LogicAnalyzerTask); // 2) call to activate logic analyzer
  //TExaS_Init(&ScopeTask);       // or 3) call to activate analog scope PD2
  SSD1306_Init(SSD1306_SWITCHCAPVCC);
  SSD1306_OutClear();   
  Random_Init(1);
  Profile_Init(); // PB5,PB4,PF3,PF2,PF1 
  SSD1306_ClearBuffer();
  SSD1306_DrawBMP(2, 62, SpaceInvadersMarquee, 0, SSD1306_WHITE);
  SSD1306_OutBuffer();
  EnableInterrupts();
  Delay100ms(20);
  SSD1306_ClearBuffer();
  SSD1306_DrawBMP(47, 63, PlayerShip0, 0, SSD1306_WHITE); // player ship bottom
  SSD1306_DrawBMP(53, 55, Bunker0, 0, SSD1306_WHITE);

  SSD1306_DrawBMP(0, 9, Alien10pointA, 0, SSD1306_WHITE);
  SSD1306_DrawBMP(20,9, Alien10pointB, 0, SSD1306_WHITE);
  SSD1306_DrawBMP(40, 9, Alien20pointA, 0, SSD1306_WHITE);
  SSD1306_DrawBMP(60, 9, Alien20pointB, 0, SSD1306_WHITE);
  SSD1306_DrawBMP(80, 9, Alien30pointA, 0, SSD1306_WHITE);
  SSD1306_DrawBMP(50, 19, AlienBossA, 0, SSD1306_WHITE);
  SSD1306_OutBuffer();
  Delay100ms(30);

  SSD1306_OutClear();  
  SSD1306_SetCursor(1, 1);
  SSD1306_OutString("GAME OVER");
  SSD1306_SetCursor(1, 2);
  SSD1306_OutString("Nice try,");
  SSD1306_SetCursor(1, 3);
  SSD1306_OutString("Earthling!");
  SSD1306_SetCursor(2, 4);
  while(1){
    Delay100ms(10);
    SSD1306_SetCursor(19,0);
    SSD1306_OutUDec2(time);
    time++;
    PF1 ^= 0x02;
  }
}

// You can't use this timer, it is here for starter code only 
// you must use interrupts to perform delays
void Delay100ms(uint32_t count){uint32_t volatile time;
  while(count>0){
    time = 727240;  // 0.1sec at 80 MHz
    while(time){
	  	time--;
    }
    count--;
  }
}

