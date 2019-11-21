#ifndef  _WIN32_IE
#define _WIN32_IE 0x0500
#endif

#include <windows.h>

#include "App.h"

#include <commctrl.h>
#include <tchar.h>
#include <dbt.h>
#include <stdio.h>



///**************************************************



Connection::Connection(HWND hwnd, char *port):hWnd(hwnd),
                                              hPort(NULL),
                                              hEvent(NULL)
{

     FillMemory(RxBuffer,16,0);

     lstrcpy(PortName,port);

     if(OpenSerial(PortName))   /// Open last serial com port
     {

        SetCommMask(hPort,EV_RXCHAR);
        hEvent= CreateEvent(NULL, FALSE, FALSE, "rs");

        if( hEvent == NULL)
        {
           printf("Err CreateEvent()");
           CloseHandle(hPort);
           return;
        }
   //     OpenEvent(SYNCHRONIZE, FALSE, "rs");

    }

}


Connection::~Connection()
{

    if(hEvent)CloseHandle(hEvent);
    if(hPort)CloseHandle(hPort);
    hPort = hEvent = NULL;

 }

BOOL Connection::IsConnected()
{
     DWORD status;
      GetCommModemStatus(hPort,&status);
  //     printf("status 0x%x\n", status);
     if(status&MS_DSR_ON) return TRUE;
     return FALSE;

}

BOOL Connection::OpenSerial(char *name)
{
     FlowControl fc = NoFlowControl;
    hPort=CreateFile(name, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);
    if(hPort == INVALID_HANDLE_VALUE) return FALSE;


     DCB dcb;
      printf("Open %s Success\n",name);
        GetCommState(hPort,&dcb);
          dcb.DCBlength = sizeof(DCB);
          dcb.BaudRate = CBR_9600;
          dcb.ByteSize = 8;
          dcb.StopBits = ONESTOPBIT;
          dcb.Parity   = NOPARITY;
       //   if(dcb.fParity  == FALSE)
          dcb.Parity = 'N';
          dcb.fAbortOnError = TRUE;
 //          cfg.fOutxDsrFlow = FALSE;            //   DCE:  permission ok
 //         cfg.fOutxCtsFlow = FALSE;            //   DCE:  permission ok
 //          dcb.fDsrSensitivity = FALSE;
 //          dcb.fDtrControl = DTR_CONTROL_ENABLE;
//          dcb.fRtsControl = RTS_CONTROL_DISABLE;


     switch (fc)
     {
          case NoFlowControl:
          {

               dcb.fOutxCtsFlow = FALSE;
               dcb.fOutxDsrFlow = FALSE;
               dcb.fOutX = FALSE;
               dcb.fInX = FALSE;
               break;
          }
          case CtsRtsFlowControl:
          {
               dcb.fOutxCtsFlow =  TRUE;
               dcb.fOutxDsrFlow = FALSE;
               dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
               dcb.fOutX = FALSE;
               dcb.fInX = FALSE;
               break;
          }
          case CtsDtrFlowControl:
          {
               dcb.fOutxCtsFlow = TRUE;
               dcb.fOutxDsrFlow = FALSE;
               dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;
               dcb.fOutX = FALSE;
               dcb.fInX = FALSE;
               break;
          }
          case DsrRtsFlowControl:
          {
               dcb.fOutxCtsFlow = FALSE;
               dcb.fOutxDsrFlow = TRUE;
               dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
               dcb.fOutX = FALSE;
               dcb.fInX = FALSE;
               break;
          }
          case DsrDtrFlowControl:
          {
               dcb.fOutxCtsFlow = FALSE;
               dcb.fOutxDsrFlow = TRUE;
               dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;
               dcb.fOutX = FALSE;
               dcb.fInX = FALSE;
               break;
          }
     }


        printf("%s: BaundRate %d ",name, dcb.BaudRate);
        printf("%d Bits ", dcb.ByteSize);
        printf("%d StopBit ",(dcb.StopBits==ONESTOPBIT? 1:2) );
       printf("%s\n",(dcb.Parity==EVENPARITY ? "EvenParity":
                      dcb.Parity==ODDPARITY  ? "OddParity":
                      dcb.Parity==MARKPARITY ? "MarkParity":
                      dcb.Parity==SPACEPARITY? "SpaceParity":"NoParity"));
/*
        printf("Dtr %d Rts %d XonXoff %d CtsFlow %d DsrFlow %d\n",
                dcb.fDtrControl,dcb.fRtsControl,
                dcb.fOutX, dcb.fOutxCtsFlow, dcb.fOutxDsrFlow);
*/
        SetCommState(hPort,&dcb);



       COMMTIMEOUTS TimeOut={0x01,0,0,0,0};
        SetCommTimeouts(hPort,&TimeOut);



      return TRUE;
}




