/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2017, The Souffle Developers. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file ExplainWeb.cpp
 *
 * Web-based interface for provenance exploration
 *
 ***********************************************************************/

#ifdef USE_WEB

#include "souffle/provenance/ExplainWeb.h"
#include "souffle/provenance/Explain.h"
#include "souffle/provenance/ExplainProvenance.h"
#include "souffle/provenance/ExplainWebAssets.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace souffle {

ExplainWeb::ExplainWeb(ExplainProvenance& provenance, std::string&& host, int port)
        : prov(provenance), serverHost(std::move(host)), serverPort(port), serverSocket(-1) {}

ExplainWeb::~ExplainWeb() {
    if (serverSocket != -1) {
        close(serverSocket);
    }
}

void ExplainWeb::explain() {
    runServer();
}

void ExplainWeb::runServer() {
    struct addrinfo hints;
    struct addrinfo* servinfo = nullptr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;  // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    std::string portStr = std::to_string(serverPort);
    int rv = getaddrinfo(serverHost.c_str(), portStr.c_str(), &hints, &servinfo);
    if (rv != 0) {
        throw std::runtime_error("getaddrinfo error: " + std::string(gai_strerror(rv)));
    }

    // Try to bind to the first available address
    struct addrinfo* p;
    for (p = servinfo; p != nullptr; p = p->ai_next) {
        serverSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (serverSocket == -1) {
            continue;
        }

        int opt = 1;
        setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        if (bind(serverSocket, p->ai_addr, p->ai_addrlen) == 0) {
            break;
        }

        close(serverSocket);
        serverSocket = -1;
    }

    freeaddrinfo(servinfo);

    if (p == nullptr) {
        throw std::runtime_error("Error binding socket: " + std::string(strerror(errno)));
    }

    if (listen(serverSocket, 32) < 0) {
        close(serverSocket);
        throw std::runtime_error("Error listening on socket: " + std::string(strerror(errno)));
    }

    std::cout << "Web server started: "
              << "http://" << serverHost << ":" << serverPort << std::endl;

    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);

        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            continue;
        }

        try {
            handleRequest(clientSocket);
        } catch (const std::exception& e) {
            std::cerr << "Error handling request: " << e.what() << std::endl;
        }
        close(clientSocket);
    }
}

void ExplainWeb::handleRequest(int clientSocket) {
    constexpr size_t bufferSize = 16 * 1024;
    char buffer[bufferSize] = {0};
    ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);

    if (bytesRead < 0) {
        throw std::runtime_error("Error reading request: " + std::string(strerror(errno)));
    } else if (bytesRead == 0) {
        throw std::runtime_error("Client disconnected before sending a request");
    } else if (bytesRead == bufferSize) {
        throw std::runtime_error("Request too long");
    }

    std::string request(buffer, bytesRead);
    std::string method;
    std::string path;

    std::string body = parseHTTPRequest(request, method, path);

    if (method == "GET") {
        if (path == "/" || path == "/index.html") {
            sendHTTPResponse(clientSocket, 200, "text/html", getStaticHTML());
        } else if (path == "/style.css") {
            sendHTTPResponse(clientSocket, 200, "text/css", getStaticCSS());
        } else if (path == "/script.js") {
            sendHTTPResponse(clientSocket, 200, "application/javascript", getStaticJS());
        } else if (path.find("/api/explain") == 0) {
            std::string query = path.substr(path.find('?') + 1);
            sendHTTPResponse(clientSocket, 200, "application/json", handleExplainAPI(urlDecode(query)));
        } else {
            sendHTTPResponse(clientSocket, 404, "text/plain", "Not Found");
        }
    } else {
        sendHTTPResponse(clientSocket, 405, "text/plain", "Method Not Allowed");
    }
}

std::string ExplainWeb::parseHTTPRequest(const std::string& request, std::string& method, std::string& path) {
    std::istringstream stream(request);
    std::string line;

    // Request line
    if (std::getline(stream, line)) {
        std::istringstream requestLine(line);
        requestLine >> method >> path;
    }

    // Headers
    while (std::getline(stream, line) && line != "" && line != "\r") {
        // Skip
    }

    // Body
    std::string body;
    std::string bodyLine;
    while (std::getline(stream, bodyLine)) {
        body += bodyLine;
        body += "\n";
    }

    return body;
}

void ExplainWeb::sendHTTPResponse(
        int clientSocket, int statusCode, const std::string& contentType, const std::string& body) {
    std::ostringstream response;
    response << "HTTP/1.1 " << statusCode << " ";

    switch (statusCode) {
        case 200: response << "OK"; break;
        case 404: response << "Not Found"; break;
        case 405: response << "Method Not Allowed"; break;
        default: response << "Unknown"; break;
    }

    response << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;

    std::string responseStr = response.str();
    write(clientSocket, responseStr.c_str(), responseStr.length());
}

std::string ExplainWeb::handleExplainAPI(const std::string& query) {
    try {
        // Parse query parameter (e.g., "tuple=relation(arg1,arg2)")
        size_t equalPos = query.find('=');
        if (equalPos == std::string::npos) {
            return "{\"error\":\"Invalid query format\"}";
        }

        std::string tupleStr = query.substr(equalPos + 1);
        auto parsed = Explain::parseTuple(tupleStr);

        if (parsed.first.empty()) {
            return "{\"error\":\"Failed to parse tuple\"}";
        }

        auto tree = prov.explain(parsed.first, parsed.second, 10);
        return treeToJSON(std::move(tree));
    } catch (const std::exception& e) {
        return "{\"error\":\"" + std::string(e.what()) + "\"}";
    }
}

std::string ExplainWeb::treeToJSON(Own<TreeNode> tree) {
    if (!tree) {
        return "{\"error\":\"No proof tree found\"}";
    }

    std::ostringstream oss;
    oss << "{\"proof\":";
    tree->printJSON(oss, 1);
    oss << "}";

    return oss.str();
}

std::string ExplainWeb::urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            int hex;
            std::istringstream iss(str.substr(i + 1, 2));
            if (iss >> std::hex >> hex) {
                result += static_cast<char>(hex);
                i += 2;
            } else {
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

std::string ExplainWeb::getStaticHTML() {
    return getEmbeddedHTML();
}

std::string ExplainWeb::getStaticCSS() {
    return getEmbeddedCSS();
}

std::string ExplainWeb::getStaticJS() {
    return getEmbeddedJS();
}

}  // end of namespace souffle

#endif  // USE_WEB
