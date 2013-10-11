/* ----------------------------------------------------------------------------
 *  IN-12b ニキシー管表示ユニット制御プログラム
 *  TWI 通信
 *
 *  Author. 楠 昌浩(Masahiro Kusunoki)
 *  http://mkusunoki.net
 *  Release 20131011 Rev 001
 * ---------------------------------------------------------------------------- */
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <util/twi.h>
#include "TWI.h"

/* ----------------------------------------------------------------------------
 *  クラスインスタンス
 * ---------------------------------------------------------------------------- */
TWI::TWI() {
	;
}

/* ----------------------------------------------------------------------------
 *  初期化処理
 * ---------------------------------------------------------------------------- */
void TWI::begin() {

	//PORTD |= (1<<PORTD0) | (1<<PORTD1);
	TWSR &= ~((1<<TWPS0) | (1<<TWPS1));
	// TWBR = (16MHz / 100kHz) - 16 / 2 = 72
	// TWBR = ( 8MHz / 100kHz) - 16 / 2 = 32
	// TWBR = ( 1MHz / 100kHz) - 16 / 2 = -3
	TWBR = 72;
	TWCR = (1<<TWEN) | (1<<TWIE) | (1<<TWEA);
}

/* ----------------------------------------------------------------------------
 *  送信開始
 * ---------------------------------------------------------------------------- */
void TWI::beginTransmission(char address) {
	txAddr = address;
	txBufIdx = 0;
	txBufLen = 0;
}

/* ----------------------------------------------------------------------------
 *  送信処理
 *  skipSTOP 0: STOP コンディション 1: RESTART コンディション
 * ---------------------------------------------------------------------------- */
void TWI::endTransmission(int skipSTOP) {

	// START Condition
	// TWCR = (1<<TWINT) | (1<<TWEA) | (1<<TWEN) | (1<<TWIE) | (1<<TWSTA);
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
	loop_until_bit_is_set(TWCR, TWINT);
	
	// Check START ACK
	;
	
	// Send address
	TWDR = TW_WRITE;
	TWDR |= txAddr<<1;
	// TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE);
	TWCR = (1<<TWINT) | (1<<TWEN);
	loop_until_bit_is_set(TWCR, TWINT);
	
	// Check MT_SLA_ACK
	;
	
	// Send Data
	for(int i = 0; i < txBufLen; i++) {

		TWDR = txBuf[i];
		// TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT) | _BV(TWEA);
		TWCR = (1<<TWINT) | (1<<TWEN);
		loop_until_bit_is_set(TWCR, TWINT);
		
		// Check MT_DATA_ACK
		;
	}
	
	// STOP Condition
	// TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT) | _BV(TWSTO);
	if(skipSTOP == 0) {
		TWCR = (1<<TWINT) | (1<<TWSTO) | (1<<TWEN);
		loop_until_bit_is_clear(TWCR, TWSTO);
	}
}

/* ----------------------------------------------------------------------------
 *  送信バッファへデータ格納
 * ---------------------------------------------------------------------------- */
int TWI::write(char ch) {

	if(txBufIdx < BUFFER_LEN) {
		txBuf[txBufIdx++] = ch;
		txBufLen = txBufIdx;
	} else {
		return(-1);
	}
	return(0);
}

int TWI::write(char *ch, int len) {
	int i;
	
	for(i = 0; i < len; i++) {
		write(ch[i]);
	}
	return(i);
}

/* ----------------------------------------------------------------------------
 *  受信処理開始
 * ---------------------------------------------------------------------------- */
void TWI::requestFrom(char addr, char len) {
	int i;

	rxBufIdx = 0;
	rxBufLen = 0;
	
	// START Condition
	// TWCR = (1<<TWINT) | (1<<TWEA) | (1<<TWEN) | (1<<TWIE) | (1<<TWSTA);
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
	loop_until_bit_is_set(TWCR, TWINT);
	
	// Check START ACK
	;
	
	// Send address
	TWDR = TW_READ;
	TWDR |= txAddr<<1;
	// TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE);
	TWCR = (1<<TWINT) | (1<<TWEN);
	loop_until_bit_is_set(TWCR, TWINT);

	// READ DATA
	
	for(i = 0; i < len; i++) {
		if(i == (len - 1)) {
			TWCR = (1<<TWINT) | (1<<TWEN);  //NACK
		} else {
			TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);  //ACK
		}
		loop_until_bit_is_set(TWCR, TWINT);
		rxBuf[rxBufLen] = TWDR;
		rxBufLen++;
	}

	// STOP Condition
	// TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT) | _BV(TWSTO);
	TWCR = (1<<TWINT) | (1<<TWSTO) | (1<<TWEN);
	loop_until_bit_is_clear(TWCR, TWSTO);
}

/* ----------------------------------------------------------------------------
 *  受信バッファから1文字読み出し
 * ---------------------------------------------------------------------------- */
char TWI::read() {
	
	if(rxBufIdx < rxBufLen) {
		return(rxBuf[rxBufIdx++]);
	} else {
		return(0);
	}
}
