if(NOT DEFINED INPUT OR NOT DEFINED OUTPUT OR NOT DEFINED SYMBOL)
    message(FATAL_ERROR "INPUT, OUTPUT, and SYMBOL are required")
endif()

file(READ "${INPUT}" program_hex HEX)
string(REGEX REPLACE "([0-9a-fA-F][0-9a-fA-F])" "0x\\1, " program_bytes "${program_hex}")
file(WRITE "${OUTPUT}"
    "#include \"embedded_program.hpp\"\n\n"
    "static const u08 ${SYMBOL}_data[] = { ${program_bytes} };\n"
    "extern \"C\" const rt_example_program ${SYMBOL} = { ${SYMBOL}_data, sizeof(${SYMBOL}_data) };\n"
)
