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
#include <commdlg.h>
#include <setupapi.h>
#include <dbt.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <cderr.h>

#include <string>
#include "Win32Port.h"

#include "resource.h"
#include "App.h"




#define PCL    0x02
#define PCLATH 0x0a
#define CCPR1L 0x15
#define CCP1CON 0x17

#define WAIT_T 100


using namespace std;

HINSTANCE hInst;
App *app = NULL;

//Connection *Link;

static const GUID GUID_DEVICEINTERFACE_LIST[] =
{
	// GUID_DEVINTERFACE_COMPORT
	{ 0x86e0d1e0, 0x8089, 0x11d0, { 0x9c, 0xe4, 0x08, 0x00, 0x3e, 0x30, 0x1f, 0x73 } }
};


BOOL CALLBACK MainProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    switch(uMsg)
    {
        case WM_INITDIALOG:
             app = new App(hwndDlg);
             return TRUE;

        case WM_COMMAND:
             app->OnCommand(wParam, lParam);
             return FALSE;

        case WM_NOTIFY:
             app->OnNotify(wParam, lParam);
             break;
  //      case WM_TIMER:
  //           app->OnTimer();
             return 0;

        case EV_APP_INIT:
             app->OnInit(wParam, lParam);
             break;

         case EV_DEVICE_MSG:
             if(app)app->OnDevMsg(wParam, lParam);
             break;

         case EV_DATA_REQUEST:
             if(app)app->OnDataRequest(wParam, lParam);
             break;

         case WM_DEVICECHANGE:
             if(app) app->OnDeviceChange(wParam, lParam);
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
     RegisterMyClass();
    StartCommonControls(ICC_WIN95_CLASSES);
    HWND hwnd=CreateDialog(hInstance, MAKEINTRESOURCE(DLG_MAIN), NULL, (DLGPROC)MainProc);


    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0)){
		if (!::IsDialogMessage (hwnd, &msg))
        {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
        }
    }
    return 0;

}



App::App(HWND hwnd):hwnd(hwnd),
                    mPageIndex(0),
                    ListSz(0),
                    ReqIdx(0),
                    DataRefreshRate(1000),
                    Initialized(false),
                    GraphMon(NULL)
{

    CreateMainMenu();
    RegisterNotificationMsg();

    std::string portList;
    DWORD numPort = GetAvailablePorts(&portList);

    if(numPort)
        Link = new Connection(hwnd, portList);    /// Open last serial com port

    TabCtrl = new  CTabCtrl(hwnd);
    TabCtrl->Insert(new CMonitorPage(TabCtrl, _T("Monitor")));
    TabCtrl->Insert(new CBattPage(TabCtrl, _T("Grupo de baterias")));
    TabCtrl->Insert(new CPainelPage(TabCtrl, _T("Paineis solares")));
    TabCtrl->Insert(new CPortPage(TabCtrl, _T("Comunicações")));
    TabCtrl->Insert(new CConfigPage(TabCtrl, _T("Configuração")));

    if(Link && Link->IsValid())
    {
        char *frame=Link->BuildCmd(READ_MEM, EEPROM, 0x00, 6);
        PostMessage(hwnd, EV_DATA_REQUEST, 0, (LPARAM)frame);
    }
    else printf("Falha ao abrir a porta %s\n.", portList.c_str());

}

App::~App()
{
    if(Link)delete Link;
    delete TabCtrl;
    UnregisterDeviceNotification(hDevNotify);

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

 	if ( DBT_DEVICEARRIVAL == wParam || DBT_DEVICEQUERYREMOVE == wParam || DBT_DEVICEREMOVECOMPLETE == wParam ) {
		PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lParam;
		PDEV_BROADCAST_PORT pDevPort;
		switch( pHdr->dbch_devicetype )
		{
 			case DBT_DEVTYP_PORT:
				pDevPort = (PDEV_BROADCAST_PORT)pHdr;
				if ( DBT_DEVICEARRIVAL == wParam )
				{
				    if(Link) delete Link;
	//			    printf("BT_DEVICEARRIVAL DBT_DEVTYP_PORT\n");

				    string pname(pDevPort->dbcp_name);

                      Link = new Connection(hwnd, pname);

                        char *frame=Link->BuildCmd(READ_MEM, EEPROM, 0x00, 6);
                         Link->SendCommand(frame);


                }
                else if ( DBT_DEVICEQUERYREMOVE == wParam)
                {


                }
                else if ( DBT_DEVICEREMOVECOMPLETE == wParam)
                {
                    delete Link;
                    Link = NULL;

                }
				break;
         }
    }
     return TRUE;
}

