#define WIN32_LEAN_AND_MEAN

#ifndef  _WIN32_IE
#define _WIN32_IE 0x0500
#endif

 #ifndef  WINVER
#define WINVER 0x0500
#endif



#include <tchar.h>
#include <windows.h>
#include <commctrl.h>
#include <setupapi.h>
#include <dbt.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>

#include <string>
#include "Win32Port.h"

#include "resource.h"
#include "App.h"



#define BANK0:1    0x00  /// xxxx xxx0
#define BANK2:3    0x01  /// xxxx xxx1
#define ROM        0x00  /// xxxx 00xx
#define RAM        0x04  /// xxxx 01xx
#define EEPROM     0x08  /// xxxx 10xx
#define READ_MEM   0x00  /// xxx0 xxxx
#define WRITE_MEM  0x10  /// xxx1 xxxx



#define PCL    0x02
#define PCLATH 0x0a
#define CCPR1L 0x15
#define CCP1CON 0x17

using namespace std;

HINSTANCE hInst;
 App *app=NULL;

//Connection *Link;

static const GUID GUID_DEVICEINTERFACE_LIST[] =
{
	// GUID_DEVINTERFACE_COMPORT
	{ 0x86e0d1e0, 0x8089, 0x11d0, { 0x9c, 0xe4, 0x08, 0x00, 0x3e, 0x30, 0x1f, 0x73 } },
};


BOOL CALLBACK MainProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    switch(uMsg)
    {
        case WM_INITDIALOG:
             app = new App(hwndDlg);
             return TRUE;
        case WM_COMMAND:
             app->OnCommand(wParam,lParam);
             return FALSE;
        case WM_NOTIFY:
             app->OnNotify(wParam,lParam);
             break;
        case EV_APP_INIT:
             app->OnInit(wParam, lParam);
             break;
         case EV_DEVICE_MSG:
             if(app)app->OnDevMsg(wParam,lParam);
             break;
         case WM_DEVICECHANGE:
             if(app) app->OnDeviceChange(wParam,lParam);
             return TRUE;
        case WM_CLOSE:
             delete app;
             DestroyWindow(hwndDlg);
             return TRUE;
        case WM_DESTROY:
             PostQuitMessage(0);
             return FALSE;
    }

    return FALSE;
}





int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    hInst = hInstance;
    StartCommonControls(ICC_WIN95_CLASSES);
    HWND hwnd=CreateDialog(hInstance, MAKEINTRESOURCE(DLG_MAIN),NULL,(DLGPROC)MainProc);


    MSG msg;
    while(GetMessage(&msg,NULL,0,0)){
		if (!::IsDialogMessage (hwnd, &msg))
        {
			::TranslateMessage (&msg);
			::DispatchMessage (&msg);
        }
    }
    return 0;

}



App::App(HWND hwnd):hwnd(hwnd), CmdLen(0),nDuty(0),mIndex(0),ListSz(0),DataRefreshRate(1000)
{
    CreateMainMenu();
    RegisterNotificationMsg();

     string pname("COM3");
      Link = new Connection(hwnd, pname);    /// Open last serial com port



     TabCtrl = new  CTabCtrl(hwnd);
     TabCtrl->Insert(new CMonitorPage(TabCtrl,"Monitor"));
     TabCtrl->Insert(new CBattPage(TabCtrl, "Grupo de baterias"));
     TabCtrl->Insert(new CPainelPage(TabCtrl, "Paineis solares"));
     TabCtrl->Insert(new CPortPage(TabCtrl, "Comunicações"));
     TabCtrl->Insert(new CConfigPage(TabCtrl, "Configuração"));


    if(Link == NULL)return;
    if(Link->IsConnected())
            PostMessage(hwnd,EV_APP_INIT,0,0);
 //    else MessageBox(hwnd,"Dispositivo está OffLine", "Aviso",MB_ICONINFORMATION|MB_ICONASTERISK|MB_OK);

}

App::~App()
{
      if(Link)delete Link;
     delete TabCtrl;
     UnregisterDeviceNotification(hDevNotify);
/**
   guardar ComPort, RefreshRate
*/
 }


