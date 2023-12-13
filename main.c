#include <avr/io.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>

#define FREE 0
#define FEED 1
#define PLAY 2
#define FEEL 3
#define ON 1
#define OFF 0

unsigned char ddrc[4];
volatile int cnt;
volatile int status = FREE;
volatile int wait_time;
volatile int feed_count = 0;
volatile int state = OFF;

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
			wait_time = 10;
			feed_count = 100;
			status = FEED;
		}
		else if (status == FEED) {
			wait_time = rand() % 7 + 1;
			status = FREE;
		}
		cnt = 0;
	}
}

void init() {
	DDRC = 0xff;
	DDRG = 0x0f;
	DDRE = 0x00;
	
	
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

int main() {
	init();
	int level = 0;
	
	
	wait_time = 3;
	
	sei(); // 타이머
	
	
	while (1) {
		
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
		}
	}
}