LRESULT App::TimeOver()
{
    char msg[256]={0};
    if(Link->GetPort() == INVALID_HANDLE_VALUE)
        sprintf(msg, "Falhou a ligação com a porta %s.\n",Link->GetPortName());

    else sprintf(msg, "O equipamento está offline ou um cabo poderá estar desligado.\nVerifique todas as ligações.\n");

    sprintf(msg+lstrlen(msg), "A aplicação iniciará em modo offline.");

     MessageBox( hwnd, msg, "Aviso", MB_OK|MB_ICONWARNING);
    return FALSE;
}

LRESULT App::OnCommand(WPARAM wParam,LPARAM lParam)
{
     switch(LOWORD(wParam))
    {
           case ID_DEVICE_CHARGE_ON:
             {
                char *frame=Link->BuildCmd(READ_MEM,RAM,0x45,1);
               PostMessage(hwnd,EV_DATA_REQUEST,0,(LPARAM)frame);
                char flags =(~AFlags)&0x04;       /// bit 2  0 -> CHARGE_OFF  1 -> CHARGE_ON
                flags = flags |(AFlags&0xfb);

               frame=Link->BuildCmd(WRITE_MEM,RAM,0x45,1);
                Link->SendCommand(frame);
                Link->SendData(1,&flags);
printf("SetFlag %x ",(flags%0x04));

               if((flags&0x04)== 0)
                {
                    char data;    /// clear CCPR1L

                    frame=Link->BuildCmd(WRITE_MEM,RAM,CCPR1L,1);
                    Link->SendCommand(frame);
                    data=0;
                     Link->SendData(1,&data);

                    frame=Link->BuildCmd(READ_MEM,RAM,CCP1CON,1);
                    Link->SendCommand(frame);     /// Read CCP1CON
//                     Link->ReadDevice(CmdLen);
                     data = *Link->GetBuffer()&0x0f;

                    frame=Link->BuildCmd(WRITE_MEM,RAM,CCP1CON,1);
                    Link->SendCommand(frame);     /// clear CCPiCON(HI nibble)
                     Link->SendData(1,&data);

                }

                frame=Link->BuildCmd(READ_MEM,RAM,0x45,1);
                if(Link->SendCommand(frame))
                {
 //                 Link->ReadDevice(CmdLen);
                  AFlags = *Link->GetBuffer();
                }
                if(AFlags&0x04)
                   CheckMenuItem(GetMenu(hwnd), ID_DEVICE_CHARGE_ON,MF_CHECKED	);
                else if((AFlags&0x04) == 0)CheckMenuItem(GetMenu(hwnd), ID_DEVICE_CHARGE_ON,MF_UNCHECKED	);


            }
                break;
          case ID_SLOW:
             {
                char data=1;
               char *frame=Link->BuildCmd(WRITE_MEM,RAM,0x40,1); /// set SLOW_CHARGE
               Link->SendCommand(frame);
               Link->SendData(1,&data);

             }
               break;
          case ID_FAST:
             {
                  char data=2;
               char *frame=Link->BuildCmd(WRITE_MEM,RAM,0x40,1); /// set FAST_CHARGE
               Link->SendCommand(frame);
               Link->SendData(1,&data);
              }
               break;
          case ID_EQUALIZE:
             {
                char data=3;
                   char *frame=Link->BuildCmd(WRITE_MEM,RAM,0x40,1); /// set EQUALIZE
                    Link->SendCommand(frame);
                    Link->SendData(1,&data);
              //       AddDuty(50);

                   ///
                  ///  set I_oct 0x10 -> 240ma
                  ///            0x08 -> 120ma
                ///
                    data=4;
                    frame=Link->BuildCmd(WRITE_MEM,RAM,0x5A,1); /// set I_oct (low byte)
                    Link->SendCommand(frame);
                    Link->SendData(1,&data);

                  ///      Medir o tempo de resposta Amp/H


             }
               break;
          case ID_FLOTING:
             {
                  char data=4;
                  char *frame=Link->BuildCmd(WRITE_MEM,RAM,0x40,1); /// set FLOTING
///                   printf("frame 0x%x 0x%x 0x%x 0x%x 0x%x\n",FrameData [0],FrameData [1],FrameData [2],FrameData [3],FrameData [4]);
                    Link->SendCommand(frame);
                     Link->SendData(1,&data);
              }
               break;
          case ID_DEVICE_RESET:
             {
               char data=0;
               char *frame=Link->BuildCmd(WRITE_MEM,RAM,PCLATH,1);
               Link->SendCommand(frame);
               Link->SendData(1,&data);

               frame=Link->BuildCmd(WRITE_MEM,RAM,PCL,1);
               Link->SendCommand(frame);
               Link->SendData(1,&data);
            }
               break;
          case ID_DEVICE_MEM:
             {
               int userData=DialogBox(hInst,MAKEINTRESOURCE(USERDATA_DLG),hwnd,UserDataProc);

             }
               break;

          case ID_DEVICE_GRAPHIC:
               DialogBox(hInst,MAKEINTRESOURCE(DLG_CREATEGRAPH),hwnd,BuildGraph);

               break;

     }
    return 0;
}