DWORD Connection::ReadDevice(int cmdLen)
{
     BYTE data;
     DWORD sz,len;
     DWORD err;
     DWORD event;
     COMSTAT stat;
     char msg[256];
     char str[50];
     DCB dcb;
     sz=0;
     OVERLAPPED Overlapp={0};
     Overlapp.hEvent=hEvent;

     if(WaitCommEvent(hPort, &event, &Overlapp) == NULL)
     {
         if((err = GetLastError()) && err != ERROR_IO_PENDING)
         {
             DisplayLastError("Reading Port",err);
             return 0;
         }
         GetOverlappedResult( hPort, &Overlapp, &sz,TRUE);
     }
     if(event == EV_RXCHAR)  /// 15 ms to get eco
     {

        if(!ReadFile(hPort, RxBuffer, cmdLen, &sz, &Overlapp))
        {
            if((err = GetLastError()) && err != ERROR_IO_PENDING)
            {
                DisplayLastError("Reading Port",err);
                return 0;
            }
            GetOverlappedResult( hPort, &Overlapp, &sz,TRUE);
        }
     }
     return sz;
}



BOOL Connection::SendCommand1 (char *comd)
{
     DWORD event,sz;
    DWORD err=0;
    OVERLAPPED Overlapp = {0 };
    Overlapp.hEvent=hEvent;
    int FrameSize=5;
    int n=0;
    char data,Old;
     BOOL Waiting=FALSE;
    if(hPort == NULL)return FALSE;
    Old=0;
  //   while(n<FrameSize)
     {


              if(!WriteFile( hPort,  &comd[n], 5, &sz,&Overlapp ))
             {
                 if((err = GetLastError()) && err != ERROR_IO_PENDING)
                 {
                     DisplayLastError("Writing to hPort",err);
                     return FALSE;
                 }

                 if(!GetOverlappedResult (hPort,&Overlapp,&sz,TRUE))
                 {
                    DisplayLastError("Writing to hPort",err);
                     return FALSE;
                 }
             }
              Waiting=TRUE;

    }
     return TRUE;
}

BOOL Connection::SendCommand (char *comd)
{
    DWORD event,sz;
    DWORD err=0;
    OVERLAPPED Overlapp = {0 };
    Overlapp.hEvent=hEvent;
    int FrameSize=5;
    int n=0;
    char data,Old;
     BOOL Waiting=FALSE;
    if(hPort == NULL)return FALSE;
    Old=0;

 //   printf("Eco_SendFrame ");
    while(n<FrameSize)
     {
         if( Waiting == FALSE)
         {
              if(!WriteFile( hPort,  &comd[n], 1, &sz,&Overlapp ))
             {
                 if((err = GetLastError()) && err != ERROR_IO_PENDING)
                 {
                     DisplayLastError("Writing to hPort",err);
                     return FALSE;
                 }

                 if(!GetOverlappedResult (hPort,&Overlapp,&sz,TRUE))
                 {
                    DisplayLastError("Writing to hPort",err);
                     return FALSE;
                 }
             }
              Waiting=TRUE;
         }
         /// Wait for eco
         WaitCommEvent(hPort, &event, &Overlapp);

         if(event == EV_RXCHAR)  /// 15 ms to get eco
         {
     //        ResetEvent(hEvent);
              if(!ReadFile(hPort, &data, 1, &sz, &Overlapp))
              {
                  if((err = GetLastError()) && err == ERROR_IO_PENDING)
                       GetOverlappedResult( hPort, &Overlapp, &sz,TRUE);
                  else if(err)
                  {
                      DisplayLastError("Reading Port",err);
                      return FALSE;
                  }
              }
              Old |= comd[n]-data;

               n++;
               Waiting=FALSE;
        }  // if(event == EV_RXCHAR)
     } // for(
 //    printf("\n");
    if(Old)
    {
 //       MessageBox(0,"Erro de comunicação","Frame Erro",MB_OK|MB_ICONEXCLAMATION);

        return FALSE;
    }

    return TRUE;
}

BOOL Connection::SendData(int len, char *data)
{
     OVERLAPPED Overlapp = {0 };
    Overlapp.hEvent=hEvent;
    DWORD sz,err;
    if(hPort == NULL)return FALSE;
    int n=0;
    while(n<len)
     {
         if(!WriteFile( hPort,  &data[n], 1, &sz,&Overlapp ))
        {
           if((err = GetLastError()) && err != ERROR_IO_PENDING)
            {
                   DisplayLastError("Writing to hPort",err);
                    return FALSE;
            }

             if(!GetOverlappedResult (hPort,&Overlapp,&sz,TRUE))
            {
                    DisplayLastError("Writing to hPort",err);
                     return FALSE;
            }
        }
        n++;
     }
}


///*********************************************
