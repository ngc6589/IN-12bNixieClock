/* ----------------------------------------------------------------------------
 *  IN-12b �j�L�V�[�Ǖ\�����j�b�g����v���O����
 *  ���C�����W���[��
 *
 *  Author. �� ���_(Masahiro Kusunoki)
 *  http://mkusunoki.net
 *  Release 20131011 Rev 001
 * ---------------------------------------------------------------------------- */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "USART.h"
#include "TWI.h"
#include "RTC8564.h"

#define LED PORTC7
#define HC595RCLK PORTB0
#define K155ID1_A PORTB4
#define K155ID1_B PORTB5
#define K155ID1_C PORTB6
#define K155ID1_D PORTB7
#define COMMA PORTE6

void steins();
void cathodePoisoning();

/* ----------------------------------------------------------------------------
 *  EEPROM
 * ---------------------------------------------------------------------------- */
uint16_t EEMEM E_displayDateIntervalValue = 30;	// 0 - 65535
uint16_t EEMEM E_displayDateSecondsValue = 3;	// 0 - 65535
uint16_t EEMEM E_userDataTimerValue = 300;		// 0 - 65535
uint8_t EEMEM E_dispTimeFormat = 1;		// 0 = short 1 = long
uint8_t EEMEM E_IN12BrightDaytime = 6;	// 0 - 9
uint8_t EEMEM E_IN12BrightNight = 3;	// 0 - 9
uint8_t EEMEM E_hourDaytime = 7;		// 0 - 23
uint8_t EEMEM E_hourNight = 23;			// 0 - 23
uint8_t EEMEM E_hour12_24format = 1;  // 0=12 1=24 hour

/* ----------------------------------------------------------------------------
 *  �O���[�o���ϐ�
 * ---------------------------------------------------------------------------- */
USART Serial1;
TWI i2c;
RTC8564 rtc;

// LED �_�Ńt���O
int led = false;

// ���荞�ݏ����p�ϐ�
unsigned int interruptCount = 0;
int interruptOVF = 0;

