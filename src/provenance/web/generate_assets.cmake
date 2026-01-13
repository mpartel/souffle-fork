# CMake script to generate embedded web assets

file(READ "${WEB_DIR}/index.html" HTML_CONTENT HEX)
file(READ "${WEB_DIR}/style.css" CSS_CONTENT HEX)
file(READ "${WEB_DIR}/script.js" JS_CONTENT HEX)

string(REGEX REPLACE "(..)" "\\\\x\\1" HTML_HEX_STRING "${HTML_CONTENT}")
string(REGEX REPLACE "(..)" "\\\\x\\1" CSS_HEX_STRING "${CSS_CONTENT}")
string(REGEX REPLACE "(..)" "\\\\x\\1" JS_HEX_STRING "${JS_CONTENT}")

string(LENGTH "${HTML_CONTENT}" HTML_HEX_LENGTH)
string(LENGTH "${CSS_CONTENT}" CSS_HEX_LENGTH)
string(LENGTH "${JS_CONTENT}" JS_HEX_LENGTH)

math(EXPR HTML_BYTE_LENGTH "${HTML_HEX_LENGTH} / 2")
math(EXPR CSS_BYTE_LENGTH "${CSS_HEX_LENGTH} / 2")
math(EXPR JS_BYTE_LENGTH "${JS_HEX_LENGTH} / 2")

set(GENERATED_CPP_CONTENT "/*
 * Generated file - DO NOT EDIT
 * Web assets for Souffle Provenance Explorer
 */

#ifdef USE_WEB

#include \"souffle/provenance/ExplainWebAssets.h\"

namespace souffle {

std::string getEmbeddedHTML() {
    static const char data[] = \"${HTML_HEX_STRING}\";
    return std::string(data, ${HTML_BYTE_LENGTH});
}

std::string getEmbeddedCSS() {
    static const char data[] = \"${CSS_HEX_STRING}\";
    return std::string(data, ${CSS_BYTE_LENGTH});
}

std::string getEmbeddedJS() {
    static const char data[] = \"${JS_HEX_STRING}\";
    return std::string(data, ${JS_BYTE_LENGTH});
}

} // namespace souffle

#endif  // USE_WEB
")

file(WRITE "${OUTPUT_FILE}" "${GENERATED_CPP_CONTENT}")
