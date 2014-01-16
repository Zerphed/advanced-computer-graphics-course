#ifndef _TLSVARIABLE_H
#define _TLSVARIABLE_H
// --------------------------------------------------------------------------

void die( const char* pstrReason );
// --------------------------------------------------------------------------

template<class T>
class TLSVariableDefaultFactory
{
public:
	T*	allocate() const
	{
		return new T;
	}
};
// --------------------------------------------------------------------------

template<typename T, class TFactory = TLSVariableDefaultFactory<T> >
class TLSVariable
{
private:
	DWORD		m_dwTLSIndex;
	TFactory	m_Factory;

public:
	TLSVariable( TFactory Factory = TFactory() )
	{
		m_dwTLSIndex = TlsAlloc();
		if ( m_dwTLSIndex == TLS_OUT_OF_INDEXES )
		{
			::printf( "Out of TLS indices!" );
			exit(0);
		}
		m_Factory = Factory;
	}
	// --------------------------------------------------------------------------

	inline T* get()
	{
		T* var = (T*)TlsGetValue( m_dwTLSIndex );
		if ( var == 0 )
		{
			var = m_Factory.allocate();
			TlsSetValue( m_dwTLSIndex, var );
		}
		return var;
	}

	inline const T* get() const
	{
		const T* var = (const T*)TlsGetValue( m_dwTLSIndex );
		if ( var == 0 )
		{
			var = m_Factory.allocate();
			TlsSetValue( m_dwTLSIndex, (T*)var );
		}
		return var;
	}

};
// --------------------------------------------------------------------------

#endif // !_TLSVARIABLE_H