/*
 * Created: 6/10/2020 12:25:02 AM
 * Author : Jose
 */ 

#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include "avr.h"
#include "avr.c"
#include "lcd.h"
#include "lcd.c"


#define A 38 // Lock ^
#define C 32 // Failed
#define G 21 // Unlock

#define Q 2 // 1.5 // 4

#define modifier 0.00003

struct note {
	int freq;
	int duration;
};

struct lock {
	int pin;
	int set;
	int index;
	int history[5];
};

struct timer {
	int seconds;	
};

int get_key(void)
/* 0 -> no key | 1-16 -> key */
{
	int r, c;
	for (r = 0; r < 4; ++r)
	{
		for (c = 0; c < 4; ++c)
		{
			if (is_pressed(r, c))
			{
				return (r * 4 + c) + 1;
			}
		}
	}
	return 0;
}

int is_pressed(int r, int c)
{
	// SET ALL PINS TO N/C
	DDRC = 0;
	PORTC = 0;
	
	// Set "r" PIN to SO
	SET_BIT(DDRC, r);
	CLR_BIT(PORTC, r);
	
	// Set "c" PIN to W1
	CLR_BIT(DDRC, c + 4);
	SET_BIT(PORTC, c + 4);
	
	// short wait
	avr_wait(1);
	
	// READ PIN at "c"
	if (GET_BIT(PINC, c + 4))
	{
		// if 0 -> return pressed
		return 0;
	}
	// return not_pressed
	return 1;
}

int set_get_pin(struct lock * lock)
{
	lcd_pos(1, 5);
	char symbol [] = {' ',
		'1', '2', '3', ' ',
		'4', '5', '6', ' ',
		'7', '8', '9', ' ',
		' ', '0', ' ', ' '};
		
	char result4 [] = "0000";
	
	int cap = 4;
	while (1)
	{
		avr_wait_msec(200);
		int i = get_key();
		switch (i)
		{
			case 1:
			case 2:
			case 3:
			case 5:
			case 6:
			case 7:
			case 9:
			case 10:
			case 11:
			case 14:
				if (cap == 4)
				{
					lcd_put(symbol[i]);
					result4[0] = symbol[i];
					cap--;
				}
				else if (cap == 3)
				{
					lcd_put(symbol[i]);
					result4[1] = symbol[i];
					cap--;
				}
				else if (cap == 2)
				{
					lcd_put(symbol[i]);
					result4[2] = symbol[i];
					cap--;
				}
				else if (cap == 1)
				{
					lcd_put(symbol[i]);
					result4[3] = symbol[i];
					cap--;
					if (lock->set == 0)
					{
						for (int i = 0; i < 5; i++)
						{
							if (atoi(result4) == lock->history[i])
							{
								lock->pin = -1;
								return atoi(result4);
							}
						}
						lock->history[lock->index] = atoi(result4);
						lock->pin = atoi(result4);
						lock->set = 1;
						lock->index++;
						if (lock->index > 4)
						{
							lock->index = 0;
						}
					}
					return atoi(result4);
				}
				break;
			case 16:
				// delete the previous key (based on cap)
				if (cap < 4)
					cap++;
				if (cap == 4)
				{
					lcd_pos(1, 5);
					lcd_puts(" ");
					lcd_pos(1, 5);
				}
				else if (cap == 3)
				{
					lcd_pos(1, 6);
					lcd_puts(" ");
					lcd_pos(1, 6);
				}
				else if (cap == 2)
				{
					lcd_pos(1, 7);
					lcd_puts(" ");
					lcd_pos(1, 7);
				}
				// repeat loop
				break;
			default:
				break;
		}
	}
}

void play_note(int frequency, int duration)
{
	int dur = duration;
	int freq = frequency;
	
	double ThTl = freq * modifier;
	double period = ThTl * 2;
	int k = dur / period;
	int i;
	for (i = 0; i < k; i++)
	{
		SET_BIT(PORTB, 3);
		avr_wait(freq);
		CLR_BIT(PORTB, 3);
		avr_wait(freq);
	}
}

int main(void)
{
	struct lock mLock = {-1, 0, 0, {-1, -1, -1, -1, -1}};
	int attempts = 0;
	lcd_init();
	SET_BIT(DDRB,3);
	while (1)
	{
		if (mLock.set == 0)
		{
			lcd_clr();
			lcd_puts("SET 4 DIGIT PIN:");
			// function that takes input
			set_get_pin(&mLock);
			// play lock sound
			if (mLock.pin == -1)
			{
				lcd_clr();
				lcd_puts("INVALID PIN:");
				lcd_pos(1, 0);
				lcd_puts("USED PREVIOUSLY");
				play_note(C, Q/2);
				play_note(C, Q/2);
				avr_wait_msec(1500);
			}
			else
			{
				avr_wait_msec(500);
				lcd_clr();
				lcd_puts("SETTING...");
				play_note(A*2, Q);
				avr_wait_msec(500);
			}
		}
		else
		{
			lcd_clr();
			lcd_puts("ENTER PIN:");
			int p = set_get_pin(&mLock);
			// check if input matches code
			if (mLock.pin == p)
			{
				// reset attempts
				// play unlock sound
				attempts = 0;
				avr_wait_msec(500);
				lcd_clr();
				lcd_puts("UNLOCKED");
				play_note(G, Q);
				avr_wait_msec(500);
				lcd_clr();
				lcd_puts("* TO RESET PIN");
				lcd_pos(1, 0);
				lcd_puts("# TO LOCK");
				int i = 1;
				while (i)
				{
					avr_wait_msec(100);
					int key = get_key();
					switch (key)
					{
						case 13: // reset password
							mLock.set = 0;
							i = 0;
							break;
						case 15: // lock
							lcd_clr();
							lcd_puts("LOCKING...");
							play_note(A*2, Q);
							avr_wait_msec(1000);
							i = 0;
							break;
						default:
							break;
					}
				}
			}
			else
			{
				// play fail sound
				avr_wait_msec(500);
				lcd_clr();
				lcd_puts("INCORRECT PIN");
				play_note(C, Q/2);
				play_note(C, Q/2);
				avr_wait_msec(500);
				// tally
				attempts++;
				if (attempts == 3)
				{
					// lock for 1 minute
					lcd_clr();
					lcd_puts("LOCKED OUT FOR");
					struct timer t = {60};
					while (t.seconds != 0)
					{
						lcd_pos(1, 0);
						char buf[17];
						sprintf(buf, "%02d secs", t.seconds);
						lcd_puts(buf);
						avr_wait_msec(1000);
						t.seconds--;
					}
					attempts = 0;
				}
			}
		}
	}
}

