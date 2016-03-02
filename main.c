/*!
*\mainpage
*\author Lingg Nico
*\brief Tempratursensor DS1621
*\date 30.06.2015
*
**\version 2.0
* FINAL - Kommentare angepasst, Toten Code entfernt, Code aufgehuebscht
**\version 1.5
* CAN verbessert - funktioniert jetzt
*\version 1.1
* Kommentare erweitert.
*\version 1.0
* Erstes funktionierendes Programm.
*\version 0.5
* Toten Code entfernt.
*\version 0.4
* Kommentare hinzugefuegt (Doxygen).
*\version 0.3
* CAN-BUS implementiert.
*\version 0.2
* LCD-Display implementiert
*\version 0.1
* I2C implementiert und erste Test mit dem Sensor.
*
*In diesem Programm wird eine Temperaturmessung mit dem Sensor DS 1621 durchgefuehrt.
*Die Kommunikation wird ueber I2C-Bus realisiert und die aktuelle Temperatur auf dem LCD-Display ausgegeben.
*Abschliessend wird der aktuelle Wert der Temperatur mit dem Identifier 0x854 an den CAN-Bus uebertragen.
*
*<b>Fuer die Mainfunktion bitte hier klicken: main()</b>
*
*/


/*********************************************************************
 *																	 *
 *								Includes							 *
 *																	 *
 * *******************************************************************/
#include "LPC17xx.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_can.h"
#include "lpc17xx_libcfg_default.h"
#include "lpc17xx_pinsel.h"
#include "lc798x.h"



/*********************************************************************
 *																	 *
 *							DEFINITIONEN							 *
 *																	 *
 * *******************************************************************/

/*------------------------- I2C-DEFINITIONEN ------------------------*/

/**  I2C-Adresse des DS1621 */
#define I2CDEV_S_ADDR		0x4F

/** Kanal 1 fuer I2C */
#define I2CDEV_M 			LPC_I2C1


/*------------------------- CAN-DEFINITIONEN ------------------------*/
/** CAN - Identifier (vorgegeben) */
#define CAN_IDENTIFIER 		0x854;

/** CAN - Schnittstelle (CAN1 oder CAN2) */
#define CAN_PERIPHERAL		LPC_CAN1


/*------------------------ DS 1621-DEFINITIONEN ----------------------*/

/** Aktivierung zum Schreiben in das Register */
#define ACCESS_CONFIG		0xAC

/** Continuous Conversion Mod einstellen */
#define SET_UP				0x00

/** Temeraturmessung starten */
#define START_CONVERT_T		0xEE

/** Temeratur lesen */
#define READ_TEMPERATURE 	0xAA


/*------------------------- TypDef-DEFINITIONEN ----------------------*/

/** CAN Variable Definition */
CAN_MSG_Type TXMsg;

/** I2C Variable Definition */
I2C_M_SETUP_Type 	transferMCfg;


/*---------------------- VARIABLEN DEFINITIONEN ----------------------*/

/** 2 Byte Feld zur Konfiguration des DS 1621 */
uint8_t Master_Buf[2];

/** 2 Byte Feld zum Ablegen der Temperaturwerte (MSB,LSB) */
uint8_t Temperatur[2];



/*********************************************************************
 *																	 *
 *						   	 PROTOTYPEN		    					 *
 *																	 *
 * *******************************************************************/
void Init_System(void);
void Init_LCD(void);
void Init_I2C(void);
void LCD_Initialausgabe(void);
void DS1621_Init(void);
void can_init(void);
void Timer0_DelayMs(uint32_t delayInMs);


/*********************************************************************
 *																	 *
 *						    MAIN-FUNKTION					 		 *
 *																	 *
 * *******************************************************************/
