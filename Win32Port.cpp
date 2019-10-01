//
//  WIN32PORT.CPP
//
//  Source code from:
//
//  Serial Communications: A C++ Developer's Guide, 2nd Edition
//  by Mark Nelson, M&T Books, 1999
//
//  Please see the book for information on usage.
//
// This file contains the class implementation for Win32Port.
// Win32Port implements a version of class RS232 that works with
// the Win32 serial API. The implementation of this class is in
// file Win32Port. Although the sample programs in this book use
// MFC, this class should work independently of any application
// framework.
//
//

#include <process.h>
#include <cassert>
#include <sstream>
#include <iomanip>
#include <stdio.h>
using namespace std;

#include "Win32Port.h"

extern void DisplayLastError(char * Operation,DWORD error=0);

//
// Under MFC, some heap use tracking is done in debug mode, in an
// attempt to track down memory leaks. This code enables that.
// Feel free to delete the five lines that enable this feature,
// as they are not necessary for proper operation of the
// Win32Port class.
//
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

void PrintDCB(DCB &m_Dcb)
{

        printf(" BaundRate %d ", m_Dcb.BaudRate);
        printf("%d Bits ", m_Dcb.ByteSize);
        printf("%s StopBit ",(m_Dcb.StopBits == ONESTOPBIT ? "1":
                              m_Dcb.StopBits == TWOSTOPBITS ? "2":"1.5") );
 //       printf("binary %s ",m_Dcb.fBinary?"TRUE":"FALSE");
       printf("%s\n",(m_Dcb.Parity==EVENPARITY ? "EvenParity":
                      m_Dcb.Parity==ODDPARITY  ? "OddParity":
                      m_Dcb.Parity==MARKPARITY ? "MarkParity":
                      m_Dcb.Parity==SPACEPARITY? "SpaceParity":"NoParity"));

        printf("%s  %s  CtsFlow %d DsrFlow %d\n",
                m_Dcb.fDtrControl == DTR_CONTROL_DISABLE?"DTR_CONTROL_DISABLE":
                m_Dcb.fDtrControl == DTR_CONTROL_ENABLE?"DTR_CONTROL_ENABLE":
                m_Dcb.fDtrControl == DTR_CONTROL_HANDSHAKE?"DTR_CONTROL_HANDSHAKE":"Invalid DTR value",
                m_Dcb.fRtsControl == RTS_CONTROL_DISABLE?"RTS_CONTROL_DISABLE":
                m_Dcb.fRtsControl == RTS_CONTROL_ENABLE?"RTS_CONTROL_ENABLE":
                 m_Dcb.fRtsControl == RTS_CONTROL_HANDSHAKE?"RTS_CONTROL_HANDSHAKE":"RTS_CONTROL_TOGGLE",
                 m_Dcb.fOutxCtsFlow, m_Dcb.fOutxDsrFlow );
}
//
// The arguments to the Win32Port constructor are the same as the
// arguments passed to the constructurs for all of the other
// RS232 derived classes, with one noticeable difference. Instead
// of passing a numeric value such as COM1 or COM2, this guy
// expects a string, e.g. "COM2". This is necessary to accomodate
// two special cases. First, com ports greater than 9 must be
// opened with a special string that looks like this:
// "\\.\COM10". Second, some drivers could be using a completely
// different name, like "USB Port 1".
//
//
// The constructor has a member initialization list that creates
// the input and output queues at the size chosen by the
// enumerated value in the class definition. The MTQueue class
// limits insertions into the internal deque<char> member to the
// value specified in the constructor.
//
Win32Port::Win32Port( const string &port,
                      long baud_rate   /* = UNCHANGED */,
                      char parity      /* = UNCHANGED */,
                      int word_length  /* = UNCHANGED */,
                      int stop_bits    /* = UNCHANGED */,
                      int dtr          /* = SET       */,
                      int rts          /* = SET       */,
                      int xon_xoff     /* = DISABLE   */,
                      int rts_cts      /* = DISABLE   */,
                      int dtr_dsr      /* = DISABLE   */ )
    : m_TxQueue( MAX_OUTPUT_BUFFER_SIZE ),
      m_RxQueue( MAX_INPUT_BUFFER_SIZE )
{
//
// Win32Port has to share the debug output with the parent class.
// To determine where our first line starts, we call the
// FormatDebugOutput() function from our parent class.
//
    first_debug_output_line = RS232::FormatDebugOutput();
    debug_line_count = FormatDebugOutput();
    string temp = port;
//
// One of the base class members holds the port_name, which for
// most other class implemenations is a value ranging from COM1
// to COM9. Although not every port passed to this class conforms
// to that naming structure, most of them do. We try to extract
// that port name from the one passed in wherever possible. If no
// possible match is made, the value -1 is inserted into the
// port_name member. The only important place where the port_name
// member is used is in the dump status output routine of the
// base class. Initializing it here ensures that the value
// displayed in the debug output will usually match up with the
// value the calling program passed as a port name.
//
    if ( temp.substr( 0, 4 ) == "\\\\.\\" )
        temp = temp.substr( 4, string::npos );
    if ( toupper( temp[ 0 ] ) == 'C' &&
         toupper( temp[ 1 ] ) == 'O' &&
         toupper( temp[ 2 ] ) == 'M' ) {
        temp = temp.substr( 3, string::npos );
        port_name = (RS232PortName) ( atoi( temp.c_str() ) - 1 );
    } else
        port_name = (RS232PortName) -1;
    //
    // Here is where the real work starts. We open the port using the
    // flags appropriate to a serial port. Using FILE_FLAG_OVERLAPPED
    // is critical, because our input and output threads depend on
    // the asynchronous capabilities of Win32.
    //
    m_hPort = CreateFile( port.c_str(),
                          GENERIC_READ | GENERIC_WRITE,
                          0,
                          0,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                          0 );

    //
    // If I get a valid handle, it means the port could at least be
    // opened. I then try to set a bunch of miscellaneous things that
    // the port needs to run properly. If everything goes according to
    // plan, when that is all done I start the input and output threads
    // and the port is then really running.
    //
    if ( m_hPort != INVALID_HANDLE_VALUE ) {
        m_dwErrors = 0;            //Clear cumulative line status errors
        m_iBreakDuration = 0;    //No break in progress, initialize to 0
        SetLastError( 0 );        //Clear any Win32 error from this thread
        read_settings();           //Read and save current port settings
        saved_settings = settings; //Only needed because base class dumps
                                   //the saved settings in debug output
        //Init timeous to ensure our overlapped reads work
        COMMTIMEOUTS timeouts = { 0x01, 0, 0, 300, 0 };
        SetCommTimeouts( m_hPort, &timeouts );
        SetupComm( m_hPort, 500, 500 ); //set buffer sizes
        error_status = RS232_SUCCESS;     //clear current class error


// DCB is ready for use.

// Couldn't build the DCB. Usually a problem
// with the communications specification string.
          settings.Dtr = DTR_CONTROL_DISABLE;//ENABLE;                //Set these five values to their
        settings.Rts = RTS_CONTROL_DISABLE;//ENABLE;                //default values, the Adjust()
        settings.XonXoff = FALSE;            //function will modify them if
        settings.RtsCts = FALSE;            //new values were passed in the args
        settings.DtrDsr = FALSE;            //to the constructor

        settings.Adjust( baud_rate,
                         parity,
                         word_length,
                         stop_bits,
                         dtr,
                         rts,
                         xon_xoff,
                         rts_cts,
                         dtr_dsr );

        //
        // Now write the new settings to the port. If this or any other
        // operation has failed, we abort here. The user will be able
        // to see that the port is not working due to having an error
        // set immediately upon opening.
        //

        error_status = write_settings();


        if ( error_status != RS232_SUCCESS ) {
            CloseHandle( m_hPort );
            m_hPort = 0;

        } else {


            //
            // Since the port opened properly, we're ready to start the
            // input and output threads. Before they start we create the
            // five Win32 events that will be used to pass requests to
            // the threads. Note that the only argument passed to the
            // thread initialization is a pointer to this. The thread
            // needs that to find all of the data in the Win32Port
            // object that it will be manipulating.
            //
            m_hKillInputThreadEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
            m_hKillOutputThreadEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
            m_hWriteRequestEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
            m_hReadRequestEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
            m_hBreakRequestEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
            m_hInputThread = _beginthread( InputThread, 0, (void *) this );
            m_hOutputThread = _beginthread( OutputThread, 0, (void *) this );
        }
    }
    //
    // If the CreateFile() function didn't succeed, we're really in a bad
    // state. Just translate any error into something intellible and
    // exit. The user will have tofigure out that the open failed due to
    // the bad error_status member.
    //
    else
        translate_last_error();
}