short App::ReadDeviceData(HWND hwnd,  char addr, char sz, char *t, AD_Value Func)
{
    short val=0;
/*      char *data=BuildCmd(READ_MEM, RAM,  addr, sz);

    if(Link->IsConnected() == FALSE)
    {
        MessageBox(App::hwnd,"O equipamento está OffLine","Erro",MB_OK|MB_ICONEXCLAMATION);
        return 0;
    }

     if(Link->SendCommand(data))
     {
         int n = CmdLen;
          n-=Link->ReadDevice(n);

         char str[32];
         if(sz==1) val = *((char*)Link->GetBuffer());
        else if(sz==2) val = *((short*)Link->GetBuffer());
        else if(sz==3) val = *((long*)Link->GetBuffer())&0x0fff;
        else if(sz==4) val = *((long*)Link->GetBuffer());
        else if(sz>0)  val = sz;

         if(Func != NULL)
         {
            double v=Func(val);
            sprintf(str,"%f",v);
         }
         else sprintf(str,"%d",val);
         char *ptr=str;
         int digit;
         if(*t == 'A') digit=4; else digit=3;
         while(*ptr)
         {
             if(*ptr=='.')
             {
                  ptr+=digit;
                 *ptr = '\0';
                  break;
             }
             ptr++;
         }
         sprintf(ptr," %s",t);
        if(hwnd!=NULL)
             if(SendMessage(hwnd,WM_SETTEXT,0,(LPARAM)str) != TRUE)
             {
                 DisplayLastError("ReadDeviceData",0);
             }

     }
     else
     {
         char msg[255];
         sprintf(msg,"ao tentar ler %d bytes no addr 0x%x",data[3],data[2]);
         MessageBox(0,msg,"Frame Error",MB_OK|MB_ICONEXCLAMATION);

     }
  */    return val;
}

LRESULT App::OnInit(WPARAM wParam, LPARAM lParam)
{
    DWORD  ThreadID;
       /// Read AFLAGS
               char *data=Link->BuildCmd(READ_MEM,EEPROM,0,6);

              Link->SendCommand(data);
/*    char *frame=BuildCmd(READ_MEM,RAM,0x45,1);
     int n;
     if(Link->SendCommand(frame))
     {
            n= Link->ReadDevice(CmdLen);
            AFlags = *Link->GetBuffer();
            if(AFlags&0x04)
                CheckMenuItem(GetMenu(hwnd), ID_DEVICE_CHARGE_ON,MF_CHECKED	);
            else if((AFlags&0x04) == 0)CheckMenuItem(GetMenu(hwnd), ID_DEVICE_CHARGE_ON,MF_UNCHECKED	);
     }

       ///   ReadLoopCnt
       frame=BuildCmd(READ_MEM,RAM,0x4d,1);

     if(Link->SendCommand(frame))
     {
            n = Link->ReadDevice(CmdLen);
            ReadLoopCount = *Link->GetBuffer();
     }


 */

   ///       TabCtrl->Populate();
/*
      /// Save MonList to device

       frame=BuildCmd(WRITE_MEM,RAM,0xd9,mon+1); // Add
          Link->SendCommand(frame);
          Link->SendData(mIndex,MonList);
        int len=0;
        printf("Frame 0x%x 0x%x 0x%x 0x%x 0x%x\ndata ",frame[0]&0xff,frame[1]&0xff,frame[2]&0xff,frame[3]&0xff,frame[4]&0xff);
         char *data=&MonList[0];
        for(int i=0;i<MonList[0];i++)
        {
            len += *(data+1);
            while(*data)printf("0x%x ",*data++);

            printf("\n");
            data++;
        }

        CmdLen=  len;

*/

      return 0;
}



LRESULT App::OnNotify(WPARAM wParam,LPARAM lParam)
{
       // get the tab message from lParam
    switch(wParam)
    {
          case IDC_TABCTRL:
               TabCtrl->OnNotify(wParam,lParam);
               break;
    }
    return 0;
}


LRESULT App::OnDeviceChange(WPARAM wParam,LPARAM lParam)
{

/*	if ( DBT_DEVICEARRIVAL == wParam || DBT_DEVICEQUERYREMOVE == wParam || DBT_DEVICEREMOVECOMPLETE == wParam ) {
		PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lParam;
		PDEV_BROADCAST_PORT pDevPort;
		switch( pHdr->dbch_devicetype )
		{
 			case DBT_DEVTYP_PORT:
				pDevPort = (PDEV_BROADCAST_PORT)pHdr;
				if ( DBT_DEVICEARRIVAL == wParam )
				{
				    if(Link) return 0;
	//			    printf("BT_DEVICEARRIVAL DBT_DEVTYP_PORT\n");
				    if(Connect(pDevPort->dbcp_name))
                       PostMessage(hwnd,EV_APP_INIT,0,0);

                }
                else if ( DBT_DEVICEQUERYREMOVE == wParam)
                {


                }
                else if ( DBT_DEVICEREMOVECOMPLETE == wParam)
                {
                    delete Link;
                    Link=NULL;
                }
				break;
         }
    }
  */  return TRUE;
}

