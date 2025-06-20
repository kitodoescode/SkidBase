#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock.h>
#include <thread>
#include <vector>
#include <string>
#include "Manager/Manager.h"

const char* Port = "0007";

lua_State* CommunicationState = 0;

namespace Communication {

    bool recvAll(SOCKET s, char* b, int len) {
        int r, recvd = 0;
        while (recvd < len && (r = recv(s, b + recvd, len - recvd, 0)) > 0)
            recvd += r;
        return recvd == len;
    }

    void COMM_Server() {
        WSADATA wsa;

        if (WSAStartup(MAKEWORD(1, 1), &wsa)) return;

        SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenSocket == INVALID_SOCKET) {
            WSACleanup();
            return;
        }

        sockaddr_in serverAddr = {};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(static_cast<u_short>(atoi(Port)));

        if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            closesocket(listenSocket);
            WSACleanup();
            return;
        }

        if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
            closesocket(listenSocket);
            WSACleanup();
            return;
        }

        while (true) {
            SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
            if (clientSocket == INVALID_SOCKET)
                continue;

            DWORD szNet = 0, sz = 0;

            if (!recvAll(clientSocket, (char*)&szNet, 4)) {
                closesocket(clientSocket);
                continue;
            }

            sz = ntohl(szNet);
            if (sz == 0 || sz > 10 * 1024 * 1024) {
                closesocket(clientSocket);
                continue;
            }

            std::vector<char> buffer(sz + 1);
            if (!recvAll(clientSocket, buffer.data(), sz)) {
                closesocket(clientSocket);
                continue;
            }

            buffer[sz] = '\0';

            std::string msg(buffer.data());
            Execution->Execute(CommunicationState, msg.c_str());

            closesocket(clientSocket);
        }

        closesocket(listenSocket);
        WSACleanup();
    }

    void Initialize() {

        COMM_Server();
    }

    void ChangeState(lua_State* LSS) {
        CommunicationState = LSS;
    }
}