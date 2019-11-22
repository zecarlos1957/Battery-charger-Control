
#ifndef  _WIN32_IE
#define _WIN32_IE 0x0500
#endif

#ifndef  WINVER
#define WINVER 0x0500
#endif

#include <windows.h>
#include <commctrl.h>
#include "app.h"

#include <string>

using std::string;

extern HINSTANCE hInst;

static CGraphic *Graphic=NULL;
extern App *app;

CGraphic::CGraphic(HWND hParent, char *nm, GData **Gdt, RECT rect,COLORREF color):color(color),
                                                                                  hFile(INVALID_HANDLE_VALUE)
{
    DWORD n;
    GDataList=Gdt;

     hwnd=CreateWindowEx(WS_EX_CLIENTEDGE , "GraphClass", "", WS_CHILD|WS_VISIBLE|WS_HSCROLL,
                        rect.left,rect.top,rect.right,rect.bottom,hParent,
                        (HMENU)NULL, hInst,  this);

    if((*Gdt)->Addr==0)  /// Data From file
    {
        printf("Data From File\n");
        FillStruct(Gdt);
        ndata=Populate(Gdt);
        PostMessage(hwnd,EV_DATA_REQUEST,0,0);
    }
   else                /// Data From APP
     {
         printf("Data From App\n");
         ndata=Populate(Gdt);
         char info[64];
         info[0]='\0';
         GData **ptr=Gdt;
          int i=0;
          while(*ptr)
          {
              sprintf(info+lstrlen(info),"%s%c\0",(*ptr)->name,(*ptr)->Addr+0x80);
              ptr++;
           }
           sprintf(info+lstrlen(info),"\n");
   //        printf("%s %d\n",info,ndata);
           lstrcat(nm,".dat");
           FillHdrFile(nm, info);
           app->SetMonitor(hwnd);
     }
  //   Buffer.Resize(ndata);

    hDelPen=CreatePen(PS_SOLID, 0,  RGB(210,210,210));

    for(int n=0;n<ndata;n++)
    {
        hPen[n]=CreatePen(PS_SOLID, 0, GDataList[n]->color);
    }  //   printf("CGraphic::w %d h %d\n",w,h);



   //   SendDlgItemMessage(hParent,id,WM_SETTEXT,0,(WPARAM)nm);


 /*  HWND hToolTip=CreateWindowEx(0, TOOLTIPS_CLASS,"", TTS_ALWAYSTIP|WS_VISIBLE,
                         0,0,0,0, hParent, (HMENU)NULL, hInst, NULL);

      if(!hToolTip)DisplayLastError("toolTip",0);
     SendMessage(hParent, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT	), (LPARAM)true);
*/

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
      app->SetMonitor(NULL);

      GData **p=GDataList;
      while(*p)delete *p++;
      delete GDataList;

      if(hFile!=INVALID_HANDLE_VALUE)
          CloseHandle(hFile);

      DeleteObject(hDelPen);
      for(int n=0;n<ndata;n++)
          DeleteObject(hPen[n]);
}

void CGraphic::FillStruct( GData **Gdt)
{
     char dat[256];
     int i=0;
     DWORD n=0;


     lstrcat((*Gdt)->name,".dat");


     hFile=CreateFile((*Gdt)->name, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,0);
     if(hFile==INVALID_HANDLE_VALUE)
           return;

 //   printf("File %s\n",(*Gdt)->name);
      delete *Gdt;

     ReadFile(hFile, &dat[0],4,&n,NULL);
     DWORD id = *(DWORD*)dat;
     if(id!=0x01415a41)
     {
         MessageBox(hwnd,"ERRO","Ficheiro inválido",MB_OK|MB_ICONSTOP);
         return;
     }
    char*buffer;
    char *ptr=buffer=&dat[0];

     while(1)
     {
        ReadFile(hFile, ptr,1,&n,NULL);
        if(*ptr==0x0a)break;

        if(*ptr & 0x80 )
        {
            *Gdt=new GData;
            int len=ptr-(buffer+i);
            CopyMemory((*Gdt)->name,(buffer+i),len);
            *((*Gdt)->name+len)='\0';
            i+=len+1;
            (*Gdt)->Addr=(*(buffer+i-1))&0x7f;
 //            printf(" %s 0x%x len %d\n", (*Gdt)->name, (*Gdt)->Addr,len);

            Gdt++;

        }
         ptr++;
     }
      *Gdt=NULL;
}

void CGraphic::ReadDataFromFile()
{
    DWORD n;
    short v;

    if(hFile==INVALID_HANDLE_VALUE)
        return;
      while(n)
     {
         for(int i=0;i<ndata;i++)
         {
             ReadFile(hFile,&v,GDataList[i]->size,&n,NULL);

              if(!n)break;

             SetData(i,v);
         }
     }

     CloseHandle(hFile);
     hFile=INVALID_HANDLE_VALUE;
}

