#ifndef  _WIN32_IE
#define _WIN32_IE 0x0500
#endif

#include <windows.h>

#include "resource.h"
#include "App.h"
#include <commctrl.h>
#include <tchar.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "RingBuffer.h"

extern HINSTANCE hInst;
extern App *app;
CGraphPage *MonGraph = NULL;



extern void PrintDCB(DCB &m_Dcb);



CMonitorPage::CMonitorPage(CTabCtrl *Ctrl,char *nm):TabPage(Ctrl,nm,DLG_MONITOR,DialogProc,this)
{
  //   HINSTANCE hInst=reinterpret_cast<HINSTANCE>(GetWindowLong(GetHwnd(),GWL_HINSTANCE));
    hFrontBmp=LoadBitmap(hInst,MAKEINTRESOURCE(IDB_SYSTEM));
 }

CMonitorPage::~CMonitorPage()
{
     DeleteObject(hFrontBmp);

}
void CMonitorPage::Populate()
{

    char name[32];
    char addr[] = {6, 0x60, 0x1, 0x42, 0};

    sprintf(name, "%c%c%c %d.%d.%d", app->GetDeviceName()[0],
                                   app->GetDeviceName()[1],
                                   app->GetDeviceName()[2],
                                   app->GetDeviceName()[3],
                                   app->GetDeviceName()[5]>>4,
                                   app->GetDeviceName()[5]&0x0f);

    HWND hDev = GetDlgItem(hWnd, ID_DEV);
    SendMessage(hDev, WM_SETTEXT, 0, (LPARAM)name);

    DataIdx = app->Monitor(this, addr);

    printf("Dev name %s\n",name);
}


void CMonitorPage::Monitor(char *data)
{
      Translate(data+DataIdx,AD_INtoVolts,2,"V",ID_UIN);
      Translate(data+DataIdx+2,ADtoVolts,2,"V",ID_UOUT);
      Translate(data+DataIdx+4,ADtoAmps,2,"A",ID_IOUT);
      Translate(data+DataIdx+6,ADtoTemp,1,"ºC",ID_TEMP);
  /*     for(int i=0;i<6;i++)
          printf("0x%x ",data[DataIdx+i]&0xff);
       printf("\n");
*/
}

LRESULT  CMonitorPage::OnNotify(WPARAM wParam,LPARAM lParam)
{

   switch(((LPNMHDR) lParam)->code)
     {
         case TTN_NEEDTEXT:
             {
                 LPTOOLTIPTEXT lpttext;
                 lpttext=(LPTOOLTIPTEXT)lParam;
                 lpttext->hinst = reinterpret_cast<HINSTANCE>(GetWindowLong(GetHwnd(), GWL_HINSTANCE));
                  switch(lpttext->hdr.idFrom)
                 {
                       case ID_DEV:
                            lpttext->lpszText="New file";
                            break;
    /*                   case ID_FILE_OPEN:
                            lpttext->lpszText="Open file";
                            break;
                       case ID_FILE_SAVE:
                            lpttext->lpszText="Save file";
                            break;
                       case ID_DEVICE_INFO:
                            lpttext->lpszText="Get device Info";
                            break;
                       case ID_DEVICE_READ:
                            lpttext->lpszText="Read device";
                            break;
                       case ID_DEVICE_WRITE:
                            lpttext->lpszText="Write to device";
                            break;
                       case ID_HELP:
                            lpttext->lpszText="Help";
                            break;
      */           }
              }
             break;
     }
     return 0;
}
LRESULT  CMonitorPage::OnPaint(WPARAM wParam,LPARAM lParam)
{
    PAINTSTRUCT ps;
    BITMAP bm;
    HDC hdc=BeginPaint(hWnd,&ps);
    HDC hdcMem=CreateCompatibleDC(hdc);
    HBITMAP hbmOld=(HBITMAP)SelectObject(hdcMem,hFrontBmp);
    GetObject(hFrontBmp,sizeof(bm),&bm);
    BitBlt(hdc,5,5,bm.bmWidth,bm.bmHeight,hdcMem,0,0,SRCAND	);
    SelectObject(hdcMem,hbmOld);
    DeleteDC(hdcMem);
    EndPaint(hWnd ,&ps);
// printf("ps.rc %d, %d, %d, %d\n",  ps.rcPaint.left,ps.rcPaint.top, ps.rcPaint.right,ps.rcPaint.bottom);
//  printf("bmWidth %d bmHeight %d\n",bm.bmWidth,bm.bmHeight);
     return 0;
}

