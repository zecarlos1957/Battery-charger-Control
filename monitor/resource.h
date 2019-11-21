#ifndef RESOURCE_H
#define RESOURCE_H


#define ID_STATIC       -1
#define DLG_MAIN        100
#define IDC_TABCTRL     101
#define ID_TOOLTIP      2303

#define DLG_MONITOR     200
#define ID_DEV          201
#define ID_UIN          202
#define ID_UOUT         203
#define ID_IOUT         204
#define ID_TEMP         205
#define ID_AMP_H        206
#define ID_RI           207
#define ID_AUTONOMI         208
#define IDB_SYSTEM      209
#define IDB_PAINEL      210
#define IDB_BATTERY     211


#define DLG_GRAPHICS    300
#define ID_GRAPH_U       301
#define ID_GRAPH_I       302
#define ID_TITLE       303




#define DLG_COMPORT     400
#define ID_PORTNUM      401
#define ID_BRATE        402
#define ID_BSIZE        403
#define ID_SBIT         404
#define ID_PAR          405
#define ID_RINT         406
#define ID_RMULT        407
#define ID_RCONST       408
#define ID_WMULT        409
#define ID_WCONST       410
#define ID_BTN_DTR       411

#define MESSAGE_DLG    500
#define ID_ERRMSG      501

#define USERDATA_DLG    600
#define IDC_MEM         601
#define IDC_ADDR        602
#define IDC_BYTELEN     603
#define IDC_UPDOWN      604
#define IDC_DATA        605
#define IDC_BTNOK       606
#define IDC_BTNCANCEL    607
#define IDC_LISTDATA     608
#define IDC_BTNREAD      609
#define IDC_BTNWRITE      610

#define DLG_CONFIG       700
#define ID_VOFF          701
#define ID_VFLOAT         702
#define ID_VOCT          703
#define ID_ITRIC          704
#define ID_IBLK         705
#define ID_IOCT          706
#define ID_UMODE         707

#define DLG_BATTERY        800

#define DLG_PAINEL         900

#define CFG_OPTION_DLG       1100
#define ID_LOOPCNT      1101
#define ID_REFRESH       1102
//#define ID_OK             1103
//#define ID_CANCEL       1104
#define ID_DEFAULT         1105
#define ID_HELP            1106


#define DLG_CREATEGRAPH    1200
#define ID_VARS             1201
#define ID_SHOWVARS         1202
#define ID_SELECT           1203
#define ID_DELETE           1204
#define ID_OK               1205
#define ID_CANCEL           1206
#define ID_OPEN             1207


#define ID_DEVICE_CHARGE_ON      1301
#define ID_DEVICE_MONITOR_ON     1303
#define ID_DEVICE_READ           1304
#define ID_DEVICE_WRITE          1305
#define ID_DEVICE_RESET          1306
#define ID_DEVICE_GRAPHIC        1307
#define ID_SHOWGRAPH             1308
#define ID_SLOW                  1309
#define ID_FAST                  1310
#define ID_EQUALIZE              1311
#define ID_FLOTING               1312
#define ID_OPTION_SETUP          1313
#define ID_HELP_HLP              1314
#define ID_HELP_ABOUT            1315
#define ID_DEVICE_MEM            1316


#define IDS_MSG1           1400


#define EV_APP_INIT     WM_USER+1
#define EV_DEVICE_MSG   WM_USER+2
#define EV_DATA_REQUEST WM_USER+3


#endif
