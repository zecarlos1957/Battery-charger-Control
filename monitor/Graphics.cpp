
#ifndef  _WIN32_IE
#define _WIN32_IE 0x0500
#endif

#ifndef  WINVER
#define WINVER 0x0500
#endif

#include <windows.h>
#include <commctrl.h>
#include "app.h"

extern HINSTANCE hInst;

static CGraphic *Graphic=NULL;

CGraphic::CGraphic(HWND hParent, char *nm, UINT id, RECT rect,COLORREF color):color(color)
{
    lstrcpy(name,nm);
 //    y1=rect.top;

  //   printf("CGraphic::w %d h %d\n",w,h);
     hwnd=CreateWindowEx(WS_EX_CLIENTEDGE , "GraphClass", "", WS_CHILD|WS_VISIBLE|WS_HSCROLL,
                        rect.left,rect.top,rect.right,rect.bottom,hParent,
                        (HMENU)id, hInst,  this);

   //   SendDlgItemMessage(hParent,id,WM_SETTEXT,0,(WPARAM)nm);


 /*  HWND hToolTip=CreateWindowEx(0, TOOLTIPS_CLASS,"", TTS_ALWAYSTIP|WS_VISIBLE,
                         0,0,0,0, hParent, (HMENU)NULL, hInst, NULL);

      if(!hToolTip)DisplayLastError("toolTip",0);
     SendMessage(hParent, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT	), (LPARAM)true);
*/
    hDelPen=CreatePen(PS_SOLID, 0,  RGB(255,255,255));
    hPen=CreatePen(PS_SOLID, 0, color);
/*
    TOOLINFO ti={0};

    ti.cbSize   = sizeof(ti);
    ti.uFlags   = TTF_SUBCLASS|TTF_IDISHWND;
    ti.uId      = (UINT_PTR)hwnd;
    ti.lpszText = LPSTR_TEXTCALLBACK;
    ti.hwnd     = hParent;
    SendMessage(hToolTip,TTM_ADDTOOL,0,(LPARAM)&ti);

*/
}
CGraphic::~CGraphic()
{
      DeleteObject(hDelPen);
      DeleteObject(hPen);
}
void CGraphic::DrawScale( HDC dc, int y1)
{
     RECT rect;
     GetWindowRect(hwnd,&rect);
     int x= 30;
      int h= rect.bottom-rect.top-40;
     int y=y1+h+10;

      char str[32];
     int range=20;///(short)(Func(max)-(short)Func(min)+1)*10;
  //    int range=(max-min);
     int space = (h/range);
     int scale = 1;//Func(min);

  ///   printf("%d %d %d %d mi %d mx %d\n", rect.left, rect.top, rect.right, rect.bottom, min, max);
 //  printf("%d \n",spc/10);

     HANDLE Old=SelectObject(dc,CreatePen(PS_SOLID,0,RGB(100,100,100)));

     SetTextColor(dc,color);

/*
     for(int idx=0;idx <= range ;idx++)
     {

          sprintf(str,"%d",scale++);
          TextOut(dc,x-30,y-8,str,lstrlen(str));
          MoveToEx(dc,x+10,y,NULL);
          LineTo(dc,x+15,y);

          y -= space;

     }
  */



      for(int idx=0;idx <= range;idx++)
     {
           if(idx%10 == 0)
         {

             sprintf(str,"%d",scale++);
             TextOut(dc,x-30,y-8,str,lstrlen(str));
             MoveToEx(dc,x,y,NULL);
             LineTo(dc,x+15,y);
    //         printf("%d ",y);
         }
         else if(idx%5 == 0)
          {
             MoveToEx(dc,x+5,y,NULL);
             LineTo(dc,x+15,y);
         }
           else   if(space>4)
          {
             MoveToEx(dc,x+10,y,NULL);
             LineTo(dc,x+15,y);
         }
          y-=space;

     }




 //     DrawTimeLine( dc, 30, y1+h+45, rect.right-rect.left);
//printf(" %d ",w);
     DeleteObject(SelectObject(dc,Old));

}


void CGraphic::SetData(short v)
{
     int x=0;
     static int oldy=0;
     HGDIOBJ hObj;
     HDC dc= GetDC(hwnd);
     CountIterator idx;

    if(Buffer.IsFull())
    {
        hObj=SelectObject(dc,(HGDIOBJ)hDelPen);
        idx = Buffer.begin();
        MoveToEx(dc,x,oldy,NULL);
        while(idx != Buffer.end())
            ::LineTo(dc,++x, Buffer[idx++] );
        SelectObject(dc,(HGDIOBJ)hPen);
     }
     else  hObj=SelectObject(dc,(HGDIOBJ)hPen);
    x=0;
// printf("%d %d\n",max,v);
    v = (max-v) ;


    Buffer.push_back(v);
    idx=Buffer.begin();
    MoveToEx(dc,x,v,NULL);

    while(idx != Buffer.end())
          ::LineTo(dc, ++x, Buffer[idx++] );

     SelectObject(dc,(HGDIOBJ)hObj);
     ReleaseDC(hwnd,dc);
     oldy = Buffer[idx-1] ;

}

void CGraphic::DrawTimeLine(HDC dc,int x,int y,int w)
{

    for(int x1=x; x1<=w; x1++)
     {
         if(x1%60 == 0)
         {
             MoveToEx(dc,x1,y,NULL);
             LineTo(dc,x1,y+5);
         }
     }


}
void CGraphic::OnMouseWheel(short delta)
{
      RECT rc;
     GetWindowRect(hwnd,&rc);
     int x= 30;
      int h= rc.bottom-rc.top-40;
     int y=h+30;
    Delta=delta;

     rc.left=0; rc.top=0; rc.right=500; rc.bottom=500;
     InvalidateRect(hwnd,&rc,TRUE);
     printf(".");
}

void CGraphic::SetRange(short min, short max,AD_Value Func)
{
    CGraphic::min=min;
    CGraphic::max=max;
    CGraphic::Func=Func;
}


float CGraphic::GetVal(short xPos )
{
    CountIterator i;
    for(i=Buffer.begin();i!= Buffer.end();i++)
       if(i==xPos) break;

    if(i == Buffer.end())
       return -1;
    short v=max-Buffer[i];

    return  v ;

}



BOOL CALLBACK  GraphProc(HWND Hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
     switch(msg)
     {
         case WM_INITDIALOG:
              {
                   RECT rect;
                   GetClientRect(Hwnd,&rect);
                   Graphic=new CGraphic(Hwnd,"name",ID_GRAPH_U,rect,RGB(250,0,0));
              }
              return FALSE;
         case WM_PAINT:
            {
              PAINTSTRUCT ps;
              HDC hdc=BeginPaint(Hwnd,&ps);
         //     Graphic->DrawScale(hdc,15);
              EndPaint(Hwnd, &ps);
              return FALSE;
            }
  /*       case WM_MOUSEMOVE:
              SetFocus(Hwnd);
              return
         case WM_MOUSEWHEEL:
              if(Graphic)
                  Graphic->OnMouseWheel(HIWORD(wParam));
              return

    */     case WM_CLOSE:
               SendMessage(Hwnd,WM_DESTROY,0,0);
               return TRUE;

         case WM_DESTROY:
              delete Graphic;
              DestroyWindow(Hwnd);
              return TRUE;
    }

   return FALSE;

}