LRESULT App::OnCommand(WPARAM wParam,LPARAM lParam)
{
     switch(LOWORD(wParam))
    {
           case ID_DEVICE_CHARGE_ON:
             {
  /*             char *frame=BuildCmd(READ_MEM,RAM,0x45,1);
               if(Link->SendCommand(frame))
               {
                  Link->ReadDevice(CmdLen);
                  AFlags = *Link->GetBuffer();
               }

               char flags =(~AFlags)&0x04;       /// bit 2  0 -> CHARGE_OFF  1 -> CHARGE_ON
                flags = flags |(AFlags&0xfb);

               frame=BuildCmd(WRITE_MEM,RAM,0x45,1);
                Link->SendCommand(frame);
                Link->SendData(1,&flags);


               if((flags&0x04)== 0)
                {
                    char data;    /// clear CCPR1L

                    frame=BuildCmd(WRITE_MEM,RAM,CCPR1L,1);
                    Link->SendCommand(frame);
                    data=0;
                     Link->SendData(1,&data);

                    frame=BuildCmd(READ_MEM,RAM,CCP1CON,1);
                    Link->SendCommand(frame);     /// Read CCP1CON
                     Link->ReadDevice(CmdLen);
                     data = *Link->GetBuffer()&0x0f;

                    frame=BuildCmd(WRITE_MEM,RAM,CCP1CON,1);
                    Link->SendCommand(frame);     /// clear CCPiCON(HI nibble)
                     Link->SendData(1,&data);

                }

                frame=BuildCmd(READ_MEM,RAM,0x45,1);
                if(Link->SendCommand(frame))
                {
                  Link->ReadDevice(CmdLen);
                  AFlags = *Link->GetBuffer();
                }
                if(AFlags&0x04)
                   CheckMenuItem(GetMenu(hwnd), ID_DEVICE_CHARGE_ON,MF_CHECKED	);
                else if((AFlags&0x04) == 0)CheckMenuItem(GetMenu(hwnd), ID_DEVICE_CHARGE_ON,MF_UNCHECKED	);

*/
            }
                break;
          case ID_SLOW:
             {
  /*             char data=1;
               char *frame=BuildCmd(WRITE_MEM,RAM,0x40,1); /// set SLOW_CHARGE
               Link->SendCommand(frame);
               Link->SendData(1,&data);
    */
             }
               break;
          case ID_FAST:
             {
   /*              char data=2;
               char *frame=BuildCmd(WRITE_MEM,RAM,0x40,1); /// set FAST_CHARGE
               Link->SendCommand(frame);
               Link->SendData(1,&data);
     */        }
               break;
          case ID_EQUALIZE:
             {
     /*            char data=3;
                   char *frame=BuildCmd(WRITE_MEM,RAM,0x40,1); /// set EQUALIZE
                    Link->SendCommand(frame);
                    Link->SendData(1,&data);
              //       AddDuty(50);

                   ///
                  ///  set I_oct 0x10 -> 240ma
                  ///            0x08 -> 120ma
                ///
                    data=4;
                    frame=BuildCmd(WRITE_MEM,RAM,0x5A,1); /// set I_oct (low byte)
                    Link->SendCommand(frame);
                    Link->SendData(1,&data);

                  ///      Medir o tempo de resposta Amp/H

*/
             }
               break;
          case ID_FLOTING:
             {
  /*               char data=4;
                  char *frame=BuildCmd(WRITE_MEM,RAM,0x40,1); /// set FLOTING
                   printf("frame 0x%x 0x%x 0x%x 0x%x 0x%x\n",FrameData [0],FrameData [1],FrameData [2],FrameData [3],FrameData [4]);
                    Link->SendCommand(frame);
                     Link->SendData(1,&data);
   */          }
               break;
          case ID_DEVICE_RESET:
             {
    /*           char data=0;
               char *frame=BuildCmd(WRITE_MEM,RAM,PCLATH,1);
               Link->SendCommand(frame);
               Link->SendData(1,&data);

               frame=BuildCmd(WRITE_MEM,RAM,PCL,1);
               Link->SendCommand(frame);
               Link->SendData(1,&data);
     */       }
               break;
          case ID_DEVICE_MEM:
             {
               int userData=DialogBox(hInst,MAKEINTRESOURCE(USERDATA_DLG),hwnd,UserDataProc);

             }
               break;

          case ID_DEVICE_GRAPHIC:
               DialogBox(hInst,MAKEINTRESOURCE(DLG_CREATEGRAPH),hwnd,BuildGraph);

               break;
          case ID_OPTION_SETUP:
                DialogBox(hInst,MAKEINTRESOURCE(CFG_OPTION_DLG),hwnd,ConfigProc);
               break;
     }
    return 0;
}



