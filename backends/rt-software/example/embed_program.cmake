if(NOT DEFINED INPUT OR NOT DEFINED OUTPUT OR NOT DEFINED SYMBOL)
	message(FATAL_ERROR "INPUT, OUTPUT, and SYMBOL are required")
endif()

file(READ "${INPUT}" program_hex HEX)
string(REGEX REPLACE "([0-9a-fA-F][0-9a-fA-F])" "0x\\1, " program_bytes "${program_hex}")
file(WRITE "${OUTPUT}"
	"#include <stddef.h>\n"
	"#include <stdint.h>\n\n"
	"const uint8_t ${SYMBOL}[] = { ${program_bytes} };\n"
	"const size_t ${SYMBOL}_size = sizeof(${SYMBOL});\n"
)