//
// The destructur only has a couple of important things to do. If
// the port was up and running, the handle that was created as a
// result of the call to CreateFile() has to be closed with a
// call to CloseHandle(). The two worker threads created when the
// port was opened both have to be killed, which is done by
// setting an Event that the threads will be continually watching
// for. Once that is complete, the five events created in the
// ctor can be destroyed as well.
//
// Note that because of the way this dtor works, we are
// vulnerable to getting hung up if one of the two worker threads
// doesn't exit properly. This is a strong incentive towards
// keeping the code in the threads nice and simple.
//
Win32Port::~Win32Port()
{
    if ( m_hPort != INVALID_HANDLE_VALUE ) {
        SetEvent( m_hKillOutputThreadEvent );
        SetEvent( m_hKillInputThreadEvent );
        long handles[ 2 ] = { m_hInputThread, m_hOutputThread };
        WaitForMultipleObjects( 2,
                                (HANDLE *) handles,
                                TRUE,
                                INFINITE );
        CloseHandle( m_hPort );
        m_hPort = INVALID_HANDLE_VALUE;
        CloseHandle( m_hKillInputThreadEvent );
        CloseHandle( m_hKillOutputThreadEvent );
        CloseHandle( m_hWriteRequestEvent );
        CloseHandle( m_hReadRequestEvent );
        CloseHandle( m_hBreakRequestEvent );
    }
}

//
// The structure of the RS232 class called for a single global
// function that was called when the library was sitting in a
// loop waiting for input data to show up. Under Win32, there are
// a couple of different appropaches you might want to take when
// implementing this routine. If you are calling the routine from
// your main thread, you want to make sure that your GUI is
// getting some CPU cycles, so you probably want to replace this
// function with a version that operates your message pump. The
// routine might also check for a cancel flag that a user can set
// from inside your app.
//
// If you are calling this routine from a background thread, you
// could conceivably have it sleep for 5 or 10 milliseconds each
// time it is called. If you are actually idle and truly waiting
// for data to either arrive or be sent, it shouldn't cause any
// degradation of your app's throughput, and it will free up time
// for other processes and threads.
//
// Since this is a virtual function, you can override it in your
// derived class and use some object specific data to determine
// how it behaves.
//
int RS232::IdleFunction( void )
{
    return RS232_SUCCESS;
}

//
// The RS232 base class expects a global ReadTime() function that
// returns the current time in milliseconds. This matches up well
// with the GetTickCount() in the Windows API.
//
long ReadTime( void )
{
    return GetTickCount();
}

//
// This is our class specific implementation of the RS232 virtual
// function called Set(). It is called to set the four principal
// attributes of the serial port: baud rate, parity, word size,
// and number of stop bits. It relies on two internal functions
// to actually do all of the work. Member function Adjust() is
// called to accept the new settings and store them in the
// internal settings member. write_settings() is then called to
// take those values, make any modifications needed to the DCB,
// then call the Win32 API functions needed to update the
// physical values in the port. Illegal values can result in the
// error_status member being set to a bad value, which will
// prevent the port from working at all.
//
RS232Error Win32Port::Set( long baud_rate  /* = UNCHANGED */,
                           int parity      /* = UNCHANGED */,
                           int word_length /* = UNCHANGED */,
                           int stop_bits   /* = UNCHANGED */ )
{
    settings.Adjust( baud_rate,
                     parity,
                     word_length,
                     stop_bits,
                     UNCHANGED,
                     UNCHANGED,
                     UNCHANGED,
                     UNCHANGED,
                     UNCHANGED );
    return write_settings();

}

//
// This is an implementation of the virtual Dtr() function from
// the base class RS232. It doesn't have to do too much work, it
// merely updates the settings member and then calls the
// write_settings() member function to do all the work. It does
// check to see if DTR/DSR handshaking is in place, and if it is,
// it returns the non-fatal warning to the calling routine. If
// that doesn't happen, it updates the Dtr member in the settings
// object and calls write_settings() to update the physical port.
// Note that it returns the current setting of the Dtr member. If
// this function is called with no arguments by the end user, it
// doesn't do anything except return that value.
//
int Win32Port::Dtr( int setting /* = UNCHANGED */ )
{
    if ( error_status < RS232_SUCCESS )
        return error_status;
    if ( setting != UNCHANGED ) {
        if ( settings.DtrDsr == 1 )
            return WIN32_HANDSHAKE_LINE_IN_USE;
        settings.Dtr = setting != 0;
        RS232Error error = write_settings();
        if ( error < RS232_SUCCESS )
            return error;
    }
    return settings.Dtr;
}

//
// This is an implementation of the virtual Rts() function from
// the base class RS232. It doesn't have to do too much work, it
// merely updates the settings member and then calls the
// write_settings() member function to do all the work. It does
// check to see if RTS/CTS handshaking is in place, and if it is,
// it returns a non-fatal warning to the calling routine. If that
// doesn't happen, it updates the Rts member in the settings
// object and calls write_settings() to update the physical port.
// Note that it returns the current setting of the Rts member. If
// this function is called with no arguments by the end user, it
// doesn't do anything except return that value.
//

int Win32Port::Rts( int setting /* = UNCHANGED */ )
{
    if ( error_status < RS232_SUCCESS )
        return error_status;
    if ( setting != UNCHANGED ) {
        if ( settings.RtsCts == 1 )
            return WIN32_HANDSHAKE_LINE_IN_USE;
        settings.Rts = setting != 0;
        RS232Error error = write_settings();
        if ( error < RS232_SUCCESS )
            return error;
    }
    return settings.Rts;
}

