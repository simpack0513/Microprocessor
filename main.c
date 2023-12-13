#include <avr/io.h>
#define F_CPU 16000000UL
#define __DELAY_BACKWARD_COMPATIBLE__
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>

#define FREE 0
#define FEED 1
#define FEEL 2
#define PLAY 3
#define ON 1
#define OFF 0
#define UP 1
#define DOWN 0

#define TRIG 6  // 포트 E의 6번핀
#define ECHO 7  // 포트 E의 7번핀

unsigned char ddrc[4];
volatile int cnt;
volatile int status = FREE;
volatile int wait_time;
volatile int feed_count = 0;
volatile int state = OFF;
volatile int u_state = UP;
volatile int feel_count = 0;


unsigned char fnd[4];
unsigned char lv[2] = {
	0x38,
	0xBE,
};

unsigned char digit[10] = {
	0x3f,
	0x06,
	0x5b,
	0x4f,
	0x66,
	0x6d,
	0x7c,
	0x07,
	0x7f,
	0x67
};
unsigned char fnd_sel[4] = {
	0x08,
	0x04,
	0x02,
	0x01
};

ISR(TIMER0_OVF_vect){
	cnt++;
	TCNT0 = 131;
	
	if(cnt == 500 * wait_time){
		if (status == FREE) {
			int num = rand() % 2 + 1;
			wait_time = 10;
			if (num == FEED) {
				feed_count = 100;
				status = FEED;
			}
			else if (num == FEEL) {
				feel_count = 20;
				status = FEEL;
			}
		}
		else if (status == FEED) {
			wait_time = rand() % 7 + 1;
			status = FREE;
		}
		else if (status == FEEL) {
			wait_time = rand() % 7 + 1;
			status = FREE;
		}
		cnt = 0;
	}
}

void init() {
	DDRC = 0xff;
	DDRG = 0x0f;
	DDRE = ((DDRE|(1<<TRIG)) & ~(1<<ECHO)); 
	
	
	// 타미어 설정
	TCCR0 = (0<<WGM01)|(0<<WGM00)|(0<<COM01)|(0<<COM00)|(1<<CS02)|(1<<CS01)|(0<<CS00);
	TIMSK = (1<<TOIE0);
	TCNT0 = 131;
	//
	
}

void show_level(int level) {
	ddrc[0] = lv[0];
	ddrc[1] = lv[1];
	ddrc[2] = digit[level / 10];
	ddrc[3] = digit[level % 10];
	
	for (int i = 0; i < 4; i++) {
		PORTC = ddrc[i];
		PORTG = fnd_sel[i];
		_delay_ms(2);
	}
}

void show_feed() {
	ddrc[0] = 0x71;
	ddrc[1] = 0x79;
	ddrc[2] = 0x79;
	ddrc[3] = 0x3f;
	
	for (int i = 0; i < 4; i++) {
		PORTC = ddrc[i];
		PORTG = fnd_sel[i];
		_delay_ms(2);
	}
}

void show_feel() {
	ddrc[0] = 0x71;
	ddrc[1] = 0x79;
	ddrc[2] = 0x79;
	ddrc[3] = 0x38;
	
	for (int i = 0; i < 4; i++) {
		PORTC = ddrc[i];
		PORTG = fnd_sel[i];
		_delay_ms(2);
	}
}

void display_fnd(unsigned int value)
{
	int i;
	fnd[0] = (value/1000)%10;
	fnd[1] = (value/100)%10;
	fnd[2] = (value/10)%10;
	fnd[3] = (value/1)%10;
	for(i=0; i<4; i++)
	{
		PORTC = digit[fnd[i]] | (i==1 ? 0x80 : 0x00);
		PORTG = fnd_sel[i];
		_delay_ms(2);
		if(i%2)
		_delay_ms(1);
	}
}

int main() {
	init();
	int level = 0;
	unsigned int distance;
	
	
	wait_time = 3;
	
	sei(); // 타이머
	
	
	while (1) {
		// 초음파 제어
		if (status == FEEL) {
			TCCR1B = 0x02;  // 분주비 설정  0b00000010
			PORTE &= ~(1<<TRIG);
			_delay_us(10);
			PORTE |= (1<<TRIG);
			_delay_us(10);
			PORTE &= ~(1<<TRIG);

			while(!(PINE & (1<<ECHO)));
			TCNT1 = 0x0000;
			while(PINE & (1<<ECHO)) ;

			TCCR1B = 0x00;
			distance = (unsigned int)(TCNT1 / 2 / 5.8);
			if (distance >= 100) {
				if (u_state == DOWN) {
					u_state = UP;
					feel_count -= 1;
					if (feel_count <= 0) {
						level += 1;
						cnt = 0;
						wait_time = rand() % 7 + 5;
						status = FREE;
					}
				}
			}
			else {
				if (u_state == UP) {
					u_state = DOWN;
					feel_count -= 1;
					if (feel_count <= 0) {
						level += 1;
						cnt = 0;
						wait_time = rand() % 7 + 5;
						status = FREE;
					}
				}
			}
		}
		//초음파 끝
		
		// 스위치 제어
		if ((PINE & 0x10) == 0x00) {
			if (state == OFF) {
				state = ON;
				if (status == FEED) {
					feed_count -= 1;
					if (feed_count == 0) {
						level += 1;
						cnt = 0;
						wait_time = rand() % 7 + 5;
						status = FREE;
					}
				}
			}
			else {
				state = OFF;
			}
		}
		// 스위치 끝
		
		switch (status) {
			case FREE:
				show_level(level);
				break;
			case FEED:
				show_feed();
				break;
			case FEEL:
				show_feel();
				break;
		}
	}
}
