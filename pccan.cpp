#include <stdio.h>
#include <windows.h>
#include <string.h>
#include "PCANBasic.h"

int menu(void);
void userCommands(TPCANHandle hdlPCAN);
int askID();
void askMessageAndSend(TPCANHandle);
void printMsgSent(TPCANMsg *pMsg);
void printMsgReceived(TPCANMsg *pMsg);

int main(void)
{
  char str[128];
  int usbChanel;
  TPCANHandle hdlPCAN;
  
  printf("Que puerto USB quieres usar con PCAN (PEAK System)? (1-8) ");
  gets(str);
  sscanf(str,"%d",&usbChanel);
  hdlPCAN=PCAN_USBBUS1+usbChanel-1;
  if(PCAN_ERROR_OK!=CAN_Initialize(hdlPCAN, PCAN_BAUD_500K, 0, 0, 0))
  {
    printf("No puedo inicializar PCAN-USB%d\n",usbChanel);
    return -1;
  }
  userCommands(hdlPCAN);
  CAN_Uninitialize(hdlPCAN);
  printf("pulsa \"Intro\" para finalizar");
  gets(str);
  return 0;
}

void userCommands(TPCANHandle hdlPCAN)
{
  TPCANMsg msg;
  TPCANTimestamp timeStamp;
  TPCANStatus status;
  char str[30];
  int choice;

  msg.MSGTYPE=PCAN_MESSAGE_STANDARD;
  while((choice=menu())!=0)
  {
    switch(choice)
    {
      case 1:
        askMessageAndSend(hdlPCAN); 
        break;
      case 2:
        status=CAN_Read(hdlPCAN, &msg, &timeStamp);
        switch(status)
        {
          case PCAN_ERROR_QRCVEMPTY:
                 printf("---- La cola de mensajes esta vacia -----\n");
                 break;
          case PCAN_ERROR_OK:
                 printMsgReceived(&msg);
                 break;
          default:
                 printf("----- Error\n -----");
                 break;        
        }
        break;
    }
  }
}

void askMessageAndSend(TPCANHandle hdlPCAN)
{
  char str[128];
  int cantidad, operario;
  TPCANStatus status;
  TPCANMsg msg;

  msg.MSGTYPE=PCAN_MESSAGE_STANDARD;
  msg.ID=(DWORD)askID();
   
  printf("\nEscribir cantidad de productos (1-999999) ");
  gets(str);
  sscanf(str,"%d",&cantidad);
  
  while ((cantidad<1) || (cantidad>999999))
  {
        printf("Cantidad %d introducida no valida\nEscribir cantidad (1-999999) ", cantidad);
        gets(str);
        sscanf(str,"%d",&cantidad);
  }  

  printf("Escribir operario (0-3) ");
  gets(str);
  sscanf(str,"%d",&operario);
  
  while ((operario<0) || (operario>3))
  {
        printf("Operario %d introducido no valido\nEscribir operario (0-3) ", operario);
        gets(str);
        sscanf(str,"%d",&operario);
  }
  
  sprintf(str, "%d%d", cantidad, operario);
    
  strncpy((char*)&msg.DATA,str,strlen(str));
  msg.LEN =(BYTE)strlen(str);
  
  status=CAN_Write(hdlPCAN, &msg);
  if(status!= PCAN_ERROR_OK) printf("error \"%d\" enviando\n",status);
  else printMsgSent(&msg);
}

int menu(void)
{
  int emaitza;
   char str[30];

   printf("\nElige opcion\n");
   printf("\t1: Enviar una trama de aplicacion\n");
   printf("\t2: Esperar una trama de aplicacion de un DPD\n");
   printf("\t0: Finalizar la aplicacion del controlador\n");
   gets(str);
   sscanf(str,"%d",&emaitza);
   return emaitza;
}

int askID()
{
  char str[128];
  int id;

  printf("\nLa trama de la aplicacion (sobre bus CAN) a enviar estara formada por: Cantidad (1-999999) Operario (0-3) ID DPD (0-2047)\n\nCual es el ID DPD (0-2047) de la trama de aplicacion?\n");
  
  gets(str);
  sscanf(str,"%d",&id);
  while (id<0 || id>2047)
  {
      printf("ID DPD %d introducido no valido\nCual es el ID DPD (0-2047) de la trama de aplicacion?\n", id);
      gets(str);
      sscanf(str,"%d",&id);
  }
  printf("De acuerdo, ID DPD es %d\n", id);
  
  return id;
}

void printMsgSent(TPCANMsg *pMsg)
{
  int cantidad, operario, dpd, longitud, i;
  char c;
  
  printf("\n\t\t --- ID11: %d ", pMsg->ID);
  printf(" Longitud mensaje: %d ",pMsg->LEN);
  printf(" Datos enviados:");
  
  i=0;
  longitud=pMsg->LEN;
  char str[longitud];
  
  while (i<(longitud-1))
  {
        c=pMsg->DATA[i];
        printf(" %c",c);
        str[i]=c;
        i++;
  }
  str[i]='\0';
  
  c=pMsg->DATA[longitud-1];
  printf(" %c",c);
  operario=atoi(&c);
  
  cantidad=atoi(&str[0]);
  dpd=pMsg->ID;
  
  printf("\n\nNuestra trama de aplicacion es:\nID DPD: %d\nCantidad: %d\nOperario: %d\n", dpd, cantidad, operario);
}

void printMsgReceived(TPCANMsg *pMsg)
{
  char c;
  int estado, dpd;
  
  printf("\n\t\t --- ID11: %d ", pMsg->ID);
  printf(" Datos recibidos: %c\n", pMsg->DATA[0]);
  
  c=pMsg->DATA[0];
  estado=atoi(&c);
  
  dpd=pMsg->ID;
  
  printf("\nNuestra trama de aplicacion recibida es:\nEstado: %d (Si se realiza correctamente, es 1)\nID DPD %d\n", estado, dpd);
}
