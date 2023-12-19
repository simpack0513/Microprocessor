#include <avr/io.h>
#define F_CPU 16000000UL
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

#define FS_SEL 131

unsigned char ddrc[4];
volatile int cnt;
volatile int status = FREE;
volatile int wait_time;
volatile int feed_count = 0;
volatile int state = OFF;
volatile int u_state = UP;
volatile int feel_count = 0;

//자이로 관련
int error_rom = 10;
int data_rom = 0;
int read_number = 0;
unsigned char data;
unsigned char dataAdress = 0x3B;//가속도 값 부터  각속독 값까지 읽어오는 주소
unsigned char datainformation[14];
unsigned int accle_X;
unsigned int accle_Y;
unsigned int accle_Z;
unsigned int temp;
float ftemp;
unsigned int angular_X;
unsigned int angular_Y;
unsigned int angular_Z;
//


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
			int num = rand() % 3 + 1;
			wait_time = 10;
			if (num == FEED) {
				feed_count = 100;
				status = FEED;
			}
			else if (num == FEEL) {
				feel_count = 20;
				status = FEEL;
			}
			else if (num == PLAY) {
				status = PLAY;
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
		else if (status == PLAY) {
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
	
	//I2C 설정
	TWBR = 12;//400KHZ 맞추기
	TWSR = 0x0;
	
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

void show_play() {
	ddrc[0] = 0x73;
	ddrc[1] = 0x38;
	ddrc[2] = 0x77;
	ddrc[3] = 0x6E;
	
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
		// 자이로 제어
		if (status == PLAY) {
			mpu6050_Write();
			mpu6050_read();
		}
		
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
			case PLAY:
				show_play();
				break;
		}
	}
}


//자이로센서 관련 함수

void mpu6050_Start(void){
	TWCR = 0xA4;//명령으로 줘서 TWINT 1 -> 0(자동클리어)이 되가지고 SCL 값이 High, SDA, SCL 유효
	//START MASTER 지정
	while(!(TWCR&0x80));//TWINT 0 -> 1 이 되가지고 SCL 값이 0 이 될떄까지 기다림.
	if((TWSR & 0xF8) != 0x08)Error((TWSR & 0xF8)); //스타트 문제 검출
}
void mpu6050_SLA_W(void){//들어가기 위한 비밀번호(SLA+W)
	TWDR = 0xD0;//무언가의 값을 쓸때 0xD0,mpu6050 읽을 떄  0xD1
	TWCR = 0x84;//명령으로 줘서 TWINT 1 -> 0(자동클리어)이 되가지고 SCL 값이 High, SDA, SCL 유효
	while(!(TWCR&0x80));//TWINT 0 -> 1 이 되가지고 SCL 값이 0 이 될떄까지 기다림.
	if((TWSR & 0xF8) != 0x18)Error((TWSR & 0xF8)); //SLA+W 문제 검출
	
	TWDR = 1;//어느 방에 쓰거나 읽을걸지
	TWCR = 0x84;//명령으로 줘서 TWINT 1 -> 0(자동클리어)이 되가지고 SCL 값이 High, SDA, SCL 유효
	while(!(TWCR&0x80));//TWINT 0 -> 1 이 되가지고 SCL 값이 0 이 될떄까지 기다림.
	if((TWSR & 0xF8) != 0x28)Error((TWSR & 0xF8)); //SLA+W 문제 검출
}
void mpu6050_Write(void){
	mpu6050_Start();
	mpu6050_SLA_W();
	TWDR = 'a';//무언가의 값을 쓸때 0xD0,mpu6050 읽을 떄  0xD1
	TWCR = 0x84;//명령으로 줘서 TWINT 1 -> 0(자동클리어)이 되가지고 SCL 값이 High, SDA, SCL 유효
	while(!(TWCR&0x80));//TWINT 0 -> 1 이 되가지고 SCL 값이 0 이 될떄까지 기다림.
	if((TWSR & 0xF8) != 0x28)Error((TWSR & 0xF8)); //데이터 쓸 때  문제 검출
	mpu6050_STOP();
}
void mpu6050_read(void){
	mpu6050_Start();
	mpu6050_SLA_W();
	TWCR = 0xA4;//명령으로 줘서 TWINT 1 -> 0(자동클리어)이 되가지고 SCL 값이 High, SDA, SCL 유효
	//START MASTER 지정
	while(!(TWCR&0x80));//TWINT 0 -> 1 이 되가지고 SCL 값이 0 이 될떄까지 기다림.
	if((TWSR & 0xF8) != 0x10)Error((TWSR & 0xF8)); //스타트 문제 검출
	
	TWDR = 0xD1;//무언가의 값을 쓸때 0xD0,mpu6050 읽을 떄  0xD1
	TWCR = 0x84;//명령으로 줘서 TWINT 1 -> 0(자동클리어)이 되가지고 SCL 값이 High, SDA, SCL 유효
	while(!(TWCR&0x80));//TWINT 0 -> 1 이 되가지고 SCL 값이 0 이 될떄까지 기다림.
	if((TWSR & 0xF8) != 0x40)Error((TWSR & 0xF8)); //SLA+W 문제 검출
	
	TWCR = 0x84;//명령으로 줘서 TWINT 1 -> 0(자동클리어)이 되가지고 SCL 값이 High, SDA, SCL 유효
	while(!(TWCR&0x80));//TWINT 0 -> 1 이 되가지고 SCL 값이 0 이 될떄까지 기다림.
	if((TWSR & 0xF8) != 0x58)Error((TWSR & 0xF8)); //읽을 준비 검출
	data = TWDR;
	
	mpu6050_STOP();
	
	while((EECR & 0x02) == 0x02); //받은 값 방 0번부터 시작해서 저장
	EEAR = data_rom++;
	EEDR = data;
	EECR |= 0x04;
	EECR |= 0x02;
}
void mpu6050_STOP(void){//들어가기 위한 비밀번호(SLA+W)
	
	TWCR = 0x94;//명령으로 줘서 TWINT 1 -> 0(자동클리어)이 되가지고 SCL 값이 High, SDA, SCL 유효
	//STOP 기능한다.
	_delay_ms(1000);
}
void Error(char error){
	if(error == 0x20)PORTC = 0x01;//SLA+W 신호송신후 확인신호가 없음
	else if(error == 0x38)PORTC = 0x02;// SLA+W 나 데이터 신호 후 문제 발생
	else if(error == 0x30)PORTC = 0x04;//테이터 신호 송신 후 확인신호가 순신되지 않음
	else PORTC = 0xFF;// TWI_START오류거나 그 외의 오류
	
	while((EECR & 0x02) == 0x02); //에러부분 저장 방 10번부터
	EEAR = error_rom++;
	EEDR = error;
	EECR |= 0x04;
	EECR |= 0x02;
	while(1);
}
