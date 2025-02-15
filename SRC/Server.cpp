#include "../Includes/Server.hpp"

bool signalInterrupt = false;

Server* Server::getInstance(void) {
    if (!Server::_instance)
        Server::_instance = new Server();
    return Server::_instance;
}

void Server::initServer(void) {
    _listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_listeningSocket == -1) {
        throw IrcException("Can't create a socket!");
    }

    if (fcntl(_listeningSocket, F_SETFL, O_NONBLOCK) == -1) {
        throw IrcException("Can't set file descriptor flags");
    }

    _serverHint.sin_family = AF_INET;
    _serverHint.sin_addr.s_addr = INADDR_ANY;
    _serverHint.sin_port = htons(_serverPort);

    int opt = 1;
    if (setsockopt(_listeningSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        close(_listeningSocket);
        throw IrcException("Can't set socket options");
    }

    if (bind(_listeningSocket, (struct sockaddr*)&_serverHint, sizeof(_serverHint)) == -1) {
        perror("bind");
        close(_listeningSocket);
        throw IrcException("Can't bind to IP/port");
    }

    if (listen(_listeningSocket, 1) == -1) {
        perror("listen");
        close(_listeningSocket);
        throw IrcException("Can't listen!");
    }

    _hintLen = sizeof(_serverHint);

    gethostname(_host, NI_MAXHOST);
    std::cout << "IRC server Listening on " << _host << " on port " << _serverPort << std::endl;
    std::cout << "Waiting for incoming connections..." << std::endl;

    pollfd listeningSocketPoll;
    memset(&listeningSocketPoll, 0, sizeof(listeningSocketPoll));
    listeningSocketPoll.fd = _listeningSocket;
    listeningSocketPoll.events = POLLIN;
    listeningSocketPoll.revents = 0;
    _fds.push_back(listeningSocketPoll);

    return;
}

void Server::signalHandler(int signal) {
    std::cerr << "Interrupt Signal (" << signal << ") received, Shutting down the Server..." << std::endl;
    signalInterrupt = true;

    return;
}

void Server::runServer(void) {
    signal(SIGTSTP, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGQUIT, signalHandler);

    while (signalInterrupt == false) {
        if (poll((&_fds[0]), _fds.size(), 1000) == -1) {
            cleanupServer();
            perror("poll");
            throw IrcException("Poll error");
        }

        if (_fds[0].revents & POLLIN) {
            handleNewConnection();
        }

        std::vector<pollfd>::iterator it = _fds.begin();
        while (it != _fds.end()) {
            if (it->fd != _listeningSocket && it->revents & POLLIN) {
                try {
                    handleClientMessage(it->fd);
                } catch (...) {
                    it->fd = -1;
                }
            } else if (it->fd != _listeningSocket && it->revents & POLLOUT) {
                sendToClient(it->fd);
            }

            if (it->fd == -1) {
                closeClient(it->fd);
            } else {
                ++it;
            }
        }
    }

    cleanupServer();
    return;
}

void Server::sendToClient(int client_fd) {
    Client* client = _clients[client_fd];
    std::vector<std::string>::iterator it = client->serverReplies.begin();

    for (; it != client->serverReplies.end(); ++it) {
        std::cout << "............................................" << std::endl;
        std::cout << "Sending message to client " << client->getNickname() << ": " << *it << std::endl;
        std::cout << "............................................" << std::endl;
        if (send(client_fd, it->c_str(), it->size(), 0) == -1) {
            std::cerr << "Error sending message to client " << client->getNickname() << " (" << strerror(errno) << ")" << std::endl;
            return;
        }
    }

    client->serverReplies.clear();

    return;
}

void Server::handleNewConnection(void) {
    sockaddr_in clientHint;
    socklen_t clientSize = sizeof(clientHint);
    int clientSocket = accept(_listeningSocket, (sockaddr*)&clientHint, &clientSize);
    if (clientSocket == -1) {
        perror("accept");
        throw IrcException("Can't accept client connection");
    }

    int result = getnameinfo((sockaddr*)&clientHint, clientSize, _host, NI_MAXHOST, _svc, NI_MAXSERV, 0);
    if (result) {
        std::cout << _host << " connected on " << _svc << std::endl;
    } else {
        inet_ntop(AF_INET, &clientHint.sin_addr, _host, NI_MAXHOST);
        std::cout << _host << " connected on " << ntohs(clientHint.sin_port) << std::endl;
    }

    Client* tmpClient = new Client(clientSocket);
    _clients[clientSocket] = tmpClient;

    pollfd clientPoll;
    memset(&clientPoll, 0, sizeof(clientPoll));
    clientPoll.fd = clientSocket;
    clientPoll.events = POLLIN | POLLOUT;
    clientPoll.revents = 0;
    _fds.push_back(clientPoll);

    return;
}

int Server::ft_recv(int fd) {
    _message.clear();
    _message.resize(BUFFER_SIZE);
    int bytesRecv = recv(fd, &_message[0], BUFFER_SIZE, 0);
    if (bytesRecv <= 0) {
        return bytesRecv;
    }

    _message.resize(bytesRecv);

    return bytesRecv;
}

void Server::handleClientDisconnection(int client_fd, int bytesRecv) {
    if (bytesRecv == 0) {
        std::cout << "Client " << client_fd << " disconnected" << std::endl;
    } else {
        std::cerr << "Error receiving message from client " << client_fd << " (" << strerror(errno) << ")" << std::endl;
    }

    for (std::vector<pollfd>::iterator it = _fds.begin(); it != _fds.end(); ++it) {
        if (it->fd == client_fd) {
            it->fd = -1;
            break;
        }
    }
    return;
}

void Server::handleClientMessage(int client_fd) {
    int bytesRecv = ft_recv(client_fd);

    if (bytesRecv <= 0) {
        handleClientDisconnection(client_fd, bytesRecv);
        return;
    }

    _clients[client_fd]->appendToBuffer(_message);
    
    std::string& buffer = _clients[client_fd]->getBuffer();
    std::vector<std::string> commandList;
    size_t pos;

    while ((pos = buffer.find('\n')) != std::string::npos) {
        std::string completeCommand = buffer.substr(0, pos + 1);
        
        buffer.erase(0, pos + 1);
        
        std::cout << "Received complete command from client " << _clients[client_fd]->getNickname() 
                 << ": " << completeCommand;
        
        ParseMessage parsedMsg(completeCommand);
        processCommand(_clients[client_fd], parsedMsg);
    }

    return;
}

void Server::closeClient(int client_fd) {
    std::map<int, Client*>::iterator it = _clients.find(client_fd);
    if (it != _clients.end()) {
        close(client_fd);
        delete it->second;
        _clients.erase(it);
    }

    for (std::vector<pollfd>::iterator it = _fds.begin(); it != _fds.end();) {
        if (it->fd == client_fd) {
            it = _fds.erase(it);
        } else {
            ++it;
        }
    }
}

std::string Server::getServerPassword(void) {
    return _serverPassword;
}

void Server::cleanupServer(void) {
    std::cout << "Cleaning up server..." << std::endl;
    for (std::vector<pollfd>::iterator it = _fds.begin(); it != _fds.end(); ++it)
        close(it->fd);

    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        delete it->second;

    shutdown(_listeningSocket, SHUT_RDWR);
    close(_listeningSocket);
    _fds.clear();
    _clients.clear();
    delete Server::_instance;
    exit(0);
}

Server* Server::_instance = NULL;