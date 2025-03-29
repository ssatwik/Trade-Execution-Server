#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
using namespace std;

int clientSocket;
int serverPort = 5555;
char serverIP[] = "127.0.0.1";
struct sockaddr_in serverAddr;

const int BUFFER_SIZE = 1025;
int socketDataReady = 0, terminalInputReady = 0;
char socketBuffer[BUFFER_SIZE], terminalBuffer[BUFFER_SIZE], tempBuffer[BUFFER_SIZE];
pthread_t socketThread, terminalThread;

void *handleTerminalInput(void *args) {
    while (1) {
        int index = 0;
        bzero(terminalBuffer, BUFFER_SIZE);
        while ((terminalBuffer[index++] = getchar()) != '\n');
        terminalInputReady = 1;
        while (terminalInputReady == 1);
    }
}

void *handleServerResponse(void *args) {
    while (1) {
        bzero(tempBuffer, BUFFER_SIZE);
        recv(clientSocket, tempBuffer, BUFFER_SIZE, 0);
        socketDataReady = 1;
        while (socketDataReady == 1);
    }
}

void readFromSocket() {
    while (socketDataReady != 1);
    bzero(socketBuffer, BUFFER_SIZE);
    strcpy(socketBuffer, tempBuffer);
    socketDataReady = 0;
}

int readFromTerminal() {
    fflush(stdout);
    while (1) {
        if (terminalInputReady == 1) {
            return 1;
        }
    }
}

int main() {
    socklen_t addrLen;

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        cout << "[-] Socket error\n";
        exit(1);
    }
    cout << "[+] TCP Client Socket created.\n";
    bzero(&serverAddr, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = serverPort;

    int connectionStatus = connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (connectionStatus != 0) {
        cout << "[-] Connection failed.\n";
        exit(1);
    }
    cout << "[+] Connected to server.\n";

    pthread_create(&socketThread, NULL, handleServerResponse, NULL);
    pthread_create(&terminalThread, NULL, handleTerminalInput, NULL);

    while (1) {
        bzero(socketBuffer, BUFFER_SIZE);

        cout << "Enter input: ";
        int inputStatus = readFromTerminal();

        for (int i = 0; i < BUFFER_SIZE; i++) {
            socketBuffer[i] = terminalBuffer[i];
        }
        terminalInputReady = 0;

        send(clientSocket, socketBuffer, BUFFER_SIZE, 0);

        bzero(socketBuffer, BUFFER_SIZE);
        readFromSocket();

        cout << socketBuffer << "\n";
    }
    return 0;
}
