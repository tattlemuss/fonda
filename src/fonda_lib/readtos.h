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
namespace tos_error
{
	enum
	{
		OK = 0,
		ERROR_READ_EOF = 1,					// Unable to read data before EOF
		ERROR_HEADER_MAGIC = 2,				// Missing "ELF" signature
		ERROR_ALLOC_FAIL = 3,				// Can't allocate file data
		ERROR_FILE_READ = 4,				// Can't read from FILE*
		ERROR_SECTION_OVERFLOW = 5			// Stated size of section is bigger than file data
	};
}

// ----------------------------------------------------------------------------
extern int process_tos_file(FILE* file, tos_results& output);

}
#endif // FONDA_LIB_READTOS_H