LRESULT App::OnDevMsg(WPARAM wParam, LPARAM lParam)
{

     switch(wParam)
     {
           case 0x6000805:   /// GetDeviceName
                CopyMemory(DeviceName,(const void*)lParam,6);
                if(memcmp((const void*)DeviceName,"AZA",3))
                   return 0;
                TabCtrl->Populate();
                /// Write MonitorList to device
                //...
                Link->Dtr(1);
                break;
           case  0xaa:        /// Monitor
                Refresh(((char*)lParam+1));
                break;
           default:          /// Memory access dialog
                {
                    char str[128];
                    str[0]='\0';
                    int n=wParam>>24;
        //             printf("msg 0x%x : ",wParam);
                     if(((wParam>>8)&0xf0) == READ_MEM)
                    {
                        char *ptr=(char*)lParam;
                        for(int i=0;i<n;i++)
                        {
                            sprintf(str+strlen(str),"0x%.2x ",(*(ptr+i))&0xff);
  //                          printf("%s ",str);
                        }
  //    printf("\n");
                        HWND HwndDlg=GetActiveWindow();
                        int i=SendDlgItemMessage(HwndDlg,IDC_LISTDATA,LB_ADDSTRING,0,(LPARAM)str);
                        SendDlgItemMessage(HwndDlg,IDC_LISTDATA,LB_SETSEL,TRUE,(LPARAM)i);
                    }
               }
                break;
     }
     return 0;
}



DWORD App::Monitor(TabPage *page, char *listInfo)
{
     int i=0;
     MonPage[mIndex]=page;
      while(*listInfo)
     {
         MonList[++ListSz] = *listInfo++;
         i++;
     }
     MonList[0]=++mIndex;
      return ListSz-i;
}

void App::Refresh(char *data)
{
    for(int n=0;n<mIndex;n++)
       MonPage[n]->Monitor(data);
}

char App::GetMonLen()
{
     char len=0;
     for(int i = 1; i < ListSz; i+=2)
     {
         len += MonList[i];
     }
     return len;
}

void App::Process(short msg, char *FrameData)
{
     if(msg == READ_MEM|EEPROM) /// read eeprom address 0
     {
        CopyMemory(DeviceName,FrameData,6);
        DeviceName[6]=0;
        printf("%c%c%c id %d ver HW %d SW %d\n",FrameData[0],FrameData[1],FrameData[2],FrameData[3],FrameData[4],FrameData[5]);
        return;
     }
     Refresh(FrameData);
}

void App::SetRefreshRate(short r)
{

}


BOOL App::RegisterNotificationMsg()
{
        DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
     ZeroMemory(&NotificationFilter,sizeof(NotificationFilter));
     NotificationFilter.dbcc_size=sizeof(DEV_BROADCAST_DEVICEINTERFACE);
     NotificationFilter.dbcc_devicetype=DBT_DEVTYP_DEVICEINTERFACE;
     for(int i=0;i<sizeof(GUID_DEVICEINTERFACE_LIST)/sizeof(GUID);i++)
     {
         NotificationFilter.dbcc_classguid=GUID_DEVICEINTERFACE_LIST[i];
         hDevNotify=RegisterDeviceNotification(hwnd,&NotificationFilter,DEVICE_NOTIFY_WINDOW_HANDLE);
         if(!hDevNotify)
         {
             MessageBox(hwnd,"Can´t register device notification:","Error",MB_OK|MB_ICONEXCLAMATION);
             return FALSE;
         }
     }
    return TRUE;
}


void App::CreateMainMenu()
{
			HMENU hMenu, hSubMenu;
			hMenu = CreateMenu();

			hSubMenu = CreatePopupMenu();
			AppendMenu(hSubMenu, MF_STRING, ID_DEVICE_CHARGE_ON, _T("&Dispositivo On/Off"));
 //			AppendMenu(hSubMenu,MF_SEPARATOR,0,0);
 			AppendMenu(hSubMenu, MF_STRING, ID_DEVICE_MEM, _T("&Acesso à memoria"));
 	//		AppendMenu(hSubMenu,MF_SEPARATOR,0,0);
			AppendMenu(hSubMenu, MF_STRING, ID_DEVICE_GRAPHIC, _T("&Gráficos"));
 	//		AppendMenu(hSubMenu,MF_SEPARATOR,0,0);
			AppendMenu(hSubMenu, MF_STRING, ID_DEVICE_RESET, _T("&Reset"));
			AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, _T("&Dispositivo"));

            hSubMenu = CreatePopupMenu();
			AppendMenu(hSubMenu, MF_STRING, ID_OPTION_SETUP, _T("&Configuração"));
  			AppendMenu(hSubMenu,MF_SEPARATOR,0,0);
			AppendMenu(hSubMenu, MF_STRING, ID_SLOW, _T("Carga &lenta"));
			AppendMenu(hSubMenu, MF_STRING, ID_FAST, _T("Carga &rápida"));
			AppendMenu(hSubMenu, MF_STRING, ID_EQUALIZE, _T("&Equalização"));
			AppendMenu(hSubMenu, MF_STRING, ID_FLOTING, _T("&Flutuação"));
			AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, _T("&Opções"));

            hSubMenu = CreatePopupMenu();
			AppendMenu(hSubMenu, MF_STRING, ID_HELP_HLP, _T("&Ajuda"));
 			AppendMenu(hSubMenu,MF_SEPARATOR,0,0);
			AppendMenu(hSubMenu, MF_STRING, ID_HELP_ABOUT, _T("&Acerca"));
			AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, _T("&Ajuda"));

			SetMenu(hwnd, hMenu);


}


