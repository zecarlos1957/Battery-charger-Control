
#ifndef  _WIN32_IE
#define _WIN32_IE 0x0500
#endif

#include <windows.h>

#include "App.h"
#include <commctrl.h>
#include <tchar.h>
#include <dbt.h>
#include <stdio.h>
#include <math.h>

extern HINSTANCE hInst;

///*****************************************************************


CTabCtrl::CTabCtrl(HWND hParent): nTab(0)
{


      hTabCtrl = GetDlgItem(hParent,IDC_TABCTRL);

    if (hTabCtrl == NULL)
    {
        MessageBox(NULL, "Tab creation failed", "Tab Example", MB_OK | MB_ICONERROR);
        return;
    }

    SetTabControlImageList();
    // set tab control's font
   	SendMessage(hTabCtrl, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)true);


}


CTabCtrl::~CTabCtrl()
{
     HIMAGELIST hImages = reinterpret_cast<HIMAGELIST>(SendMessage(hTabCtrl,TCM_GETIMAGELIST,0,0));

     ImageList_Destroy(hImages);
}

BOOL CTabCtrl::SetTabControlImageList()
{
    HIMAGELIST hImages = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                                          GetSystemMetrics(SM_CYSMICON),
                                          ILC_COLOR32|ILC_MASK,1,1);
    if(hImages == 0)
    {
         DisplayLastError("CreateImageList",0);
         return FALSE;
    }
    HINSTANCE hLib = LoadLibrary("shell32.dll");
    int icon_index[] = {23, 41, 43, 44, 47,48};
    int i;
    HICON hIcon;

    for(i=0;i<MAX_TABPAGE; i++)
    {
        hIcon = reinterpret_cast<HICON>(LoadImage(hLib, MAKEINTRESOURCE(icon_index[i]),IMAGE_ICON,0,0,LR_SHARED));
        ImageList_AddIcon(hImages,hIcon);
    }
    FreeLibrary(hLib);
    TabCtrl_SetImageList(hTabCtrl, hImages);
}


HWND CTabCtrl::Insert(TabPage *page)
{
     TCITEM tie={0};
     int i;
     if(nTab == MAX_TABPAGE  )
     {
        MessageBox(NULL, "Couldn't add Tab. (Full) ",page->GetName(), MB_OK | MB_ICONERROR);
        return NULL;
     }

     tie.mask = TCIF_TEXT|TCIF_IMAGE;
     tie.pszText = page->GetName();
     tie.cchTextMax=lstrlen(page->GetName()+1);
     tie.iImage=nTab;
     if((i=TabCtrl_InsertItem(hTabCtrl,nTab,&tie)) == -1)
     {
         DestroyWindow(hTabCtrl);
        MessageBox(NULL, "Couldn't add Tab ",page->GetName(), MB_OK | MB_ICONERROR);
        return NULL;
     }

     Page[nTab++] = page;

    return page->GetHwnd();
}

LRESULT CTabCtrl::OnNotify(WPARAM wParam,LPARAM lParam)
{
       // get the tab message from lParam
       LPNMHDR lpnmhdr = (LPNMHDR)lParam;
       if (lpnmhdr->code == TCN_SELCHANGING)
       {
           int iPage = TabCtrl_GetCurSel(hTabCtrl);
           CurrPage = Page[iPage];
       }

       if (lpnmhdr->code == TCN_SELCHANGE)
       {

           int iPage = TabCtrl_GetCurSel(hTabCtrl);

           ShowWindow(CurrPage->GetHwnd(), SW_HIDE);  // first hide tab view 2
           ShowWindow(Page[iPage]->GetHwnd(), SW_SHOWNORMAL);

      }
    return 0;
}


/*
void CTabCtrl::SetPage(int i)
{
     if(i < MAX_TABPAGE)
    {
         int n=TabCtrl_GetCurPage();
         ShowWindow(Page[n]->GetHwnd(),SW_HIDE);
         TabCtrl_SetCurSel(hTabCtrl,i);
        CurrPage=Page[i];
        ShowWindow(Page[i]->GetHwnd(),SW_SHOW);
    }

}
*/

void CTabCtrl::Populate()
{
     for(int i = 0; i < nTab; i++)
     {
         Page[i]->Populate();
     }
}

///*****************************************************






TabPage::TabPage(CTabCtrl *Ctrl,char *nm,UINT id, DLGPROC Proc, TabPage *Obj)
{
    lstrcpy(name,nm);

    RECT tr = {0}; // rect structure to hold tab size
    TabCtrl_GetItemRect( Ctrl->GetHwnd(), 0, &tr);

    hWnd=CreateDialogParam(hInst, MAKEINTRESOURCE(id),Ctrl->GetHwnd(),(DLGPROC) Proc, (LPARAM)Obj);
    if(!hWnd)
       DisplayLastError("CreateDialog",0);

}

TabPage::~TabPage()
{


}

void TabPage::Translate(char *data, AD_Value Func, char *t,DWORD ID_VAL)
{
     char str[32];
     short AD= *(short*)data;

     double val = Func(AD);
     sprintf(str,"%f",val);
     char *ptr = str;
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

     SendDlgItemMessage(hWnd, ID_VAL, WM_SETTEXT, 0,(LPARAM)str);

}

