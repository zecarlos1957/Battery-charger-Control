
#ifndef _APP_H
#define _APP_H

#ifndef  _WIN32_IE
#define _WIN32_IE 0x0500
#endif

#ifndef  WINVER
#define WINVER 0x0500
#endif

#include <windows.h>
#include <stdio.h>

#include "resource.h"
#include "RingBuffer.hpp"


#include <string>
#include "Win32Port.h"


#define MAX_TABPAGE 6
#define MAX_DUTY    1


 enum FlowControl
  {
    NoFlowControl,
    CtsRtsFlowControl,
    CtsDtrFlowControl,
    DsrRtsFlowControl,
    DsrDtrFlowControl,
    XonXoffFlowControl
  };




double AD_INtoVolts(short data);
double ADtoVolts(short data);
double ADtoAmps(short AD);
double ADtoTemp(short AD);
short VoltsToAD(double V);
short AmpsToAD(double I);
short VoltsToPWM(double Rms, short Uin);






typedef double (*AD_Value)(short );
typedef short (*Value_AD)(double);
typedef short (*Bin_Dec)(short);

void DisplayLastError(char * Operation,DWORD error=0);
VOID CALLBACK  OnMonitor( HWND hwnd, UINT uMsg,	 UINT idEvent, DWORD dwTime  );
BOOL CALLBACK MessageProc(HWND hwnd , UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK UserDataProc(HWND HwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ConfigProc(HWND HwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK BuildGraph(HWND HwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK  GraphProc(HWND Hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

DWORD WINAPI ListenToSerial(LPVOID Param);

void StartCommonControls(DWORD flags);
void RegisterMyClass();





typedef struct
{
 /*   char cmd;
    char addr;
    char len;
    char chkSum;
  */
    char title[128];
    char msg[128];
}PackInfo;


class App;
class TabPage;

class CTabCtrl
{

     HWND hTabCtrl;
     int nTab;
     TabPage *Page[MAX_TABPAGE];
     TabPage *CurrPage;
public:
      CTabCtrl(HWND parent);
     ~CTabCtrl();
      HWND GetHwnd(){return hTabCtrl;}
      HWND Insert(TabPage *page);
      LRESULT OnNotify(WPARAM wParam,LPARAM lParam);
      BOOL SetTabControlImageList();
//      void SetPage(int i);
      virtual  void Populate();

};

class TabPage
{

protected:
      HWND hWnd;
      char name[32];
      void Translate(char *data, AD_Value Func, char *t, DWORD ID_VAL);
public:
      TabPage(CTabCtrl *Ctrl,char *name,UINT id, DLGPROC Proc,TabPage *Obj);
      virtual ~TabPage();
      char *GetName(){return name;}
      HWND GetHwnd(){return hWnd;}
      virtual LRESULT OnNotify(WPARAM wParam,LPARAM lParam){return 0;}
      virtual LRESULT OnPaint(WPARAM wParam,LPARAM lParam){return 0;}
      virtual void Populate(){}
      virtual void Monitor(char *data){}
};

class Connection: public Win32Port
{
      HWND hWnd;
      bool OnLine;
      char FrameData[32];
      char mode;
public:
      Connection(HWND hwnd, std::string &port);
     ~Connection();
      DWORD ReadDevice(int len);
      char * BuildCmd(char cmd, char memTyp, char addr,char len);
      BOOL SendCommand(char *comd);
      char *GetBuffer(){return FrameData;}
      BOOL SendData(int len, char *data);
      bool IsConnected();
       void DsrNotify(bool status)
      {
           OnLine=status;
          printf("%s\n", OnLine?"OnLine":"OffLine");
      }
      void RxNotify( int byte_count );
      void TxNotify();
    virtual void ParityErrorNotify(){printf("ParityErrorNotify ");}
    virtual void FramingErrorNotify(){printf("FramingErrorNotify ");}
    virtual void HardwareOverrunErrorNotify(){printf("HardwareOverrunErrorNotify ");}
    virtual void SoftwareOverrunErrorNotify(){printf("SoftwareOverrunErrorNotify ");}
    virtual void BreakDetectNotify(){printf("BreakDetectNotify ");}
};




class App
{
     int mIndex;
    HANDLE ThreadHandle;
      HWND hwnd;
      void CreateMainMenu();
      char DeviceName[32];

      char MonList[32];
      int CmdLen;
      CTabCtrl *TabCtrl;
      BYTE AFlags;
      BYTE ReadLoopCount;
      short DataRefreshRate;
      DWORD nDuty;
      DWORD ListSz;
       TabPage * MonPage[MAX_TABPAGE];
       HDEVNOTIFY hDevNotify;
      BOOL RegisterNotificationMsg();
public:
       Connection *Link;

      App(HWND hwnd);
     ~App();
      LRESULT OnInit(WPARAM wParam, LPARAM lParam);
      LRESULT OnDevMsg(WPARAM w, LPARAM l);
      LRESULT OnCommand(WPARAM wParam,LPARAM lParam);
      LRESULT OnNotify(WPARAM wParam,LPARAM lParam);
      LRESULT OnDeviceChange(WPARAM wParam,LPARAM lParam);
      LRESULT OnPaint(WPARAM wParam,LPARAM lParam);

      int GetCmdLen(){return CmdLen;}
      short ReadDeviceData(HWND u1, char addr, char sz, char *t, AD_Value Func=NULL);

      char *GetDeviceName(){return DeviceName;}
      DWORD Monitor( TabPage *page,char *addr);
      int GetListSz(){return ListSz;}
      short GetRefreshRate(){return DataRefreshRate;}
      void SetRefreshRate(short r);
      TabPage *GetObject(int n){return MonPage[n];}
      HWND GetHandle(){return hwnd;}
      void Refresh(char *data);
      void Process(short msg, char *data);
      char GetMonLen();
};





class CMonitorPage:public TabPage
{

      HBITMAP hFrontBmp;
      int DataIdx;
public:
      CMonitorPage(CTabCtrl *Ctrl,char *name);
      ~CMonitorPage();
      virtual LRESULT OnNotify(WPARAM wParam,LPARAM lParam);
      virtual LRESULT OnPaint(WPARAM wParam,LPARAM lParam);
      static BOOL CALLBACK DialogProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
      virtual void Populate();
      void Monitor(char *data);
};


class CGraphic
{
    HWND hwnd;
    COLORREF color;
    HPEN hPen;
    HPEN hDelPen;
    short min,max;
    short AD_min;
    short AD_max;
    AD_Value Func;
    RingBuffer Buffer;
    char name[32];
    short Delta;
 public:

       CGraphic(HWND hParent, char *nm,UINT id, RECT rect, COLORREF c);
      ~CGraphic();
       void DrawTimeLine(HDC dc, int x,int y, int w);
       void DrawScale(HDC dc,int y1);
       float GetVal(short xPos);

       void SetData(short v);
       void OnMouseWheel(short d);
       void SetRange(short min, short max,AD_Value Func);
  //    static  BOOL CALLBACK GraphProc(HWND HwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
};

class CGraphPage:public TabPage
{
      void DrawUScale(HWND hwnd, HDC dc );
      void DrawAScale(HWND hwnd, HDC dc );
      CGraphic *UGraph;
      CGraphic *IGraph;
      int h;
 public:
       CGraphPage(CTabCtrl *Ctrl,char *name);
      ~CGraphPage();
        virtual LRESULT OnPaint(WPARAM wParam,LPARAM lParam);
       static BOOL CALLBACK DialogProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
       virtual void Populate();
       virtual void Monitor(char *data);
       void SetData(short v, short i);
};

class CBattPage:public TabPage
{
 //   friend class Connection;
      HBITMAP hBattBmp;
      int nSerial;
      int nParalel;
      short ChargeState;
public:
    CBattPage(CTabCtrl *Ctrl,char *name);
    ~CBattPage();
    virtual LRESULT OnPaint(WPARAM wParam,LPARAM lParam);
    virtual void Populate();
    static BOOL CALLBACK DialogProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void DrawStatus(HDC dc, int x, int y);
};

class CPainelPage:public TabPage
{
 //   friend class Connection;
      HBITMAP hPainelBmp;
public:
    CPainelPage(CTabCtrl *Ctrl,char *name);
    ~CPainelPage();
    virtual void Populate();
    virtual LRESULT OnPaint(WPARAM wParam,LPARAM lParam);
    static BOOL CALLBACK DialogProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

};

class CPortPage:public TabPage
{
 //   friend class Connection;

public:
    CPortPage(CTabCtrl *Ctrl,char *name);
    ~CPortPage();
    virtual void Populate();
    static BOOL CALLBACK DialogProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void OnDTRButton();
};

class CConfigPage:public TabPage
{
 //   friend class Connection;
      short ModeVoltage;
      HWND Hwnd;
public:
    CConfigPage(CTabCtrl *Ctrl,char *name);
    ~CConfigPage();
    virtual void Populate();
    static BOOL CALLBACK DialogProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

};


#endif
