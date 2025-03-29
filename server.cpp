#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
using namespace std;

const int PORT = 5555;
const char SERVER_IP[] = "127.0.0.1";
const int BUFFER_SIZE = 1025;

char messageBuffer[BUFFER_SIZE];
socklen_t addressLength;
int serverSocket, newSocket;
int socketDescriptor = 0, maxSocketDescriptor = 0, clientCount = 0, clientIdCounter = 0;
int clientSockets[BUFFER_SIZE];
int clientIDs[BUFFER_SIZE];
struct sockaddr_in serverAddress;

class ClientInput {
public:
    int clientSocket;
    int clientId;
    string rawText = "";
    vector<string> parsedText = {""};
    string commandType = "";
    
    ClientInput(int sockfd, int id, string text) {
        clientSocket = sockfd;
        clientId = id;
        rawText = text;
        
        for(char c : text) {
            if(c == ' ') {
                parsedText.push_back("");
            } else {
                parsedText.back() += c;
            }
        }
        commandType = parsedText[0];
    }
    
    string getCommandType() {
        return parsedText[0];
    }
};

class TradeOrder {
public:
    int orderId = 0;
    int clientId = 0;
    int stockId = 0;
    int quantity;
    int price;
    bool isBuy = false;
    bool isSell = false;
    string stockSymbol = "";
    string originalText = "";
    string orderType = "";
    
    TradeOrder() {}
    TradeOrder(int id, ClientInput input) {
        orderId = id;
        clientId = input.clientId;
        originalText = input.rawText;
        orderType = input.parsedText[0];
        isBuy = (orderType == "BUY");
        isSell = (orderType == "SELL");
        stockSymbol = input.parsedText[1];
        quantity = stoi(input.parsedText[2]);
        price = stoi(input.parsedText[3]);
    }
    
    void setStockId(int id) {
        stockId = id;
    }
    
    string getOrderDetails() {
        string output = "orderID:" + to_string(orderId) + " | ";
        output += originalText;
        while(!output.empty() && output.back() == '\n') {
            output.pop_back();
        }
        output += " --> from Client - " + to_string(clientId) + "\n";
        return output;
    }
};

class StockMarket {
public:
    int stockId = 0;
    int orderCount = 0;
    string stockName = "";
    vector<TradeOrder> orders;
    vector<string> transactionLog = {""};
    
    StockMarket(int id = 0, string name = "") {
        stockId = id;
        stockName = name;
    }
    
    void logTransaction(TradeOrder order) {
        transactionLog.push_back(order.getOrderDetails());
    }
    
    string generateOrderList() {
        string log = "\nLIST orders for Stock: " + stockName + "\n";
        for(const auto& entry : transactionLog) {
            log += entry;
        }
        return log;
    }
};

class TradingEngine {
public:
    int orderCounter = 1;
    int stockCounter = 1;
    vector<TradeOrder> activeOrders;
    vector<StockMarket> stockMarkets;
    map<string, int> stockIdMap;
    
    static bool compareHigherPrice(TradeOrder a, TradeOrder b) {
        return a.price > b.price;
    }
    
    static bool compareLowerPrice(TradeOrder a, TradeOrder b) {
        return a.price < b.price;
    }
    
    static bool compareOlderOrders(TradeOrder a, TradeOrder b) {
        return a.orderId < b.orderId;
    }
    
    string processCommand(ClientInput input) {
        if(input.commandType == "BUY" || input.commandType == "SELL") {
            TradeOrder newOrder(orderCounter, input);
            orderCounter++;
            
            if(stockIdMap[newOrder.stockSymbol] == 0) {
                stockIdMap[newOrder.stockSymbol] = stockCounter;
                stockMarkets.emplace_back(stockCounter, newOrder.stockSymbol);
                newOrder.setStockId(stockCounter);
                stockCounter++;
            }
            
            int stockIndex = stockIdMap[newOrder.stockSymbol] - 1;
            stockMarkets[stockIndex].logTransaction(newOrder);
            activeOrders.push_back(newOrder);
            
            string result = "\n";
            if(newOrder.isBuy) {
                auto matchedOrders = matchBuyOrders(newOrder);
                result += "Executing buy order ...\n";
                if(matchedOrders.empty()) {
                    result += "\tNo orders matched.\n";
                } else {
                    for(const auto& order : matchedOrders) {
                        result += "\t" + order.getOrderDetails();
                    }
                }
                result += "Remaining: " + to_string(newOrder.quantity) + " shares to be bought.\n";
            } else {
                auto matchedOrders = matchSellOrders(newOrder);
                result += "Executing sell order ...\n";
                if(matchedOrders.empty()) {
                    result += "\tNo orders matched.\n";
                } else {
                    for(const auto& order : matchedOrders) {
                        result += "\t" + order.getOrderDetails();
                    }
                }
                result += "Remaining: " + to_string(newOrder.quantity) + " shares to be sold.\n";
            }
            return result;
        } else if(input.commandType == "LIST") {
            string stockName = input.parsedText[1];
            int marketIndex = stockIdMap[stockName] - 1;
            return stockMarkets[marketIndex].generateOrderList();
        }
        return "Error: Invalid command\n";
    }

private:
    vector<TradeOrder> matchBuyOrders(TradeOrder &buyOrder) {
        vector<TradeOrder> matchedOrders;
        vector<TradeOrder> remainingOrders;
        
        sort(activeOrders.begin(), activeOrders.end(), compareLowerPrice);
        
        for(auto& order : activeOrders) {
            bool isValidMatch = order.isSell && 
                              order.price <= buyOrder.price && 
                              order.clientId != buyOrder.clientId && 
                              buyOrder.quantity > 0;
            
            if(isValidMatch) {
                TradeOrder matched = order;
                matched.quantity = min(order.quantity, buyOrder.quantity);
                
                order.quantity -= matched.quantity;
                buyOrder.quantity -= matched.quantity;
                matchedOrders.push_back(matched);
            }
            
            if(order.quantity > 0) {
                remainingOrders.push_back(order);
            }
        }
        
        activeOrders = remainingOrders;
        sort(matchedOrders.begin(), matchedOrders.end(), compareOlderOrders);
        return matchedOrders;
    }

