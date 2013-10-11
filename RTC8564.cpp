/* ----------------------------------------------------------------------------
 *  IN-12b ニキシー管表示ユニット制御プログラム
 *  RTC-8564 RealTimeClock Module
 *
 *  Author. 楠 昌浩(Masahiro Kusunoki)
 *  http://mkusunoki.net
 *  Release 20131011 Rev 001
 * ---------------------------------------------------------------------------- */
#include "RTC8564.h"
#include "TWI.h"

#define I2CRTC8564 0x51

extern TWI i2c;

/* ----------------------------------------------------------------------------
 *  初期化処理
 * ---------------------------------------------------------------------------- */
void RTC8564::begin() {

	if(isPowerFail() != 0) {
		initialize();
	}
}

/* ----------------------------------------------------------------------------
 *  起動直後の VL(Voltage Low) ビット読み出し
 * ---------------------------------------------------------------------------- */
int RTC8564::isPowerFail() {
	unsigned char seconds = 0x80;

	i2c.beginTransmission(I2CRTC8564);
	i2c.write(0x02);
	i2c.endTransmission(1);
	i2c.requestFrom(I2CRTC8564, 1);
	seconds = i2c.read();
	return(seconds & 0x80);
}

/* ----------------------------------------------------------------------------
 *  RTC デフォルト値書き込み
 * ---------------------------------------------------------------------------- */
void RTC8564::initialize() {

	i2c.beginTransmission(I2CRTC8564);
	i2c.write(0x00);  // Set Address 0
	i2c.write(0x20);  // Address 0 STOP=1
	i2c.write(0x00);  // Address 1
	i2c.write(0x00);  // Address 2 Seconds
	i2c.write(0x41);  // Address 3 Minutes
	i2c.write(0x17);  // Address 4 Hours
	i2c.write(0x08);  // Address 5 Days
	i2c.write(0x04);  // Address 6 Weekdays
	i2c.write(0x08);  // Address 7 Month
	i2c.write(0x10);  // Address 8 Years
	i2c.write(0x00);  // Address 9 Alarm Seconds
	i2c.write(0x00);  // Address A Alarm
	i2c.write(0x00);  // Address B Alarm
	i2c.write(0x00);  // Address C Alarm
	i2c.write(0x00);  // Address D CLOCKOUT
	i2c.write(0x00);  // Address E Timer
	i2c.write(0x00);  // Address F Timer
	i2c.write(0x00);  // Address 0 STOP=0
	i2c.endTransmission(0);
}

/* ----------------------------------------------------------------------------
 *  日時読み出し
 * ---------------------------------------------------------------------------- */
void RTC8564::get() {
	int i;
	unsigned char ch[7];

	i2c.beginTransmission(I2CRTC8564);
	i2c.write(2);							// Set address 2
	i2c.endTransmission(1);					// Set RESTART Condition
	i2c.requestFrom(I2CRTC8564, 7);
	for(i = 0; i < 7; i++) {
		ch[i] = i2c.read();
	}
	Seconds = ch[0] & 0x7F;
	Minutes = ch[1] & 0x7F;
	Hours = ch[2] & 0x3F;
	Days =  ch[3] & 0x3F;
	Weekday = ch[4] & 0x07;
	Month = ch[5] & 0x1F;
	Years = ch[6];
	secondsHigh = Seconds >> 4;
	secondsLow = Seconds & 0x0F;
	minutesHigh = Minutes >> 4;
	minutesLow = Minutes & 0x0F;
	hourHigh = Hours >> 4;
	hourLow = Hours & 0x0F;
	hourHigh12 = Hours12 >> 4;
	hourLow12 = Hours12 & 0x0F;
	dayHigh = Days >> 4;
	dayLow = Days & 0x0F;
	monthHigh = Month >> 4;
	monthLow = Month & 0x0F;
	yearHigh = Years >> 4;
	yearLow = Years & 0x0F;
	i = bcdToBin(Hours);
	if(i >= 12) {
		i -= 12;
		Hours12 = binToBcd(i);
	} else {
		Hours12 = Hours;
	}
	hourHigh12 = Hours12 >> 4;
	hourLow12 = Hours12 & 0x0F;
}

/* ----------------------------------------------------------------------------
 *  日時設定
 * ---------------------------------------------------------------------------- */
void RTC8564::set(unsigned char years, unsigned char month, unsigned char weekday, unsigned char days, unsigned char hours, unsigned char minutes, unsigned char seconds) {
	int i;
	unsigned char ch[7];

	ch[0] = seconds;
	ch[1] = minutes;
	ch[2] = hours;
	ch[3] = days;
	ch[4] = weekday;
	ch[5] = month;
	ch[6] = years;
	// stop RTC
	i2c.beginTransmission(I2CRTC8564);
	i2c.write(0x00);    // Set addoress 0
	i2c.write(0x20);    // STOP=1
	i2c.endTransmission(0);
	// write value
	i2c.beginTransmission(I2CRTC8564);
	i2c.write(0x02);    // Set addoress 2
	for(i = 0; i < 7; i++) {
		i2c.write(ch[i]);
	}
	i2c.endTransmission(0);
	// start RTC
	i2c.beginTransmission(I2CRTC8564);
	i2c.write(0x00);    // Set addoress 0
	i2c.write(0x00);    // STOP=0
	i2c.endTransmission(0);
}

/* ----------------------------------------------------------------------------
 *  BCD からバイナリ変換
 * ---------------------------------------------------------------------------- */
unsigned char RTC8564::bcdToBin(unsigned char bcd) {
	return(bcd - (6 * (bcd >> 4)));
}

/* ----------------------------------------------------------------------------
 *  バイナリから BCD 変換
 * ---------------------------------------------------------------------------- */
unsigned char RTC8564::binToBcd(unsigned char bin) {
	return(bin + (6 * (bin / 10)));
}

