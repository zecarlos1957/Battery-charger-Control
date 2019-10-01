
#ifndef MTDEQUE_H
#define MTDEQUE_H


class RSdeque
{
protected:
    const int m_iMaxSize;
    deque<char> m_Queue;
    CRITICAL_SECTION m_Lock;

public:
    RSdeque( int max_size ) : m_iMaxSize( max_size )
    {
        ::InitializeCriticalSection( &m_Lock );
    }
     ~RSdeque()
    {
        ::DeleteCriticalSection( &m_Lock );
    }
    int SpaceFree()
    {
        ::EnterCriticalSection( &m_Lock );
        int size = m_iMaxSize - m_Queue.size();
        ::LeaveCriticalSection( &m_Lock );
        return ( size < 0 ) ? 0 : size;
    }
    int SpaceUsed()
    {
        ::EnterCriticalSection( &m_Lock );
        int size = m_Queue.size();
        ::LeaveCriticalSection( &m_Lock );
        return size;
    }
    int Insert( char c )
    {
        int return_value;
        ::EnterCriticalSection( &m_Lock );
        if ( m_Queue.size() < m_iMaxSize ) {
            m_Queue.push_back( c );
            return_value = c & 0xff;
        } else
            return_value = -1;
        ::LeaveCriticalSection( &m_Lock );
        return return_value;
    }
    int Insert( char *data, int count )
    {
        ::EnterCriticalSection( &m_Lock );
        int actual = m_iMaxSize - m_Queue.size() ;
        if ( actual < 0 )
            actual = 0;
        if ( count < actual )
            actual = count;
        for ( int i = 0 ; i < actual ; i++ )
            m_Queue.push_back( *data++ );
        ::LeaveCriticalSection( &m_Lock );
        return actual;
    }
    int Extract( char *data, int max )
    {
        int i = 0;
        ::EnterCriticalSection( &m_Lock );
        while ( i < max && m_Queue.size() ) {
            data[ i++ ] = m_Queue.front();
            m_Queue.pop_front();
        }
        ::LeaveCriticalSection( &m_Lock );
        return i;
    }
     int Peek( char *data, int max )
    {
        int i;
        ::EnterCriticalSection( &m_Lock );
        if ( max > m_Queue.size() )
            max = m_Queue.size();
        for (i = 0 ; i < max ; i++ )
            data[ i ] = m_Queue.begin()[ i ];
        ::LeaveCriticalSection( &m_Lock );
        return i;
    }
    int Extract()
    {
        int ret_val = -1;
        ::EnterCriticalSection( &m_Lock );
        if ( m_Queue.size() ) {
            ret_val = m_Queue.front() & 0xff;
            m_Queue.pop_front();
        }
        ::LeaveCriticalSection( &m_Lock );
        return ret_val;
    }
    //
    // This member just empties the queue.
    //
    void Clear()
    {
        ::EnterCriticalSection( &m_Lock );
        m_Queue.clear();
        ::LeaveCriticalSection( &m_Lock );
    }

};


#endif