LRESULT App::OnDataRequest(WPARAM wParam, LPARAM lParam)
{
    char *frame = (char*)lParam;
    Link->SendCommand(frame);
    return TRUE;
}

LRESULT App::OnDevMsg(WPARAM wParam, LPARAM lParam)
{

    switch(wParam)
    {
         case 0x6000805:   /// GetDeviceName
         {
              CopyMemory(DeviceName,(const void*)lParam,6);
              if(memcmp((const void*)DeviceName,"AZA",3))
                   return 0;

              Initialized = true;

              TabCtrl->Populate();

              PostMessage(hwnd,EV_APP_INIT,0,0);

              return 1;
         }
         case 0x01450405:   /// GetFlags
         {
              AFlags = *(char*)lParam;
            //        char flags =(~AFlags)&0x04;       /// bit 2  0 -> CHARGE_OFF  1 -> CHARGE_ON
            //        flags = flags |(AFlags&0xfb);
              return 1;
         }
         case  0xaa:        /// Monitor
              Refresh(((char*)lParam+1));
              break;
         default:          /// Request memory access
         {
              char str[128];
              char tmp[32];
              str[0] = '\0';
              int n = wParam >> 24;
        //             printf("msg 0x%x : ",wParam);
              if(((wParam>>8)&0xf0) == READ_MEM)
              {
                   char *ptr=(char*)lParam;
                   for(int i=0;i<n;i++)
                   {
                       sprintf(str+i*2,"0x%.2x ",(*(ptr+i))&0xff);
  //                          printf("%s ",str);
                   }


                   HWND Hwnd = GetActiveWindow();
                   if(Hwnd != hwnd)  /// Request from Memory access dialog
                   {
                        int i;
                        i=SendDlgItemMessage(Hwnd,IDC_LISTDATA,LB_GETCURSEL,0,0);
                        SendDlgItemMessage(Hwnd,IDC_LISTDATA,LB_SETSEL,FALSE,i);
                        i=SendDlgItemMessage(Hwnd,IDC_LISTDATA,LB_ADDSTRING,0,(LPARAM)str);
                        SendDlgItemMessage(Hwnd,IDC_LISTDATA,LB_SETSEL,TRUE,(LPARAM)i);
                   }
                   else             /// Request from App::ReadDeviceData
                   {
                        SendMessage(DataRequest[ReqIdx]->hwnd, EV_DATA_REQUEST,wParam,lParam);
                        delete DataRequest[ReqIdx];
                        PostMessage(hwnd,EV_APP_INIT,0,0);
                   }
              }
         }
         break;
     }
     return 0;
}


LRESULT App::OnInit(WPARAM wParam, LPARAM lParam)
{
    if(ReqIdx>0)
    {
        ReqIdx--;
        char *frame=Link->BuildCmd( READ_MEM, RAM, DataRequest[ReqIdx]->addr, DataRequest[ReqIdx]->len);
        Link->SendCommand(frame);
    }
    else
    {
        char *frame=Link->BuildCmd( WRITE_MEM, RAM, 0xd9, ListSz + 1);
        Link->SendCommand(frame);
        Link->SendData(ListSz + 1, MonList);
        Link->Rts(ENABLE);
    }
    return 0;
}

/**
    This metode build a array with Size, Addr
    to read data once
    Add a TDataRequest structure to DataRequest list
*/ 