/*!
*
*	\brief  <b>Main-Programm<b>:
*
*			Zu Beginn werden alle Initalisierungen durchgefuehrt.
*
*			Hier werden zunaechst alle noetigen Initialisierungen vorgenommen:
*
*			Init_System() - LED auf Board wird auf Ausgang gestellt
*
*			Init_LCD() - LCD-Display wird initialisiert
*
*			Init_I2C() - I2C-Bus wird initialisiert
*
*			can_init() - CAN-Bus wird initialisiert
*
			Mit der Funktion DS1621_Init() wird der Temperatursensor DS1621 ueber den I2C-Bus konfiguriert und fuer
			die Temperaturmessung vorbereitet.

*			Abschliessend erfolgt die gesamte Kommunikation und Messung in einer Endlosschleife (while-Schleife).
*			Die Messung wird alle 500 ms wiederholt und mit dem Blinken der LED visualisiert.
*
*/
int main(void)
{
	//* Variablen
	uint8_t TempKomma=0;			 //* Variable zur Identifikation der Temperatur nach dem Komma
	uint8_t TempZehner, TempRest;    //* Variablen zum Aufteilen der Temp fuer die Uebertragung an den CAN-BUS

	//* System initialisieren
	Init_System();

	//* LCD-Display initialisieren
	Init_LCD();

	//* LCD-Display - Initialausgabe
	LCD_Initialausgabe();

	//* I2C-Bus initalisieren
	Init_I2C();

	//* CAN-Bus initialisieren
	can_init();

	//* DS1621 Einschalten
	DS1621_Init();


	/* WHILE-SCHLEIFE - Messung und Ausgabe der Temperatur auf LCD-Display ueber CAN */
    while(1)
    {

    	/* LED blicken lassen */
    	LPC_GPIO0->FIOSET = (1<<10);		/* LED einschalten */
    	Timer0_DelayMs(250);
    	LPC_GPIO0->FIOCLR = (1<<10);		/* LED ausschalten */
    	Timer0_DelayMs(250);

    	/* Temeperatur ueber I2C auslesen */
    	I2C_MasterTransferData(I2CDEV_M, &transferMCfg, I2C_TRANSFER_POLLING);

    	/* Bit-Filterung der Datenkommunikation mit dem DS1621 -> Erkennung des halben Temperaturgrades */
    	if (Temperatur[1] && 0x80) TempKomma=5;
    	else TempKomma=0;

    	/* Temeperatur an LCD-Display ausgeben */
    	lcd_set_cursor(0,0);
    	printf("Temperatur: %d.%d C", Temperatur[0], TempKomma);


		/* ------ CAN-Uebertragung ------ */

		// Zerlegung der Temperatur in Dezimalzahlen fuer Ausgabe als ASCII-Code
		TempZehner = Temperatur[0]/10;					//* erste Zahl der Temperatur
		TempRest   = Temperatur[0] - TempZehner*10;		//* zweite Zahl


		TXMsg.dataA[0] = 48+TempZehner;
		TXMsg.dataA[1] = 48+TempRest;
		TXMsg.dataA[2] = '.';
		TXMsg.dataA[3] = 48+TempKomma;
		TXMsg.dataB[0] = ' ';
		TXMsg.dataB[1] = 'G';
		TXMsg.dataB[2] = 'd';
		TXMsg.dataB[3] = ' ';

		// Senden der Daten an CAN-Bus
		CAN_SendMsg(CAN_PERIPHERAL, &TXMsg);

    }
}



/*!
*
*	\brief	<b>System-Initialisierung<b>:
*
*			Initialisierung des Boards (Bis jetzt nur LED)
*
*  \param[in]	none
*  \return 		none
*/
void Init_System(void)
{
	// Direction of Pin10 at Port0 (LED)
	LPC_GPIO0->FIODIR |= (1<<10);

}



/*!
*
*	\brief	<b>LCD-Initialisierung<b>:
*
*			Initialisierung des LCD-Displays
*
*   \param[in]	none
*   \return 	none
*/
void Init_LCD(void)
{
	//* LCD Schnittstelle vorbereiten
	lcd_init_interface();

	//* Den Controller initialisieren, hier für Textausgabe mit dem internen Zeichensatz
	lcd_set_system(LCD_FONT_INTERNAL, 6, 8);

	//* Den Cursor einstellen (falls man den benutzt)
	lcd_set_cursor_mode(LCD_CURSOR_OFF,0);

	//* Die Anzeige löschen und das Anzeigefenster auf Adresse 0 zurückstellen
	lcd_clear_screen();

	//* Als letztes das Display einschalten
	lcd_control(LCD_ON);


}



