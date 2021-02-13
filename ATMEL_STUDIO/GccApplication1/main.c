/*
	���������� ������ "���".
	������ ������������ �����.
	- ���������� �������� � ��������
	- ��������� / ���������� �����
 */ 

#include "main.h"

/*
Attiny13A
PB0 -OC0A      ����� LED
PB1 -OC0B      ������ LED
PB3 -PCINT3    ������� A
PB4 -IN        ������� B

Attiny24A
PB2 - [out] - OC0A     ����� LED
PA7 - [out] - OC0B     ������ LED
PA0 - [in] - PCINT0    ������� A
PA1 - [in] - PCINT1    ������� B
PB0 - [in] - PCINT8    ����� �������� �����
PA2 - [in] - PCINT2    ���������/����������
*/

int main(void)
{
	sei();
	
	// ��������� GPIO
	/*������ ���������������� �.�. �� ��������� ��� ���� ��� �����*/
	//CLEAR_BIT(DDRA, DD0); // PB3 - ���� �������� �
	//CLEAR_BIT(DDRA, DD1); // PB4 - ���� �������� B
	//CLEAR_BIT(DDRA, DD2); // PB4 - ���� ������ ����������
	
	SET_BIT(PORTA, DD0); // pull-up ����� �������� �
	SET_BIT(PORTA, DD1); // pull-up ����� �������� B
	SET_BIT(PORTA, DD2); // pull-up ������ ����������
	
	/*������ ���������������� �.�. ������� ����� ���������, �.�. ������ ���������*/
	//SET_BIT(DDRB, DD2); // PWM ���������� ������ LED
	//SET_BIT(DDRA, DD7); // PWM ���������� ������� LED
	
	
	// ����������
	SET_BIT(PCMSK0, PCINT0); // �������� ���������� �� ����� PA0 (����� � ��������)
	SET_BIT(PCMSK0, PCINT1); // �������� ���������� �� ����� PA1 (����� B ��������)
	SET_BIT(PCMSK0, PCINT2); // �������� ���������� �� ����� PA2 (������ ���������/����������)
	SET_BIT(PCMSK1, PCINT8); // �������� ���������� �� ����� PB0 (��������� ������ ����� �����)
	SET_BIT(GIMSK, PCIE0); // �������� ���������� GPIO 0..7
	SET_BIT(GIMSK, PCIE1); // �������� ���������� GPIO 8..11
	
	brightness = max; // �� ��������� ������� 50%
	OCR0A = OCR0B = pgm_read_word_near(br + brightness);
	DeviceIsON = FALSE; // ����� ���������
	LEDW = LEDY = TRUE; // ����� ��� ���������� (����� � ������) ��������
	mode = 0;

	TCCR0A = (1<<COM0A1 | 0<<COM0A0) |       // FastPWM, set OC0A on TOP, clear on Compare Match
			 (1<<COM0B1 | 0<<COM0B0) |       // FastPWM, set OC0B on TOP, clear on Compare Match
			 (1<<WGM01 | 1<<WGM00);          // Mode: FastPWM
	TCCR0B = (0<<FOC0A | 0<<FOC0B) |         // Force Output Compare disabled
			 (0<<WGM02) |                    // Mode: FastPWM
			 (0<<CS02 | 1<<CS01 | 0<<CS00);  // Enable PWM. Freq = SysCLK/8 = 125 kHz
	
    while (1) 
    {	

	}
}


// ���������� � ����� PCINT0..7
ISR (PCINT0_vect)
{
	uint8_t R = PINA;
	
	// --- ������� �������� �� ���� �� ��� ���������� �� ������
	/*
	�������� (S-������� ���������, L-���������� ���������):
	L=1
	S=0; L=1 => L=0 //����������: S xor L = 1
	S=0; L=0 => L=0 //�� ����������: S xor L = 0
	S=1; L=0 => L=1 //����������: S xor L = 1
	S=1; L=1 => L=1 //�� ����������: S xor L = 0
	*/
	uint8_t BtnState = R & P2;
	if ((BtnState ^ KEY_ONOFF_lastState) == P2)
	{
		// ��������� ���������� �� ���������������
		if (BtnState == P2) // ������ ������
		{
			DeviceIsON ^= 1;
			
			if (DeviceIsON)
			{
				// ��������
				if (LEDW) SET_BIT(DDRB, DD2);
				if (LEDY) SET_BIT(DDRA, DD7);
			}
			else
			{
				// ���������
				CLEAR_BIT(DDRB, DD2);
				CLEAR_BIT(DDRA, DD7);
			}			
		}
		
		KEY_ONOFF_lastState = BtnState;
	}
	
	// --- �������� �� ��������� �� ���������� --------------------------------
	if (!DeviceIsON) return; // ���������� ���������
	
	// --- ������������� ��������� �������� -----------------------------------
	uint8_t E = R & 3; // ������ ��� ���� ��� �������
	uint8_t D = 0; // 0-error, 1-�CW, 2-CW
	//�������� ���������: CW=10-11-01-00, CCW=01-11-10-00
	if (S == 2 && E == 3) D=2; //10-11
	if (S == 3 && E == 1) D=2; //11-01
	if (S == 1 && E == 0) D=2; //01-00
	if (S == 0 && E == 2) D=2; //00-10
	
	if (S == 1 && E == 3) D=1; //01-11
	if (S == 3 && E == 2) D=1; //11-10
	if (S == 2 && E == 0) D=1; //10-00
	if (S == 0 && E == 1) D=1; //00-01	
	
	if (D != 0) S = E;

	switch (D)
	{
		case 1: //CW - ���������� �������
			if (brightness >= step) brightness-=step;
			if (brightness < step) brightness = 0;
			OCR0A = OCR0B = pgm_read_word_near(br + brightness);
			break;
		case 2: //CCW - ���������� �������
			if (brightness <= (max-step)) brightness+=step;
			if (brightness >= (max-step)) brightness = max;
			OCR0A = OCR0B = pgm_read_word_near(br + brightness);
			break;
		case 0: //������ �� ��������������
			break;
	}	
}


// ���������� � ����� PCINT8..11
ISR (PCINT1_vect)
{
	// --- �������� �� ��������� �� ���������� --------------------------------
	if (!DeviceIsON) return; // ���������� ���������
	
	// --- �������� ������� ������ ����� �������� ����� -----------------------
	uint8_t BtnState = PINB & P0;
	if ((BtnState ^ KEY_MODE_lastState) == P0)
	{
		// ��������� ���������� �� ���������������
		if (BtnState == P0) // ������ ������
		{
			mode++;
			if (mode > 2) mode = 0;
			switch (mode)
			{
				case 0:
					LEDY = LEDW = TRUE;
					break;
				case 1:
					LEDY = FALSE;
					LEDW = TRUE;
					break;
				case 2:
					LEDY = TRUE;
					LEDW = FALSE;
					break;
				default:
					break;
			}
			
			if (!LEDW)
				CLEAR_BIT(DDRB, DD2); // ��������� PWM ���������� ������ LED (������� �������)
			else
				SET_BIT(DDRB, DD2); // �������� PWM ���������� ������ LED
			
			if (!LEDY)
				CLEAR_BIT(DDRA, DD7); // ��������� PWM ���������� ������� LED (������� �������)
			else
				SET_BIT(DDRA, DD7); // �������� PWM ���������� ������� LED
		}
		
		KEY_MODE_lastState = BtnState;
	}
}