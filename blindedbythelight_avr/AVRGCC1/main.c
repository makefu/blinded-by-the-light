#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdint.h>	//integer variabeln
#define F_CPU 16000000UL //muss vor delay.h sein
#include <util/delay.h>	//für sleep
#include <avr/power.h>	//für fcpu einstellung
#include <avr/interrupt.h>
#include <string.h> // Für "strcmp"
#include <stdlib.h> // Für "itoa"
#include <stdbool.h> // erlaubt boolean benutzung

#include <stddef.h>	//stringbearbeitung
//#include <math.h>  //für winkelfunktion

#include "usb_serial.h"

//debug led
#define E6_on PORTE |= (1<<PINE6);
#define E6_off PORTE &= ~(1<<PINE6);
#define E6_toggle PORTE ^=(1<<PINE6);	//debug led toggle

///////// Definiere Funktionen ////////////////
void sleep_us(uint16_t us);  
void sleep_ms(uint16_t ms);   
void servo_set (uint8_t servo_NR,int16_t servo_position);

int16_t asci_to_integer (char asci_char[8]);


char *strtok_r_empty( char *p_str, const char *p_delim, char **pp_save ) //stringfunktion-tut das richtige
{
  // *pp_save will save the pointer to the start of the next
  // unprocessed token or NULL if there are no more tokens

  char *p_start = ( p_str ? p_str : *pp_save );
  if( p_start )
  {
    // look for start of next token, or NULL
    *pp_save = strpbrk( p_start, p_delim );

    if( *pp_save )
    {
      // delimiter found
      **pp_save = '\0'; // terminate current token
      ++*pp_save;       // skip delimiter on next call
    }
  }
  // return current token or NULL
  return p_start;
}  


int servostatus[6] = { 500, 500, 500, 500, 500, 500 };