//
// This is the local implementation of the RS232 member function
// DtrDsrHandshaking. All it has to do is set the member in the
// settings function, then rely on the write_settings() member to
// do all the work. Note that setting this handshaking type to be
// true means that you no longer have direct control over the DTR
// output line. Any attempt to modify DTR will be futile until
// this handshaking value is turned back off.
//
// After finishing its work, this function returns the setting of
// DTR/DSR handshaking for the port. Note that if the user calls
// it with no arguments, it will skip all the setting work and
// just return the value of the current setting.
//
int Win32Port::DtrDsrHandshaking( int setting /* = UNCHANGED */ )
{
    if ( error_status < RS232_SUCCESS )
        return error_status;
    if ( setting != UNCHANGED ) {
        settings.DtrDsr = setting != 0;
        RS232Error error = write_settings();
        if ( error < RS232_SUCCESS )
            return error;
    }
    return settings.DtrDsr;
}

//
// This is the local implementation of the RS232 member function
// RtsCtsHandshaking. All it has to do is set the member in the
// settings function, then rely on the write_settings() member to
// do all the work. Note that setting this handshaking type to be
// true means that you no longer have direct control over the RTS
// output line. Any attempt to modify RTS will be futile until
// this handshaking value is turned back off.
//
// After finishing its work, this function returns the setting of
// RTS/CTS handshaking for the port. Note that if the user calls
// it with no arguments, it will skip all the setting work and
// just return the value of the current setting.
//

int Win32Port::RtsCtsHandshaking( int setting )
{
    if ( error_status < RS232_SUCCESS )
        return error_status;
    if ( setting != UNCHANGED ) {
        settings.RtsCts = setting != 0;
        RS232Error error = write_settings();
        if ( error < RS232_SUCCESS )
            return error;
    }
    return settings.RtsCts;
}

//
// This member function is called to either check or modify the
// current software handshaking state of the port. It doesn't have
// to do any of the hard work, that is all taken care of by the
// write_settings() member of the class. The function returns
// the current state of software handshaking in the port. Note that
// if it is called with no argument, it will skip over all of the
// code that modifies the value and simply return the current state.
//
int Win32Port::XonXoffHandshaking( int setting /* = UNCHANGED */ )
{
    if ( error_status < RS232_SUCCESS )
        return error_status;
    if ( setting != UNCHANGED ) {
        settings.XonXoff = setting != 0;
        RS232Error error = write_settings();
        if ( error < RS232_SUCCESS )
            return error;
    }
    return settings.XonXoff;
}

//
// The following four functions implement the RS232 class
// member functions that return the values of the modem
// status lines. In this case all four functions are so similar
// that there is no point in documenting them individually.
//
// All of these functions simply look at a specific bit in the
// m_dwModemStatus word to see if their individual status line is
// set. The modem status word is read in every time we see a change
// in the status lines. We have set up WaitCommEvent so that it
// should signal an event every time one of the status lines
// changes.
//
// Note that you can set up a derived class so that your app is notified
// every time one of the lines change. This is particularly valuable
// for monitoring the RI line, as it may be difficult to catch an
// incoming ring in progress.
//

int Win32Port::Cd()
{
    return ( MS_RLSD_ON & m_dwModemStatus ) != 0;
}

int Win32Port::Cts()
{
    return ( MS_CTS_ON & m_dwModemStatus ) != 0;
}

int Win32Port::Dsr()
{
    return ( MS_DSR_ON & m_dwModemStatus ) != 0;
}

int Win32Port::Ri()
{
    return ( MS_RING_ON & m_dwModemStatus ) != 0;
}

//
// Just like the four modem status routines, the four line status
// error routines all take the same form. Each of the checks the
// m_dwErrors for their specific error bit, returning true if the
// bit is set and false if it is not. If the caller invoked the function
// with the clearing option, the bit in the cumulative line status
// error word will be cleared.
//
int Win32Port::ParityError( int clear /* = UNCHANGED */ )
{
    int return_value;

    if ( error_status < RS232_SUCCESS )
        return error_status;
    return_value = ( m_dwErrors & CE_RXPARITY ) != 0;
    if ( clear != UNCHANGED && clear != 0 )
        m_dwErrors &= ~CE_RXPARITY;
    return return_value;
}

int Win32Port::FramingError( int clear /* = UNCHANGED */ )
{
    int return_value;

    if ( error_status < RS232_SUCCESS )
        return error_status;
    return_value = ( m_dwErrors & CE_FRAME ) != 0;
    if ( clear != UNCHANGED && clear != 0 )
        m_dwErrors &= ~CE_FRAME;
    return return_value;
}

int Win32Port::HardwareOverrunError( int clear /* = UNCHANGED */ )
{
    int return_value;

    if ( error_status < RS232_SUCCESS )
        return error_status;
    return_value = ( m_dwErrors & CE_OVERRUN ) != 0;
    if ( clear != UNCHANGED && clear != 0 )
        m_dwErrors &= ~CE_OVERRUN;
    return return_value;
}

int Win32Port::BreakDetect( int clear /* = UNCHANGED */ )
{
    int return_value;

    if ( error_status < RS232_SUCCESS )
        return error_status;
    return_value = ( m_dwErrors & CE_BREAK ) != 0;
    if ( clear != UNCHANGED && clear != 0 )
        m_dwErrors &= ~CE_BREAK;
    return return_value;
}

//
// This routine acts just like the previous four. The only
// difference is the type of error being handled. The software
// overrun error is set when the driver receives a character
// but doesn't have room to store it. Unfortunately, there isn't
// an EV_XXXX routine to force this error to generate an event
// when it happens, so we end up checking the comm status
// after every input packet.
//
int Win32Port::SoftwareOverrunError( int clear /* = UNCHANGED */ )
{
    int return_value;

    if ( error_status < RS232_SUCCESS )
        return error_status;
    return_value = ( m_dwErrors & CE_RXOVER ) != 0;
    if ( clear != UNCHANGED && clear != 0 )
        m_dwErrors &= ~CE_RXOVER;
    return return_value;
}

//
// This function is called to provoke the output thread into sending
// a break of a specified duration. We haven't set up any path by
// which to set the duration of the break, instead we just stored
// the desired duration in the object, and count on the output thread
// checking there to see what it should be. The output thread is
// continually checking for the m_hBreakRequestEvent, and when it sees
// it, it faithfully sends the break.
//
int Win32Port::Break( long milliseconds )
{
    if ( milliseconds > 1000 )
        m_iBreakDuration = 1000;
    else
        m_iBreakDuration = milliseconds;
    SetEvent( m_hBreakRequestEvent );
    return RS232_SUCCESS;
}

//
// The Peek() function is fairly close to read_buffer(). It takes
// whatever it can get out the input buffer, and returns a count
// of bytes read in ByteCount.
//
int Win32Port::Peek( void *buffer, unsigned int count )
{
    if ( error_status < RS232_SUCCESS )
        return error_status;
    ByteCount = m_RxQueue.Peek( (char *) buffer, count );
    ( (char *) buffer )[ ByteCount ] = '\0';
    return RS232_SUCCESS;
}