    vector<TradeOrder> matchSellOrders(TradeOrder &sellOrder) {
        vector<TradeOrder> matchedOrders;
        vector<TradeOrder> remainingOrders;
        
        sort(activeOrders.begin(), activeOrders.end(), compareHigherPrice);
        
        for(auto& order : activeOrders) {
            bool isValidMatch = order.isBuy && 
                              order.price >= sellOrder.price && 
                              order.clientId != sellOrder.clientId && 
                              sellOrder.quantity > 0;
            
            if(isValidMatch) {
                TradeOrder matched = order;
                matched.quantity = min(order.quantity, sellOrder.quantity);
                
                order.quantity -= matched.quantity;
                sellOrder.quantity -= matched.quantity;
                matchedOrders.push_back(matched);
            }
            
            if(order.quantity > 0) {
                remainingOrders.push_back(order);
            }
        }
        
        activeOrders = remainingOrders;
        sort(matchedOrders.begin(), matchedOrders.end(), compareOlderOrders);
        return matchedOrders;
    }
};

int main() {
    fd_set readDescriptors;
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    
    if(serverSocket < 0) {
        cerr << "[-] Socket creation error" << endl;
        exit(EXIT_FAILURE);
    }
    cout << "[+] TCP server socket created successfully" << endl;
    
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    if(bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        cerr << "[-] Binding failed" << endl;
        close(serverSocket);
        exit(EXIT_FAILURE);
    }
    cout << "[+] Successfully bound to port " << PORT << endl;
    
    addressLength = sizeof(serverAddress);
    listen(serverSocket, 5);
    cout << "[+] Server listening for connections..." << endl;
    
    TradingEngine exchange;
    
    while(true) {
        FD_ZERO(&readDescriptors);
        FD_SET(serverSocket, &readDescriptors);
        maxSocketDescriptor = serverSocket;
        
        for(int i = 0; i < BUFFER_SIZE; i++) {
            socketDescriptor = clientSockets[i];
            if(socketDescriptor > 0) {
                FD_SET(socketDescriptor, &readDescriptors);
            }
            maxSocketDescriptor = max(maxSocketDescriptor, socketDescriptor);
        }
        
        struct timeval pollInterval {0, 0};
        int activeSockets = select(maxSocketDescriptor + 1, &readDescriptors, nullptr, nullptr, &pollInterval);
        
        if(FD_ISSET(serverSocket, &readDescriptors)) {
            newSocket = accept(serverSocket, (struct sockaddr*)&serverAddress, &addressLength);
            if(newSocket < 0) {
                cerr << "[-] Connection acceptance error" << endl;
                continue;
            }
            
            cout << "[+] New connection - Socket: " << newSocket 
                 << " | IP: " << inet_ntoa(serverAddress.sin_addr)
                 << " | Port: " << ntohs(serverAddress.sin_port) << endl;
            
            clientCount++;
            for(int i = 0; i < BUFFER_SIZE; i++) {
                if(clientSockets[i] == 0) {
                    clientSockets[i] = newSocket;
                    clientIDs[i] = ++clientIdCounter;
                    break;
                }
            }
        }
        
        for(int i = 0; i < BUFFER_SIZE; i++) {
            socketDescriptor = clientSockets[i];
            if(FD_ISSET(socketDescriptor, &readDescriptors)) {
                int bytesRead = read(socketDescriptor, messageBuffer, BUFFER_SIZE);
                if(bytesRead > 0) {
                    cout << "Client " << clientIDs[i] << ": " << messageBuffer << endl;
                    
                    ClientInput input(socketDescriptor, clientIDs[i], string(messageBuffer));
                    string response = exchange.processCommand(input) + "\n";
                    
                    memset(messageBuffer, 0, BUFFER_SIZE);
                    strncpy(messageBuffer, response.c_str(), BUFFER_SIZE-1);
                    send(socketDescriptor, messageBuffer, strlen(messageBuffer), 0);
                    memset(messageBuffer, 0, BUFFER_SIZE);
                } else {
                    getpeername(socketDescriptor, (struct sockaddr*)&serverAddress, &addressLength);
                    cout << "[+] Client disconnected - IP: " << inet_ntoa(serverAddress.sin_addr)
                         << " | Port: " << ntohs(serverAddress.sin_port) << endl;
                         
                    clientCount--;
                    close(socketDescriptor);
                    clientSockets[i] = 0;
                    clientIDs[i] = 0;
                }
            }
        }
    }
    return 0;
}