double AD_INtoVolts(short data)
{      /// R = (R1+R2) / R2 = 7.9117
       double res = (double)((double)5 * (double)data / 1023) * 7.9117;///6.53;
          return res;
}


double ADtoVolts(short data)
{      /// R = (R1+R2) / R2 = 6.7353
       double res = (double)((double)5 * (double)data / 1023) * 6.7353;///6.53;
          return res;
}

double ADtoAmps(short AD)
{    ///  float I = ((float)5/1023)*(float)AD/0.33;
     double I =     (((double).004887) * (double)AD) / 0.33;
       return I;
}

double ADtoTemp(short AD)
{
  //   double T = (((double).004887) * (double)AD) ;
       double T = (double)((double)5 * (double)AD / 1023) * 6.7353;///6.7318325;
   return  18+T;
}

short VoltsToAD(double V)
{
      short v= (V*1023/6.7353)/5;
     return v;
}
short AmpsToAD(double I)
{    /// 5/1023= .004887
    return (.33* I / .004887);
}

short VoltsToPWM(double Rms, short Uin)
{
    static double T=0.00000025;
    double uin = ADtoVolts(Uin);
    double aux = pow( Rms/uin, 2);
    double T1 =  T * aux;
    short duty=(short)((T1/T)*100);    /// in percent
    short pwm=0x3ff*duty/100;


   return pwm;
}
VOID CALLBACK  OnMonitor( HWND hwnd, UINT uMsg,	 UINT idEvent, DWORD dwTime  )
{
 /*    if(app->GetDuty())return;
     for(int n=0;n<app->GetMon();n++)
         app->GetObject(n)->Monitor();
*/
}

void StartCommonControls(DWORD flags)
{
    INITCOMMONCONTROLSEX iccx;
    iccx.dwSize=sizeof(INITCOMMONCONTROLSEX);
    iccx.dwICC=flags;
    InitCommonControlsEx(&iccx);
}
/*
void RegisterMyClass()
{
    WNDCLASSEX wcx={0};  //used for storing information about the wnd 'class'

    wcx.cbSize         = sizeof(WNDCLASSEX);
    wcx.style          = CS_DBLCLKS;
    wcx.lpfnWndProc    = GraphProc;             //wnd Procedure pointer
    wcx.hInstance      = hInst;               //app instance
    wcx.hIcon         = NULL; //reinterpret_cast<HICON>(LoadImage(0,IDI_APPLICATION,    IMAGE_ICON,0,0,LR_SHARED));
    wcx.hCursor       = NULL;//reinterpret_cast<HCURSOR>(LoadImage(0,IDC_ARROW,    IMAGE_CURSOR,0,0,LR_SHARED));
    wcx.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOWFRAME);
    wcx.lpszClassName = "GraphClass";

    if (!RegisterClassEx(&wcx))
    {
       printf("Failed to register wnd class");

    }
}
*/
void DisplayLastError(char * Operation,DWORD error)
{
	//Display a message and the last error in the log List Box.
	if(error==0)error=GetLastError();

	LPVOID lpMsgBuf;
	USHORT Index = 0;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0,
		NULL
	);
//   printf("%s %s (0x%x)\n", Operation,lpMsgBuf,error);
   MessageBox(0,(LPCSTR)lpMsgBuf,Operation,MB_OK|MB_ICONERROR);
	LocalFree(lpMsgBuf);

}


void OnDefaultBtn()
{
/**
  set ReadCnt default value
  set 0xff to EEPROM 0x0a

*/
}

void OnOkBtn()
{
 /**
  set new ReadCnt value
  save new value to EEPROM 0x0a
*/
}
void OnCancelBtn()
{

}
static ULONG To_do=0;

