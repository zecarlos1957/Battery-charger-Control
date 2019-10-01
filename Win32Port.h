//
//  WIN32PORT.H
//
//  Source code from:
//
//  Serial Communications: A C++ Developer's Guide, 2nd Edition
//  by Mark Nelson, IDG Books, 1999
//
//  Please see the book for information on usage.
//
// This file contains the class definition for Win32Port. This class
// implements a version of class RS232 that works with the Win32 serial
// API. The implementation of this class is in file Win32Port.cpp.
// Although the sample programs in this book use MFC, this class should
// work independently of any application framework.
//

#ifndef WIN32PORT_H
#define WIN32PORT_H

#include <deque>
#include <string>
using namespace std;

#include <windows.h>

 #include "rs232.h"
 #include "RSDeque.h"

//
// These are the enumerated error values added to the class specifically
// to support the Win32Port class. One of these items requires a bit of
// explanation. If an error type of WIN32_CHECK_WINDOWS_ERROR is returned
// the caller will have to examine m_dwWindowsError to see what Windows
// thinks the error is. Since there is a nearly limitless number of possible
// error returns from calls the Windows API, it isn't possible to account
// for every single error return, so this is the kitchen sink value.
//
enum Win32PortError {
    WIN32_CHECK_WINDOWS_ERROR = RS232_NEXT_FREE_ERROR,
    WIN32_SETTINGS_FAILURE,
    WIN32_HANDSHAKE_LINE_IN_USE
};



class Win32Port : public RS232
{
//
// Constructors and destructors
//
   COMSTAT ComStat;
public :
    Win32Port( const string &port_name,
               long baud_rate = UNCHANGED,
               char parity = UNCHANGED,
               int word_length = UNCHANGED,
               int stop_bits = UNCHANGED,
               int dtr = SET,
               int rts = SET,
               int xon_xoff = DISABLE,
               int rts_cts = DISABLE,
               int dtr_dsr = DISABLE );
    virtual ~Win32Port();

protected :
    Win32Port() : m_TxQueue( MAX_OUTPUT_BUFFER_SIZE ),
                  m_RxQueue( MAX_INPUT_BUFFER_SIZE ){}

//
// These enumerated values are used internally by the Win32Port
// class. Note that the two buffer size values can be modified
// if your application has different requirements.
//
    enum { MAX_INPUT_BUFFER_SIZE = 2048 };
    enum { MAX_OUTPUT_BUFFER_SIZE = 2048 };
  ///  enum { EV_RINGTE = 0x2000 };  for Win95
     enum { EV_RINGTE = 0 }; /// for Win NT4
//
// These members are all used internally, and aren't needed
// by any code outside the Win32Port class.
//
    HANDLE m_hPort;                    // Handle of the port, used everywhere
    int m_iBreakDuration;
    int first_debug_output_line;
    bool m_bInputThreadReading;
    DWORD m_dwWindowsError;
    long m_hOutputThread;
    long m_hInputThread;
    RSdeque m_TxQueue;               //Outbound data queue
    RSdeque m_RxQueue;               //Incoming data queue
    Dcb32 m_Dcb;                     //Current DCB settings
    DWORD m_dwErrors;                //Cumulative line status error bits
    DWORD m_dwModemStatus;           //Current modem status bits
    //
    // These five Win32 events are all used to communicate requests to the
    // input and output threads.
    //
    HANDLE m_hKillOutputThreadEvent;
    HANDLE m_hKillInputThreadEvent;
    HANDLE m_hWriteRequestEvent;
    HANDLE m_hReadRequestEvent;
    HANDLE m_hBreakRequestEvent;
//
// Private member functions not available to code outside the class
//
    RS232Error write_settings();
    void read_settings();
    void check_modem_status( bool first_time, DWORD event_mask );
    void clear_error( COMSTAT *comstat = 0  );
    RS232Error translate_last_error();
    bool output_worker();
    static void OutputThread( void * arglist );
    static void InputThread( void *arglist );
    //
    // The following are the declarations for the RS232 class members
    // that are implemented by Win32Port. Most of these functionsonly
    // have stubbed versions in the base class, but have actual useful
    // implementations in Win32Port.
    //
public :
    RS232Error Set( long baud_rate = UNCHANGED,
                    int parity = UNCHANGED,
                    int word_length = UNCHANGED,
                    int stop_bits = UNCHANGED );
    int Dtr( int setting = UNCHANGED );
    int Rts( int setting = UNCHANGED );
    int TXSpaceFree( void ){ return m_TxQueue.SpaceFree(); }
    int TXSpaceUsed( void ){ return m_TxQueue.SpaceUsed(); }
    int RXSpaceUsed( void ){ return m_RxQueue.SpaceUsed(); }
    int RXSpaceFree( void ){ return m_RxQueue.SpaceFree(); }
    int DtrDsrHandshaking( int setting = UNCHANGED );
    int RtsCtsHandshaking( int setting = UNCHANGED );
    int XonXoffHandshaking( int setting = UNCHANGED );
    int FormatDebugOutput( char *buffer = 0, int line_number = -1 );
    int ParityError( int clear = UNCHANGED );
    int BreakDetect( int clear = UNCHANGED );
    int FramingError( int clear = UNCHANGED );
    int HardwareOverrunError( int clear = UNCHANGED );
    int SoftwareOverrunError( int clear = UNCHANGED );
    int Break( long milliseconds = 300 );
    int Cd( void );
    int Ri( void );
    int Cts( void );
    int Dsr( void );
    int Peek( void *buffer, unsigned int count );
    int FlushRXBuffer( void );
    int FlushTXBuffer( void );
    char *ErrorName( int error );

//
// Virtual functions that this library must implement in order to support
// public library routines in Win32Port
//
protected :
    int read_buffer( char *buffer, unsigned int count );
    int write_buffer( char *buffer, unsigned int count = -1 );
    int read_byte( void );
    int write_byte( int c );
//
// The following notification functions all have null implemementations
// that do nothing in Win32Port. You implement notification in your
// program by creating a new class derived from Win32Port, and create
// your own versions of these virtual functions. The sample program
// from Chapter 10 shows a simple way to accomplish this.
//
    virtual void RxNotify( int byte_count ){}
    virtual void TxNotify(){}
    virtual void ParityErrorNotify(){}
    virtual void FramingErrorNotify(){}
    virtual void HardwareOverrunErrorNotify(){}
    virtual void SoftwareOverrunErrorNotify(){}
    virtual void BreakDetectNotify(){}
    virtual void CtsNotify( bool status ){}
    virtual void DsrNotify( bool status ){}
    virtual void CdNotify( bool status ){}
    virtual void RiNotify( bool status ) {}
};

#endif // #infdef WIN32PORT_H

// EOF Win32Port.h