BOOL CALLBACK CMonitorPage::DialogProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static CMonitorPage *Obj=NULL;
     switch(uMsg)
    {
        case WM_INITDIALOG:
             Obj=(CMonitorPage*)lParam;
             return FALSE;
        case WM_PAINT:
             if(Obj)Obj->OnPaint(wParam,lParam);
             break;
    }

    return FALSE;
}

///**********************************************************



///**********************************************************************

CGraphPage::CGraphPage(CTabCtrl *Ctrl,char *name):TabPage(Ctrl,name,DLG_GRAPHICS, CGraphPage::DialogProc,this)
{
     RECT rect;
    GetClientRect(hWnd,&rect);
     SendMessage(hWnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT	), (LPARAM)true);

    h=150;
    rect.top=rect.top+15;
    rect.left+=50;rect.right-=50;
    rect.bottom=rect.top+h;
/*    UGraph=new CGraphic(hWnd,"Volts",ID_GRAPH_U, rect,RGB(128,0,255));

    rect.top=rect.top+rect.bottom+20;
     IGraph=new CGraphic(hWnd,"Amps",ID_GRAPH_I, rect, RGB(200,64,64));

*/
}

CGraphPage::~CGraphPage()
{
     delete UGraph;
      delete IGraph;
}


LRESULT CGraphPage::OnPaint(WPARAM wParam,LPARAM lParam)
{
    int           i;
    HDC           hdc, hMemDC;
    RECT          theRect, destRect;
    HBITMAP       theBitmap;
    PAINTSTRUCT   ps;
    BITMAP bm;

    hdc = BeginPaint(hWnd, &ps);

    UGraph->DrawScale(hdc,15);
     IGraph->DrawScale(hdc,15+h+15+20);


    EndPaint(hWnd, &ps);


    return FALSE;
}

void CGraphPage::Populate()
{
 /*    short min=0;
     short max = app->ReadDeviceData( NULL, 0x58, 2, "", NULL);
 //     IGraph->SetRange(min,(short)ADtoAmps(max)+1,ADtoAmps);
      IGraph->SetRange(min,max,ADtoAmps);


     min = app->ReadDeviceData( NULL, 0x50, 2, "", NULL);
     max = app->ReadDeviceData( NULL, 0x54, 2, "", NULL);
 //     UGraph->SetRange((short)ADtoVolts(min),(short)ADtoVolts(max)+1,ADtoVolts);
      UGraph->SetRange(min,max,ADtoVolts);

*/
  //   app->Monitor(this);
}

void CGraphPage::Monitor(char *data)
{
 //   short a=app->ReadDeviceData( NULL, 0x5c, 2, "",NULL);
// printf("%d ",((a>>8)&0xff)-(a&0xff));

}

void CGraphPage::SetData(short v, short i)
{

}



 BOOL CALLBACK CGraphPage::DialogProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
 {

     switch(msg)
    {
        case WM_INITDIALOG:

             MonGraph = (CGraphPage*)lParam;
             return TRUE;
        case WM_PAINT:
             MonGraph->OnPaint(wParam,lParam);
             break;
        case WM_NOTIFY:
              if(((LPNMHDR)lParam)->code == TTN_NEEDTEXT)
              {
                  LPTOOLTIPTEXT lpttext;
                  lpttext=(LPTOOLTIPTEXT)lParam;
                  RECT rect;
                  ULONG pos = LOWORD(GetMessagePos());
                  GetWindowRect((HWND)lpttext->hdr.idFrom, &rect);
                  pos -= rect.left+2;

                  if((HWND)lpttext->hdr.idFrom == GetDlgItem(hwnd,ID_GRAPH_U))
                  {
                      short p;//=MonGraph->UGraph->GetVal(pos);
                      float v=ADtoVolts(p);
                      if(p==-1)
                           sprintf(lpttext->szText," --- ");
                       else
                      {
                          sprintf(lpttext->szText,"%f",v);
                          char *ptr=lpttext->szText;
                          while(*ptr!='.')ptr++;
                         *(ptr+4)='V';*(ptr+5)='\0';
                      }
                  }
                  else if((HWND)lpttext->hdr.idFrom == GetDlgItem(hwnd,ID_GRAPH_I))
                  {
                      short p;//=MonGraph->IGraph->GetVal(pos);
                      float v=ADtoAmps(p);
                      if(p==-1)
                           sprintf(lpttext->szText," --- ");
                      else
                      {
                          sprintf(lpttext->szText,"%f",v);
                          char *ptr=lpttext->szText;
                          while(*ptr!='.')ptr++;
                          *(ptr+4 )='A';*(ptr+5)='\0';
                      }
                  }
               }

             break;
    }

    return FALSE;
 }


