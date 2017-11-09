#include <iostream>
#include <winsock2.h>
#include <Ws2tcpip.h>

#include <string>
#include <thread>
#include <mutex>

#define DEFAULT_PORT "3030"
#define DEFAULT_ADDRESS "34.230.29.192"

std::mutex recievingLock;
bool recieving = false;

void recieveMessages(int connectSocket) {
    // 515 chars for name + " : " + message
    char buffer[515];
    while (true) {
        if (recv(connectSocket, buffer, sizeof(buffer), 0) < 0)
        {
            // if we are no longer recieving, it is not an error
            recievingLock.lock();
            if (!recieving) break;
            recievingLock.unlock();
            std::cout << "ERROR: failed to recieve message." << std::endl;
            break;
        }
        std::cout << buffer << std::endl;
    }
}

// client code I think
int main(){

    // first get the clients name
    char name[256];
    std::cout << "Please input your name: ";
    std::cin.getline(name, 256);

    // intialize winsock library
    WSADATA wsaData;
    int initResult;
    // the WSAStartup function initiates use of the winsock DLL by a process
    // first argument specifies highest version of sockets specification the caller can use
    // second argument is a data structure to recieve details of sockets implementation
    initResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    // check result = 0 for success
    if (initResult != 0) {
        std::cout << "WSAStartup failed, ERROR CODE: " << initResult << std::endl;
        return 1;
    }

    // addrinfo contains a sockaddr struct
    struct addrinfo *result = NULL, *ptr = NULL, hints;
    // initialize the addrInfo structs
    ZeroMemory(&hints, sizeof(hints));  // fills a block of memory with 0s
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // resolve the server address and port
    initResult = getaddrinfo(DEFAULT_ADDRESS, DEFAULT_PORT, &hints, &result);
    if (initResult != 0) {
        std::cout << "getaddrinfo failed, ERROR CODE: " << initResult << std::endl;
        WSACleanup();
        return 1;
    }

    // then create the actual socket that will connect to the server
    SOCKET connectSocket = INVALID_SOCKET;
    // attempt to connect to the first address returned by the call to getaddrinfo
    ptr = result;
    // create a SOCKET for the connecting server
    connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    // error check to make sure it is a valid socket
    if (connectSocket == INVALID_SOCKET) {
        std::cout << "Error at socket(): " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // actually connect to the server
    int connectResult = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
    if (connectResult == SOCKET_ERROR) {
        closesocket(connectSocket);
        connectSocket = INVALID_SOCKET;
    }

    freeaddrinfo(result);

    if (connectSocket == INVALID_SOCKET) {
        std::cout << "Unable to connect to server..." << std::endl;
        WSACleanup();
        return 1;
    }

    std::cout << "Successfully connected to server" << std::endl;

    // send the name of the client first
    if (send(connectSocket, name, sizeof(name), 0) < 0)
    {
        std::cout << "Failed to send name to server." << std::endl;
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Message sent successfully" << std::endl;

    // create a thread for recieving messages from the server
    recievingLock.lock();
    recieving = true;
    recievingLock.unlock();
    std::thread recievethread(recieveMessages, connectSocket);
    recievethread.detach();

    // continuously send messages
    char buffer[256];
    while (true)
    {
        std::cin.getline(buffer, 256);
        if (strcmp(buffer, "quit") == 0) {
            recievingLock.lock();
            recieving = false;
            recievingLock.unlock();
            break;
        }
        if (send(connectSocket, buffer, sizeof(buffer), 0) < 0)
        {
            std::cout << "ERROR: failed to send message." << std::endl;
            closesocket(connectSocket);
            WSACleanup();
            return 1;
        }
    }
    // finally, we shut down the client
    int shutdownResult = shutdown(connectSocket, SD_RECEIVE);
    if (shutdownResult == SOCKET_ERROR)
    {
        std::cout << "Shutdown failed, ERROR CODE: " << shutdownResult << std::endl;
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }
    shutdownResult = shutdown(connectSocket, SD_SEND);
    if (shutdownResult == SOCKET_ERROR) {
        std::cout << "Shutdown failed, ERROR CODE: " << shutdownResult << std::endl;
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    closesocket(connectSocket);
    WSACleanup();

    return 0;

}