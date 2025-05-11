#include "readtos.h"

namespace fonda
{
// ----------------------------------------------------------------------------
// Bounded access to a range of memory
class buffer_reader
{
public:
	buffer_reader(const uint8_t* pData, uint32_t length, uint32_t base_address) :
		m_pData(pData),
		m_length(length),
		m_baseAddress(base_address),
		m_pos(0)
	{}

	// Returns 0 for success, 1 for failure
	int read_byte(uint8_t& data)
	{
		if (m_pos + 1 > m_length)
			return tos_error::ERROR_READ_EOF;
		data = m_pData[m_pos++];
		return tos_error::OK;
	}

	// Returns 0 for success, 1 for failure
	int read_word(uint16_t& data)
	{
		if (m_pos + 2 > m_length)
			return tos_error::ERROR_READ_EOF;
		data = m_pData[m_pos++];
		data <<= 8;
		data |= m_pData[m_pos++];
		return tos_error::OK;
	}

	// Returns 0 for success, 1 for failure
	int read_long(uint32_t& data)
	{
		if (m_pos + 4 > m_length)
			return tos_error::ERROR_READ_EOF;
		data = m_pData[m_pos++];
		data <<= 8;
		data |= m_pData[m_pos++];
		data <<= 8;
		data |= m_pData[m_pos++];
		data <<= 8;
		data |= m_pData[m_pos++];
		return tos_error::OK;
	}

	// Copy bytes into the buffer
	// Returns 0 for success, 1 for failure
	int read(uint8_t* data, int count)
	{
		if (m_pos + count > m_length)
			return tos_error::ERROR_READ_EOF;
		for (int i = 0; i < count; ++i)
			*data++ = m_pData[m_pos++];
		return tos_error::OK;
	}

	void advance(uint32_t count)
	{
		m_pos += count;
		if (m_pos > m_length)
			m_pos = m_length;
	}

	void set_pos(uint32_t pos)
	{
		m_pos = pos;
		if (m_pos > m_length)
			m_pos = m_length;
	}

	const uint8_t* get_data() const
	{
		return m_pData + m_pos;
	}

	uint32_t get_pos() const
	{
		return m_pos;
	}

	uint32_t get_address() const
	{
		return m_baseAddress + m_pos;
	}

	uint32_t get_remain() const
	{
		return m_length - m_pos;
	}

private:
	const uint8_t*  m_pData;
	const uint32_t  m_length;
	const uint32_t  m_baseAddress;
	uint32_t		m_pos;
};


// ----------------------------------------------------------------------------
//	TOS EXECUTABLE READING
// ----------------------------------------------------------------------------
struct tos_header
{
   //  See http://toshyp.atari.org/en/005005.html for TOS header details
	uint16_t  ph_branch;	  /* Branch to start of the program  */
							  /* (must be 0x601a!)			   */