/*!
 * \brief <b>Initialausgabe des LCDs<b>
 *
 *         Funktion zum Testen des LCD-Display, es wird ein ein Initialtext ausgegeben
 *
 *  \param[in]	none
 *  \return 	none
 */
void LCD_Initialausgabe(void)
{
	lcd_clear_screen();
	printf("*** Temperatursensor  DS1621 ***");
	printf("\n\r Auer, Lingg, Maier - SoSe 2015");
	Timer0_DelayMs(3000);  							//* Anzeigedaur in Millisekunden
	lcd_clear_screen();
}



/*!
*
*	\brief  <b>I2C-Initialisierung<b>:
*
*			In dieser Funktion erfolgt die Initialisierung des I2C-Buses fuer die Kommunikation
*			mit dem Temperatursensor DS 1621.
*
*			Es erfolgt:
*			Clockrate: 100000 Hz
*
*   \param[in]	none
*   \return 	none
*/
void Init_I2C(void)
{

	PINSEL_CFG_Type 	PinCfg;

	/* I2C block ------------------------------------------------------------------- */
	/*
	 * Init I2C pin connect
	 */
	PinCfg.OpenDrain	= PINSEL_PINMODE_NORMAL;		// Pin is in the normal (not open drain) mode
	PinCfg.Pinmode 		= PINSEL_PINMODE_PULLUP;		// Internal pull-up resistor
	PinCfg.Funcnum 		= PINSEL_FUNC_3;				// third or reserved alternate function -> select SDA1/SCL1
	PinCfg.Pinnum 		= PINSEL_PIN_0; 				// Pin 0 -> SDA1
	PinCfg.Portnum		= PINSEL_PORT_0;				// PORT 0
	PINSEL_ConfigPin(&PinCfg);							// Set Config for SDA
	PinCfg.Pinnum 		= PINSEL_PIN_1;					// Pin 0 -> SCL1
	PINSEL_ConfigPin(&PinCfg);							// Set Config for SCL1


	/* Initialize Slave I2C peripheral with 100 kHz */
	I2C_Init(I2CDEV_M, 100000);

	/* Enable Slave I2C operation */
	I2C_Cmd(I2CDEV_M, ENABLE);

}



/*!
*
*	\brief	<b>DS1621 einschalten<b>:
*
*			Funktion zum Einschalten/Konfigurieren des DS1621.
*			Am Ende der Initialisierung werden 3 Dump-Messungen gemacht und die Messwerte verworfen. Das soll vorbeugen, dass gleich zu beginn auf dem Display falsche Werte angezeigt werden.
*
*   \param[in]	none
*   \return 	none
*/
void DS1621_Init(void)
{
	uint8_t temp[1];

	/* Paket 1: Konfiguration des Configuration Registers des DS 1621 */
	Master_Buf[0]	= ACCESS_CONFIG;
	Master_Buf[1]	= SET_UP;
	transferMCfg.sl_addr7bit			= I2CDEV_S_ADDR;
	transferMCfg.tx_data				= Master_Buf;
	transferMCfg.tx_length				= sizeof(Master_Buf);
	transferMCfg.rx_data				= NULL;
	transferMCfg.rx_length				= 0;
	transferMCfg.retransmissions_max	= 5;
	I2C_MasterTransferData(I2CDEV_M, &transferMCfg, I2C_TRANSFER_POLLING);


	/*  Warten bis Configuration Registers gesetzt ist -> Auslesen durch das DONE-Bit in Configuration Registers */
	do{
	transferMCfg.sl_addr7bit			= I2CDEV_S_ADDR;
	transferMCfg.tx_data				= Master_Buf;
	transferMCfg.tx_length				= 1;
	transferMCfg.rx_data				= temp;
	transferMCfg.rx_length				= 1;
	transferMCfg.retransmissions_max	= 5;
	I2C_MasterTransferData(I2CDEV_M, &transferMCfg, I2C_TRANSFER_POLLING);
	}while(!(temp[0] & (1<<7)));


	/* Paket 2: Aufforderung zur Konvertierung der Temperatur */
	Master_Buf[0]	= START_CONVERT_T;
	transferMCfg.tx_data				= Master_Buf;
	transferMCfg.tx_length				= 1;
	transferMCfg.rx_data				= NULL;
	transferMCfg.rx_length				= 0;
	transferMCfg.retransmissions_max	= 5;
	I2C_MasterTransferData(I2CDEV_M, &transferMCfg, I2C_TRANSFER_POLLING);

	/* Paket 3: Temperatur auslesen */
	Master_Buf[0]	= READ_TEMPERATURE;
	transferMCfg.sl_addr7bit			= I2CDEV_S_ADDR;
	transferMCfg.tx_data				= Master_Buf;
	transferMCfg.tx_length				= 1;
	transferMCfg.rx_data				= Temperatur;
	transferMCfg.rx_length				= sizeof(Temperatur);
	transferMCfg.retransmissions_max	= 5;


	/* Erste 3 Messungen messen und verwerfen */
	uint8_t var=0;
	for (var = 0; var < 3; ++var){
		I2C_MasterTransferData(I2CDEV_M, &transferMCfg, I2C_TRANSFER_POLLING);
		Timer0_DelayMs(50);
	}


}