//
// The two FlushXxBuffer routines do the same things. Both calls
// have to take care to clear both the internal buffers used by
// the driver, plus the external buffers we keep in the class. One
// is done by a simple member of class MTQueue, the other by using
// a Win32 API call.
//
int Win32Port::FlushRXBuffer()
{
    if ( error_status < RS232_SUCCESS )
        return error_status;
    m_RxQueue.Clear();
    PurgeComm( m_hPort, PURGE_RXCLEAR );
    if ( !m_bInputThreadReading )
        SetEvent( m_hReadRequestEvent );
    return RS232_SUCCESS;
}

int Win32Port::FlushTXBuffer()
{
    if ( error_status < RS232_SUCCESS )
        return error_status;
    m_TxQueue.Clear();
    PurgeComm( m_hPort, PURGE_TXCLEAR );
    return RS232_SUCCESS;
}

//
// Since we have a few error codes that are unique to the Win32Port class,
// it makes sense to translate them ourselves when requested. That's what
// this function does. Note that most of the time it passes the job up to
// the base class.
//
char *Win32Port::ErrorName( int error )
{
    if ( error < RS232_NEXT_FREE_ERROR && error >= RS232_ERROR )
        return RS232::ErrorName( error );
    if ( error < RS232_NEXT_FREE_WARNING && error >= RS232_WARNING )
        return RS232::ErrorName( error );
    if ( error >= RS232_SUCCESS )
        return RS232::ErrorName( error );
    switch ( error ) {
        case WIN32_CHECK_WINDOWS_ERROR :
            return "Check Windows error code in m_dwWindowsError";
        case WIN32_SETTINGS_FAILURE :
            return "Failure to set port parameters";
        case WIN32_HANDSHAKE_LINE_IN_USE :
            return "Handshake line is already in use";
        default :
            return( "Undefined error" );
    }
}

//
// The last of our locally implemented versions of base class
// functions is FormatDebugOutput(). It should be fairly easy
// to figure out, it simply creates a line of output upon request.
// Note that it has a helper functions that is used to format single
// characters for display in a nice usable format.
//

static string DisplayChar( int val ) {
    char temp[ 2 ] = { val, 0 };
    if ( val >= ' ' && val <= 0x73 )
        return string( temp );
    ostringstream s;
    s << "0x" << setw( 2 ) << setfill( '0' ) << hex << val;
    return s.str();
}

int Win32Port::FormatDebugOutput( char *buffer, int line_number )
{
    char *StopBits[ 4 ] = {
        "1",
        "1.5",
        "2",
        "???"
    };
    char *DtrControl[ 4 ] = {
        "DISABLE",
        "ENABLE",
        "HANDSHAKE",
        "???"
    };
    char *RtsControl[ 4 ] = {
        "DISABLE",
        "ENABLE",
        "HANDSHAKE",
        "TOGGLE"
    };
    if ( buffer == 0 )
        return( first_debug_output_line +  9 );
    if ( line_number < first_debug_output_line )
        return RS232::FormatDebugOutput( buffer, line_number );
    switch ( line_number - first_debug_output_line ) {
    case 0 :
        sprintf( buffer,
                 "DCB-> Baud: %6d  "
                 "fBinary: %1d  "
                 "fParity: %1d  "
                 "fOutxCtsFlow: %1d  "
                 "fOutxDsrFlow: %1d",
                 m_Dcb.BaudRate,
                 m_Dcb.fBinary,
                 m_Dcb.fParity,
                 m_Dcb.fOutxCtsFlow,
                 m_Dcb.fOutxDsrFlow );
        break;
    case 1 :
        sprintf( buffer,
                 "DCB-> fDtrControl: %9s  "
                 "fDsrSensitivity: %1d  "
                 "fTXContinueOnXoff: %1d  ",
                 DtrControl[ m_Dcb.fDtrControl ],
                 m_Dcb.fDsrSensitivity,
                 m_Dcb.fTXContinueOnXoff );
        break;
    case 2 :
        sprintf( buffer,
                 "DCB-> fOutX: %1d  "
                 "fInX: %1d  "
                 "fErrorChar: %1d  "
                 "fNull: %1d  "
                 "fRtsControl: %9s",
                 m_Dcb.fOutX,
                 m_Dcb.fInX,
                 m_Dcb.fErrorChar,
                 m_Dcb.fNull,
                 RtsControl[ m_Dcb.fRtsControl ] );
        break;
    case 3 :
        sprintf( buffer,
                 "DCB-> fAbortOnError: %1d  "
                 "XonLim: %4d  "
                 "XoffLim: %4d  "
                 "ByteSize: %1d  "
                 "Parity: %1c ",
                 m_Dcb.fAbortOnError,
                 m_Dcb.XonLim,
                 m_Dcb.XoffLim,
                 m_Dcb.ByteSize,
                 "NOEMS"[ m_Dcb.Parity] );
        break;
    case 4 :
        sprintf( buffer,
                 "DCB-> StopBits: %3s  "
                 "XonChar: %s  "
                 "XoffChar: %s  "
                 "ErrorChar: %s  ",
                 StopBits[ m_Dcb.StopBits & 3 ],
                 DisplayChar( m_Dcb.XonChar ).c_str(),
                 DisplayChar( m_Dcb.XoffChar ).c_str(),
                 DisplayChar( m_Dcb.ErrorChar ).c_str() );
        break;
    case 5 :
        sprintf( buffer,
                 "DCB-> EofChar: %s  "
                 "EvtChar: %s  ",
                 DisplayChar( m_Dcb.EofChar ).c_str(),
                 DisplayChar( m_Dcb.EvtChar ).c_str() );
        break;
    case 6 :
    {
        COMSTAT comstat;
        clear_error( &comstat );
        sprintf( buffer,
                 "COMSTAT-> fCtsHold: %1d  "
                 "fDsrHold: %1d  "
                 "fRlsHold: %1d  "
                 "fXoffHold: %1d  "
                 "fXoffSent: %1d",
                 comstat.fCtsHold,
                 comstat.fDsrHold,
                 comstat.fRlsdHold,
                 comstat.fXoffHold,
                 comstat.fXoffSent );
        break;
    }
    case 7 :
    {
        COMSTAT comstat;
        clear_error( &comstat );
        sprintf( buffer,
                 "COMSTAT-> fEof: %1d  "
                 "fTxim: %1d  "
                 "cbInQueue: %5d  "
                 "cbOutQueue: %5d",
                 comstat.fEof,
                 comstat.fTxim,
                 comstat.cbInQue,
                 comstat.cbOutQue );
        break;
    }
    case 8 :
        sprintf( buffer,
                 "CE_-> BREAK: %1d  "
                 "FRAME: %1d  "
                 "OVERRUN: %1d  "
                 "RXOVER: %1d  "
                 "RXPARITY: %1d  "
                 "TXFULL: %1d",
                 ( m_dwErrors & CE_BREAK ) ? 1 : 0,
                 ( m_dwErrors & CE_FRAME ) ? 1 : 0,
                 ( m_dwErrors & CE_OVERRUN ) ? 1 : 0,
                 ( m_dwErrors & CE_RXOVER ) ? 1 : 0,
                 ( m_dwErrors & CE_RXPARITY ) ? 1 : 0,
                 ( m_dwErrors & CE_TXFULL ) ? 1 : 0 );
        break;
    default :
        return RS232_ILLEGAL_LINE_NUMBER;
    }
    return RS232_SUCCESS;
}

