/* Δ
	УПРАВЛЕНИЕ ЛАМПОЙ "ЭРА".
	Аналог оригинальной платы.
	- Управление яркостью с энкодера
	- Включение / выключение лампы
 */ 

#include "main.h"

/*
Attiny13A
PB0 -OC0A      белый LED
PB1 -OC0B      желтый LED
PB3 -PCINT3    энкодер A
PB4 -IN        энкодер B

Attiny24A
PB2 - [out] - OC0A     белый LED
PA7 - [out] - OC0B     желтый LED
PA0 - [in] - PCINT0    энкодер A
PA1 - [in] - PCINT1    энкодер B
PB0 - [in] - PCINT8    смена цветовой гаммы
PA2 - [in] - PCINT2    включение/выключение
*/

int main(void)
{
	sei();
	
	// настройка GPIO
	/*строки закомментированы т.к. по умолчанию все пины это входы*/
	//CLEAR_BIT(DDRA, DD0); // PB3 - вход энкодера А
	//CLEAR_BIT(DDRA, DD1); // PB4 - вход энкодера B
	//CLEAR_BIT(DDRA, DD2); // PB4 - вход кнопки выключения
	
	SET_BIT(PORTA, DD0); // pull-up входа энкодера А
	SET_BIT(PORTA, DD1); // pull-up входа энкодера B
	SET_BIT(PORTA, DD2); // pull-up кнопки выключения
	
	/*строки закомментированы т.к. вначале лампа выключена, т.е. выходы отключены*/
	//SET_BIT(DDRB, DD2); // PWM управление белыми LED
	//SET_BIT(DDRA, DD7); // PWM управление желтыми LED
	
	
	// прерывания
	SET_BIT(PCMSK0, PCINT0); // включить прерывания от порта PA0 (канал А энкодера)
	SET_BIT(PCMSK0, PCINT1); // включить прерывания от порта PA1 (канал B энкодера)
	SET_BIT(PCMSK0, PCINT2); // включить прерывания от порта PA2 (кнопка включения/выключения)
	SET_BIT(PCMSK1, PCINT8); // включить прерывания от порта PB0 (сенсорная кнопка смены гаммы)
	SET_BIT(GIMSK, PCIE0); // включить прерывания GPIO 0..7
	SET_BIT(GIMSK, PCIE1); // включить прерывания GPIO 8..11
	
	brightness = max; // на включении яркость 50%
	OCR0A = OCR0B = pgm_read_word_near(br + brightness);
	DeviceIsON = FALSE; // лампа выключена
	LEDW = LEDY = TRUE; // режим все светодиоды (белые и желтые) включены
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


// прерывания с линий PCINT0..7
ISR (PCINT0_vect)
{
	uint8_t R = PINA;
	
	// --- вначале проверим не было ли это прерывание от кнопки
	/*
	Алгоритм (S-текущее состояние, L-предыдущее состояние):
	L=1
	S=0; L=1 => L=0 //изменилось: S xor L = 1
	S=0; L=0 => L=0 //не изменилось: S xor L = 0
	S=1; L=0 => L=1 //изменилось: S xor L = 1
	S=1; L=1 => L=1 //не изменилось: S xor L = 0
	*/
	uint8_t BtnState = R & P2;
	if ((BtnState ^ KEY_ONOFF_lastState) == P2)
	{
		// состояние изменилось на противоположное
		if (BtnState == P2) // кнопка отжата
		{
			DeviceIsON ^= 1;
			
			if (DeviceIsON)
			{
				// включить
				if (LEDW) SET_BIT(DDRB, DD2);
				if (LEDY) SET_BIT(DDRA, DD7);
			}
			else
			{
				// выключить
				CLEAR_BIT(DDRB, DD2);
				CLEAR_BIT(DDRA, DD7);
			}			
		}
		
		KEY_ONOFF_lastState = BtnState;
	}
	
	// --- проверим не выключено ли устройство --------------------------------
	if (!DeviceIsON) return; // устройство выключено
	
	// --- декодирование состояния энкодера -----------------------------------
	uint8_t E = R & 3; // первые два бита это энкодер
	uint8_t D = 0; // 0-error, 1-СCW, 2-CW
	//алгоритм кодировки: CW=10-11-01-00, CCW=01-11-10-00
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
		case 1: //CW - увеличение яркости
			if (brightness >= step) brightness-=step;
			if (brightness < step) brightness = 0;
			OCR0A = OCR0B = pgm_read_word_near(br + brightness);
			break;
		case 2: //CCW - уменьшение яркости
			if (brightness <= (max-step)) brightness+=step;
			if (brightness >= (max-step)) brightness = max;
			OCR0A = OCR0B = pgm_read_word_near(br + brightness);
			break;
		case 0: //ошибка не обрабатывается
			break;
	}	
}


// прерывания с линий PCINT8..11
ISR (PCINT1_vect)
{
	// --- проверим не выключено ли устройство --------------------------------
	if (!DeviceIsON) return; // устройство выключено
	
	// --- проверим нажатие кнопки смены цветовой гаммы -----------------------
	uint8_t BtnState = PINB & P0;
	if ((BtnState ^ KEY_MODE_lastState) == P0)
	{
		// состояние изменилось на противоположное
		if (BtnState == P0) // кнопка отжата
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
				CLEAR_BIT(DDRB, DD2); // отключить PWM управление белыми LED (нулевая яркость)
			else
				SET_BIT(DDRB, DD2); // включить PWM управление белыми LED
			
			if (!LEDY)
				CLEAR_BIT(DDRA, DD7); // отключить PWM управление желтыми LED (нулевая яркость)
			else
				SET_BIT(DDRA, DD7); // включить PWM управление желтыми LED
		}
		
		KEY_MODE_lastState = BtnState;
	}
}