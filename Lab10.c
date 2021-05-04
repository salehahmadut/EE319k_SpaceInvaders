  
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
void DisplayScore(int32_t thescore, int32_t x, int32_t y);

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

//Defining structs, global variables

struct sprite {
	int32_t x;
	int32_t y;
	int32_t vx,vy;
	const uint8_t *image1;
	status_t status;
	int32_t health;
};

struct player {
	int32_t x;
	int32_t y;
	int32_t vx,vy;
	const uint8_t *image;
};

struct heart {
	int32_t x;
	int32_t y;
  const uint8_t *img;
};

typedef struct player player_t;
typedef struct sprite sprite_t;
typedef struct heart heart_t;


#define MaxAliens 8
#define MaxMissiles 8
#define MaxHearts 3
#define MaxLasers 5

int NeedToWrite = 0;
int adcData = 0;
int lvl=0;
int health = 6;
int32_t score = 0;

sprite_t aliens[MaxAliens];
sprite_t aliensrow2[MaxAliens];
sprite_t aliensrow3[MaxAliens];
player_t spaceship;
sprite_t missiles[MaxMissiles];
heart_t hearts[MaxHearts];
sprite_t lasers[MaxLasers];


//Initializations

void AlienRowInit(sprite_t aliens[], int32_t y, status_t status)
{
	for(int i = 0; i<MaxAliens; i++)
	{
		aliens[i].x = 12*i + 10;
		aliens[i].y = y;
		aliens[i].vx = 1;
		aliens[i].vy = 1;
		aliens[i].image1 = Alien10pointA;
		aliens[i].status = status;
		aliens[i].health = 3;
	}
}

void Init(void) {
	AlienRowInit(aliens, 20, dead);
	AlienRowInit(aliensrow2, 30, dead);
	AlienRowInit(aliensrow3, 40, dead);
	for(int j = 0; j<MaxHearts; j++)
	{
		hearts[j].x = 12*j;
		hearts[j].y = 9;
		hearts[j].img = halfheart;
	}
	spaceship.x = 63;
	spaceship.y = 62;
	spaceship.vx = 0;
	spaceship.vy = 0;
	spaceship.image = PlayerShip1;
	for(int j = 0; j<MaxMissiles; j++)
	{
		missiles[j].status = dead;
	}
	for(int k = 0; k<MaxLasers; k++)
	{
		lasers[k].status = dead;
	}
	score = 0;
	health = 6;
}

void Systick_Init(){
	NVIC_ST_CTRL_R = 0;
	NVIC_ST_RELOAD_R = 80000000/20 -1;
	NVIC_ST_CURRENT_R = 0;
	NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x40000000;
	NVIC_ST_CTRL_R = 7;
}

//Sprite & Player Movement
void AlienMove(sprite_t aliens[]) {
	if(((aliens[MaxAliens-1].x + aliens[MaxAliens-1].vx) > 115) || ((aliens[0].x + aliens[0].vx) < 0))
	{
		for(int i = 0; i<MaxAliens; i++)
		{
			aliens[i].vx = -aliens[i].vx;
			aliens[i].vy = 1;
		}
	}
	for(int j = 0; j<MaxAliens; j++)
	{
		aliens[j].x += aliens[j].vx;
		aliens[j].y += aliens[j].vy;
	}
	for(int k = 0; k<MaxAliens; k++)
	{
		aliens[k].vy = 0;
	}
}

void PlayerMove() {
	adcData = ADC_In();
	spaceship.x = (110*adcData)/4096;
	spaceship.y = 55;
}