///*********************************************************




CBattPage::CBattPage(CTabCtrl *Ctrl,char *name):TabPage(Ctrl,name,DLG_BATTERY, CBattPage::DialogProc,this),
                                                nSerial(1),
                                                nParalel(1)
{
    hBattBmp=LoadBitmap(hInst,MAKEINTRESOURCE(IDB_BATTERY));
   if(!hBattBmp)printf("Error Loading BmpBattery\n");



  //  SendMessage(hNSerial, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)true);

  }

CBattPage::~CBattPage()
{
    DeleteObject(hBattBmp);
}


void CBattPage::Populate()
{
  //     char str[4];
  //  sprintf(str,"%s","--");
//      SendMessage(hNSerial,WM_SETTEXT,0,(LPARAM)str);

      app->ReadDeviceData( hWnd, 0x49, 2);
 /*
     float _U = ADtoVolts(ad)-10.5;
  // float _U= 11.0-10.5;
     ChargeState = (short)((float)(_U*100)/2.2);
     if(ChargeState > 100) ChargeState = 100;

     if(ad < 0x1b5)
        nSerial=1;
    else if(ad < 0x363)
        nSerial=2;
 */
      char data[]={1,0x42,4,0x78,0};
      DataIdx=app->Monitor(this,data);
}

void CBattPage::Monitor(char *data)
{

      Translate(data+DataIdx,ADtoTemp,1,"ºC",ID_TEMP);
  //    Translate(data+DataIdx+1,ADtoVolts,"V",ID_UOUT);
      Translate(data+DataIdx+1,NULL,4,"A/h",ID_AMP_H);

}

LRESULT  CBattPage::OnPaint(WPARAM wParam,LPARAM lParam)
{
       PAINTSTRUCT ps;
      BITMAP bm;
      int x1=150;
     HDC hdc=BeginPaint(hWnd,&ps);
     HDC hdcMem=CreateCompatibleDC(hdc);

     HBITMAP hbmOld=(HBITMAP)SelectObject(hdcMem,hBattBmp);
     GetObject(hBattBmp,sizeof(bm),&bm);
     for(int y=0; y<nParalel;y++)
     {
         for(int x=0;x<nSerial;x++)
        {
            BitBlt(hdc,x1-x*100, 80+y*80,bm.bmWidth,bm.bmHeight,hdcMem,0,0,SRCAND	);

            MoveToEx(hdc,x1-25-x*100+100, 80+15+y*80,NULL);
            LineTo(hdc,x1-25-x*100+140, 80+15+y*80);
            if(x == nSerial-1)
            {
                 MoveToEx(hdc,x1-25-x*100, 80+15+y*80,NULL);
                 LineTo(hdc,x1-25-x*100+40, 80+15+y*80);
            }
        }
        if(y==0) MoveToEx(hdc,x1-25-((nSerial-1)*100), 45+y*80,NULL);
        else MoveToEx(hdc,x1-25-((nSerial-1)*100), 15+y*80,NULL);
        LineTo(hdc,x1-25-((nSerial-1)*100), 80+15+y*80);

        MoveToEx(hdc,x1+115, 15+y*80,NULL);
        LineTo(hdc,x1+115, 80+15+y*80);

     }


       MoveToEx(hdc,x1-100, 15,NULL);
        LineTo(hdc,x1+115, 15);
       MoveToEx(hdc,x1-100, 45,NULL);

     HBRUSH br= CreateSolidBrush(RGB(250,0,0));
     HGDIOBJ obj=SelectObject(hdc,(HGDIOBJ)br);
     if(nSerial==1)LineTo(hdc,x1, 45);
     else LineTo(hdc,225, 45);
     SelectObject(hdc,obj);

     DeleteObject(br);
     DrawStatus(hdc,510,160);

     char str[64];
     sprintf(str,"%dV ",nSerial*12);
     short Amp_H;
     char *ptr=&str[lstrlen(str)];
     SendDlgItemMessage(hWnd, ID_AMP_H,WM_GETTEXT,64-lstrlen(str),(LPARAM)&ptr);
  //   sprintf(&ptr[lstrlen(ptr)],"A/h");
  //   TextOut(hdc,150,25,str,lstrlen(str));

     SelectObject(hdcMem,hbmOld);

     DeleteDC(hdcMem);
     EndPaint(hWnd ,&ps);


     return 0;
}