BOOL CALLBACK ConfigProc(HWND hwnd , UINT msg, WPARAM wParam, LPARAM lParam)
{
     switch(msg)
    {
          case WM_INITDIALOG:
             {
               char str[32];
                for(int n=1;n<=20;n++)
               {
                    sprintf(str,"%d",n);
                    SendDlgItemMessage(hwnd,ID_LOOPCNT,CB_ADDSTRING,0,(LPARAM)str);
               }
               short i=app-> ReadDeviceData( NULL, 0x4d, 1, "", NULL);
               sprintf(str,"%d",i);
                SendDlgItemMessage(hwnd, ID_LOOPCNT,CB_SETCURSEL,i-1,0);

               for(int n=250;n<=4000;n+=250)
               {
                    sprintf(str,"%d",n);
                    SendDlgItemMessage(hwnd,ID_REFRESH,CB_ADDSTRING,0,(LPARAM)str);
               }
               i = app->GetRefreshRate();
               sprintf(str,"%d",i);
               i=SendDlgItemMessage(hwnd, ID_REFRESH,CB_FINDSTRING,-1,(LPARAM)str);
               if(i!=CB_ERR) SendDlgItemMessage(hwnd,ID_REFRESH,CB_SETCURSEL,(WPARAM)i,0);

              }
              To_do=0;
               break;
          case WM_COMMAND:
               switch(LOWORD(wParam))
               {
                     case ID_LOOPCNT:
                          if(HIWORD(wParam) == CBN_SELCHANGE)
                          {
                              To_do=ID_LOOPCNT;
                              To_do|=(0x4d<<16);
                          }
                          break;
                     case ID_REFRESH:
                          if(HIWORD(wParam) == CBN_SELCHANGE)
                          {
                               To_do=ID_REFRESH;

                              char str[32];
                              int i = SendDlgItemMessage(hwnd,ID_REFRESH,CB_GETCURSEL,0,0);
                              SendDlgItemMessage(hwnd,ID_REFRESH,WM_GETTEXT,32,(LPARAM)str);
                              i=atoi(str);
                              To_do|=(i<<16);
                         }
                          break;
                     case ID_DEFAULT:
                          OnDefaultBtn();
                          break;
                     case ID_OK:
                        {
          /*                  char data= SendDlgItemMessage(hwnd,(To_do&0xffff),CB_GETCURSEL,0,0)+1;
                            if((To_do&0xffff)==ID_LOOPCNT)
                            {
                                char *frame = app->BuildCmd(WRITE_MEM,RAM,(To_do>>16),1);
                                if(app->Link->SendCommand(frame))
                                {
                                    Link->SendData(1,&data);
                                }
                            }
                            else if((To_do&0xffff)==ID_REFRESH)
                            {
                                 app->SetRefreshRate(To_do>>16);
                            }
                            To_do=0;
                            SendMessage(hwnd,WM_CLOSE,0,0);
            */            }
                          break;
                     case ID_CANCEL:
                          To_do=0;
                          SendMessage(hwnd,WM_CLOSE,0,0);
                          break;
               }
               break;
          case WM_CLOSE:
               EndDialog(hwnd,0);
               break;

    }
    return 0;
}






BOOL CALLBACK MessageProc(HWND hwnd , UINT msg, WPARAM wParam, LPARAM lParam)
{
     switch(msg)
    {
          case WM_INITDIALOG:
                SendDlgItemMessage(hwnd,ID_ERRMSG,WM_SETTEXT,0,(LPARAM)((PackInfo*)lParam)->msg);
                break;
         case WM_CLOSE:
               SendMessage(hwnd,WM_DESTROY,0,0);
               break;
          case WM_DESTROY:
               DestroyWindow(hwnd);
               break;
    }
    return 0;
}

char *VarsName[8]={"Iout","Pwm","Temp","Uin","Uout"};

BOOL CALLBACK BuildGraph(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
          case WM_INITDIALOG:
               for(int i=0;i<5;i++)
                   SendDlgItemMessage(hwnd,ID_VARS,LB_ADDSTRING,0,(LPARAM)VarsName[i]);

                break;
          case WM_COMMAND:
               switch(LOWORD(wParam))
               {
                   case ID_SELECT:
                      {
                        int n=SendDlgItemMessage(hwnd,ID_VARS,LB_GETCURSEL,0,0);
                        SendDlgItemMessage(hwnd,ID_SHOWVARS,LB_ADDSTRING,0,(LPARAM)VarsName[n]);
                      }
                        break;
                   case ID_DELETE:
                      {
                        int n=SendDlgItemMessage(hwnd,ID_SHOWVARS,LB_GETCURSEL,0,0);
                        SendDlgItemMessage(hwnd,ID_SHOWVARS,LB_DELETESTRING,(WPARAM)n,0);
                      }
                        break;
                   case ID_OK:
                        CreateDialog(hInst,MAKEINTRESOURCE(DLG_GRAPHICS),GetParent(hwnd), (DLGPROC)GraphProc);
                   case ID_CANCEL:
                        SendMessage(hwnd,WM_CLOSE,0,0);
                        break;
               }
               break;
         case WM_CLOSE:
               SendMessage(hwnd,WM_DESTROY,0,0);
               break;
          case WM_DESTROY:
               EndDialog(hwnd,0);
               break;
    }
    return 0;
}