short App::ReadDeviceData(HWND hwnd,  char addr, char sz)
{
 /*   if (Link->IsConnected() == FALSE)
    {
        MessageBox(App::hwnd, "O equipamento está OffLine", "Erro", MB_OK|MB_ICONEXCLAMATION);
        return 0;
    }
*/
    if (ReqIdx > 60)
    {
        printf("!!!!ReqIdx exced 60\n");
        return 0;
    }

    DataRequest[ReqIdx++] = new TDataRequest(hwnd, addr, sz);

    return 1;
}
/**
      This metode receive data at Data Refresh Rate
*/
DWORD App::AddDataMonitor(TabPage *page, char *listInfo)
{
    int i = GetMonLen();

    MonPage[mPageIndex++] = page;
    while(*listInfo)
    {
        if(ListSz == 63)
        {
            printf("MonList is full\n");
            break;
        }

        MonList[++ListSz] = *listInfo++;
    }
    MonList[0] = ListSz >> 1;
    return i;
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


void App::SetMonitor(HWND hnd)
{

    GraphMon = hnd;
}

void App::Refresh(char *data)
{
    for(int n = 0; n < mPageIndex; n++)
       MonPage[n]->Monitor(data);

    if(GraphMon)
       SendMessage(GraphMon, EV_DATA_REQUEST, (WPARAM)MonList, (LPARAM)data);
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
	AppendMenu(hSubMenu,MF_SEPARATOR,0,0);
	AppendMenu(hSubMenu, MF_STRING, ID_DEVICE_MEM, _T("&Acesso à memoria"));
	AppendMenu(hSubMenu,MF_SEPARATOR,0,0);
	AppendMenu(hSubMenu, MF_STRING, ID_DEVICE_GRAPHIC, _T("&Gráficos"));
	AppendMenu(hSubMenu,MF_SEPARATOR,0,0);
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


// ==============================================================
//				enumerate available ports
// ==============================================================
DWORD App::GetAvailablePorts ( std::string* portList )
{
	//HKEY_LOCAL_MACHINE\HARDWARE\DEVICEMAP\SERIALCOMM
	HKEY hKey = NULL;

	if ( RegOpenKey( HKEY_LOCAL_MACHINE, _T("Hardware\\DeviceMap\\SerialComm"), &hKey ) != ERROR_SUCCESS )
	{
		return 0;
	}

	DWORD    cValues;              // number of values for key
    DWORD    cchMaxValue;          // longest value name
    DWORD    cbMaxValueData;       // longest value data

	BYTE  bData [100];
	DWORD dwDataLen = 100;

	DWORD i, retCode, iFindedPorts=0;

	TCHAR  achValue[255];
	DWORD cchValue = 255; //16300;  //16383

    // Get the class name and the value count.
    retCode = RegQueryInfoKey( hKey, NULL, NULL, NULL, NULL, NULL, NULL,
								&cValues, &cchMaxValue, &cbMaxValueData, NULL, NULL);

    // enumerate all keys.
    if (cValues)
    {
		DWORD dwType;
		for (i=0, retCode=ERROR_SUCCESS; i<cValues; i++)
        {
            cchValue = 255;
            achValue[0] = '\0';
			bData [0] = '\0';
			dwDataLen = 100;
            retCode = RegEnumValue(hKey, i, achValue, &cchValue, NULL, &dwType, bData, &dwDataLen );

            if (retCode == ERROR_SUCCESS )
            {
				portList ->append((const char*) bData );
				iFindedPorts++;
            }
        }
    }
    if (portList->compare( 0, 3, "COM") == 0)
	    return iFindedPorts;
    else return 0;
}

double AD_INtoVolts(short data)
{      /// R = (R1+R2) / R2 = 7.9117
       double res = (double)((double)5 * (double)data / 1023) * 7.82;///6.53;
          return res;///data*0.038;
}


double ADtoVolts(short data)
{      /// R = (R1+R2) / R2 = 6.7353
       double res = (double)((double)5 * (double)data / 1023) * 6.68;///6.53;
          return res;///data*0.0328;
}

double ADtoAmps(short AD)
{    ///  float I = ((float)5/1023)*(float)AD/0.33;
     double I =     (((double).004887) * (double)AD) / 0.20;//0.33;
       return I;///AD* .023;
}

double ADtoTemp(short AD)
{
  //   double T = (((double).004887) * (double)AD) ;
     //  double T = (double)((double)5 * (double)AD / 1023) * 6.7353;///6.7318325;
     AD=AD&0x0ff;
    double T = AD*0.160;

   return  T;
}

short VoltsToAD(double V)
{
      short v= (V*1023/6.7353)/5;
     return v;
}
short AmpsToAD(double I)
{    /// 5/1023= .004887
    return (.20* I / .004887);
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

}

void StartCommonControls(DWORD flags)
{
    INITCOMMONCONTROLSEX iccx;
    iccx.dwSize=sizeof(INITCOMMONCONTROLSEX);
    iccx.dwICC=flags;
    InitCommonControlsEx(&iccx);
}

void RegisterMyClass()
{
    WNDCLASSEX wcx={0};  //used for storing information about the wnd 'class'

    wcx.cbSize         = sizeof(WNDCLASSEX);
    wcx.style          = CS_DBLCLKS;
    wcx.lpfnWndProc    = CGraphic::GraphProc;             //wnd Procedure pointer
    wcx.hInstance      = hInst;               //app instance
    wcx.hIcon         = NULL; //reinterpret_cast<HICON>(LoadImage(0,IDI_APPLICATION,    IMAGE_ICON,0,0,LR_SHARED));
    wcx.hCursor       = NULL;//reinterpret_cast<HCURSOR>(LoadImage(0,IDC_ARROW,    IMAGE_CURSOR,0,0,LR_SHARED));
    wcx.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BACKGROUND);
    wcx.lpszClassName = "GraphClass";

    if (!RegisterClassEx(&wcx))
    {
       printf("Failed to register wnd class");

    }
}

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

char *VarsName[8]={"Iout","Temp","Uin","Uout"};
char  VarsAddr[8]={0x64,0x42,0x60,0x62};

BOOL CALLBACK BuildGraph(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
          case WM_INITDIALOG:
               for(int i=0;i<4;i++)
               {
                   SendDlgItemMessage(hwnd,ID_VARS,LB_ADDSTRING,0,(LPARAM)VarsName[i]);
               }
                break;
          case WM_COMMAND:
               switch(LOWORD(wParam))
               {
                   case ID_OPEN:
                      {

                               char   szFileName[MAX_PATH]={""};
                               char fileTitle[MAX_PATH];

                              	OPENFILENAME ofn;
                            	ZeroMemory(&ofn, sizeof(ofn));

                            	ofn.lStructSize = sizeof(ofn);
                            	ofn.hwndOwner = hwnd;
                            	ofn.lpstrFilter = "Data Files (*.dat)\0*.dat\0\0";
                            	ofn.lpstrFile = szFileName;
                            	ofn.nMaxFile = MAX_PATH;
                            // 	ofn.lpstrInitialDir= curr_dir;
                            	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
                            	ofn.lpstrDefExt = "dat";

                            	if(GetOpenFileName(&ofn))
                            	{
                            	    GetFileTitle(szFileName,fileTitle,MAX_PATH);

                            	    SendDlgItemMessage(hwnd,ID_SHOWVARS,LB_RESETCONTENT,0,0);
                                    SendDlgItemMessage(hwnd,ID_SHOWVARS,LB_ADDSTRING,0,(LPARAM)fileTitle);
                                }


                      }
                      break;
                   case ID_SELECT:
                      {
                        int n = SendDlgItemMessage(hwnd,ID_VARS,LB_GETCURSEL,0,0);
                        int i = SendDlgItemMessage(hwnd,ID_SHOWVARS,LB_ADDSTRING,0,(LPARAM)VarsName[n]);
                        SendDlgItemMessage(hwnd,ID_SHOWVARS,LB_SETITEMDATA,i,(LPARAM)VarsAddr[n]);

                   }
                        break;
                   case ID_DELETE:
                      {
                        int n=SendDlgItemMessage(hwnd,ID_SHOWVARS,LB_GETCOUNT,0,0);
                        SendDlgItemMessage(hwnd,ID_SHOWVARS,LB_DELETESTRING,(WPARAM)n,0);
                      }
                        break;
                   case ID_OK:
                      {
                        char buf[32];
                        int n=SendDlgItemMessage(hwnd,ID_SHOWVARS,LB_GETCOUNT,0,0);
                        GData **GList = new  GData*[5];
                        for(int i=0;i<n;i++)
                        {
                            GList[i]=new GData;
                            SendDlgItemMessage(hwnd,ID_SHOWVARS,LB_GETTEXT,(WPARAM)i,(LPARAM)GList[i]->name);
                            GList[i]->Addr=SendDlgItemMessage(hwnd,ID_SHOWVARS,LB_GETITEMDATA,(WPARAM)i,0);

                        }
                        GList[n]= NULL;
                        CreateDialogParam(hInst,MAKEINTRESOURCE(DLG_GRAPHICS),GetParent(hwnd), (DLGPROC)WinGraphProc,(LPARAM)GList);
                       }
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