void  CBattPage::DrawStatus(HDC dc, int x, int y)
{
     char str[32];
      int x1,y1;
 //    x=540;y=100;
     Rectangle(dc,x,y-1,x+40,y+200);
     int i=0;
     for(y1=y;y1<y+200;y1+=20)
     {
              sprintf(str,"%d",100-i*10);
              TextOut(dc,x+70,y1-10,str,lstrlen(str));
             MoveToEx(dc,x+45,y1,NULL);
             LineTo(dc,x+55,y1);
             i++;
     }


     short pCent=(short)ChargeState;
     RECT rect;
     rect.left=x+2;
     rect.top= y+(100-pCent)*2;
     rect.right=x+38;
     rect.bottom=y+198;

     HBRUSH br= CreateSolidBrush(RGB(250,250,64));


     HGDIOBJ obj=SelectObject(dc, br);
     FillRect(dc,&rect,br);
     DeleteObject(SelectObject(dc,obj));

    sprintf(str,"%d%%",(short)ChargeState);
     SetTextColor(dc,RGB(0,0,250));
     SetBkMode(dc,TRANSPARENT);
     TextOut(dc,x+3,y+160,str,lstrlen(str));

}


BOOL CALLBACK CBattPage::DialogProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static CBattPage *Obj=NULL;
     switch(uMsg)
    {
        case WM_INITDIALOG:
             Obj=(CBattPage*)lParam;
             return FALSE;
        case WM_PAINT:
             Obj->OnPaint(wParam,lParam);
             return FALSE;
        case EV_DATA_REQUEST:
           {
               char str[32];
               short AD;
               double v;
               if(wParam == 0x2490405)
               {
                  AD=*(char*)lParam;
                  sprintf(str,"%d A/h",AD);
                  SendDlgItemMessage(Obj->hWnd,ID_CAPACITY,WM_SETTEXT,0,(LPARAM)str);

                   AD=*(char*)(lParam+1);
                  sprintf(str,"%d V",AD);
                  SendDlgItemMessage(Obj->hWnd,ID_UOUT,WM_SETTEXT,0,(LPARAM)str);

              }
           }
               return FALSE;
    }

    return FALSE;
}


///***************************************************



CPainelPage::CPainelPage(CTabCtrl *Ctrl,char *name):TabPage(Ctrl,name,DLG_PAINEL, CPainelPage::DialogProc,this)
{
     hPainelBmp = LoadBitmap(hInst,MAKEINTRESOURCE(IDB_PAINEL));
   if(!hPainelBmp)printf("Error Loading BmpPainel\n");
/*    hPainelHwnd=CreateWindowEx(WS_EX_STATICEDGE,"STATIC","#Painel",
                      WS_CHILD | WS_VISIBLE  | SS_CENTER | SS_CENTERIMAGE,  // control style
                      40,  40, 100, 20, hWnd,
                      reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_NPAINEL)), hInst,  NULL );
     hWatsHwnd=CreateWindowEx(WS_EX_STATICEDGE,"STATIC","Wats",
                      WS_CHILD | WS_VISIBLE  | SS_CENTER | SS_CENTERIMAGE,  // control style
                      30,  70, 100, 20, hWnd,
                      reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_PWAT)), hInst,  NULL );
     hVoltHwnd=CreateWindowEx(WS_EX_STATICEDGE,"STATIC","Volts",
                      WS_CHILD | WS_VISIBLE  | SS_CENTER | SS_CENTERIMAGE,  // control style
                      130,  100, 100, 20, hWnd,
                      reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_PVOUT)), hInst,  NULL );
     hAreaHwnd=CreateWindowEx(WS_EX_STATICEDGE,"STATIC","Area",
                      WS_CHILD | WS_VISIBLE  | SS_CENTER | SS_CENTERIMAGE,  // control style
                      100,  130, 100, 20, hWnd,
                      reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_AREA)), hInst,  NULL );

    SendMessage(hPainelHwnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)true);
   	SendMessage(hWatsHwnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)true);
   	SendMessage(hVoltHwnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)true);
   	SendMessage(hAreaHwnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)true);
*/}