//
// write_byte() is a virtual function declared by class RS232 as a
// pure virtual function. Classes derived from RS232 must implement
// this function to work with their particular hardware setup. In our
// case, that simply means checking to see if there is room in the
// output queue, and if there is, adding a byte to it. We set the
// event flag that kick starts the output thread just in case it was
// idle, then return our status. Note that if the port is already in
// an error condition, this routine refuses to do anything except
// return the same error. If the buffer is full, we return the
// non-fatal warning error RS232_TIMEOUT.
//

int Win32Port::write_byte( int c )
{
    if ( error_status < 0 )
        return error_status;
    if ( m_TxQueue.SpaceFree() < 0 )
        return RS232_TIMEOUT;
    m_TxQueue.Insert( c );
    ::SetEvent( m_hWriteRequestEvent );
    return RS232_SUCCESS;
}

//
// write_buffer() is another one of the virtual routines that is declared
// by RS232, the base class, as pure virtual. That means it is our
// responsibility to implement it as best we can in this derived class.
// Writing data to our device is easy, we just have to load it into
// the output queue and set the event that wakes him up. The actual
// number of bytes transferred to the output thread is passed back in
// member ByteCount. If not all bytes could fit in the output buffer,
// the non-fatal warning message RS232_TIMEOUT is passed back to
// the caller.
//

int Win32Port::write_buffer( char *buffer, unsigned int count )
{
    ByteCount = 0;
    if ( error_status < 0 )
        return error_status;
    ByteCount = m_TxQueue.Insert( buffer, count );
    ::SetEvent( m_hWriteRequestEvent );
    if ( ByteCount == count )
        return RS232_SUCCESS;
    else
        return RS232_TIMEOUT;
}

//
// read_byte() is one of the functions that is declared as pure virtual
// in base class RS232. We have to implement a specific version for
// our class that knows how to talk to our hardware. In our case,
// we don't have to talk to the hardware or even the driver. If any
// data has arrived, it has been stuffed into the input queue. We
// get a byte if we can, which will be returned to the caller. If no
// byte is available, the caller receives the non-fatal warning
// message RS232_TIMEOUT.
//
// One slightly tricky bit in this code is seen where we test to
// see if the input thread is currently reading. If the input thread
// runs out of space in its input buffer, it has to stop reading.
// When we read a new character, we just might free up some space
// so it can start reading again. We aren't going to make that decision
// for the input port, but we do set his event to let him know that
// some space might have freed up.
//
int Win32Port::read_byte( void )
{
    if ( error_status < 0 )
        return error_status;
    int ret_val = m_RxQueue.Extract();
    if ( !m_bInputThreadReading )
        SetEvent( m_hReadRequestEvent );
    if ( ret_val < 0 )
        return RS232_TIMEOUT;
    else
        return ret_val;
}

//
// read_buffer is one of the support routines declared as pure virtual
// in the base class. This means that as a derived class we are
// responsible for implementing a version that talks to our specific
// hardware or driver. Our implementation here doesn't talk directly
// to the driver, we leave that to the input thread. We simply try
// to take all the characters we can from the input buffer, returning
// the actual count we got in the ByteCount member. If we got every
// character we asked for, we return RS232_SUCCESS. If we didn't
// get everything we asked for, we return the non-fatal warning
// message RS232_TIMEOUT.
//
//
// One slightly tricky bit in this code is seen where we test to
// see if the input thread is currently reading. If the input thread
// runs out of space in its input buffer, it has to stop reading.
// When we read a new character, we just might free up some space
// so it can start reading again. We aren't going to make that decision
// for the input port, but we do set his event to let him know that
// some space might have freed up.
//
int Win32Port::read_buffer( char *buffer, unsigned int count )
{
    ByteCount = 0;
    if ( error_status < 0 )
        return error_status;
    ByteCount = m_RxQueue.Extract( buffer, count );
    buffer[ ByteCount ] = '\0';
    if ( !m_bInputThreadReading )
        SetEvent( m_hReadRequestEvent );
    if ( ByteCount < count )
        return RS232_TIMEOUT;
    else
        return RS232_SUCCESS;
}

//
// clear_error() is an internal support routine that is called in
// response to the reception of an asynchronous error event. It
// first clears the comm error flag so that normal operation of
// the port can resume. It ORs the newly received error flags with
// the ones we have already seen in m_dwErrors. Finally, it calls
// a notification function for any incoming errors that have just occurred.
//
// One potential trouble spot in this function call is that it is
// called from both the FormatDebugOutput() routine as well as inside
// the receive thread. This opens the window for at least the
// possibility of missing an incoming error if FormatDebugOutput()
// is called while an incoming error is seen. This could be guarded
// against by adding a critical section, but since FormatDebugOutput()
// is usually only called during testing and diagnostics, I didn't
// add it to this routine.
//

void Win32Port::clear_error( COMSTAT *comstat /* = 0 */ )
{
    COMSTAT c;
    if ( comstat == 0 )
        comstat = &c;
    DWORD errors;
    ClearCommError( m_hPort, &errors, comstat );
    m_dwErrors |= errors;
    if ( errors & CE_BREAK )
        BreakDetectNotify();
    if ( errors & CE_FRAME )
        FramingErrorNotify();
    if ( errors & CE_OVERRUN )
        HardwareOverrunErrorNotify();
    if ( errors & CE_RXPARITY )
        ParityErrorNotify();
    if ( errors & CE_RXOVER )
        SoftwareOverrunErrorNotify();
}

//
// write_settings() is an internal support routine that is only used
// in Win32Port(). Its job is to write the data in the settings member
// to the DCB being used for this port. The settings member has the
// four commonly used settings: baud rate, word size, parity, and
// number of stop bits. It also has current settings for the three
// different types of handshaking, plus the settings for the desired
// output of DTR and RTS. Once all of these things are punched into
// the DCB, a call to SetCommState() will take care of updating
// the physical device.
//
// There are a lot of things that can go wrong when attempting to set
// the port. We don't consider any of these things to be fatal errors,
// we simply pass the error back to the calling routine. However, the
// calling routine might decide to treat it as a fatal error. The
// constructor does this, treating an error at this point as fatal.
//
// Note also that this routine relies heavily on the DCB support class.
// It takes care of writing the correct values to its members by means
// of a bunch of accessor functions.
//

RS232Error Win32Port::write_settings()
{
    RS232Error error = RS232_SUCCESS;
    m_Dcb.SetBaudRate( settings.BaudRate );
    m_Dcb.SetParity( settings.Parity, error );
    m_Dcb.SetWordLength( settings.WordLength, error );
    m_Dcb.SetStopBits( settings.StopBits, error );
    //
    // Even though we think that we're setting up DTR and RTS,
    // we might not actually be pulling it off. If one of the
    // two hardware handshaking protocols is enabled, it will
    // wipe out the DCB setting for the corresponding control
    // line and change it to use handshaking instead.
    //
    m_Dcb.SetDtr(settings.Dtr );
    m_Dcb.SetRts( settings.Rts );
    m_Dcb.SetXonXoff( settings.XonXoff );
    m_Dcb.SetRtsCts( settings.RtsCts );
    m_Dcb.SetDtrDsr( settings.DtrDsr );

 ///*****************
 //    PrintDCB(m_Dcb);
 /// *******************


    SetCommState( m_hPort, &m_Dcb );
    if ( GetLastError() != 0 ) {
        if ( GetLastError() == ERROR_INVALID_HANDLE )
            return (RS232Error) WIN32_SETTINGS_FAILURE;
        else {
            m_dwWindowsError = GetLastError();
            return (RS232Error) WIN32_CHECK_WINDOWS_ERROR;
        }
    }
    return error;
}

