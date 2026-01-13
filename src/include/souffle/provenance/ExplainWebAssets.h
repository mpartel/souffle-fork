/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2017, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file ExplainWebAssets.h
 *
 * Embedded web assets for provenance web interface
 *
 ***********************************************************************/

#pragma once

#ifdef USE_WEB

#include <string>

namespace souffle {

/* Get embedded HTML content */
std::string getEmbeddedHTML();

/* Get embedded CSS content */
std::string getEmbeddedCSS();

/* Get embedded JavaScript content */
std::string getEmbeddedJS();

}  // end of namespace souffle

#endif  // USE_WEB