	uint32_t  ph_tlen;		  /* Length of the TEXT segment	  */
	uint32_t  ph_dlen;		  /* Length of the DATA segment	  */
	uint32_t  ph_blen;		  /* Length of the BSS segment	   */
	uint32_t  ph_slen;		  /* Length of the symbol table	  */
	uint32_t  ph_res1;		  /* Reserved, should be 0;		  */
							  /* Required by PureC			   */
	uint32_t  ph_prgflags;	  /* Program flags				   */
	uint16_t  ph_absflag;	  /* 0 = Relocation info present	 */
};

// ----------------------------------------------------------------------------
//	DEBUG LINE READING
// ----------------------------------------------------------------------------
// Read N bytes of string data in to a std::string, omitting pad bytes
static int read_string(buffer_reader& buf, uint32_t length, std::string& str)
{
	uint8_t ch;
	for (uint32_t i = 0; i < length; ++i)
	{
		if (buf.read_byte(ch))
			return tos_error::ERROR_READ_EOF;
		if (ch)	// Don't add any padded zero bytes
			str += (char)ch;
	}
	return tos_error::OK;
}

// ----------------------------------------------------------------------------
// Read hunk of "LINE" format line information.
// This is a simple set of "line", "pc" 8-byte structures
static int read_debug_line_info(buffer_reader& buf, fonda::compilation_unit& cu, uint32_t offset)
{
	// Filename length is stored as divided by 4
	uint32_t flen;
	if (buf.read_long(flen))
		return tos_error::ERROR_READ_EOF;
	flen <<= 2;
	std::string fname;
	if (read_string(buf, flen, fname))
		return tos_error::ERROR_READ_EOF;

	size_t file_index = cu.files.size();
	compilation_unit::file f;
	f.dir_index = 0;
	f.length = 0;
	f.timestamp = 0;
	f.path = fname.c_str();
	cu.files.push_back(f);

	// Calculate remaining number of structures to read
	uint32_t numlines = buf.get_remain() / 8;
	while (numlines)
	{
		uint32_t line, pc;
		if (buf.read_long(line))
			return tos_error::ERROR_READ_EOF;
		if (buf.read_long(pc))
			return tos_error::ERROR_READ_EOF;

		code_point cp;
		cp.address = pc + offset;
		cp.column = 0;
		cp.file_index = file_index;
		cp.line = line;
		cu.points.push_back(cp);
		--numlines;
	}
	return tos_error::OK;
}

// ----------------------------------------------------------------------------
// Read a single compressed HCLN value (line or PC)
// The format is a simple prefix compression:
// 		 1 byte: return value if != 0
// 	else 2 byte word: return value if != 0
//  else 4 byte long.
static int read_hcln_long(buffer_reader& buf, uint32_t& val)
{
	uint8_t tmpB;
	if (buf.read_byte(tmpB))
		return tos_error::ERROR_READ_EOF;
	if (tmpB)
	{
		val = tmpB;
		return tos_error::OK;
	}

	uint16_t tmpW;
	if (buf.read_word(tmpW))
		return tos_error::ERROR_READ_EOF;
	if (tmpW)
	{
		val = tmpW; 
		return tos_error::OK;
	}

	return buf.read_long(val);
}

// ----------------------------------------------------------------------------
// Read hunk of "HCLN" (HiSoft Compressed Line Number) format line information.
static int read_debug_hcln_info(buffer_reader& buf, fonda::compilation_unit& cu, uint32_t offset)
{
	uint32_t flen, numlines;
	// Filename length is stored as divided by 4
	if (buf.read_long(flen))
		return tos_error::ERROR_READ_EOF;
	std::string fname;
	flen <<= 2;
	if (read_string(buf, flen, fname))
		return tos_error::ERROR_READ_EOF;

	size_t file_index = cu.files.size();
	compilation_unit::file f;
	f.dir_index = 0;
	f.length = 0;
	f.timestamp = 0;
	f.path = fname.c_str();
	cu.files.push_back(f);

	if (buf.read_long(numlines))
		return tos_error::ERROR_READ_EOF;
	uint32_t curr_line = 0;
	uint32_t curr_pc = offset;
	while (numlines)
	{
		uint32_t line, pc;
		if (read_hcln_long(buf, line))
			return tos_error::ERROR_READ_EOF;
		if (read_hcln_long(buf, pc))
			return tos_error::ERROR_READ_EOF;
		// Accumulate current position
		curr_line += line;
		curr_pc += pc;

		code_point cp;
		cp.address = pc + offset;
		cp.column = 0;
		cp.file_index = file_index;
		cp.line = line;
		cu.points.push_back(cp);
		--numlines;
	}
	return tos_error::OK;
}

// ----------------------------------------------------------------------------
// Read relocation information and debug line number information.
static int read_reloc(buffer_reader& buf, compilation_unit& cu)
{
	uint32_t addr;
	if (buf.read_long(addr))
		return tos_error::ERROR_READ_EOF;

	// 0 at start meeans "no reloc info"
	if (addr)
	{
		while (1)
		{
			uint8_t offset;
			if (buf.read_byte(offset))
				return tos_error::ERROR_READ_EOF;
			if (offset == 0)
				break;

			if (offset == 1)
			{
				addr += 254;
				continue;
			}
			else
			{
				addr += offset;
			}
		}
	}

	// Read debugging information
	// Align to word
	if (buf.get_pos() & 1)
		buf.advance(1);

	// Debug information is a set of blocks ("hunks"?) with a
	// type and size, similar to IFF.
	bool got_header = false;
	while (buf.get_remain() >= 4)
	{
		uint32_t hunk_header;
		uint32_t hlen, hstart, offset, htype;

		if (buf.read_long(hunk_header))
			return tos_error::ERROR_READ_EOF;
		if ( (hunk_header==0) && got_header)
			break;			// zero might mean padded, so stop without error

		// Expect the magic "this is a hunk" value
		if (hunk_header != 0x3F1)
			return tos_error::ERROR_READ_EOF;			// (bad file)

		// Read hunk length, which is number of 32-byte longs
		if (buf.read_long(hlen))
			return tos_error::ERROR_READ_EOF;
		// Convert header length to bytes
		hlen <<= 2;
		hstart = buf.get_pos();	// record position for jumping

		// Create a sub-reader so we don't read off end of the hunk
		buffer_reader hunk_buffer(buf.get_data(), hlen, buf.get_address());
		// This appears to be an offset to apply to the PC values
		if (hunk_buffer.read_long(offset))
			return tos_error::ERROR_READ_EOF;
		if (hunk_buffer.read_long(htype))
			return tos_error::ERROR_READ_EOF;

		switch (htype)
		{
			// NOTE: the X-Debug example does stricter checks on format here.
			// We just record that a header is found before the per-file hunks
			// are encountered.
			case 0x48454144: // "HEAD"
				got_header = true;
				break;
			case 0x4c494e45: // "LINE"
				read_debug_line_info(hunk_buffer, cu, offset);
				break;
			case 0x48434c4e: // "HCLN"
				read_debug_hcln_info(hunk_buffer, cu, offset);
				break;
			default:
				return tos_error::ERROR_READ_EOF;
		}

		// Jump to next hunk
		buf.set_pos(hstart + hlen);
	}
	return tos_error::OK;
}

// ----------------------------------------------------------------------------
int process_tos_file(const uint8_t* data_ptr, long size, tos_results& results)
{
	buffer_reader buf(data_ptr, size, 0);
	tos_header header = {};

	if (buf.read_word(header.ph_branch))
		return tos_error::ERROR_READ_EOF;
	if (buf.read_long(header.ph_tlen))
		return tos_error::ERROR_READ_EOF;
	if (buf.read_long(header.ph_dlen))
		return tos_error::ERROR_READ_EOF;
	if (buf.read_long(header.ph_blen))
		return tos_error::ERROR_READ_EOF;
	if (buf.read_long(header.ph_slen))
		return tos_error::ERROR_READ_EOF;
	if (buf.read_long(header.ph_res1))
		return tos_error::ERROR_READ_EOF;
	if (buf.read_long(header.ph_prgflags))
		return tos_error::ERROR_READ_EOF;
	if (buf.read_word(header.ph_absflag))
		return tos_error::ERROR_READ_EOF;

	if (header.ph_branch != 0x601a)
		return tos_error::ERROR_HEADER_MAGIC;

	if (header.ph_tlen > buf.get_remain())
		return tos_error::ERROR_SECTION_OVERFLOW;
	buffer_reader text_buf(buf.get_data(), header.ph_tlen, 0);

	// Skip the text
	buf.advance(header.ph_tlen);
	if (header.ph_dlen > buf.get_remain())
		return tos_error::ERROR_SECTION_OVERFLOW;
	buffer_reader data_buf(buf.get_data(), header.ph_dlen, 0);

	// Skip the data
	buf.advance(header.ph_dlen);

	// (No BSS in the file, so symbols should be next)
	if (header.ph_slen > buf.get_remain())
		return tos_error::ERROR_SECTION_OVERFLOW;
	buffer_reader symbol_buf(buf.get_data(), header.ph_slen, 0);

	// Skip symbols here

	// Parse the reloc section, which also contains the line information
	buf.advance(header.ph_slen);

	// Length of the reloc buffer is implicit from the remaining size
	buffer_reader reloc_buf(buf.get_data(), buf.get_remain(), 0);

	// Add a single compilation unit with a dummy directory entry
	compilation_unit single_cu;
	single_cu.dirs.push_back(std::string("."));
	results.line_info_units.push_back(single_cu);

	return read_reloc(reloc_buf, results.line_info_units.back());
}

// ----------------------------------------------------------------------------
int process_tos_file(FILE* file, tos_results& output)
{
	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	fseek(file, 0, SEEK_SET);
	uint8_t* data = new uint8_t[length];
	if (!data)
		return tos_error::ERROR_ALLOC_FAIL;

	size_t count = fread(data, 1, length, file);
	if ((long)count != length)
	{
		delete [] data;
		return tos_error::ERROR_FILE_READ;
	}
	int ret = process_tos_file(data, length, output);
	delete [] data;
	return ret;
}

}
