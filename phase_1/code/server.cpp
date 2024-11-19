#include"server.hpp"
#include"defAndFuc.hpp"

int serverSocketFDForSignal;

// 信号处理函数
void handle_signal(int sig) {
    if (sig == SIGINT) {
        printf("\nServer shutting down...\n");
        close(serverSocketFDForSignal);
        exit(EXIT_FAILURE);
    }
}

Server::Server(int port){
    serverSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    serverSocketFDForSignal = serverSocketFD;
    if (serverSocketFD < 0) {
        errorCaller("Server socket creation failed");
    }
    // clear address structure
    memset(&serverADDR, 0, sizeof(serverADDR));
    serverADDR.sin_family = AF_INET;
    serverADDR.sin_addr.s_addr = INADDR_ANY;
    serverADDR.sin_port = htons(port);
}

// Server::~Server() {
//     close(serverSocketFD);
// }

void Server::proc_server(int port){
    //? Binding socket
    if(bind(serverSocketFD, (struct sockaddr*)&serverADDR, sizeof(serverADDR)) < 0){
        closeSocket(serverSocketFD);
        errorCaller("--- ERROR on server binding ---");
    }
    //? Listening
    if(listen(serverSocketFD, MAX_CLIENTS) < 0){
        closeSocket(serverSocketFD);
        errorCaller("--- ERROR on listening ---");
    }
    printf("Server now listening on port %d --- (ready)\n", port);
}

int Server::waitForConnecting(){
    struct sockaddr_in clientADDR;
    socklen_t client_len = sizeof(clientADDR);
    int connSocketFD = accept(serverSocketFD, (struct sockaddr*)&clientADDR, &client_len);
    if(errno == EINTR){ return -2;}
    return connSocketFD;
}

int Server::handleUserStatus(int connSocketFD){
    userList.is_cur_logged_in = 0;

    // send(connSocketFD, optionForClient, strlen(optionForClient), 0);

    memset(msgBufferServer, 0, sizeof(msgBufferServer));
    int recv_msg = recv(connSocketFD, msgBufferServer, sizeof(msgBufferServer), 0);
    if(recv_msg == 0)     { return 0;}
    else if(recv_msg < 0) { return -1;}

    if(strlen(msgBufferServer) >= BUFFER_SIZE) {
        send(connSocketFD, msgToolong, strlen(msgToolong), 0);
        return -2;
    }

    cout << "\n--- " << msgBufferServer  << endl;

    //! Parse the command
    string cmd = string(msgBufferServer, strlen(msgBufferServer));
    istringstream stream(cmd);
    stream >> cmd;
    toLowerCase(cmd);

    string username = "", password = "";
    if(cmd == "register"){
        stream >> username >> password;
        
        if (username.empty() || password.empty() || stream.peek() != EOF) { //? Check input parameters
            send(connSocketFD, invalidRegister, strlen(invalidRegister), 0);
            return -2;
        }else if(username.length() > MAX_USERNAME_LENGTH || password.length() > MAX_PASSWORD_LENGTH){ //? Check username and password length
            send(connSocketFD, nameToolong, strlen(nameToolong), 0);
            return -2;
        }else if(userList.users.find(username) != userList.users.end()){ //? Check if username already exists
            send(connSocketFD, userNameExist, strlen(userNameExist), 0);
            return -2;
        }

        userList.users[username] = password;
        saveUserToFile(username, password);
        ostringstream oss;
        oss << "Registering user: " << username << ", with password " << password << " successfuly !!" << endl;
        string reply = oss.str();
        send(connSocketFD, reply.c_str(), reply.size(), 0);

    }else if(cmd == "login"){
        if(userList.is_cur_logged_in == 1){
            send(connSocketFD, alreadyLogin, strlen(alreadyLogin), 0);
            return -2;
        }
        stream >> username >> password;
        if (username.empty() || password.empty() || stream.peek() != EOF) {
            send(connSocketFD, invalidLogin, strlen(invalidLogin), 0);
            return -2;
        }else if(userList.users.find(username) == userList.users.end()){
            send(connSocketFD, accNotFound, strlen(accNotFound), 0);
            return -2;
        }else if(userList.users[username] != password){
            send(connSocketFD, incorrectPwd, strlen(incorrectPwd), 0);
            return -2;
        }

        userList.is_cur_logged_in = 1;
        userList.curUser = username;
        userList.curPwd = password;
        ostringstream oss;
        oss << "User: " << username << ", LOGIN " << " successfuly !!" << endl;
        string reply = oss.str();
        send(connSocketFD, reply.c_str(), reply.size(), 0);

    }else if(cmd == "exit"){
        if(userList.is_cur_logged_in == 1){
            send(connSocketFD, logoutFirst, strlen(logoutFirst), 0);
            return -2;
        }else{
            send(connSocketFD, goodByeMsg, strlen(goodByeMsg), 0);
            return 2; //! Special case !! (Need to close the connection from server side)
        }
    }else{
        send(connSocketFD, invalidCommand, strlen(invalidCommand), 0);
        return -2;
    }
    return 1;
}