// IN-12 �����Z�O�����g�A�J���}
unsigned char IN12Num[8] = { 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F };
unsigned char IN12Com[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
unsigned char IN12Digit = 0;
unsigned char IN12BrightDaytime = 9;
unsigned char IN12BrightNight = 3;

// ���v�\���b��
unsigned int displayDateIntervalValue = 600;  // �N�����\������b���Ԋu
unsigned int displayDateIntervalCount = 0;
unsigned int displayDateSecondsValue = 10;    // �N�����\���b��
unsigned char dispTimeFormat = 0;             // �����b�\���t�H�[�}�b�g
unsigned char hourDaytime = 7;
unsigned char hourNight = 22;
unsigned char hour12_24format = 1;  // 0 = 12, 1 = 24 hour display

// ���[�U�f�[�^�\���b��
unsigned int userDataTimerValue = 30;         // ���[�U�[�f�[�^��\�����鎞��
unsigned int userDataTimerCount = 0;

// �V���A���f�[�^
#define SERIALBUF_LEN 32
int serial0StringComplete = false;
char serial0String[SERIALBUF_LEN];
int serial0StringIdx = 0;
char msg[40];

/* ----------------------------------------------------------------------------
 * �^�C�}�[0 ���荞�ݏ���(1ms��)
 * ---------------------------------------------------------------------------- */
ISR(TIMER0_COMPA_vect) {
	unsigned char bright = 5;
	
	// K155ID1 �� A�`D �ɒl���Z�b�g
	// PORTB4�`7���N���A���āAIN12Num ���Z�b�g
	PORTB &= 0x0F;
	PORTB |= (IN12Num[IN12Digit]<<4);

	// �J���}�̐ݒ�
	if(IN12Com[IN12Digit] != 0) {
		PORTE |= (1<<COMMA);
		} else {
		PORTE &= ~(1<<COMMA);
	}

	// �j�L�V�[�ǃA�m�[�h ON
	PORTB &= ~(1<<HC595RCLK);
	SPDR = 1<<IN12Digit;
	while (!(SPSR & _BV(SPIF))) {
		;
	}
	PORTB |= (1<<HC595RCLK);

	// �j�L�V�[�ǂ̓_���f�B���C�𒋊ԂƖ�ԂŐ؂�ւ�����
	if(hourNight > hourDaytime) {
		if(rtc.bcdToBin(rtc.Hours) < hourDaytime || rtc.bcdToBin(rtc.Hours) >= hourNight) {
			bright = IN12BrightNight;
		} else {
			bright = IN12BrightDaytime;
		}
	} else {
		if(rtc.bcdToBin(rtc.Hours) >= hourNight && rtc.bcdToBin(rtc.Hours) < hourDaytime) {
			bright = IN12BrightNight;
		} else {
			bright = IN12BrightDaytime;
		}
	}
	
	switch(bright) {
		case 1:
		_delay_us(0);
		break;
		case 2:
		_delay_us(100);
		break;
		case 3:
		_delay_us(200);
		break;
		case 4:
		_delay_us(300);
		break;
		case 5:
		_delay_us(400);
		break;
		case 6:
		_delay_us(500);
		break;
		case 7:
		_delay_us(600);
		break;
		case 8:
		_delay_us(700);
		break;
		case 9:
		_delay_us(800);
		break;
		default:
		_delay_us(800);
	}
	
	// �j�L�V�[�ǃA�m�[�h OFF
	PORTB &= ~(1<<HC595RCLK);
	SPDR = 0;
	while (!(SPSR & _BV(SPIF))) {
		;
	}
	PORTB |= (1<<HC595RCLK);

	switch(bright) {
		case 1:
		_delay_us(800);
		break;
		case 2:
		_delay_us(700);
		break;
		case 3:
		_delay_us(600);
		break;
		case 4:
		_delay_us(500);
		break;
		case 5:
		_delay_us(400);
		break;
		case 6:
		_delay_us(300);
		break;
		case 7:
		_delay_us(200);
		break;
		case 8:
		_delay_us(100);
		break;
		case 9:
		_delay_us(0);
		break;
		default:
		_delay_us(0);
		break;
	}

	// ���̌��ʒu������
	IN12Digit++;
	if(IN12Digit > 7) {
		IN12Digit = 0;
	}
	
	// ���荞�݃J�E���^
	interruptCount++;
	// ���荞�݃}�X�N���Z�b�g
	TIFR0 |= (1<<OCF0A);
}

/* ----------------------------------------------------------------------------
 * �Z�b�g�A�b�v����
 * ---------------------------------------------------------------------------- */
void setup() {
	
	// Watchdog �^�C�}��~
	MCUSR = 0;
	wdt_disable();
	
	// SPI �ݒ�
	DDRB |= (1<<PORTB1);
	DDRB |= (1<<PORTB2);
	// Master �ݒ�
	SPCR |= (1<<MSTR);
	// ClockDivide 000 = 1/4
	SPCR &= ~(1<<SPR1);
	SPCR &= ~(1<<SPR0);
	SPSR &= ~(1<<SPI2X);
	// MODE 0
	SPCR &= ~(1<<CPOL);
	SPCR &= ~(1<<CPHA);
	// MSB First
	SPCR &= ~(1<<DORD);
	// SPI Enable
	SPCR |= (1<<SPE);

	// LED �|�[�g�o��
	DDRC |= (1<<LED);
	
	PORTD = 0;
	
	Serial1.begin();
	i2c.begin();

	// ���Z�b�g����̎��ԑ҂�(1�b)
	for(int i = 0; i < 100; i++) {
		_delay_ms(10);
	}
	rtc.begin();
	
	// 74HC595
	DDRB |= (1<<HC595RCLK);
	PORTB &= ~(1<<HC595RCLK);

	// K155ID1
	DDRB |= (1<<K155ID1_A);
	DDRB |= (1<<K155ID1_B);
	DDRB |= (1<<K155ID1_C);
	DDRB |= (1<<K155ID1_D);

	// IN12 �J���}
	DDRE |= (1<<COMMA);

	// EEPROM �ǂݏo��
	eeprom_busy_wait();
	displayDateIntervalValue = eeprom_read_word(&E_displayDateIntervalValue);
	eeprom_busy_wait();
	displayDateSecondsValue = eeprom_read_word(&E_displayDateSecondsValue);
	eeprom_busy_wait();
	userDataTimerValue = eeprom_read_word(&E_userDataTimerValue);
	eeprom_busy_wait();
	dispTimeFormat = eeprom_read_byte(&E_dispTimeFormat);
	eeprom_busy_wait();
	IN12BrightDaytime = eeprom_read_byte(&E_IN12BrightDaytime);
	eeprom_busy_wait();
	IN12BrightNight = eeprom_read_byte(&E_IN12BrightNight);
	eeprom_busy_wait();
	hourDaytime = eeprom_read_byte(&E_hourDaytime);
	eeprom_busy_wait();
	hourNight = eeprom_read_byte(&E_hourNight);
	eeprom_busy_wait();
	hour12_24format = eeprom_read_byte(&E_hour12_24format);

	// �^�C�}�[�A�J�E���^ 0 ������
	// Disbale interrupt
	TIMSK0 &= ~(1<<OCIE0B);
	TIMSK0 &= ~(1<<OCIE0A);
	TIMSK0 &= ~(1<<TOIE0);
	// ��r��v�^�C�}/�J�E���^����(CTC)����
	TCCR0B &= ~(1<<WGM02);
	TCCR0A |= (1<<WGM01);
	TCCR0A &= ~(1<<WGM00);
	// clkI/O/64 (64����)
	TCCR0B &= ~(1<<CS02);
	TCCR0B |= (1<<CS01);
	TCCR0B |= (1<<CS00);
	TCNT0 = 0;
	// 16MHz / 64 = 4us. 4us * 250 = 1.0ms
	OCR0A =  249;
	// set interrupt mask
	TIFR0 |= (1<<OCF0A);
	// Enable interrupt
	TIMSK0 |= (1<<OCIE0A);

	// Watchdog �^�C�}�ݒ�
	wdt_enable(WDTO_1S);
	
	// ���荞�ݏ�����L���ɂ���
	sei();
}

/* ----------------------------------------------------------------------------
 * ���C�����[�v
 * ---------------------------------------------------------------------------- */
void loop() {
	int i;
	int j;
	
	// 1�b���̏���
	if(interruptCount > 999) {
		// ���v���[�h
		if(userDataTimerCount == 0) {
			rtc.get();
			// �N�����\���������\�����̔���
			if(displayDateIntervalCount <= displayDateIntervalValue) {
				// �����b�\��
				// �J���}���N���A
				for(int i = 0; i < 8; i++) {
					IN12Com[i] = 0;
				}
				// 6���t�H�[�}�b�g 999999
				if(dispTimeFormat == 0) {
					IN12Num[0] = 0x0F;
					if(hour12_24format == 1) {
						IN12Num[1] = rtc.hourHigh;
						IN12Num[2] = rtc.hourLow;
					} else {
						IN12Num[1] = rtc.hourHigh12;
						IN12Num[2] = rtc.hourLow12;
					}
					IN12Num[3] = rtc.minutesHigh;
					IN12Num[4] = rtc.minutesLow;
					IN12Num[5] = rtc.secondsHigh;
					IN12Num[6] = rtc.secondsLow;
					IN12Num[7] = 0x0F;
				}
				// 8���t�H�[�}�b�g 99, 99, 99
				if(dispTimeFormat == 1) {
					if(hour12_24format == 1) {
						IN12Num[0] = rtc.hourHigh;
						IN12Num[1] = rtc.hourLow;
					} else {
						IN12Num[0] = rtc.hourHigh12;
						IN12Num[1] = rtc.hourLow12;
					}
					IN12Num[2] = 0x0F;
					IN12Num[3] = rtc.minutesHigh;
					IN12Num[4] = rtc.minutesLow;
					IN12Num[5] = 0x0F;
					IN12Num[6] = rtc.secondsHigh;
					IN12Num[7] = rtc.secondsLow;
					IN12Com[2] = 1;
					IN12Com[5] = 1;
				}
			} else {
				// �N�����\��
				// �J���}�N���A
				for(int i = 0; i < 8; i++) {
					IN12Com[i] = 0;
				}
				IN12Num[0] = 2;
				IN12Num[1] = 0;
				IN12Num[2] = rtc.yearHigh;
				IN12Num[3] = rtc.yearLow;
				IN12Num[4] = rtc.monthHigh;
				IN12Num[5] = rtc.monthLow;
				IN12Num[6] = rtc.dayHigh;
				IN12Num[7] = rtc.dayLow;
				if(displayDateIntervalCount >= ( displayDateIntervalValue + displayDateSecondsValue)) {
					displayDateIntervalCount = 0;
				}
			}
			displayDateIntervalCount++;
		}
		// ���[�U�[�f�[�^�\����
		if(userDataTimerCount > 0) {
			userDataTimerCount--;
		}
		// LED �_��
		if(led == false) {
			led = true;
			PORTC |= (1<<LED);
		} else {
			led = false;
			PORTC &= ~(1<<LED);
		}
		interruptCount = 0;
	}
	
	// �V���A��1 ��M����
	while(Serial1.available()) {
		char ch = Serial1.getch();
		serial0String[serial0StringIdx] = ch;
		Serial1.putch(ch);
		if(serial0String[serial0StringIdx] == '\r') {
			Serial1.putch('\n');
			serial0String[serial0StringIdx] = 0;
			serial0StringComplete = true;
		}
		serial0StringIdx++;
		if(serial0StringIdx >= SERIALBUF_LEN) {
			serial0StringIdx = SERIALBUF_LEN - 1;
		}
		// CR �R�[�h�����̓��͖͂���
		if(serial0StringComplete == true && serial0StringIdx == 1) {
			serial0StringIdx = 0;
			serial0StringComplete = false;
		}
	}
	
	// ��M�f�[�^����
	if(serial0StringComplete == true) {
		// �j�L�V�[�\���f�[�^�`�F�b�N
		int isUserData = true;
		for(i = 0; i < (serial0StringIdx - 1); i++) {
			if(serial0String[i] == ' ') continue;
			if(serial0String[i] == '.') continue;
			if((serial0String[i] >= '0') && (serial0String[i] <= '9')) continue;
			isUserData = false;
		}
		// ���[�U�[�f�[�^��\��
		if(isUserData == true) {
			for(i = 0; i < 8; i++) {
				IN12Num[i] = 0x0F;
				IN12Com[i] = 0;
			}
			j = 7;
			for(i = (serial0StringIdx - 1); i >= 0; i--) {
				if(serial0String[i] >= '0' && serial0String[i] <= '9') {
					if(j >= 0) {
						IN12Num[j--] = serial0String[i] - 0x30;
					}
				} else if(serial0String[i] == ' ') {
					if(j >= 0) {
						IN12Num[j--] = 0x0F;
					}
				}
			}
			j = 8;
			int previusCharValid = false;
			for(i = (serial0StringIdx - 1); i >= 0; i--) {
				if((serial0String[i] >= '0' && serial0String[i] <= '9') || serial0String[i] == ' ') {
					previusCharValid = true;
					if(j > 0) {
						j--;
					}
					continue;
				}
				if(serial0String[i] == '.' && previusCharValid == true) {
					IN12Com[j] = 1;
					previusCharValid = false;
				}
			}
			userDataTimerCount = userDataTimerValue;
		}
		if(isUserData == false) {
			// �R�}���h����
			if(strncmp(serial0String, "help", 4) == 0) {
				Serial1.putstr("set time YYMMDD hhmmss\r\n");
				Serial1.putstr("set timeformat [0 or 1]\r\n");
				Serial1.putstr("set dateinterval 99999\r\n");
				Serial1.putstr("set datesec 99999\r\n");
				Serial1.putstr("set udatasec 99999\r\n");
				Serial1.putstr("set bright daytime [1 to 9]\r\n");
				Serial1.putstr("set bright night [1 to 9]\r\n");
				Serial1.putstr("set hour daytime [0 to 23]\r\n");
				Serial1.putstr("set hour night [0 to 23]\r\n");
				Serial1.putstr("set 12/24 format [0 or 1]\r\n");
				Serial1.putstr("cathod -> Cathod poisoning\r\n");
				Serial1.putstr("sekai  -> STEINS;GATE like effect\r\n");
				Serial1.putstr("save   -> write EEPROM\r\n");
				Serial1.putstr("show   -> show all setting\r\n");
				Serial1.putstr("time   -> change to clock mode\r\n");
				Serial1.putstr("\r\n");
				Serial1.putstr("99999: numeric number 0 to 65535\r\n");
				Serial1.putstr("---\r\n");
			}
			if(strncmp(serial0String, "set timeformat ", 15) == 0) {
				unsigned int i = atoi(&serial0String[15]);
				if(i == 0 || i == 1) {
					dispTimeFormat = i;
					sprintf(msg, "value = %d\r\n", dispTimeFormat);
					Serial1.putstr(msg);
				}
			}
			if(strncmp(serial0String, "set time ", 9) == 0) {
				unsigned char year = ((serial0String[9] & 0x0F)<<4);
				year |= serial0String[10] & 0x0F;
				unsigned char month = ((serial0String[11] & 0x0F)<<4);
				month |= serial0String[12] & 0x0F;
				unsigned char days = ((serial0String[13] & 0x0F)<<4);
				days |= serial0String[14] & 0x0F;
				unsigned char hour = ((serial0String[16] & 0x0F)<<4);
				hour |= serial0String[17] & 0x0F;
				unsigned char minutes = ((serial0String[18] & 0x0F)<<4);
				minutes |= serial0String[19] & 0x0F;
				unsigned char seconds = ((serial0String[20] & 0x0F)<<4);
				seconds |= serial0String[21] & 0x0F;
				rtc.set(year, month, 1, days, hour, minutes, seconds);
			}
			if(strncmp(serial0String, "set dateinterval ", 17) == 0) {
				displayDateIntervalValue = atoi(&serial0String[17]);
				sprintf(msg, "value = %d\r\n", displayDateIntervalValue);
				Serial1.putstr(msg);
			}
			if(strncmp(serial0String, "set datesec ", 12) == 0) {
				displayDateSecondsValue = atoi(&serial0String[12]);
				sprintf(msg, "value = %d\r\n", displayDateSecondsValue);
				Serial1.putstr(msg);
			}
			if(strncmp(serial0String, "set udatasec ", 13) == 0) {
				userDataTimerValue = atoi(&serial0String[13]);
				sprintf(msg, "value = %d\r\n", userDataTimerValue);
				Serial1.putstr(msg);
			}
			if(strncmp(serial0String, "cathod", 6) == 0) {
				cathodePoisoning();
				Serial1.putstr("Done.");
			}
			if(strncmp(serial0String, "sekai", 5) == 0) {
				steins();
				//serialPrintln("Done.");
			}
			if(strncmp(serial0String, "set bright daytime ", 19) == 0) {
				unsigned int i = atoi(&serial0String[19]);
				if(i > 0 && i < 10) {
					IN12BrightDaytime = i;
					sprintf(msg, "value = %d\r\n", IN12BrightDaytime);
					Serial1.putstr(msg);
				}
			}
			if(strncmp(serial0String, "set bright night ", 17) == 0) {
				unsigned int i = atoi(&serial0String[17]);
				if(i > 0 && i < 10) {
					IN12BrightNight = i;
					sprintf(msg, "value = %d\r\n",IN12BrightNight);
					Serial1.putstr(msg);
				}
			}
			if(strncmp(serial0String, "set hour daytime ", 17) == 0) {
				unsigned int i = atoi(&serial0String[17]);
				if(i >= 0 && i < 24) {
					hourDaytime = i;
					sprintf(msg, "value = %d\r\n", hourDaytime);
					Serial1.putstr(msg);
				}
			}
			if(strncmp(serial0String, "set hour night ", 15) == 0) {
				unsigned int i = atoi(&serial0String[15]);
				if(i >= 0 && i < 24) {
					hourNight = i;
					sprintf(msg, "value = %d\r\n", hourNight);
					Serial1.putstr(msg);
				}
			}
			if(strncmp(serial0String, "set 12/24 format ", 17) == 0) {
				unsigned int i = atoi(&serial0String[17]);
				if(i == 0 || i == 1) {
					hour12_24format = i;
					sprintf(msg, "value = %d\r\n", hour12_24format);
					Serial1.putstr(msg);
				}
			}
			if(strncmp(serial0String, "save", 4) == 0) {
				// EEPROM ��������
				eeprom_busy_wait();
				eeprom_write_word(&E_displayDateIntervalValue, displayDateIntervalValue);
				eeprom_busy_wait();
				eeprom_write_word(&E_displayDateSecondsValue, displayDateSecondsValue);
				eeprom_busy_wait();
				eeprom_write_word(&E_userDataTimerValue, userDataTimerValue);
				eeprom_busy_wait();
				eeprom_write_byte(&E_dispTimeFormat, dispTimeFormat);
				eeprom_busy_wait();
				eeprom_write_byte(&E_IN12BrightDaytime, IN12BrightDaytime);
				eeprom_busy_wait();
				eeprom_write_byte(&E_IN12BrightNight, IN12BrightNight);
				eeprom_busy_wait();
				eeprom_write_byte(&E_hourDaytime, hourDaytime);
				eeprom_busy_wait();
				eeprom_write_byte(&E_hourNight, hourNight);
				eeprom_busy_wait();
				eeprom_write_byte(&E_hour12_24format, hour12_24format);
				Serial1.putstr("EEPROM write complete\r\n");
			}
			if(strncmp(serial0String, "time", 4) == 0) {
				userDataTimerCount = 0;
			}
			if(strncmp(serial0String, "show", 4) == 0) {
				sprintf(msg, "set timeformat %d\r\n",dispTimeFormat);
				Serial1.putstr(msg);
				sprintf(msg, "set dateinterval: %d\r\n",displayDateIntervalValue);
				Serial1.putstr(msg);
				sprintf(msg, "set datesec: %d\r\n", displayDateSecondsValue);
				Serial1.putstr(msg);
				sprintf(msg, "set udatasec: %d\r\n", userDataTimerValue);
				Serial1.putstr(msg);
				sprintf(msg, "set bright daytime: %d\r\n", IN12BrightDaytime);
				Serial1.putstr(msg);
				sprintf(msg, "set bright night: %d\r\n", IN12BrightNight);
				Serial1.putstr(msg);
				sprintf(msg, "set hour daytime: %d\r\n", hourDaytime);
				Serial1.putstr(msg);
				sprintf(msg, "set hour night: %d\r\n", hourNight);
				Serial1.putstr(msg);
				sprintf(msg, "set 12/24 format: %d\r\n", hour12_24format);
				Serial1.putstr(msg);
				Serial1.putstr("---\r\n");
			}
		}
		serial0StringIdx = 0;
		serial0StringComplete = false;
	}
	wdt_reset();
}

/* ----------------------------------------------------------------------------
 * �J�\�[�h�|�C�Y�j���O�h�~(�S�Z�O�����g�_��)
 * ---------------------------------------------------------------------------- */
void cathodePoisoning() {
	unsigned char a[17] = {
	0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6     };

	userDataTimerCount = userDataTimerValue;
	for(int i = 0; i < 8; i++) {
		IN12Com[i] = 1;
	}
	for(int j = 0; j < 5; j++) {
		for(int i = 0; i < 10; i++) {
			IN12Num[0] = a[i];
			IN12Num[1] = a[i+1];
			IN12Num[2] = a[i+2];
			IN12Num[3] = a[i+3];
			IN12Num[4] = a[i+4];
			IN12Num[5] = a[i+5];
			IN12Num[6] = a[i+6];
			IN12Num[7] = a[i+7];
			for(int i = 0; i < 2; i++) {
				_delay_ms(10);
			}
			wdt_reset();
		}
	}
	for(int i = 0; i < 8; i++) {
		IN12Num[i] = 0;
		IN12Com[i] = 0;
	}
	userDataTimerCount = 0;
}

/* ----------------------------------------------------------------------------
 * STEINS;GATE �_�C�o�[�W�F���X���[�^�[���G�t�F�N�g
 * ---------------------------------------------------------------------------- */
void steins() {
	long hendouRitsu[19] = {
		571024,
		571015,
		523299,
		456903,
		409420,
		37187,
		409431,
		456914,
		523307,
		571046,
		334581,
		1130426,
		1129848,
		1130205,
		1130212,
		1130238,
		2615074,
		3014368,
		4389117
	};
	char hendouRitsuStr[10];
	unsigned char charValid[8] = {
		1, 1, 0, 0, 0, 0, 0, 0
	};
	int charValidIdx;
	int randNumber;
	char randNumber2;
	int loopComplete = false;
	int i;
	int j;

	// �J���}�N���A
	for(i = 0; i < 8; i++) {
		IN12Com[i] = 0;
	}
	// ���v�\�����烆�[�U�[�f�[�^�\���ɐ؂�ւ�
	userDataTimerCount = userDataTimerValue;

	// �ϓ������s�b�N�A�b�v����7���̕�����ϊ�
	randNumber = rand() % 20;
	sprintf(hendouRitsuStr, "%07ld", hendouRitsu[randNumber]);

	// 2���ڂ̓J���}�A�X�y�[�X�Œ�
	IN12Num[1] = 0x0F;
	IN12Com[1] = 1;

	// 1, 3�`8���ڂ������_���ɕ\����1���ڂ̕\�����m�肳����
	randNumber = rand() % 400;
	for(i = 0; i < randNumber; i++) {
		for(j = 0; j < 8; j++) {
			randNumber2 = rand() % 10;
			IN12Num[j] = randNumber2;
			IN12Num[1] = 0x0F;
		}
		_delay_ms(1);
		wdt_reset();
	}
	IN12Num[0] = hendouRitsuStr[0] - 0x30;

	// 3�`8���ڂ������_���\���B
	do {
		charValidIdx = rand() % 6 + 2;
		// �\���m�肵�Ă��錅�̓X�L�b�v
		if(charValid[charValidIdx] == 1) {
			continue;
		}

		randNumber = rand() % 200;
		for(i = 0; i < randNumber; i++) {
			for(j = 2; j < 8; j++) {
				randNumber2 = rand() % 10;
				if(charValid[j] == 0) {
					IN12Num[j] = randNumber2;
				}
			}
			_delay_ms(1);
			wdt_reset();
		}
		IN12Num[charValidIdx] = hendouRitsuStr[charValidIdx - 1] - 0x30;
		charValid[charValidIdx] = 1;
		
		// �S���\�������������`�F�b�N
		loopComplete = true;
		for(i = 0; i < 8; i++) {
			if(charValid[i] == 0) {
				loopComplete = false;
			}
		}
	} while(loopComplete == false);
	userDataTimerCount = userDataTimerValue;
}

/* ----------------------------------------------------------------------------
 * ���C������
 * ---------------------------------------------------------------------------- */
int main(void)
{
	setup();
	while(1)
	{
		loop();
		Serial1.getrxd();
	}
}