CPainelPage::~CPainelPage()
{
     DeleteObject(hPainelBmp);
}


void CPainelPage::Populate()
{
 /*   char str[4];
    sprintf(str,"%d",nPainels);
     SendMessage(hPainelHwnd,WM_SETTEXT,0,(LPARAM)str);
   sprintf(str,"%d",5);
     SendMessage(hWatsHwnd,WM_SETTEXT,0,(LPARAM)str);
   sprintf(str,"%d",15);
     SendMessage(hVoltHwnd,WM_SETTEXT,0,(LPARAM)str);
   sprintf(str,"%d",20);
     SendMessage(hAreaHwnd,WM_SETTEXT,0,(LPARAM)str);
*/
}

LRESULT  CPainelPage::OnPaint(WPARAM wParam,LPARAM lParam)
{
       PAINTSTRUCT ps;
      BITMAP bm;
     HDC hdc=BeginPaint(hWnd,&ps);
     HDC hdcMem=CreateCompatibleDC(hdc);
     HBITMAP hbmOld=(HBITMAP)SelectObject(hdcMem,hPainelBmp);
     GetObject(hPainelBmp,sizeof(bm),&bm);
     BitBlt(hdc,100, 180,bm.bmWidth,bm.bmHeight,hdcMem,0,0,SRCAND	);
     SelectObject(hdcMem,hbmOld);
     DeleteDC(hdcMem);
     EndPaint(hWnd ,&ps);


     return 0;
}


BOOL CALLBACK CPainelPage::DialogProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static CPainelPage *Obj=NULL;
     switch(uMsg)
    {
        case WM_INITDIALOG:
             Obj=(CPainelPage*)lParam;
             return FALSE;
        case WM_PAINT:
             Obj->OnPaint(wParam,lParam);
             return FALSE;

    }

    return FALSE;
}



///*******************************************************************




CPortPage::CPortPage(CTabCtrl *Ctrl,char *nm):TabPage(Ctrl,nm,DLG_COMPORT, CPortPage::DialogProc,this)
{

}

CPortPage::~CPortPage()
{

}


void CPortPage::Populate()
{
       Connection *Link=app->GetConnection();
      if(Link==NULL)return;

      const char *nm = Link->GetPortName();

      SendMessage(GetDlgItem(hWnd,ID_PORTNUM),WM_SETTEXT,0,(WPARAM)nm);

      DCB dcb;
      dcb.DCBlength = sizeof(DCB);
      GetCommState(Link->GetPort(),&dcb);

      COMMTIMEOUTS TimeOut;
      GetCommTimeouts(Link->GetPort(),&TimeOut);

      char str[32];
    sprintf(str,"%d bps",dcb.BaudRate);
    SendMessage(GetDlgItem(hWnd,ID_BRATE), WM_SETTEXT, 0, (LPARAM)str);
     sprintf(str,"%d",dcb.ByteSize);
  	SendMessage(GetDlgItem(hWnd,ID_BSIZE), WM_SETTEXT, 0, (LPARAM)str);
     sprintf(str,"%s",dcb.StopBits == ONESTOPBIT?"1":
                      dcb.StopBits == ONE5STOPBITS?"1.5":"2");

  	SendMessage(GetDlgItem(hWnd,ID_SBIT), WM_SETTEXT, 0, (LPARAM)str);
     sprintf(str,"%s",dcb.Parity == EVENPARITY?"EVENPARITY":
                      dcb.Parity == ODDPARITY? "ODDPARITY":
                      dcb.Parity == MARKPARITY? "MARCPARITY":"NOPARITY");
  	SendMessage(GetDlgItem(hWnd,ID_PAR), WM_SETTEXT, 0, (LPARAM)str);

     sprintf(str,"%d ms",TimeOut.ReadIntervalTimeout);
   	SendMessage(GetDlgItem(hWnd,ID_RINT), WM_SETTEXT, 0, (LPARAM)str);

    sprintf(str,"%d ms",TimeOut.ReadTotalTimeoutMultiplier);
   	SendMessage(GetDlgItem(hWnd,ID_RMULT), WM_SETTEXT, 0, (LPARAM)str);

     sprintf(str,"%d ms",TimeOut.ReadTotalTimeoutConstant);
  	SendMessage(GetDlgItem(hWnd,ID_RCONST), WM_SETTEXT, 0, (LPARAM)str);

    sprintf(str,"%d ms",TimeOut.WriteTotalTimeoutMultiplier);
   	SendMessage(GetDlgItem(hWnd,ID_WMULT), WM_SETTEXT, 0, (LPARAM)str);

    sprintf(str,"%d ms",TimeOut.WriteTotalTimeoutConstant);
   	SendMessage(GetDlgItem(hWnd,ID_WCONST), WM_SETTEXT, 0, (LPARAM)str);

}