int Server::handleClient(int connSocketFD){
    //! send the selection list to client (processing...)
    //! For now, just assume the client will send the message to server
    //? Server just receive the message from client

    memset(msgBufferServer, 0, sizeof(msgBufferServer));
    int recv_msg = recv(connSocketFD, msgBufferServer, sizeof(msgBufferServer), 0);
    if(recv_msg == 0)     { return 0; }
    else if(recv_msg < 0) { return -1; }
    if(strlen(msgBufferServer) - 5 >= BUFFER_SIZE) {
        cout << "\n--- You send some messages that is too long to accept !!" << endl;
        send(connSocketFD, msgToolong, strlen(msgToolong), 0);
        return 2;
    }

    string msg = string(msgBufferServer, strlen(msgBufferServer));
    string tmp = msg;
    string content = "";
    string checkCase = tmp.substr(0, 4);
    toLowerCase(checkCase);
    toLowerCase(msg);
    if(msg == "logout"){
        userList.is_cur_logged_in = 0;
        userList.curUser = "";
        userList.curPwd = "";
        cout << "\n--- " << msg << endl;
        send(connSocketFD, logoutMsg, strlen(logoutMsg), 0);
        return 1;
    }else if(msg == "exit"){
        cout << "\n--- " << msg << endl;
        send(connSocketFD, logoutFirst, strlen(logoutFirst), 0);
        return 2;
    }else if(tmp.rfind("send ", 0) == 0 || checkCase == "send"){
        size_t pos = tmp.find(" ");
        content = tmp.substr(pos + 1);
        if(content.empty()){
            send(connSocketFD, emptyMsg, strlen(emptyMsg), 0);
            return 2;
        }
    }else{
        cout << "\n--- Something strange: " << tmp << endl;
        send(connSocketFD, invalidCommand, strlen(invalidCommand), 0);
        return 2;
    }
    cout << "\n--- Message from client: " << content << endl;
    string reply = "Your message is: " + content + "\n";
    send(connSocketFD, reply.c_str(), reply.size(), 0);
    return 2;
}

int main(int argc, char *argv[]) {
    // signal(SIGPIPE, SIG_IGN);
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    //? Get the input argv
    int portNumber = atoi(argv[1]);

    // 设置信号处理
    // signal(SIGINT, handle_signal);

    //? Calloc the space for server structure variable
    Server server(portNumber);

    //? Step of server initialization with socket and given port number
    server.proc_server(portNumber);

    signal(SIGINT, handle_signal);

    loadUserFromFile();

    while (1) {
        
        int connSocketFD = server.waitForConnecting(); //! 等待客戶端連接
        if (connSocketFD == -2) { //! 如果被信號中斷則跳出循環
            break;
        }else if (connSocketFD == -1){
            perror("~~~ Accept failed ~~~");
            continue; //! wait for new connection
        }
        
        printf("New connection of client accepted !!\n");
        
        while (1) {
            int keepTypeMsg = 2;

            int checkClient = server.handleUserStatus(connSocketFD);

            if(checkClient == 0){
                dprintf(STDERR_FILENO, "Connection lost\n");
                break;
            }else if(checkClient == -1){
                dprintf(STDERR_FILENO, "Error on recv\n");
                break;
            }else if(checkClient == -2){
                continue;
            }else if(checkClient == 2){
                // close the client connection
                printf("Client disconnected\n");
                break;
            }

            if(userList.is_cur_logged_in != 1){ continue; }

            //! Might change to be selection list for different transfer option
            while(keepTypeMsg != 1 && keepTypeMsg != 0 && keepTypeMsg != -1){
                int checkClientMsg = server.handleClient(connSocketFD);
                keepTypeMsg = checkClientMsg; //! 1 -> logout
                if(checkClient == 0){
                    dprintf(STDERR_FILENO, "Connection lost\n");
                    break;
                }else if(checkClient == -1){
                    dprintf(STDERR_FILENO, "Error on recv\n");
                    break;
                }
            }
            if(keepTypeMsg == 0 || keepTypeMsg == -1){
                break;
            }
        }
        closeSocket(connSocketFD);
    }
    dprintf(STDERR_FILENO, "Server closed\n");
    int serverSocket_to_close = server.getServerSocketFD();
    closeSocket(serverSocket_to_close);
    return 0;
}