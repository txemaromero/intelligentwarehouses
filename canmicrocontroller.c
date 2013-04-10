#include <stdio.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_can.h"
#include "inc/hw_ints.h"
#include "driverlib/can.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/systick.h"
#include "consolaLCD/console.h"

#define TRANSMIT_MESSAGE_ID     515
#define RECEIVE_MESSAGE_ID      515
#define RECEIVE_MESSAGE_MASK    0xFFF
#define CAN_BITRATE             500000
#define RECEIVE_MESSAGE_OBJ    	1
#define TRANSMIT_MESSAGE_OBJ    2

typedef struct s_CAN_MSG
{
  tCANMsgObject msgObj;
  unsigned char data[8];
}CAN_MSG;

void transmisionCANandLog(void);
void recepcionCANandLog(void);
void inicializar(void);
void msgToConsole(CAN_MSG *pMsg );
void pressConfirm();

volatile unsigned long intCount = 0;
volatile unsigned long sentCount = 0;
volatile unsigned long recvCount=0;
volatile unsigned long rMsgCount=0;
volatile unsigned long ticks=0;

CAN_MSG transmitMsg={{TRANSMIT_MESSAGE_ID,0, MSG_OBJ_TX_INT_ENABLE, 8, transmitMsg.data},{0,0,0,0,0,0,0,0}};
CAN_MSG receivedMsg={{RECEIVE_MESSAGE_ID,RECEIVE_MESSAGE_MASK,
                      MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER,8,receivedMsg.data}};

int main(void)
{
  inicializar();
  while(1)
  {
	if(rMsgCount!=recvCount)
	{
	   recvCount++;
	   recepcionCANandLog();
	   pressConfirm();
	   transmisionCANandLog();
	   refreshConsole();
	}
  }
}

void recepcionCANandLog(void)
{
  char str[32];

  CANMessageGet(CAN0_BASE, RECEIVE_MESSAGE_OBJ, &receivedMsg.msgObj, 0);
  sprintf(str,"Tramas recibidas: %lu",recvCount);
  consolePrintStr(0,2,str);
  msgToConsole(&receivedMsg);
}

void transmisionCANandLog(void)
{
  char str[32];

  sentCount++;

  transmitMsg.data[0]='1'; //estado del DPD
  CANMessageSet(CAN0_BASE, TRANSMIT_MESSAGE_OBJ, &transmitMsg.msgObj, MSG_OBJ_TYPE_TX);
  sprintf(str,"Tramas enviadas: %lu",sentCount);
  consolePrintStr(0,4,str);
}

void inicializar()
{
  char str[32];

  SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);//SYSCTL_XTAL_16MHZ
  initConsole();
  sprintf(str,"Tramas recibidas: %lu",recvCount);
  consolePrintStr(0,2,str);
  sprintf(str,"Tramas enviadas: %lu",sentCount);
  consolePrintStr(0,4,str);
  refreshConsole();
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
  GPIOPinConfigure(GPIO_PD0_CAN0RX);
  GPIOPinConfigure(GPIO_PD1_CAN0TX);
  GPIOPinTypeCAN(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);
  CANInit(CAN0_BASE);
  CANBitRateSet(CAN0_BASE, 8000000, CAN_BITRATE);
  CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);
  SysTickPeriodSet(SysCtlClockGet());
  SysTickIntEnable();
  SysTickEnable();
  IntEnable(INT_CAN0);
  IntMasterEnable();
  CANMessageSet(CAN0_BASE, RECEIVE_MESSAGE_OBJ, &receivedMsg.msgObj, MSG_OBJ_TYPE_RX);
  CANEnable(CAN0_BASE);

  //Configurar botón de confirmar

  //125ns
  SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |SYSCTL_XTAL_8MHZ);
  //Display
  RIT128x96x4Init(1000000);

  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
  GPIOPinTypeGPIOInput( GPIO_PORTE_BASE, GPIO_PIN_1 | GPIO_PIN_0 | GPIO_PIN_2| GPIO_PIN_3);
  GPIOPadConfigSet( GPIO_PORTE_BASE, GPIO_PIN_1 | GPIO_PIN_0 | GPIO_PIN_2| GPIO_PIN_3, GPIO_STRENGTH_2MA , GPIO_PIN_TYPE_STD_WPU);

  //
}

void msgToConsole(CAN_MSG *pMsg )
{
  const int MAX=32;
  char str[MAX];
  int longitud, i;

  sprintf(str,"     ID DPD: %lu",pMsg->msgObj.ulMsgID);
  consolePrintStr(0,6,str);

  longitud=pMsg->msgObj.ulMsgLen;
  sprintf(str,"     Cantidad: ");
  for (i=0; i<(longitud-1); i++)	str[15+i]=pMsg->data[i];
  for (i=15+longitud-1; i<MAX; i++) str[i]=' ';		//limpiar buffer
  consolePrintStr(0,8,str);

  sprintf(str,"     Operario: %c", pMsg->data[longitud-1]);
  consolePrintStr(0,10,str);

  refreshConsole();
}

void CANIntHandler(void)
{
  unsigned long ulStatus;

    //
    // Read the CAN interrupt status to find the cause of the interrupt
    //
  intCount++;
  ulStatus = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);

    //
    // If the cause is a controller status interrupt, then get the status
    //
  if(ulStatus == CAN_INT_INTID_STATUS)
  {
    ulStatus = CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);
  }
  else if(ulStatus == TRANSMIT_MESSAGE_OBJ)
  {
    CANIntClear(CAN0_BASE, TRANSMIT_MESSAGE_OBJ);
  }
  else if(ulStatus == RECEIVE_MESSAGE_OBJ)
  {
    CANIntClear(CAN0_BASE, RECEIVE_MESSAGE_OBJ);
	rMsgCount++;
  }
  //else	 // Spurious interrupt handling can go here.
}

void sysTickIntHandler(void)
{
  ticks++;
}

void pressConfirm()
{
	unsigned long pressed_data;

	pressed_data = (GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_1 | GPIO_PIN_0 | GPIO_PIN_2| GPIO_PIN_3)); //leer de ese puerto ese pin

	//usa una máscara porque sólo me interesa un botón

	while (!((~pressed_data) & (GPIO_PIN_0)))
	{
		if ((~pressed_data) & (GPIO_PIN_0))
		{
			RIT128x96x4StringDraw("Confirmando ...", 10, 5, 15);
		}
		pressed_data = (GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_1 | GPIO_PIN_0 | GPIO_PIN_2| GPIO_PIN_3)); //leer de ese puerto ese pin
	}
}
