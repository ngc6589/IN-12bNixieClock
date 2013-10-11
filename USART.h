/* ----------------------------------------------------------------------------
 *  IN-12b ニキシー管表示ユニット制御プログラム
 *  シリアル通信
 *
 *  Author. 楠 昌浩(Masahiro Kusunoki)
 *  http://mkusunoki.net
 *  Release 20131011 Rev 001
 * ---------------------------------------------------------------------------- */
#ifndef USART_H_
#define USART_H_

#define BAUD 9600
#define UBRRVAL ((F_CPU / (BAUD * 16UL)) - 1)
#define USARTBUF_LEN 32

class USART {
	
	private:
	char rxBuf[USARTBUF_LEN];
	int rxBufStoIdx;
	int rxBufGetIdx;
		
	public:
	USART();
	void begin();
	void putstr(char *buf);
	void putch(char ch);
	void getrxd();
	char getch();
	int available();
};

#endif /* USART_H_ */