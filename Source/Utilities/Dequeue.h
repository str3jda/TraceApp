#pragma once

template < typename T, size_t TBlockSize = 24 >
class C_Dequeue
{
private:

	struct S_Block
	{
		uint8_t m_Data[TBlockSize * sizeof(T)];
		S_Block* m_Next = nullptr;
	};	

public:

	template < typename TObject >
	class C_Iterator
	{
		friend class C_Dequeue;

		C_Iterator( S_Block* block, size_t offset )
			: m_Block( block )
			, m_Offset( offset )
		{}

	public:
		
		C_Iterator()
			: m_Block( nullptr )
			, m_Offset( 0 )
		{}

		C_Iterator& operator++()
		{
			if ( m_Offset == TBlockSize - 1 )
			{
				m_Offset = 0;
				m_Block = m_Block->m_Next;
			}
			else
			{
				++m_Offset;
			}

			return *this;
		}

		TObject& operator*() const
		{
			return reinterpret_cast< TObject& >( m_Block->m_Data[m_Offset * sizeof( T )] );
		}

		TObject& operator->() const
		{
			return reinterpret_cast< TObject& >( m_Block->m_Data[m_Offset * sizeof( T )] );
		}

		bool operator==( C_Iterator const& iterator ) const
		{
			return ( m_Block == iterator.m_Block ) && ( m_Offset == iterator.m_Offset );
		}

		bool operator!=( C_Iterator const& iterator ) const
		{
			return ( m_Block != iterator.m_Block ) || ( m_Offset != iterator.m_Offset );
		}

	private:

		S_Block* m_Block;
		size_t m_Offset;

	};

	typedef C_Iterator<T const> const_iterator;
	typedef C_Iterator<T> iterator;
	typedef T const& const_reference;
	typedef T& reference;

public:

	~C_Dequeue()
	{
		clear();
		delete m_FirstBlock;
	}

	void clear()
	{
		S_Block* bl = m_FirstBlock;

		while ( bl != nullptr )
		{
			size_t i = 0;
			size_t n = TBlockSize;
			bool removeBlok = true;

			if ( bl == m_FirstBlock )
			{
				i = m_Begin;
				removeBlok = false;
			}

			if ( bl == m_LastBlock )
			{
				n = m_End;
			}

			for ( ; i < n; ++i )
			{
				reinterpret_cast< T& >( bl->m_Data[i * sizeof( T )] ).~T();
			}

			S_Block* next = bl->m_Next;
			if ( removeBlok )
			{
				delete bl;
			}
			else
			{
				bl->m_Next = nullptr;
			}

			bl = next;
		}

		m_LastBlock = m_FirstBlock;
		m_Begin = 0;
		m_End = 0;
		m_Size = 0;
	}

	template < typename TValue >
	void push_back( TValue&& item )
	{
		if ( m_LastBlock == nullptr )
		{
			m_FirstBlock = m_LastBlock = new S_Block();
		}
		else if ( m_End == TBlockSize )
		{
			S_Block* bl = new S_Block();
			m_LastBlock->m_Next = bl;
			m_LastBlock = bl;

			m_End = 0;
		}

		new ( &m_LastBlock->m_Data[m_End * sizeof( T )] ) T( std::forward< TValue >( item ) );
		++m_End;
		++m_Size;
	}

	void pop_front()
	{
		reinterpret_cast< T* >( &m_FirstBlock->m_Data[m_Begin * sizeof( T )] )->~T();

		if ( m_Begin == TBlockSize - 1 )
		{ // last item in block

			S_Block* old = m_FirstBlock;
			m_FirstBlock = old->m_Next;
			delete old;

			m_Begin = 0;
		}
		else
		{
			++m_Begin;
		}
		
		--m_Size;
	}

	const_reference front() const
	{
		return reinterpret_cast< const_reference >( m_FirstBlock->m_Data[m_Begin * sizeof( T )] );
	}

	reference front()
	{
		return reinterpret_cast< reference >( m_FirstBlock->m_Data[m_Begin * sizeof( T )] );
	}

	const_reference back() const
	{
		return reinterpret_cast< const_reference >( m_LastBlock->m_Data[ ( m_End - 1 ) * sizeof( T )] );
	}

	reference back()
	{
		return reinterpret_cast< reference >( m_LastBlock->m_Data[ ( m_End - 1 ) * sizeof( T )] );
	}

	size_t size() const 
	{ 
		return m_Size;
	}

	bool empty() const 
	{
		return m_Size == 0;
	}

	const_iterator begin() const { return const_iterator( m_FirstBlock, m_Begin ); }
	iterator begin() { return iterator( m_FirstBlock, m_Begin ); }
	const_iterator end() const { return const_iterator( m_LastBlock, m_End ); }
	iterator end() { return iterator( m_LastBlock, m_End ); }

	reference operator[]( size_t index )
	{
		S_Block* bl = m_FirstBlock;

		index += m_Begin;
		
		size_t blockMinorId = index % TBlockSize;
		size_t blockMajorId = index / TBlockSize;

		while ( blockMajorId-- > 0 )
		{
			bl = bl->m_Next;
		}

		return reinterpret_cast< reference >( bl->m_Data[blockMinorId * sizeof( T )] );
	}

	const_reference operator[]( size_t index ) const
	{
		S_Block const* bl = m_FirstBlock;

		index += m_Begin;

		size_t blockMinorId = index % TBlockSize;
		size_t blockMajorId = index / TBlockSize;

		while ( blockMajorId-- > 0 )
		{
			bl = bl->m_Next;
		}

		return reinterpret_cast< const_reference >( bl->m_Data[blockMinorId * sizeof( T )] );
	}

private:

	S_Block* m_FirstBlock = nullptr;
	S_Block* m_LastBlock = nullptr;

	size_t m_Begin = 0;
	size_t m_End = 0;
	size_t m_Size = 0;

};