//
// read_settings() is an internal support routine only used by member
// functions of Win32Port. It is actually only used in one place: the
// constructor. It is responsible for reading the settings from the
// port as soon as we open it, giving us a baseline to start with.
// If necessary, this means that we can open the port in the default
// state that the system wants it to be opened in.
//
// Note that I didn't add accessor functions to the DCB to pull these
// settings out, we just look directly at the DCB members to get them.
// It would have probably been better to add translation routines, but
// it is not really an important issue.
//
void Win32Port::read_settings()
{
    DCB dcb;
    GetCommState( m_hPort, &dcb );
 ///   PrintDCB(dcb);
    settings.BaudRate = dcb.BaudRate;
    if ( !dcb.fParity )
        settings.Parity = 'N';
    else
        switch ( dcb.Parity ) {
        case EVENPARITY  : settings.Parity = 'E'; break;
        case ODDPARITY   : settings.Parity = 'O'; break;
        case MARKPARITY  : settings.Parity = 'M'; break;
        case SPACEPARITY : settings.Parity = 'S'; break;
        default          : settings.Parity = 'N'; break;
        }
    settings.WordLength = dcb.ByteSize;
    if ( dcb.StopBits == ONESTOPBIT )
        settings.StopBits = 1;
    else
        settings.StopBits = 2;
    if ( dcb.fDtrControl == DTR_CONTROL_DISABLE )
        settings.Dtr = 0;
    else
        settings.Dtr = 1;
    if ( dcb.fRtsControl == RTS_CONTROL_DISABLE )
        settings.Rts = 0;
    else
        settings.Rts = 1;
    if ( dcb.fOutX || dcb.fInX )
        settings.XonXoff = 1;
    else
        settings.XonXoff = 0;
    if ( dcb.fOutxCtsFlow || dcb.fRtsControl == RTS_CONTROL_HANDSHAKE )
        settings.RtsCts = 1;
    else
        settings.RtsCts = 0;
    if ( dcb.fOutxDsrFlow || dcb.fDtrControl == DTR_CONTROL_HANDSHAKE )
        settings.DtrDsr = 1;
    else
        settings.DtrDsr = 0;
}

//
// translate_last_error() is called any time an error occurs when
// a Win32 API function is called. We know how to translate a couple
// of Win32 errors to native versions, but for the most part we just
// pass the responsibility for interpretation on to the caller of the
// Win32Port function. The value we see is stored in the m_dwWindowsError
// member so that it can be checked even in the presence of other
// intervening problems.
//

RS232Error Win32Port::translate_last_error()
{
    switch ( m_dwWindowsError = GetLastError() )
    {
    case ERROR_ACCESS_DENIED  : return error_status = RS232_PORT_IN_USE;
    case ERROR_FILE_NOT_FOUND : return error_status = RS232_PORT_NOT_FOUND;
    }
    return error_status = (RS232Error) WIN32_CHECK_WINDOWS_ERROR;
}

//
// InputThread is the function that runs as a separate thread that is
// responsible for reading all input data and status information. It is
// a fairly complex thread, which makes it a bit harder to read than
// it should be. Outside of setup and teardown, the routine sits in
// a giant loop that basically performs two functions. First, it makes
// sure that it is properly set up to be notified when either incoming
// data or status messages arrive. Status messages consist of line status
// errors and modem status changes. (Note that we don't try to read if
// there is no room for data in the input buffer.) Once that is set up,
// we simply wait for one of four potential events to be signaled. The
// events are 1) a kill message fromt the main thread, which comes when
// the port is being closed, 2) incoming data that has been read in from
// the serial port, 3) a line status error or modem status change on the
// serial port, and 4) a read request message, which indicates that some
// room may have been opened up in the input buffer.
//
// The rest of the work in the routine is devoted to figuring out what
// to do in response to those incoming events.
//