///**********************************************************************************


int result=-1;
char Hex[16]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};



char HexToBin(char len,char *str)
{
      char value=0;
      for(int n=len;n>2;n--)
           for(int i=0;i<16;i++)
           {
               if(Hex[i] == toupper(str[n-1]))
                {
                     value|=(i<<(len-n)*4);
 //                   printf("str(%c)<<%d ",str[n-1],(len-n)*4);
                }
           }
//   printf("val 0x%x\n",value);
       return value;
}

BOOL CALLBACK UserDataProc(HWND HwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{

    switch(msg)
    {
          case WM_INITDIALOG:
 /*              HWND hUpDown=CreateWindowEx(WS_EX_CLIENTEDGE, UPDOWN_CLASS, 0,
                                           UDS_ALIGNRIGHT|UDS_WRAP|UDSSETBUDYINT,
                                           112, 60, 160, 12,HwndDlg,(HMENU)IDC_UPDOWN,hInst,0)
   */        {
               SendDlgItemMessage(HwndDlg,IDC_MEM,CB_ADDSTRING,0,(LPARAM)"ROM");
               SendDlgItemMessage(HwndDlg,IDC_MEM,CB_ADDSTRING,0,(LPARAM)"RAM");
               SendDlgItemMessage(HwndDlg,IDC_MEM,CB_ADDSTRING,0,(LPARAM)"EEPROM");
               SendDlgItemMessage(HwndDlg,IDC_MEM,CB_SETCURSEL,1,0);
               SendDlgItemMessage(HwndDlg,IDC_ADDR,WM_SETTEXT,0,(LPARAM)"0x");
               SendDlgItemMessage(HwndDlg,IDC_DATA,WM_SETTEXT,0,(LPARAM)"0x");

               SendDlgItemMessage(HwndDlg,IDC_UPDOWN, UDM_SETRANGE,0,(LPARAM)MAKELONG(10,1));
               HWND hEdit=GetDlgItem(HwndDlg,IDC_BYTELEN);
               SendDlgItemMessage(HwndDlg,IDC_UPDOWN, UDM_SETBUDDY,(WPARAM)hEdit,0);
              }
               return FALSE;  /// focus ON
           case WM_COMMAND:
                if(HIWORD(wParam)==EN_CHANGE)
                {
                      TCHAR str[64];
                      if(LOWORD(wParam) == IDC_DATA)
                      {
                          SendDlgItemMessage(HwndDlg,IDC_DATA,EM_GETLINE,0,(LPARAM)str);
                          int i=SendDlgItemMessage(HwndDlg,IDC_LISTDATA,LB_GETCURSEL,0,0);
                          if(str[lstrlen(str)-1]==' ')
                          {
                              lstrcat(str,"0x");
                              int i=lstrlen(str);
                              SendDlgItemMessage(HwndDlg,IDC_DATA,WM_SETTEXT,0,(LPARAM)str);
                              SendDlgItemMessage(HwndDlg,IDC_DATA,EM_SETSEL,i,i);
                          }
                          SendDlgItemMessage(HwndDlg,IDC_LISTDATA,LB_DELETESTRING,i,0);
                          SendDlgItemMessage(HwndDlg,IDC_LISTDATA,LB_ADDSTRING,0,(LPARAM)str);

                      }
                 }
                 else if(LOWORD(wParam)== IDC_BTNWRITE)
                 {

                      BOOL err;
                      char str[64];
                      char bit= SendDlgItemMessage(HwndDlg,IDC_MEM,CB_GETCURSEL,0,0);
                      char mem;
                      if(bit && bit!= CB_ERR)mem=2<<bit; else mem=0;
                      UINT len=GetDlgItemText(HwndDlg,IDC_ADDR,str,5);
                      int addr=HexToBin(len,str);
                      len=GetDlgItemInt(HwndDlg,IDC_BYTELEN,&err,FALSE);

                       int i=SendDlgItemMessage(HwndDlg,IDC_LISTDATA,LB_GETCURSEL,0,0);
                       SendDlgItemMessage(HwndDlg,IDC_LISTDATA,LB_GETTEXT,i,(LPARAM)str);
                       char *ptr=str;
                    //   printf("data:");
                       for(i=0;i<len;i++)
                        {
                            str[i]=HexToBin(4,ptr);
                            ptr+=5;
                     //        printf("0x%x ",str[i]);
                        }
                     //    printf("\n");
                        char *frame=app->Link->BuildCmd(WRITE_MEM,mem,addr,len);
                        if(app->Link->SendCommand(frame))
                            app->Link->SendData(len,str);

                 }
                 else if(LOWORD(wParam)== IDC_BTNREAD)
                 {
                      BOOL err;
                      char str[64];
                      char bit= SendDlgItemMessage(HwndDlg,IDC_MEM,CB_GETCURSEL,0,0);
                      char mem;
                      if(bit && bit!= CB_ERR)mem=2<<bit; else mem=0;
                      UINT len=GetDlgItemText(HwndDlg,IDC_ADDR,str,5);
                      int addr=HexToBin(len,str);
                      len=GetDlgItemInt(HwndDlg,IDC_BYTELEN,&err,FALSE);

      //               printf("Read addr 0x%x len %d mem %d\n", addr, len, mem);

                       char *frame=app->Link->BuildCmd(READ_MEM,mem,addr,len);
                       app->Link->SendCommand(frame);

           /*
                       *str=0;
                      for(int i=0;i<len;i++)
                          sprintf(str+strlen(str),"0x%.2x ",*(app->Link->GetBuffer()+i)&0xff);
                       //printf("%s\n",str);
                       int i=SendDlgItemMessage(HwndDlg,IDC_LISTDATA,LB_ADDSTRING,0,(LPARAM)str);
                        SendDlgItemMessage(HwndDlg,IDC_LISTDATA,LB_SETSEL,TRUE,(LPARAM)i);
 */
                  }
                else if(LOWORD(wParam)==IDC_BTNOK)
                {

                   SendMessage(HwndDlg,WM_CLOSE,0,0);
                }
                else if(LOWORD(wParam)==IDC_BTNCANCEL)
                {
                    result = -1;
                   SendMessage(HwndDlg,WM_CLOSE,0,0);
               }
                break;

         case WM_NOTIFY:
              {
      /*          switch(wParam)
                {
                     case IDC_UPDOWN:
                          NM_UPDOWN *lpnmhdr = (NM_UPDOWN*)lParam;
                          if (lpnmhdr->hdr.code == UDN_DELTAPOS)
                          {}
                          break;
                }
*/
              }
                break;
           case WM_CLOSE:
             {

               EndDialog(HwndDlg,result);
             }
               break;
    }
   return 0;
}


Connection::Connection(HWND hwnd, std::string &port):Win32Port(port, CBR_9600, NOPARITY, 8, ONESTOPBIT, FALSE, FALSE),
                                                     OnLine(FALSE),
                                                     mode(0),
                                                     hWnd(hwnd)
{
  //  char buf[256];
  // FormatDebugOutput(buf,4);
  // printf(buf);
}
Connection::~Connection()
{

}
DWORD Connection::ReadDevice(int len)
{

}
BOOL Connection::SendCommand(char *comd)
{

     if(write_buffer(comd,5)!= RS232_SUCCESS)
     {
         DisplayLastError("WritePort");
         return FALSE;
     }

    return TRUE;
}

BOOL Connection::SendData(int len, char *data)
{
     if(write_buffer(data,len)!= RS232_SUCCESS)
    {
        printf("Write_Err\n");
        return FALSE;
    }
    return TRUE;
}


void Connection::RxNotify( int byte_count )
{
    if(byte_count>32)
    {
        printf("RX:byte_count == %d !!!\n",byte_count);
       byte_count=32;
    }
 //     printf("RX %d: ",byte_count);
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
    if(idx < (CmdMsg>>24)+1)return;

   // printf("idx %d %d\n",idx, (CmdMsg>>24)  );

    for(int i=0;i<idx-1;i++)
    {
        ChkSum += (FrameData[i]&0xff);
    }
    if(ChkSum != (FrameData[idx-1]&0xff))
    {
        printf("CheckSum Error\n");
     printf("len %d :",idx );
         for(int i=0;i<idx;i++)
        {
            printf("0x%x ", (FrameData[i]&0xff));
        }
        printf("ChkSum 0x%x\n",ChkSum);
   }
    if((CmdMsg&0xff)==0xaa)CmdMsg=0xaa;

/*

      SYSTEMTIME t;
     GetLocalTime(&t);
       printf("%d:%d\n",t.wSecond,t.wMilliseconds );
*/

       PostMessage(app->GetHandle(),EV_DEVICE_MSG,(WPARAM)CmdMsg,(LPARAM)FrameData);

     idx=0;
    CmdMsg=0;

}



void Connection::TxNotify( )
{
     DWORD event;
     char len = FrameData[3]&0xff;
     char *comd=&FrameData[0];
      printf("Tx: 0x%x 0x%x 0x%x 0x%x 0x%x\n",
             comd[0]&0xff, comd[1]&0xff, comd[2]&0xff, comd[3]&0xff, comd[4]&0xff);


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
{
   //   check_modem_status( false, MS_CTS_ON );
   return OnLine;
}