void LaserMove(void){
	for(int i = 0; i<MaxLasers; i++)
	{
		if(lasers[i].y > 63)
		{
			lasers[i].status = dead;
		}
		if(lasers[i].status == alive)
		{
			lasers[i].x += lasers[i].vx;
			lasers[i].y += lasers[i].vy;
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

//Alien & Player Fire

void Fire(int32_t vx, int32_t vy, const uint8_t *img)
{
  int i = 0;
	while(missiles[i].status == alive)
	{
		i++;
		if(i==MaxMissiles) return;
	}
	missiles[i].x = spaceship.x+8;
	missiles[i].y = spaceship.y-6;
	missiles[i].vx = vx;
	missiles[i].vy = vy;
	missiles[i].image1 = Missile0;
	missiles[i].status = alive;
	return;
}

void AlienFire(void)
{
	int rand = Random32() % MaxAliens;
	int i = 0;
	while(lasers[i].status == alive || aliens[rand].status == dead)
	{
	  if(lasers[i].status == alive)
		{
			i++;
		}
		if(aliens[rand].status == dead)
		{
			rand = Random32() % MaxAliens;
		}
		if(i==MaxMissiles) return;
	}
	
	lasers[i].x = aliens[rand].x+6;
	lasers[i].y = aliens[rand].y;
	lasers[i].vx = 0;
	lasers[i].vy = 2;
	lasers[i].image1 = Laser0;
	lasers[i].status = alive;
	return;
	
}

void Collisions(void){
  for(int i = 0; i<MaxMissiles; i++)
	{
		for(int l = 0; l<MaxAliens; l++)
		{
			if(missiles[i].status == alive && aliensrow3[l].status == alive)
			{
				if(abs(aliensrow3[l].x - missiles[i].x)<12 && abs(aliensrow3[l].y - missiles[i].y) <8)
				{
					missiles[i].status = dead;
					aliensrow3[l].health--;
					if(aliensrow3[l].health == 0)
					{
						aliensrow3[l].status = dead;
						Sound_invaderkilled();
						score += 100; 
					}
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
					aliensrow2[k].health--;
					if(aliensrow2[k].health == 0)
					{
						aliensrow2[k].status = dead;
						Sound_invaderkilled();
						score += 125;
					}
				}
			}
		}
		for(int j = 0; j<MaxAliens; j++)
		{
			if(missiles[i].status == alive && aliens[j].status == alive)
			{
				if(abs(aliens[j].x - missiles[i].x)<12 && abs(aliens[j].y - missiles[i].y) <8)
				{
					missiles[i].status = dead;
					aliens[j].health--;
					if(aliens[j].health == 0)
					{
						aliens[j].status = dead;
						Sound_invaderkilled();
						score += 150;
					}
				}
			}
		}
		for(int m = 0; m<MaxLasers; m++)
		{
			if(lasers[m].status == alive)
			{
				if(abs(lasers[m].x - spaceship.x)<16 && abs(lasers[m].y - spaceship.y)<6)
				{
					lasers[m].status = dead;
					health--;
					Sound_Explosion();
					score -= 200;
					if(score<0)
					{
						score = 0;
					}
				}					
			}
		}
	} 
		
}

int gonext=0;

void LevelOver(void){
	uint32_t btnInput = Switch_In();
	if(lvl==0){
		while((btnInput&0x01)==0)
	{
		btnInput = Switch_In();
	}
	gonext=1;
	}
	else if(lvl==1){
		for(int i=0; i<MaxAliens; i++){
			if(aliens[i].status == alive){
				return;
			}
		}
		gonext=1;
	}
	else if(lvl==2){
		for(int i=0; i<MaxAliens; i++){
			if(aliens[i].status == alive || aliensrow2[i].status==alive){
				return;
			}
		}
		gonext=1;
	}
	else if(lvl==3){
		for(int i=0; i<MaxAliens; i++){
			if(aliens[i].status == alive || aliensrow2[i].status==alive || aliensrow3[i].status==alive){
				return;
			}
		}
		gonext=1;
	}
	else if(lvl==4){
		if((btnInput&0x02)==2)
	{
		lvl=0;
	}
	}
	else if(lvl==5){
		if((btnInput&0x02)==2)
	{
		lvl=0;
	}
	}
}

void NextLevel(void){
	if(gonext==1){
		lvl++;
		if(lvl==1){
			health = 6;
			score = 0;
			AlienRowInit(aliens, 20, alive);
		}
		if(lvl==2){
			AlienRowInit(aliens, 20, alive);
			AlienRowInit(aliensrow2, 30, alive);
		}
		else if(lvl==3){
			AlienRowInit(aliens, 20, alive);
			AlienRowInit(aliensrow2, 30, alive);
			AlienRowInit(aliensrow3, 40, alive);
		}
		else if(lvl==4){
			AlienRowInit(aliens, 20, dead);
			AlienRowInit(aliensrow2, 30, dead);
			AlienRowInit(aliensrow3, 40, dead);
			SSD1306_ClearBuffer();
			SSD1306_OutClear();  
			SSD1306_SetCursor(1, 1);
			SSD1306_OutString("Congratulations!");
			SSD1306_SetCursor(1, 2);
			SSD1306_OutString("You Win!");
			SSD1306_SetCursor(1, 3);
			SSD1306_OutString("Score:");
			SSD1306_OutSDec(score);
			SSD1306_SetCursor(1,4);
			SSD1306_OutString("Press Button 2 to ");
			SSD1306_SetCursor(1, 5);
			SSD1306_OutString("Play Again");
		}
		else if(lvl==5){
			AlienRowInit(aliens, 20, dead);
			AlienRowInit(aliensrow2, 30, dead);
			AlienRowInit(aliensrow3, 40, dead);
			SSD1306_ClearBuffer();
			SSD1306_OutClear();  
			SSD1306_SetCursor(1, 1);
			SSD1306_OutString("Game Over");
			SSD1306_SetCursor(1, 2);
			SSD1306_OutString("You Lose");
			SSD1306_SetCursor(1, 3);
			SSD1306_OutString("Score:");
			SSD1306_OutSDec(score);
			SSD1306_SetCursor(1,4);
			SSD1306_OutString("Press Button 2 to ");
			SSD1306_SetCursor(1, 5);
			SSD1306_OutString("Play Again");
		}
		gonext=0;
	}
	
}
void IsLost(void){
	if(lvl==1){
		for(int i=0; i<MaxAliens; i++){
			if((aliens[i].status==alive && aliens[i].y>spaceship.y) || health == 0){
				lvl=4;            //goes to lvl 4 so that the gonext function automatically increments to lvl5
				gonext=1;
			}
		}
	}
	else if(lvl==2){
		for(int i=0; i<MaxAliens; i++){
			if((aliens[i].status==alive && aliens[i].y>spaceship.y) || (aliensrow2[i].status==alive && aliensrow2[i].y>spaceship.y) || health == 0){
				lvl=4;
				gonext=1;
			}
		}
	}
	else if(lvl==3){
		for(int i=0; i<MaxAliens; i++){
			if((aliens[i].status==alive && aliens[i].y>spaceship.y) || (aliensrow2[i].status==alive && aliensrow2[i].y>spaceship.y) || (aliensrow3[i].status==alive && aliensrow3[i].y>spaceship.y) || health == 0){
				lvl=4;
			gonext=1;
			}
		}
	}
}
void SetHearts(int32_t health, int32_t numHearts)
{
	for(int i = 0; i<numHearts; i++)
	{
		if(health >= 2*i + 2)
		{
			hearts[i].img = fullheart;
		}
		else if(health == 2*i+1)
		{
			hearts[i].img = halfheart;
		}
		else
		{
			hearts[i].img = emptyheart;
		}
	}
}
void UpdateAlienImg(sprite_t aliens[])
{
	for(int i = 0; i<MaxAliens; i++)
	{
		if(aliens[i].status == alive)
		{
			if(aliens[i].health == 3)
			{
				aliens[i].image1 = Alien10pointA;
			}
			if(aliens[i].health == 2)
			{
				aliens[i].image1 = alien10pointAdamaged;
			}
			if(aliens[i].health == 1)
			{
				aliens[i].image1 = alien10pointAverydamaged;
			}
		}
	}
}

char scoreArr[5] = {'0','0','0','0','0'};

char ScoreToString(int32_t digit)
{
	char letter = ' ';
  switch(digit) {
		case 0:
			letter = '0';
			break;
		case 1:
			letter = '1';
			break;
		case 2:
			letter = '2';
			break;
		case 3:
			letter = '3';
			break;
		case 4:
			letter = '4';
			break;
		case 5: 
			letter = '5';
			break;
		case 6:
			letter = '6';
			break;
		case 7:
			letter = '7';
			break;
		case 8:
			letter = '8';
			break;
		case 9:
			letter = '9';
			break;
	}
	return letter;
}

void DisplayScore(int32_t thescore, int32_t x, int32_t y)
{
	int ind = 4;
	char numchar = '0';
	int dig = 0;
	if(thescore==0)
	{
		for(int i = 0; i<5; i++)
		{
			scoreArr[i] = '0';
		}
	}
	while(thescore>0)
	{
		dig = thescore % 10;
		numchar = ScoreToString(dig);
		scoreArr[ind] = numchar;
		ind--;
		thescore /= 10;
	}
	
	SSD1306_DrawString(x, y, scoreArr, SSD1306_WHITE);
}

void SysTick_Handler(void) {
	uint32_t btnInput = Switch_In();
	if(lvl==0){
	SSD1306_ClearBuffer();
  SSD1306_DrawBMP(2, 62, SpaceInvadersMarquee, 0, SSD1306_WHITE);
  SSD1306_OutBuffer();
	
	}
	if((btnInput&0x01)==1)
	{
		Fire(0, -2, Missile0);
		Sound_Shoot();
	}
	AlienMove(aliens);
	AlienMove(aliensrow2);
	AlienMove(aliensrow3);
	PlayerMove();
	Collisions();
	MissileMove();
	LaserMove();
	IsLost();
	LevelOver();
	NextLevel();
	SetHearts(health, MaxHearts);
	UpdateAlienImg(aliens);
	UpdateAlienImg(aliensrow2);
	UpdateAlienImg(aliensrow3);
	if(lvl!=4 && lvl!=5){
		NeedToWrite = 1;
	}
}

void Draw(void) {
	SSD1306_ClearBuffer();
	DisplayScore(score, 97, 0);
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
	for(int m = 0; m<MaxLasers; m++)
	{
		if(lasers[m].status == alive)
		{
			SSD1306_DrawBMP(lasers[m].x, lasers[m].y, lasers[m].image1, 0, SSD1306_INVERSE);
		}
	}
	for(int k = 0; k<MaxHearts; k++)
	{
		SSD1306_DrawBMP(hearts[k].x, hearts[k].y, hearts[k].img, 0, SSD1306_INVERSE);
	}
	if(lvl!=0){
	SSD1306_DrawBMP(spaceship.x, spaceship.y, spaceship.image, 0, SSD1306_INVERSE);
	}
	SSD1306_OutBuffer();
}

int main(void){
	DisableInterrupts();
	PLL_Init();
	SSD1306_Init(SSD1306_SWITCHCAPVCC);
  SSD1306_OutClear();
	Init();
	Systick_Init();
	ADC_Init(4);
	Switch_Init();
	Sound_Init();
	EnableInterrupts();
	Timer1_Init(&AlienFire, 80000000);
	while(1)
	{
		if(NeedToWrite)
		{
			Draw();
			NeedToWrite = 0;
		}
	}
}
	