void Win32Port::InputThread( void * arglist )
{
    //
    // The thread is passed a pointer to the Win32Port object when it is
    // started up. We have to cast that back to a Win32Port pointer,
    // because of the way Win32 starts a thread function. Since this is
    // a static member function of the class, that gives us carte blanche
    // to access all the protected members of the class. It also means we
    // don't have to mess with creation or management of thread-specific
    // data, as anything we need will be in the object itself.
    //
    Win32Port *port = (Win32Port *) arglist;
    //
    // We call these two functions once when we start up so that all of
    // the initial settings in the modem status and line status words
    // are initialized properly. This also guarantees that the notification
    // functions for the modem status will be called once with the initial
    // values, which will often be a useful thing for the calling program.
    //

    port->check_modem_status( true, 0 );
    port->clear_error();
    //
    // We need OVERLAPPED structures for both of our overlapped
    // functions in this thread: the data read called with ReadFile(),
    // and the status read called with WaitCommEvent. We could have included
    // these in the Win32Port object, but since they aren't used
    // anywhere else, it seemed better to confine them to being automatic
    // objects. Each of these objects gets an event that we create here
    // as well, they are the events used to signal us when we are
    // waiting in the big loop.
    //
    OVERLAPPED AsyncReadInfo = { 0 };
    OVERLAPPED AsyncStatusInfo = { 0 };
    AsyncReadInfo.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    AsyncStatusInfo.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    assert( AsyncReadInfo.hEvent );
    assert( AsyncStatusInfo.hEvent );
    //
    // This word is used as an argument to WaitForCommEvent()
    //
    DWORD dwCommEvent;
    //
    //  Some initialization
    //
    bool waiting_on_status = false;
    port->m_bInputThreadReading = false;
    //
    // This array holds the four event handles that are used to
    // signal this thread. On each pass through the main loop we
    // will be waiting for one of the four to go to the signal
    // state, at which time we take action on it.
    //
    HANDLE handles[ 4 ] = { port->m_hKillInputThreadEvent,
                            AsyncReadInfo.hEvent,
                            AsyncStatusInfo.hEvent,
                            port->m_hReadRequestEvent };

    //
    // We set all of the conditions that we will be waiting for.
    // Note the inclusion of EV_RINGTE, which is an undocumented
    // flag that we define in Win32Port.h.
    //
    SetCommMask( port->m_hPort,
                 EV_BREAK | EV_CTS  | EV_DSR   | EV_RXCHAR |
                 EV_ERR   | EV_RING | EV_RLSD  | EV_RINGTE );
    //
    // This is the main loop. It executes until the done flag is set,
    // which won't happen until the kill thread message is sent.
    // The first part of the loop sets up the read actions, the second
    // part waits for something to happen, and the final part of the
    // loop deals with whatever happened.
    //
    for ( bool done = false ; !done ; ) {
        //
        // Under normal conditions this loop should have a read action
        // in progress at all times. The only time this won't be true
        // is when there is no room in the RX queue. We have a member
        // in class Win32Port that defines whether or not a read is
        // presently active. This section of code just makes sure that
        // if no read is currently in progress, we do our best to get
        // one started.
        //
        int bytes_to_read = 0;
        char read_buffer[ 256 ];
        DWORD dwBytesRead;
        if ( !port->m_bInputThreadReading ) {
            bytes_to_read = port->m_RxQueue.SpaceFree();
            if ( bytes_to_read > 256 )
                bytes_to_read = 256;
            //
            // If there is room to add new bytes to the RX queue, and
            // we currently aren't reading anything, we kick off the
            // read right here with a call to ReadFile(). There are two
            // possible things that can then happen. If there isn't any
            // data in the buffer, ReadFile() can return immediately
            // with the actual input in progress but not complete. If
            // there was enough data in the input stream already to
            // fulfill the read, it might return with data present.
            //
            if ( bytes_to_read > 0 ) {
                if ( !ReadFile( port->m_hPort, read_buffer, bytes_to_read, &dwBytesRead, &AsyncReadInfo ) )
                {
                    // The only acceptable error condition is the I/O
                    // pending error, which isn't really an error, it
                    // just means the read has been deferred and will
                    // be performed using overlapped I/O.

                    port->m_bInputThreadReading = true;
                } else {
                    // If we reach this point, ReadFile() returned
                    // immediately, presumably because it was able
                    // to fill the I/O request. I put all of the bytes
                    // just read into the RX queue, then call the
                    // notification routine that should alert the caller
                    // to the fact that some data has arrive.
                    if ( dwBytesRead ) {
 //                       printf(":%d ",dwBytesRead);
                        port->m_RxQueue.Insert( read_buffer, dwBytesRead );
                        port->RxNotify( dwBytesRead );
                    }
                }
            } else {
                // If we reach this point, it means there is no room in
                // the RX queue. We reset the read event (just in case)
                // and go on to the rest of the code in this loop.
                ResetEvent( AsyncReadInfo.hEvent );
            }
        }
        //
        // Unlike the read event, we will unconditionally always have
        // a status even read in progress. The flag waiting_on_status
        // shows us whether or not we are currently waiting. If not,
        // we have to make a call to WaitCommEvent() so that one gets
        // kicked off.
        //
        if ( !waiting_on_status  ) {
            if ( !WaitCommEvent( port->m_hPort,
                                 &dwCommEvent,
                                 &AsyncStatusInfo ) ) {
                // WaitCommEvent() can return immediately if there are
                // status events queued up and waiting for us to read.
                // But normally it should return with an error code of
                // ERROR_IO_PENDING, which means that no events are
                // currently queued, and we will have to wait for
                // something noteworthy to happen.
                waiting_on_status = true;
            } else {
                // If we reach this point it means that WaitCommEvent()
                // returned immediately, so either a line status error
                // or a modem line state change has occurred. These two
                // routines are called to deal with all those possibilities.
                // The event bits are in dwCommEvent, which was passed to
                // WaitCommEvent() when we called it. The first of these
                // two functions handles all changes in modem status lines,
                // the second deals with line status errors.
                port->check_modem_status( false, dwCommEvent );
                port->clear_error();
            }
        }
        //
        // We've completed the preliminary part of the loop and we are
        // now read to wait for something to happen. Note that it is
        // possible that either the call to ReadFile() or
        // WaitCommEvent() returned immediately, in which case we aren't
        // actively waiting for data of that event type. If that's true,
        // we have to go back through the loop and try to set up the
        // ReadFile() or WaitCommEvent() again. That's what this first
        // conditional statement is checking for. It would be a simpler
        // statement, but we have to take into account the possibility
        // that we aren't reading because there is no room in the
        // RX queue, in which case we can wait right away.
        if ( waiting_on_status &&
           ( port->m_bInputThreadReading || bytes_to_read == 0 ) ) {
            DWORD result = WaitForMultipleObjects( 4,
                                                   handles,
                                                   FALSE,
                                                   INFINITE );
             //
            // We can return from the wait call with one of four possible
            // results. 0-3 mean that one of the event handles in the
            // array defined above was set by some process. The other is
            // a timeout. If you are nervous about waiting forever, you can
            // put a 10 or 20 second timeout on this wait, and print a TRACE
            // message every time the wait times out. This gives you a little
            // bit of a warm feeling that lets you know things are really
            // working.
            //
            switch ( result ) {
            // If the first event handle in the array was set, it means
            // the main thread of the program wants to close the port.
            // It sets the kill event, and we in turn set the done flag.
            // We will then exit from the bottom of the loop.
            case 0 : // kill thread event
                done = true;
                break;
            // This case is selected if an overlapped read of data has
            // signalled that it is complete. If the ReadFile() call
            // completed because it passed in some data, we stuff that
            // data into the RX queue and call the notification function
            // to alert the user's process.
            case 1 :
                if ( GetOverlappedResult( port->m_hPort,
                                          &AsyncReadInfo,
                                          &dwBytesRead,
                                          FALSE ) )
                {      // read completed successfully
                    if ( dwBytesRead ) {
  //                      printf(",%d ",dwBytesRead);
                        port->m_RxQueue.Insert( read_buffer, dwBytesRead );
                        port->RxNotify( dwBytesRead );
                    }
                }
                // Since the last ReadFile() operation completed, we are
                // no longer reading data. We set this flag to false so
                // that we can kick off a new read at the top of the loop.
                port->m_bInputThreadReading = false;
                break;
            // When we reach case 2, it means that the WaitCommEvent()
            // call completed, which means that one of the line status
            // or modem status events has been triggered. We don't
            // know which one it is, so we call the handler for both
            // possibilities, allowing them to decide if something has
            // happened, and who to notify about it.
            case 2 : { /* Status event */
                DWORD dwOverlappedResult;
                if ( GetOverlappedResult( port->m_hPort,
                                          &AsyncStatusInfo,
                                          &dwOverlappedResult,
                                          FALSE ) )
                {
                    port->check_modem_status( false, dwCommEvent );
                    port->clear_error();
                }
                // Clear this flag so that a new call to WaitCommEvent()
                // can be made at the top of the loop.
                waiting_on_status = false;
                break;
            }
            // When we reach case 3, it means that another thread read
            // some data from the RX queue, opening up some room, and
            // noticed that we weren't actively reading data. When this
            // happens, we need to wake up and start a new call to
            // ReadFile() so that we can fill up all that empty
            // space in the RX queue.
            case 3 :
                break;
            default :
                assert( false );
            }
        }
    }
    //
    // When the input thread has decided to exit, it first kills the
    // two events it created, then exits via a return statement.
    CloseHandle( AsyncReadInfo.hEvent );
    CloseHandle( AsyncStatusInfo.hEvent );
}

