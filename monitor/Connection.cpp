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



Connection::Connection(HWND hwnd, std::string &port):Win32Port(port, CBR_9600, NOPARITY, 8, ONESTOPBIT, ENABLE,DISABLE),
                                                     OnLine(FALSE),
                                                     mode(0),
                                                     hWnd(hwnd)
{
    lstrcpy(PortName,port.c_str());
    if(error_status) 
        DisplayLastError("Connectio::");
}

Connection::~Connection()
{
    Dtr(0);
    Rts(0);
    SetCommMask(m_hPort,0);
}

BOOL Connection::SendCommand(char *comd)
{
    if(write_buffer(comd, 5) != RS232_SUCCESS)
    {
        DisplayLastError("WritePort");
        return FALSE;
    }
    return TRUE;
}

BOOL Connection::SendData(int len, char *data)
{
    CopyMemory(&FrameData[5],data,len);

   return TRUE;
}


void Connection::DsrNotify(bool status)
{
      OnLine=status;
      printf("%s\n", OnLine?"OnLine":"OffLine");
}

void Connection::CtsNotify( bool status )
{
       DWORD error;
     /**
           CTS Hi == disable  CTS low == Enable
     */

  //   if(*(DWORD*)FrameData==0xbd91405)
  //          printf("Enable Monitor\n");// Dtr(1);


     printf("%s\n",status==false?"CTS_CONTROL_ENABLE":"CTS_CONTROL_DISABLE");
 //printf(" cmd 0x%x adr 0x%x len 0x%x :",FrameData[1],FrameData[2],len  );



      if(status == false || (FrameData[1]&0x10) == 0) return;

 /// if (CTS is disable(true) and cmd==WRITE_MEM(0x01)) send data now.

      error=write_buffer(&FrameData[5],FrameData[3]);
      if(error != RS232_SUCCESS)
      {
          DisplayLastError("WritePort");
      }

 //     printf(" Frame_Data 0x%x 0x%x 0x%x data:",FrameData[1]&0xff,FrameData[2]&0xff,FrameData[3]&0xff );
      printf("Frame_Data: ");
      for(int n=0;n<FrameData[3];n++)
          printf("0x%x ", FrameData[5+n] );

     printf("\n");


}

void Connection::TxNotify( )
{
      DWORD error;
      if( FrameData[0]==0) return;

     if(!IsConnected() )
      {

          return;
      }
      printf("Frame_Cmd 0x%x 0x%x 0x%x 0x%x \n",FrameData[0]&0xff, FrameData[1]&0xff, FrameData[2]&0xff, FrameData[3]&0xff  );



}

void Connection::RxNotify( int byte_count )
{
    if(byte_count>60)
    {
        printf("RX:byte_count == %d !!!\n",byte_count);
       byte_count=60;
    }

    char data;
    static int idx=0;
    unsigned char ChkSum=0;
    static DWORD CmdMsg=0;



    if(CmdMsg == 0)
    {
        CmdMsg = *((DWORD*)&FrameData[0]);

    }

    read_buffer(FrameData+idx, byte_count );
    idx+=byte_count;

    if(FrameData[0] == 0xffffffaa )
    {
        CmdMsg=((app->GetMonLen()+1)<<24)|0xaa;
    }

    /** if is not a complete frame return */
    if(idx < (CmdMsg>>24)+1)return;

    if(idx > (CmdMsg>>24)+1)idx=(CmdMsg>>24);

    if((CmdMsg&0xff) != 0xaa)
        printf("0x%x data ", CmdMsg) ;

    for(int i=0;i<idx-1;i++)
    {
        ChkSum += (FrameData[i]&0xff);
    }
    if(ChkSum != (FrameData[idx-1]&0xff))
        printf("CheckSum Error\n");

    if((CmdMsg&0xff) != 0xaa)
    {
        for(int i=0;i<idx;i++)
       {
           printf("0x%x ", (FrameData[i]&0xff));
       }
       printf("\n");
    }
    if((CmdMsg&0xff)==0xaa)
        CmdMsg=0xaa;

/*

      SYSTEMTIME t;
     GetLocalTime(&t);
       printf("%d:%d\n",t.wSecond,t.wMilliseconds );
*/

       PostMessage(app->GetHandle(),EV_DEVICE_MSG,(WPARAM)CmdMsg,(LPARAM)FrameData);

     idx=0;
    CmdMsg=0;

}



char *  Connection::BuildCmd(char cmd, char memTyp, char addr,char len)
{


     cmd    = cmd&0xf0;
     memTyp = memTyp&0x0c;
     char bank   = 0; /// bank23 -> cmd bit 0 + addr bit 7 bank01


     FrameData [0] = 5;
     FrameData [1] = cmd | memTyp | bank ;
     FrameData [2] = addr;
     FrameData [3] = len;
     FrameData [4] = cmd ^ memTyp ^ bank ^ addr ^ len;

 //   printf("frame 0x%x 0x%x 0x%x 0x%x 0x%x\n",FrameData [0]&0xff,FrameData [1]&0xff,FrameData [2]&0xff,FrameData [3]&0xff,FrameData [4]&0xff);
     return FrameData;
}

bool Connection::IsConnected()
{//DSR status
   return OnLine;
}


///*********************************************
