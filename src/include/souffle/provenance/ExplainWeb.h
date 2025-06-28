/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2017, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file ExplainWeb.h
 *
 * Web-based interface for provenance exploration
 *
 ***********************************************************************/

#pragma once

#ifdef USE_WEB

#include "souffle/provenance/ExplainProvenance.h"
#include "souffle/provenance/ExplainTree.h"
#include <string>

namespace souffle {

class ExplainWeb {
public:
    explicit ExplainWeb(ExplainProvenance& provenance, std::string&& host, int port);
    ~ExplainWeb();

    // Start the web server
    void explain();

private:
    ExplainProvenance& prov;
    std::string serverHost;
    int serverPort;
    int serverSocket;

    // HTTP server methods
    void runServer();
    void handleRequest(int clientSocket);
    std::string parseHTTPRequest(const std::string& request, std::string& method, std::string& path);
    void sendHTTPResponse(
            int clientSocket, int statusCode, const std::string& contentType, const std::string& body);

    // API endpoints
    std::string handleExplainAPI(const std::string& query);

    // Utility methods
    std::string treeToJSON(Own<TreeNode> tree);
    std::string urlDecode(const std::string& str);
    std::string getStaticHTML();
    std::string getStaticCSS();
    std::string getStaticJS();
};

}  // end of namespace souffle

#endif  // USE_WEB