//
// The output thread is very similar in structure to the input thread,
// with a few notable differences. Instead of including the wait
// for output data as part of its main loop, the output thread calls
// a subroutine to actually perform the data output. And that output
// function runs to completion, so we will never wait for output to
// be complete in the main body of the thread shown here. It would
// probably be a useful modification of this thread to change it so
// that pending output is waited for in the main thread.
//
void Win32Port::OutputThread(void * arglist)
{
    //
    // As was the case for the input thread, we get a pointer to the
    // Win32Port object passed to us from the calling routine. We just
    // have to cast the pointer, then we have full access to all members
    // ofthe port's structure.
    //
    Win32Port *port = (Win32Port *) arglist;
    // The array of handles I wait for in the main loop has one
    // fewer element than the same array in the input thread.
    // We're only waiting for three things here: a request to send
    // more data via WriteFile(), a request to kill the thread, or
    // a request to send a break.
    HANDLE handles[ 3 ] = { port->m_hKillOutputThreadEvent,
                            port->m_hWriteRequestEvent,
                            port->m_hBreakRequestEvent };

    port->TxNotify();
    for ( bool done = false ; !done ; ) {
        switch ( WaitForMultipleObjects( 3, handles, FALSE, INFINITE ) ) {
        //
        // There are three possible returns from the call that waits
        // for something to happens: the three events and a timeout.
        // The first case means that a thread has attempted to close
        // the port, which results in the kill thread event being
        // set. When that's the case, we modify the control flag for
        // the loop so that we'll all out of the loop when we reach
        // the bottom.
        //
        case 0 : //m_hKillOutputThreadEvent
            done = true;
            break;
        //
        // Much of the time this loop will be sitting here doing nothing.
        // When an output request comes through, the appropriate event
        // is set and we end up here. We then call the output worker
        // routine to actually dump the output through the serial port.
        //
        case 1 : //m_hWriteRequestEvent
            done = port->output_worker();
            break;
        //
        // If the break event is set, it means we are being requested
        // to send a break out through the given port. The duration
        // of the break has already been set in a member of the object,
        // so all we have to do here is make it happen. Since this is
        // a background thread with no GUI, it's safe to just sit in
        // a sleep call for the duration of the break.
        //
        case 2 : //m_hBreakRequestEvent
            SetCommBreak( port->m_hPort );
            SleepEx( port->m_iBreakDuration, FALSE );
            ClearCommBreak( port->m_hPort );
            break;
        //
        // This can only be bad!
        //
        default :
            assert( false );
            break;
        }
    }

}

//
// When a request comes in to transmit some data, this routine is called.
// It starts things up by calling WriteFile(), then waits for the
// asynchronous I/O to complete. Note that while it is waiting for
// WriteFile() to complete it can also be notified of a kill thread
// event, in which case it returns immediately.
//

bool Win32Port::output_worker()
{
    OVERLAPPED AsyncWriteInfo = { 0 };
    AsyncWriteInfo.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    assert( AsyncWriteInfo.hEvent );
    HANDLE handles[ 2 ] = { m_hKillOutputThreadEvent,
                            AsyncWriteInfo.hEvent };
    bool killed = false;
    for ( bool done = false ; !done ; ) {
        //
        // First get as much data from the output buffer as we can
        //
        char data[ 500 ];
        int count = m_TxQueue.Extract( data, 500 );
        if ( count == 0 ) {
            TxNotify();
            break;
        }
        //
        // Now we enter the transmit loop, where the data will actually
        // be sent out the serial port.
        //
        DWORD result_count;
        if ( !WriteFile( m_hPort,
                         data,
                         count,
                         &result_count,
                         &AsyncWriteInfo ) ) {
            //
            // WriteFile() returned an error. If the error tells us that the I/O
            // operation didn't complete, that's okay.
            //
            if ( GetLastError() == ERROR_IO_PENDING ) {
                switch ( WaitForMultipleObjects( 2, handles, FALSE, INFINITE ) ) {
                //
                // Case 0 is the thread kill event, we need to give this up. Since
                // the event is cleared when I detect it, I have to reset it
                //
                case 0 : //m_hKillOutputThreadEvent
                    done = true;
                    killed = true;
                    PurgeComm( m_hPort, PURGE_TXABORT );
                    break;
                //
                // Case 1 means the WriteFile routine signaled completion.
                // We're not out of the woods completely, there are a
                // few errors we have to check.
                //
                case 1 : //AsyncWriteInfo.hEvent
                    // The overlapped result can show that the write
                    // event completed, or it stopped for some other
                    // reason. If it is some other reason we exit the
                    // loop immediately. Otherwise we will pass through
                    // the loop and kick off another write. Note that
                    // a TXFlush() call creates an error here, which
                    // we ignore.
                    if ( !GetOverlappedResult( m_hPort,
                                               &AsyncWriteInfo,
                                               &result_count,
                                               FALSE ) ||
                          result_count != count ) {
                        if ( GetLastError() == ERROR_IO_PENDING )
                            clear_error();
                        else
                            translate_last_error();
                        done = true;
                    }
                    break;
                //
                // We had better not ever land here
                //
                default :
                    assert( false );
                }
            } else {
              translate_last_error();
              done = true;
            }
        } else {
            //
            // If we fall through to this point, it means that WriteFile()
            // returned immediately. If it did, it means that we were able
            // to send all of the characters requested in the call. If
            // we get here and all the characters weren't send,
            // something bad happened.
            //
            if ( result_count != count ) {
                translate_last_error();
                done = true;
            }
        }
    }
    //
    // On the way out, close the event handle
    //
    CloseHandle( AsyncWriteInfo.hEvent );

    return killed;
}

//
// When an asynchronous event is processed from the call to
// WaitCommEvent(), we have to call this guy to determine what
// happened. We check each of the modemstatus lines to see who
// changed, then call the notification functions to let the
// calling process know what happened.
//


void Win32Port::check_modem_status(bool first_time, DWORD event_mask )
{
    //
    // There shouldn't be anything to prevent us from reading
    // the input lines. If an error occurs, it is bad.
    ///
    if ( !GetCommModemStatus( m_hPort, &m_dwModemStatus ) )
    {
           DisplayLastError("Comm");
            assert( false );
    }
    //
    // The first_time flag is set one time when this guy is
    // called to force the notification functions to be called.
    // Forcing this to happen means that an application can be
    // sure that it will can use the notification functions to
    // determine status.
    //

    if ( first_time ) //report everything
    {
        CtsNotify( ( MS_CTS_ON & m_dwModemStatus ) != 0 );
        DsrNotify( ( MS_DSR_ON & m_dwModemStatus ) != 0 );
        CdNotify( ( MS_RLSD_ON & m_dwModemStatus ) != 0 );
        RiNotify( 0 );
    } else { //Only report events
        //
        // If it isn't the first time, we only send notification for
        // events that actually occured in the event that caused
        // this function to be invoked/
        //
        if ( event_mask & EV_CTS )
            CtsNotify( ( MS_CTS_ON & m_dwModemStatus ) != 0 );
        if ( event_mask & EV_DSR )
            DsrNotify( ( MS_DSR_ON & m_dwModemStatus ) != 0 );
        if ( event_mask & EV_RLSD )
            CdNotify( ( MS_RLSD_ON & m_dwModemStatus ) != 0 );
        //
        // Win95/98 *always* reports an RI event if RI is high when any
        // other line changes. This is not really a good thing. All
        // I'm interested is seeing EV_RINGTE, so I only report an
        // event if RI is low
        //
        if ( event_mask & ( EV_RING | EV_RINGTE ) )
            if ( ( MS_RING_ON & m_dwModemStatus ) == 0 )
                RiNotify( 0 );
    }
}

// EOF Win32Port.cpp
