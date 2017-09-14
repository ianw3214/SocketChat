#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <map>
#include <mutex>
#include <string.h>
#include <string>
#include <unistd.h>

#define PORT 3030
#define MAX_CLIENTS 5

std::mutex clientmaplock;
std::map<int, int> clients;
int numclients;

void sendmessages(char message[], int messagesize, int sender) {
    std::lock_guard<std::mutex> lock(clientmaplock);
    for (const auto e: clients) {
        if (e.second == sender) continue;
        if (send(e.second, message, messagesize, 0) == -1) {
            std::cout << "ERROR: failed to send message." << std::endl;
        }
    }
}

void recievemessages(int fd) {
    // recieve the name from the client first
    char name[256];
    if (recv(fd, name, sizeof(name), 0) <= 0) {
        std::cout << "ERROR: failed to recieve the name from user." << std::endl;
    }
    char namemessage[276];
    strcpy(namemessage, name);
    strcat(namemessage, " has joined the room");
    std::cout << namemessage << std::endl;   
    sendmessages(namemessage, sizeof(namemessage), fd); 
    
    // continuously get messages until the client disconnects
    char buffer[256];
    while (true) {
        if (recv(fd, buffer, sizeof(buffer), 0) <= 0) break;
        // if the connection wasn't lost, add update the message
        char message[515];      // 515 for name + " : " + message
        strcpy(message, name);
        strcat(message, " : ");
        strcat(message, buffer);
        sendmessages(message, sizeof(message), fd);
        std::cout << message << std::endl;
    }
    
    char quitmessage[272];
    strcpy(quitmessage, name);
    strcat(quitmessage, " disconnected..");
    std::cout << quitmessage << std::endl;
    sendmessages(quitmessage, sizeof(quitmessage), fd);
    // remove the user from the map and decrease client count
    int desiredid;
    for (const auto e : clients) {
        if (e.second == fd) {
            desiredid = e.first;
            break;
        }
    }
    clients.erase(desiredid);
}

int main(int argc, char* argv[]){

    numclients = 0;

    // a socket descriptor for listening for connections
    int listenfd;
    
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0){
        std::cout << "ERROR: failed to create socket." << std::endl;
        return 1;
    }
    std::cout << "Successfully created socket." << std::endl;    

    // bind the socket descriptor 
    sockaddr_in serveraddress;
    serveraddress.sin_family = AF_INET;
    serveraddress.sin_port = htons(PORT);
    serveraddress.sin_addr.s_addr = INADDR_ANY;
    if (bind(listenfd, (sockaddr*) &serveraddress, sizeof(serveraddress)) == -1) {
        std::cout << "ERROR: failed to bind socket." << std::endl;
        return 1;
    } 
    std::cout << "Successfully bound socket." << std::endl;

    // listen for a connection
    if (listen(listenfd, MAX_CLIENTS) == -1) {
        std::cout << "ERROR: socket failed to listen." << std::endl;
        return 1;
    }
    std::cout << "Listening for connections..." << std::endl;

    // keep accepting connections as long as the server is still runninng
    while (true) {
        // wait for a bit if the number clients is already at the max
        clientmaplock.lock();
        bool shouldwait = numclients >= MAX_CLIENTS;
        clientmaplock.unlock();
        if (shouldwait) sleep(5);
        // look for new connections on the listening socket
        int connfd;
        sockaddr_in clientaddress;
        socklen_t clilen = sizeof(clientaddress);
        connfd = accept(listenfd, (sockaddr*) &clientaddress, &clilen);
        if (connfd < 0) {
            std::cout << "ERROR: failed to connect to client." << std::endl;
            return 1;
        }
        // use the mutex lock to ensure thread safety
        std::lock_guard<std::mutex> lock(clientmaplock);
        // add the new client to the map
        for (int i = 0; i < MAX_CLIENTS; i++) {
            // find an ID for the client in the map and add it
            if (clients.find(i) == clients.end()) {
                clients[i] = connfd;
                numclients++;
                break;
            }
        }
        std::thread * temp = new std::thread(recievemessages, connfd);
        temp->detach();
    }

    // close the socket descriptors
    for (const auto e : clients) {
        close(e.second);
    }
    close (listenfd);

    return 0;

}
