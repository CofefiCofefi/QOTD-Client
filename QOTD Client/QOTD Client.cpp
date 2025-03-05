#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <cctype>

using std::cin;
using std::cout;
using std::endl;
using std::string;

#pragma comment(lib, "Ws2_32.lib")

const int DEFAULT_BUFLEN = 512;

const char* DEFAULT_HOST = "PE-HRFIELDS-1";
const char* DEFAULT_PORT = "17";

int wait(SOCKET s, int seconds, int msec) {
    // Parameter List:
    // [IN] s        : SOCKET handle of connected socket.
    // [IN] seconds : Number of seconds to wait for incoming traffic
    // [IN] msec    : Number of milliseconds to wait
    // Return value : 1 = Data is waiting in TCP/UDP buffer; 0 = Timeout or error detected
    int    stat;                     // Status of waiting socket
    struct timeval stTimeOut;        // Used to set timeout value
    fd_set stReadFDS;                // "File Descriptor Set" for select() call
    fd_set stXcptFDS;                // "File Descriptor Set" for exception handling

    // Set TimeOut value
    stTimeOut.tv_sec = seconds;      // Number of seconds
    stTimeOut.tv_usec = msec;        //   + msec milliseconds to wait

    FD_ZERO(&stReadFDS);             // Zero the set of "Read access" sockets
    FD_ZERO(&stXcptFDS);             // Zero the set of "Exception" sockets
    FD_SET(s, &stReadFDS);           // Add "s" to the set of "Read access" sockets
    FD_SET(s, &stXcptFDS);           // Add "s" to the set of "Exception" sockets

    // Check to see if Read access is enabled for socket "s"
    stat = select(-1, &stReadFDS, NULL, &stXcptFDS, &stTimeOut);
    if (stat == SOCKET_ERROR) {
        std::cout << std::endl << "wait() function failed" << std::endl;
        stat = 0;
    }
    else if (stat > 0) {
        if (FD_ISSET(s, &stXcptFDS)) {   // Some error was detected or OOB data
            stat = 0;
        }
        if (!FD_ISSET(s, &stReadFDS)) {  // No incoming data!? (check just in case)
            stat = 0;
        }
    }

    return stat;
}

void toUpperCase(char* str) {
    for (int i = 0; str[i]; i++) {
        str[i] = toupper(str[i]);
    }
}

int main(int argc, char* argv[])
{
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (iResult != 0) {
        cout << "WSAStartup failed: " << iResult << endl;
        return 1;
    }

    struct addrinfo* result = NULL;
    struct addrinfo hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    const char* host = "PE-HRFIELDS-1";
    const char* port = "17";

    iResult = getaddrinfo(host, port, &hints, &result);
    if (iResult != 0) {
        cout << "getaddrinfo failed: " << iResult << endl;
        WSACleanup();
        return 1;
    }

    SOCKET ClientSocket = INVALID_SOCKET;
    ClientSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ClientSocket == INVALID_SOCKET) {
        cout << "Error at socket(): " << WSAGetLastError();
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    int recvbuflen = DEFAULT_BUFLEN;
    char recvbuf[DEFAULT_BUFLEN];
    char sendbuf[DEFAULT_BUFLEN];

    bool quote_received = true;

    cout << "Command to send? ";
    cin.getline(sendbuf, DEFAULT_BUFLEN);
    toUpperCase(sendbuf);

    while ((_stricmp(sendbuf, "quit") != 0)) {
        iResult = sendto(ClientSocket, sendbuf, strlen(sendbuf) + 1, 0, result->ai_addr, result->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            cout << "send failed: " << WSAGetLastError() << endl;
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }
        sockaddr_in senderAddr;
        int senderAddrSize = sizeof(senderAddr);
        toUpperCase(sendbuf);

        if ((_stricmp(sendbuf, "sendqotd") == 0)) {
            cout << "\nQuote of the Day:\n";
            do {
                iResult = recvfrom(ClientSocket, recvbuf, recvbuflen, 0, (sockaddr*)&senderAddr, &senderAddrSize);
                if (iResult > 0) {
                    recvbuf[iResult] = '\0';
                    cout << recvbuf << endl;
                }
                else if (iResult == 0) {
                    cout << "No more data received." << endl;
                }
                else {
                    cout << "recvfrom failed: " << WSAGetLastError() << endl;
                    break;
                }
            } while (wait(ClientSocket, 1, 0) > 0);
        }
        cout << "Command to send? ";
        cin.getline(sendbuf, DEFAULT_BUFLEN);
    }

    closesocket(ClientSocket);
    WSACleanup();
    return 0;
}