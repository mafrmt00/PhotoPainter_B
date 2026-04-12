# Find all source files in a single current directory
# Save the name to DIR_minIni_SRCS
aux_source_directory(. DIR_minIni_SRCS)

# Generate the link library
add_library(minIni ${DIR_minIni_SRCS})
target_link_libraries(minIni PUBLIC pico_stdlib FatFs_SPI)