#ifndef FONDA_LIB_BUFFER_ACCESS_H
#define FONDA_LIB_BUFFER_ACCESS_H

#include <stdint.h>
#include <string.h>
#include <string>

namespace fonda
{
// ----------------------------------------------------------------------------
// buffer_access -- Bounded access to a block of memory
class buffer_access
{
public:
	buffer_access() :
		m_pData(nullptr),
		m_length(0),
		m_pos(0),
		m_errored(false)
	{}

	buffer_access(const uint8_t* pData, uint64_t length) :
		m_pData(pData),
		m_length(length),
		m_pos(0),
		m_errored(false)
	{}

	// Copy bytes into the buffer
	// Returns 0 for success, 1 for failure
	int read(uint8_t* data, int count)
	{
		if (m_pos + count > m_length)
			return set_errored(data, count);
		for (int i = 0; i < count; ++i)
			*data++ = m_pData[m_pos++];
		return 0;
	}

	// Templated read of a single object
	template<typename T>
		int read(T& data)
		{
			return read((uint8_t*)&data, sizeof(T));
		}

	int read_null_term_string(std::string& result)
	{
		const uint8_t* start = get_data();
		uint8_t val;
		int ret = 0;
		do
		{
			ret |= read(val);
		} while (val);
		result = std::string((char*)start);
		return ret;
	}

	int set(uint64_t pos)
	{
		m_pos = pos;
		if (m_pos > m_length)
			return set_errored(nullptr, 0);
		return 0;
	}

	const uint8_t* get_data() const 	{ return m_pData + m_pos; }
	uint64_t get_pos() const 			{ return m_pos;	}
	bool errored() const				{ return m_errored; }

private:
	int set_errored(uint8_t* data, int count)
	{
		if (data)
			memset(data, 0, count);
		m_errored = true;
		return 1;
	}
	const uint8_t*  m_pData;
	uint64_t 		m_length;
	uint64_t		m_pos;
	bool			m_errored;
};

}
#endif
