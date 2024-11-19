#include"client.hpp"
#include"defAndFuc.hpp"

// bool is_logged_in = false;
int client_socket;

void handle_signal(int sig) {
    if (sig == SIGINT && userList.is_cur_logged_in) {
        printf("\nLogout first, and don't use Ctrl C !!\n");
    } else if (sig == SIGINT) {
        // closeSocket(client_socket);
        printf("\nCan't use Ctrl C to kill process !!\n");
    }
}

Client::Client(char * serverIP, int port) {
    clientSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocketFD < 0) {
        closeSocket(client_socket);
        errorCaller("Client socket creation failed");
    }
    client_socket = clientSocketFD;
    // clear address structure
    memset(&clientADDR, 0, sizeof(clientADDR));
    clientADDR.sin_family = AF_INET;
    clientADDR.sin_addr.s_addr = inet_addr(serverIP);
    clientADDR.sin_port = htons(port);
}

void Client::connectTheServer(int port) {
    if(connect(clientSocketFD, (struct sockaddr*)&clientADDR, sizeof(clientADDR)) < 0){
        //! errno
        // ECONNREFUSED：目標拒絕連接。
        // ETIMEDOUT：連接超時，無法到達目標。
        // ENETUNREACH：網路不可達，例如無法訪問該 IP 地址的網路。
        closeSocket(client_socket);
        errorCaller("Connection failed");
    }
    string connectSuccMsg = "You are now connect to the server listening on port " + to_string(port);
    cout << connectSuccMsg << "\n";
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_IP_address> <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *serverIP = argv[1];
    int client_port = atoi(argv[2]);

    if(!isValidIPAddress(serverIP)){
        errorCaller("Invalid IP address");
    }

    Client client(serverIP, client_port);

    client.connectTheServer(client_port);

    signal(SIGINT, handle_signal);
    while (1) {
        if(!userList.is_cur_logged_in){
            printf("%s", optionForClient);
        }else{
            printf("%s", justSaySomething);
        }
        string input = "";
        if (!getline(cin, input)) {
            printf("\nGet input failed.\n");
            break;
        }
        if(input.empty()){
            printf("\nPlease input something.\n");
            continue;
        }
        if (send(client_socket, input.c_str(), input.length(), 0) < 0) {
            perror("Send failed");
            break;
        }
        if(input == "logout"){
            userList.is_cur_logged_in = false;
        }
        memset(msgBufferClient, 0, sizeof(msgBufferClient));
        int recv_msg = recv(client_socket, msgBufferClient, sizeof(msgBufferClient), 0);
        if(recv_msg <= 0){
            closeSocket(client_socket);
            errorCaller("recv failed");
        }else{
            printf("\n--- %s\n", msgBufferClient);
        }
        if(string(msgBufferClient) == string(goodByeMsg)){
            userList.is_cur_logged_in = false;
            break;
        }else if(string(msgBufferClient).find("LOGIN") != string::npos){
            userList.is_cur_logged_in = true;
        }
    }
    
    closeSocket(client_socket);
    return 0;
}