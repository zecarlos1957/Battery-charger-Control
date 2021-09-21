
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



#define BANK0:1    0x00  /// xxxx xxx0
#define BANK2:3    0x01  /// xxxx xxx1
#define ROM        0x00  /// xxxx 00xx
#define RAM        0x04  /// xxxx 01xx
#define EEPROM     0x08  /// xxxx 10xx
#define READ_MEM   0x00  /// xxx0 xxxx
#define WRITE_MEM  0x10  /// xxx1 xxxx


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
BOOL CALLBACK  WinGraphProc(HWND Hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK  GraphProc(HWND Hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

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
      int DataIdx;
      void Translate(char *data, AD_Value Func,char sz, char *t, DWORD ID_VAL);
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
      char FrameData[64];
      char mode;
      char PortName[8];
public:
      Connection(HWND hwnd, std::string &port);
     ~Connection();
      bool IsValid(){return m_hPort != INVALID_HANDLE_VALUE;}
      char * BuildCmd(char cmd, char memTyp, char addr,char len);
      BOOL SendCommand(char *comd);
      const char *GetPortName()const{return (const char*)PortName;}
      char *GetBuffer(){return FrameData;}
      HANDLE GetPort(){return m_hPort;}
      BOOL SendData(int len, char *data);
      bool IsConnected();
      virtual void RxNotify( int byte_count );
      virtual void TxNotify();
      virtual void CtsNotify( bool status );
      virtual void DsrNotify(bool status);
    virtual void ParityErrorNotify(){printf("ParityErrorNotify ");}
    virtual void FramingErrorNotify(){printf("FramingErrorNotify ");}
    virtual void HardwareOverrunErrorNotify(){printf("HardwareOverrunErrorNotify ");}
    virtual void SoftwareOverrunErrorNotify(){printf("SoftwareOverrunErrorNotify ");}
    virtual void BreakDetectNotify(){printf("BreakDetectNotify ");}
};

typedef struct  TDataRequest
{
    HWND hwnd;
    char addr;
    char len;
    char *t;
    AD_Value Func;
    TDataRequest(HWND hnd, char add, char s):hwnd(hnd),addr(add),len(s){}
};


class App
{
     int mIndex;
    HANDLE ThreadHandle;
      HWND hwnd;
      void CreateMainMenu();
      char DeviceName[32];
      TDataRequest *DataRequest[32];
      int ReqIdx;
      char MonList[64];
      CTabCtrl *TabCtrl;
      BYTE AFlags;
      BYTE ReadLoopCount;
      short DataRefreshRate;
      DWORD ListSz;
       TabPage * MonPage[MAX_TABPAGE];
       HDEVNOTIFY hDevNotify;
      BOOL RegisterNotificationMsg();
      HWND GraphMon;
      bool Initialized;
public:
       Connection *Link;

      App(HWND hwnd);
     ~App();
      LRESULT OnInit(WPARAM wParam, LPARAM lParam);
      LRESULT OnDevMsg(WPARAM w, LPARAM l);
      LRESULT OnDataRequest(WPARAM wParam, LPARAM lParam);
      LRESULT OnCommand(WPARAM wParam,LPARAM lParam);
      LRESULT OnNotify(WPARAM wParam,LPARAM lParam);
      LRESULT OnDeviceChange(WPARAM wParam,LPARAM lParam);
      LRESULT OnPaint(WPARAM wParam,LPARAM lParam);
      LRESULT TimeOver();

      int GetListSz(){return ListSz;}
      short GetRefreshRate(){return DataRefreshRate;}
      char *GetDeviceName(){return DeviceName;}
      TabPage *GetObject(int n){return MonPage[n];}
      Connection *GetConnection(){return Link;}
      char GetMonLen();
      HWND GetHandle(){return hwnd;}
      short ReadDeviceData(HWND u1, char addr, char sz);
      DWORD Monitor( TabPage *page,char *addr);
      void SetMonitor(HWND);
      void SetRefreshRate(short r);
      void Refresh(char *data);
      DWORD GetAvailablePorts ( std::string* portList );

};


extern App* app;


class CMonitorPage:public TabPage
{

      HBITMAP hFrontBmp;

public:
      CMonitorPage(CTabCtrl *Ctrl,char *name);
      ~CMonitorPage();
      virtual LRESULT OnNotify(WPARAM wParam,LPARAM lParam);
      virtual LRESULT OnPaint(WPARAM wParam,LPARAM lParam);
      static BOOL CALLBACK DialogProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
      virtual void Populate();
      void Monitor(char *data);
};

   typedef struct
    {
        char name[32];
        char Addr;
        char size;
        COLORREF color;
        short min,max;
        AD_Value Func;
    }GData;

class CGraphic
{
    HWND hwnd;
    COLORREF color;
    HPEN hPen[8];
    HPEN hDelPen;
    int ndata;
    RingBuffer Buffer;
    char name[32];
    HANDLE hFile;
    short Delta;
  //  BOOL AppDataMode;
     GData **GDataList;
    int GetAddrIndex(char*,char);
 public:

       CGraphic(HWND hParent, char *nm,  GData **Gdt, RECT rect, COLORREF c);
      ~CGraphic();
       void DrawTimeLine(HDC dc, int x,int y, int w);
       void DrawScale(HDC dc,int y1);
       float GetVal(short i, short xPos);
       void FillStruct( GData **Gdt);
       int Populate(GData **Gdt);
       void SetData(short i,short v);
       void DrawData(short i,short v);
       void OnMouseWheel(short d);
       void SetRange(short i, short min, short max,AD_Value Func);
       static  LRESULT CALLBACK GraphProc(HWND HwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
       void Monitor(char *addr, char *data);
       RingBuffer *GetBuffer(){return &Buffer;}
       void FillHdrFile(const char *nm,char *info);
       void ReadDataFromFile();
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
      HBITMAP hBattBmp;
      int nSerial;
      int nParalel;
      short ChargeState;
public:
    CBattPage(CTabCtrl *Ctrl,char *name);
    ~CBattPage();
    virtual LRESULT OnPaint(WPARAM wParam,LPARAM lParam);
    virtual void Populate();
    virtual void Monitor(char *data);
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
    void OnRTSButton();
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