int main(void)
{
	MCUCR = (1 << JTD); MCUCR = (1 << JTD); //disable jtag
	clock_prescale_set(clock_div_1); //set clock prescaler to 1 (CPU-clock to 16MHz)

	DDRE |= (1 << DDE6) ;			//port e 6  als  ausgang
	DDRB = 0xff; //alle als ausgang
//	DDRF = 0x00; //alle eingang
	DDRD &= ~((1 << DDD6) | (1 << DDD5));//eingANG adc vss UND amp
	DDRC &= ~( 1 << DDC6 );        /* PIN PC6 auf Eingang Taster)  */
    PORTC |= ( 1 << DDC6 );        /* Pullup-Widerstand aktivieren */

	usb_init();	
	
	usb_send_str("uart init done \r\n");
	usb_send_str("format:   servo1,servo2,,servo4,,servo_6  \r\n  werte zwischen 1 und 1000 \r\n  max 6 servos \r\n");
	usb_send_str("bsp:  250,,5,950,,20 \r\n");
	usb_send_str("bsp:  550,,5,950,,20 \r\n");

	uint8_t command = 0;		// befehl verfügbar 0/1
	char inputbuffer[250]={0};
	uint16_t counter = 0;
	
//p power
//n no power aus
//t toggle
	
while (1) 
{
	while(usb_serial_available())
	{
		inputbuffer[counter]= usb_serial_getchar();
		usb_serial_putchar(inputbuffer[counter]);
		//		usb_send_int((int32_t) inputbuffer[counter] ); //gib als zeichen aus
		//		usb_serial_flush_input();
		counter++;
		if (inputbuffer[counter-1]==127)// 127 = del-?  co nsole sagt 127=backspace  auf jeden fall löschen...
		{
			if (counter>1)//countrer is mind 2
			{
				counter-=2; //del und zeichen zuvor überschreiben
			}
			else
			{
				counter=0;
			}
		}

		if (inputbuffer[counter-1]=='p')// 127 = del-?  co nsole sagt 127=backspace  auf jeden fall löschen...
		{
			E6_on;
			for (uint8_t i =0;i<250; i++ ) // delete all string elemts
			{
				inputbuffer[i]= '0';
			}
			counter=0;
		}
		if (inputbuffer[counter-1]=='n')// 127 = del-?  co nsole sagt 127=backspace  auf jeden fall löschen...
		{
			E6_off;
			for (uint8_t i =0;i<250; i++ ) // delete all string elemts
			{
				inputbuffer[i]= '0';
			}
			counter=0;
		}
		if (inputbuffer[counter-1]=='t')// 127 = del-?  co nsole sagt 127=backspace  auf jeden fall löschen...
		{
			E6_toggle;
			for (uint8_t i =0;i<250; i++ ) // delete all string elemts
			{
				inputbuffer[i]= '0';
			}
			counter=0;
		}		

		if ( (counter>= 200)|| (inputbuffer[counter-1]==13) ) //is enter pressed or there are more than 20 letters
		{
			inputbuffer[counter-1]='\0'; //remove enter, add stringterminator
			
			usb_send_str("\r\n>");
			usb_send_str(inputbuffer);
			usb_send_str("\r\n");
			command = 1;   //beffehlsstring komplett verfügbar ist
			counter = 0; //reset position counter
		}
	
	}	
	
	
if (command == 1)	//wenn befehlsstring verfügbar ist
{

	char *servo[8];
	char *saveptr = NULL;

		  if( ( servo[0] = strtok_r_empty(inputbuffer, ",", &saveptr ) )
			  &&
			  (( servo[1] = strtok_r_empty( NULL, ",", &saveptr ) )||( servo[1] = "" ) )
			  &&
			  (( servo[2] = strtok_r_empty( NULL, ",", &saveptr ) )||( servo[2] = "" ) )
			  &&
			  (( servo[3] = strtok_r_empty( NULL, ",", &saveptr ) )||( servo[3] = "" ) )
			  &&
			  (( servo[4] = strtok_r_empty( NULL, ",", &saveptr ) )||( servo[4] = "" ) )
			  &&
			  (( servo[5] = strtok_r_empty( NULL, ",", &saveptr ) )||( servo[5] = "" ) )
			  &&
			  (( servo[6] = strtok_r_empty( NULL, ",", &saveptr ) )||( servo[6] = "" ) )
			  &&
			  (( servo[7] = strtok_r_empty( NULL, ",", &saveptr ) )||( servo[7] = "" ) ) )
				 {	//string verarbeitung erfolgreich
				

				for (uint8_t i = 0; i < 8; i++) // servos 8 mal pos befehl senden (genug zeit zum fahren)
					{
						 int16_t servo_buffer = asci_to_integer(servo[i]);
						if (!(servo_buffer == -1)) {  servostatus[i]=servo_buffer;}   
					}				

				}
		else { usb_send_str( "uart error!\n" );}  //ende stringferarbetung

	command = 0; //
	for (uint8_t i =0;i<250; i++ ) // delete all string elemts
	{
		inputbuffer[i]= '0';
	}
} //ende if befehlsstring verfügbar
	
	//hier adc messung oder sonstiges zeug
	servo_set(0, servostatus[0]);
	servo_set(1, servostatus[1]);
	servo_set(2, servostatus[2]);
	servo_set(3, servostatus[3]);	
	servo_set(4, servostatus[4]);
	servo_set(5, servostatus[5]);
	servo_set(6, servostatus[6]);	
	

sleep_ms(25);

  } // ende while (1)
} //ende máin()



int16_t asci_to_integer(char asci_char[8])
{
	int16_t temp = -1;
	for(int i=0;i<8;i++) {
		if(asci_char[i] == '\0') {
			if (i == 0){return temp=-1;} // string ist leer
			temp = atoi (asci_char);
			if (temp > 1200){return temp = -1;} // wenn der wert über 1200 ist return -1
			return temp;			//string ist nicht leer und enthält nur 'zahlen'
			break;
			} else if(asci_char[i] > '9' || asci_char[i] < '0') {
			// keine zahl weil ein anderes zeichen drin vorkommt.
			return temp =-1;
			break;
		}
	}
	return temp=-1; // was anderes lief schief
}

void servo_set (uint8_t servo_NR,int16_t servo_position)
{
	
	if (servo_NR < 0 || servo_NR > 8){usb_send_str("err servoNR \r\n"); return;} //servonummer falsch || keine pos. angegeben

	PORTB |= (1<<servo_NR); //servo pin high
	sleep_us (700);		// sleep 1000µs
	sleep_us (servo_position);	//sleep wert für pos. bestimmung 0-1000µs
	PORTB &= ~(1<<servo_NR);  //servo pin low
}
void sleep_ms(uint16_t ms){
	while(ms){
		ms--;
		_delay_ms(1);
	}
}
void sleep_us(uint16_t us){
	while(us){
		us--;
		_delay_us(1);
	}
}