BOOL CALLBACK CPortPage::DialogProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static CPortPage *Obj=NULL;
     switch(uMsg)
    {
        case WM_INITDIALOG:
             Obj=(CPortPage*)lParam;
             return FALSE;
        case WM_COMMAND:
             if(LOWORD(wParam) == ID_BTN_RTS )
             {
                  Obj->OnRTSButton();
             }
             break;
    }

    return FALSE;
}


///*******************************************************************



void CPortPage::OnRTSButton()
{
    int rts=app->Link->Rts();
    if(rts < RS232_SUCCESS)
    {
        printf("Error Rts()\n");
        return;
    }

    if(rts)rts=0;
    else rts=1;

    app->Link->Rts(rts);


}


///**************************************************************************



CConfigPage::CConfigPage(CTabCtrl *Ctrl,char *nm):TabPage(Ctrl,nm,DLG_CONFIG, CConfigPage::DialogProc,this)
{


}

CConfigPage::~CConfigPage()
{

}


void CConfigPage::Populate()
{
     char str[64];

 /*   HDC dc=GetDC(hWnd);
    ModeVoltage=12;

     SetTextColor(dc,RGB(10,10,10));
    sprintf(str,"Parametros para o modo de %d Volts:",ModeVoltage);
    TextOut(dc,20,20,str,lstrlen(str));
    //  SendDlgItemMessage(hWnd,ID_UMODE,WM_SETTEXT,0,(LPARAM)str);
*/

     app->ReadDeviceData( hWnd,0x50, 12);

//    ReleaseDC(hWnd,dc);
 }


BOOL CALLBACK CConfigPage::DialogProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static CConfigPage *Obj=NULL;
     switch(uMsg)
    {
        case WM_INITDIALOG:
             Obj=(CConfigPage*)lParam;
         //    Obj->Hwnd=hwnd;
             return FALSE;

        case EV_DATA_REQUEST:
           {
               char str[32];
               short AD;
               double v;
               switch(wParam)
               {
                   case 0x0c500405:
                      {
                        AD=*(short*)lParam;
                        v=ADtoVolts(AD);
                        sprintf(str,"%f",v);
                        SendDlgItemMessage(Obj->hWnd,ID_VOFF,WM_SETTEXT,0,(LPARAM)str);

                        AD=*(short*)(lParam+2);
                        v=ADtoVolts(AD);
                        sprintf(str,"%f",v);
                        SendDlgItemMessage(Obj->hWnd,ID_VFLOAT,WM_SETTEXT,0,(LPARAM)str);

                        AD=*(short*)(lParam+4);
                        v=ADtoVolts(AD);
                        sprintf(str,"%f",v);
                        SendDlgItemMessage(Obj->hWnd,ID_VOCT,WM_SETTEXT,0,(LPARAM)str);

                        AD=*(short*)(lParam+6);
                        v=ADtoAmps(AD);
                        sprintf(str,"%f",v);
                        SendDlgItemMessage(Obj->hWnd,ID_ITRIC,WM_SETTEXT,0,(LPARAM)str);

                        AD=*(short*)(lParam+8);
                        v=ADtoAmps(AD);
                        sprintf(str,"%f",v);
                        SendDlgItemMessage(Obj->hWnd,ID_IBLK,WM_SETTEXT,0,(LPARAM)str);

                        AD=*(short*)(lParam+10);
                        v=ADtoAmps(AD);
                        sprintf(str,"%f",v);
                        SendDlgItemMessage(Obj->hWnd,ID_IOCT,WM_SETTEXT,0,(LPARAM)str);
                        break;
                         /// Tensão de disparo 12,6v
                      }

               }

               break;
           }
    }

    return FALSE;
}
