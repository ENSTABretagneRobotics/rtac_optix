# This will generate a header file containing content of all ptx files stored
# in a std::unordered_map

file(WRITE  ${OUTPUT_FILE} "") # ensuring file is empty

# Filtering target name to ensure compatibility with c++
# First replacing -,+,. characters by _ (authorized in cmake but not c++)
string(REGEX REPLACE "[-\\+\\.]" "_" TARGET_NAME ${TARGET_NAME})
# Then adding a _ in front for TARGET_NAME if it is starting with a number
string(REGEX REPLACE "^([0-9])" "_\\1" TARGET_NAME ${TARGET_NAME})

# Arguments where passed as a space sparated list.
# Generating back a standard camke semi-colon sperated list.
separate_arguments(SOURCE_FILES UNIX_COMMAND ${SOURCE_FILES})
separate_arguments(PTX_FILES    UNIX_COMMAND ${PTX_FILES})

# Generating a C++ header file.
string(APPEND output_content "#ifndef _DEF_${TARGET_NAME}_PTX_FILES_H_\n")
string(APPEND output_content "#define _DEF_${TARGET_NAME}_PTX_FILES_H_\n\n")

string(APPEND output_content "#include <iostream>\n")
string(APPEND output_content "#include <unordered_map>\n\n")

string(APPEND output_content "namespace ${TARGET_NAME} {\n\n")

string(APPEND output_content "inline std::unordered_map<std::string,std::string> get_ptx_files()\n")
string(APPEND output_content "{\n")

string(APPEND output_content "    std::unordered_map<std::string,std::string> ptxDict\;\n\n")

list(LENGTH PTX_FILES list_length)
while(${list_length} GREATER 0)
    list(POP_FRONT SOURCE_FILES source_name)
    list(POP_FRONT PTX_FILES    ptx_name)
    string(APPEND output_content "    ptxDict[\"${source_name}\"] = R\"(")

    # string(APPEND output_content "${ptx_content}")
    # Have to do it this way because of cmake handling of special characters.
    # (; characters get deleted from ptx content).
    file(APPEND ${OUTPUT_FILE} ${output_content})
    set(output_content "")
    file(READ ${ptx_name} ptx_content)
    file(APPEND ${OUTPUT_FILE} "${ptx_content}")

    string(APPEND output_content "    )\"\;\n\n")
    list(LENGTH PTX_FILES list_length)
endwhile()


string(APPEND output_content "    return ptxDict\;\n")
string(APPEND output_content "}\n\n")

string(APPEND output_content "}\; //namespace ${TARGET_NAME}\n\n")
string(APPEND output_content "#endif //_DEF_${TARGET_NAME}_PTX_FILES_H_\n")

# writing the file
file(APPEND ${OUTPUT_FILE} ${output_content})

