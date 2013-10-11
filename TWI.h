/* -----------------------------------------------------------------------------
 *  IN-12b �j�L�V�[�Ǖ\�����j�b�g����v���O����
 *  TWI �ʐM
 *
 *  Author. �� ���_(Masahiro Kusunoki)
 *  http://mkusunoki.net
 *  Release 20131011 Rev 001
 * ----------------------------------------------------------------------------- */
#ifndef TWI_H_
#define TWI_H_

#define BUFFER_LEN 32

class TWI {
	
	private:
	char txBuf[BUFFER_LEN];
	int txBufIdx;
	int txBufLen;
	char txAddr;
	char rxBuf[BUFFER_LEN];
	int rxBufIdx;
	int rxBufLen;

	public:	
	TWI();
	void begin();
	void beginTransmission(char address);
	void endTransmission(int skipSTOP);
	int write(char ch);
	int write(char *ch, int len);
	void requestFrom(char addr, char len);
	char read();
};

#endif /* TWI_H_ */