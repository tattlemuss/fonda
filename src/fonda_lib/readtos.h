#ifndef FONDA_LIB_READTOS_H
#define FONDA_LIB_READTOS_H

#include "lineinfo.h"

namespace fonda
{
// ----------------------------------------------------------------------------
struct tos_results
{
	std::vector<compilation_unit>	line_info_units;
};

// ----------------------------------------------------------------------------
enum tos_error
{
	TOS_OK = 0,
	READ_EOF = 1,						// Unable to read data before EOF
	HEADER_MAGIC_FAIL = 2,				// Missing "ELF" signature
	ALLOC_FAIL = 3,						// Can't allocate file data
	FILE_READ_FAIL = 4,					// Can't read from FILE*
	SECTION_OVERFLOW = 5				// Stated size of section is bigger than file data
};

// ----------------------------------------------------------------------------
extern tos_error process_tos_file(FILE* file, tos_results& output);

}
#endif // FONDA_LIB_READTOS_H