/*!
*
*	\brief  <b>CAN-Initialisierung<b>:
*
*			In dieser Funktion wird der CAN-Bus fuer die Uebertragung initialisiert.
*
*			Es erfolgt:
*
*			Baudrate: 125000 Hz
*
*			Extended ID format (da ID: 0x854)
*
*			Laenge des Datenfelds: 8
*
*
*
*   \param[in]	none
*   \return 	none
*/
void can_init(void)
{

	PINSEL_CFG_Type		PinCfg;

	/* Initialize CAN1 peripheral
	 * Note: Self-test mode doesn't require pin selection
	 */
	PinCfg.Funcnum 		= 3;
	PinCfg.OpenDrain 	= 0;
	PinCfg.Pinmode 		= 0;
	PinCfg.Portnum 		= 0;
	PinCfg.Pinnum 		= 21;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum 		= 22;
	PINSEL_ConfigPin(&PinCfg);

	CAN_Init(CAN_PERIPHERAL, 125000);

	/* Enable operating mode */
	CAN_ModeConfig(CAN_PERIPHERAL, CAN_OPERATING_MODE, ENABLE);

	/* Disable Interrupt */
	CAN_IRQCmd(CAN_PERIPHERAL, CANINT_RIE, DISABLE);
	CAN_IRQCmd(CAN_PERIPHERAL, CANINT_TIE1, DISABLE);

	/* Enable CAN Interrupt */
	NVIC_EnableIRQ(CAN_IRQn);
	CAN_SetAFMode(LPC_CANAF,CAN_AccBP);


	/* Initialisierung der Nachrichten fuer die CAN_Uebertragung */
	TXMsg.format = EXT_ID_FORMAT;				//* 11 Bit Id
	TXMsg.id = CAN_IDENTIFIER;					//* Identifier
	TXMsg.len = 8;								//* Laenge des Datenfelds in Bytes
	TXMsg.type = DATA_FRAME;					//* Datenfeld soll uebertragen werden

}



/*!
*
*	\brief		<b>Warte-Funktion mit Timer0<b>:
*
*				Wartefunktion mit Timer0
*
*	\param[in]	Wartezeit in ms
*	\return 	none
*/
void Timer0_DelayMs(uint32_t delayInMs)
{

	LPC_TIM0->TCR = 0x02;		/* reset timer */
	LPC_TIM0->PR  = 0x00;		/* set prescaler to zero */

	LPC_TIM0->MR0 = delayInMs * (25000000 / 1000-1);

	//LPC_TIM0->MR0 = (100000000 / 4) / (1000/delayInMs);  /* enter delay time */
	LPC_TIM0->IR  = 0xff;		/* reset all interrrupts */
	LPC_TIM0->MCR = 0x04;		/* stop timer on match */
	LPC_TIM0->TCR = 0x01;		/* start timer */

	/* wait until delay time has elapsed */
	while (LPC_TIM0->TCR & 0x01);

 }