int CGraphic::Populate( GData **Gdt)
{
     GData **ptr=Gdt;
    int i=0;
    while(*ptr)
    {

        (*ptr)->min=0;


        switch ((*ptr)->Addr)
        {
            case 0x41:
                  (*ptr)->max=200;
                  (*ptr)->Func=ADtoTemp;
                  (*ptr)->size=1;
                  (*ptr)->color=RGB(0,255,0);
                  break;
            case 0x60:
                  (*ptr)->max=950;
                  (*ptr)->Func= ADtoVolts;
                  (*ptr)->size=2;
                  (*ptr)->color=RGB(255,0,255);
                  break;
            case 0x62:
                  (*ptr)->max=500;//900;///500;
                  (*ptr)->Func=ADtoVolts;
                   (*ptr)->size=2;
                   (*ptr)->color=RGB(0,0,255);
                break;
            case 0x64:
                 (*ptr)->max=200;
                 (*ptr)->Func=ADtoAmps;
                  (*ptr)->size=2;
                   (*ptr)->color=RGB(255,0,0);
                break;
        }
        ptr++;
        i++;
     }



  return i;



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

void CGraphic::DrawData(short i,short v)
{
     int x=-1;
     static int oldy[4]={GDataList[i]->min, GDataList[i]->min, GDataList[i]->min, GDataList[i]->min};
     HGDIOBJ hObj;
     HDC dc= GetDC(hwnd);

}

void CGraphic::SetData(short i, short v)
{
     short max=GDataList[i]->max;

     int x=-1;
     static int oldy[4]={GDataList[i]->min, GDataList[i]->min, GDataList[i]->min, GDataList[i]->min};
     HGDIOBJ hObj;
     HDC dc= GetDC(hwnd);
     CountIterator idx;

    if(Buffer.IsFull())
    {
        hObj=SelectObject(dc,(HGDIOBJ)hDelPen);
        idx = Buffer.begin();
        MoveToEx(dc,x,oldy[i],NULL);
  //     if(i==0)  printf("del %d:",oldy[i]);
        while(idx != Buffer.end())
        {
           if(idx%ndata==i)
           {
               ::LineTo(dc, ++x, Buffer[idx] );
   //            if(i==0)  printf("%d:",Buffer[idx]);
           }
           idx++;
        }
 //       if(i==0)  printf("\n");
        SelectObject(dc,(HGDIOBJ)hPen[i]);
     }
     else  hObj=SelectObject(dc,(HGDIOBJ)hPen[i]);
     x=-1;

 //       if(i==2)printf("\n");
    v = (GDataList[i]->max-v) ;

 //    printf("%d ",v);

    Buffer.push_back(v);
    idx=Buffer.begin();
    MoveToEx(dc,x,Buffer[idx],NULL);
// if(i==0)printf("draw %d:",Buffer[idx]);
    while(idx != Buffer.end())
    {
        if(idx%ndata==i)
        {
            ::LineTo(dc, ++x, Buffer[idx] );
 //           if(i==0)printf("%d:",Buffer[idx]);
        }
         idx++;
    }
 //   printf("\n");
     SelectObject(dc,(HGDIOBJ)hObj);
     ReleaseDC(hwnd,dc);
     oldy[i] = Buffer[idx-1] ;

}

/**
void CGraphic::SetData(short i, short v)
{
     short max=GDataList[i]->max;

     int x=0;
     static int oldy[4]={0,0,0,0};
     HGDIOBJ hObj;
     HDC dc= GetDC(hwnd);
     CountIterator idx;

    if(Buffer.IsFull())
    {
        hObj=SelectObject(dc,(HGDIOBJ)hDelPen);
        idx = Buffer.begin();
        MoveToEx(dc,x,oldy[i],NULL);
        while(idx != Buffer.end())
        {
           if(idx%ndata==i)
               ::LineTo(dc, ++x, Buffer[idx] );
           idx++;
        }
        SelectObject(dc,(HGDIOBJ)hPen[i]);
     }
     else  hObj=SelectObject(dc,(HGDIOBJ)hPen[i]);
    x=0;
 //       printf("%d ",v);
 //       if(i==2)printf("\n");
    v = (GDataList[i]->max-v) ;
    Buffer.push_back(v);
    idx=Buffer.begin();
    MoveToEx(dc,x,v,NULL);

    while(idx != Buffer.end())
    {
        if(idx%ndata==i)
          ++x;//  ::LineTo(dc, ++x, Buffer[idx] );
         idx++;
    }
     SelectObject(dc,(HGDIOBJ)hObj);
     ReleaseDC(hwnd,dc);
     oldy[i] = Buffer[idx-1] ;

}
*/
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

void CGraphic::SetRange(short i, short min, short max,AD_Value Func)
{
    GDataList[i]->min=min;
    GDataList[i]->max=max;
    GDataList[i]->Func=Func;
}


float CGraphic::GetVal(short idx, short xPos )
{
    CountIterator i;
    for(i=Buffer.begin();i!= Buffer.end();i++)
       if(i==xPos) break;

    if(i == Buffer.end())
       return -1;
    short v=GDataList[idx]->max-Buffer[i];

    return  v ;

}

/**
    Calculate the index of w

*/
int CGraphic::GetAddrIndex(char *Req_addr, char addr)
{
    int distance=0;
    for(int n=1;n<Req_addr[0];n++)
    {
   //     printf("addr 0x%x <= 0x%x && 0x%x > 0x%x\n",*(Req_addr+n*2),addr,
   //                                                (*(Req_addr+n*2)+*(Req_addr+n*2-1)) ,addr);
         if(*(Req_addr+n*2) <= addr && (*(Req_addr+n*2)+*(Req_addr+n*2-1)> addr))
         {
      //       printf("return %d\n",distance+(addr-*(Req_addr+n*2)));
             return distance+(addr-*(Req_addr+n*2));
         }
         distance+=*(Req_addr+n*2-1);
    }
    return -1;
}

void CGraphic::Monitor(char *Req_addr,char *data)
{
     int n=0;
     int val;

 //for(int i=0;i<app->GetListSz();i++)
 //    printf("0x%x ",*(data+i)&0xff);

     while(GDataList[n]!=NULL)
     {
          int idx=GetAddrIndex(Req_addr,GDataList[n]->Addr);
          if(idx != -1)
          {
              if(GDataList[n]->size==1)
              {/** ERRO no calculo de idx */
                  val= data[idx]&0xff;

              }
              if(GDataList[n]->size==2)
              {
                  val = *(short*)&data[idx];

              }
              if(GDataList[n]->size==4)
              {
                  val = *(int*)&data[idx];

              }
  // printf("%d+(0x%x-*(0x%x))= %d \n",i, GDataList[n]->Addr ,*(Req_addr+i+2),
//          i + (GDataList[n]->Addr - *(Req_addr+i+2)));

              SetData(n,val);
   //           printf("0x%x -> 0x%x\n",GDataList[n]->Addr,val);
              if(hFile!=INVALID_HANDLE_VALUE)
              {
                   DWORD rb;
                    WriteFile(hFile,&val,GDataList[n]->size,&rb,NULL);
              }
         }
          n++;
     }
 //     printf("\n");
}


LRESULT CALLBACK  CGraphic::GraphProc(HWND Hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
     switch (msg)
     {
            case EV_DATA_REQUEST:
                 if(wParam)Graphic->Monitor((char*)wParam, (char*)lParam);
                 else Graphic->ReadDataFromFile();
                 return 0;

     }
     return DefWindowProc(Hwnd,msg,wParam,lParam);
}

BOOL CALLBACK  WinGraphProc(HWND Hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
     switch(msg)
     {
         case WM_INITDIALOG:
              {

                   RECT rect;
                   GetClientRect(Hwnd,&rect);
                   rect.top+=20; rect.left+=60;
                   rect.right-=80; rect.bottom-=80;

                   GData **ptr=(GData**)lParam;
                   if((*ptr)->Addr==0)
                   {
                       SetWindowText(Hwnd,(*ptr)->name);

                   }
                   char fname[32];
                   lstrcpy(fname,"CargaYuasa");
                     Graphic=new CGraphic(Hwnd,fname,ptr,rect,RGB(250,0,0));

              }
              return FALSE;
          case WM_PAINT:
            {
              PAINTSTRUCT ps;
              HDC hdc=BeginPaint(Hwnd,&ps);
               if(Graphic)Graphic->DrawScale(hdc,15);
              EndPaint(Hwnd, &ps);
              return FALSE;
            }
       /*  case WM_MOUSEMOVE:
              SetFocus(Hwnd);
              return
         case WM_MOUSEWHEEL:
              if(Graphic)
                  Graphic->OnMouseWheel(HIWORD(wParam));
              return

    */     case WM_CLOSE:
                delete Graphic;
                Graphic=NULL;
                SendMessage(Hwnd,WM_DESTROY,0,0);
                return TRUE;

         case WM_DESTROY:
               DestroyWindow(Hwnd);
              return TRUE;
    }

   return FALSE;

}



void CGraphic::FillHdrFile(const char *nm,char *info)
{
     DWORD n;

     hFile=CreateFile(nm,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,0);
     if(hFile==INVALID_HANDLE_VALUE)
         return;
     char ver[32];
     sprintf(ver,"%s%c","AZA",0x01);
     WriteFile(hFile,ver,lstrlen(ver),&n,0);
     WriteFile(hFile,info,lstrlen(info),&n,0);
}
