/**
 * @file Server.cpp
 * @brief Implementation of the IRC Server core functionality
 * 
 * This file contains the implementation of the IRC server's core functionality,
 * including connection handling, message processing, and client management.
 * The server is implemented using the Singleton pattern to ensure only one instance exists.
 */

#include "../Includes/Server.hpp"

bool signalInterrupt = false;

/**
 * @brief Get the singleton instance of the Server
 * @return Server* Pointer to the server instance
 * 
 * Implements the Singleton pattern to ensure only one server instance exists
 * throughout the application's lifecycle.
 */
Server* Server::getInstance(void) {
    if (!Server::_instance)
        Server::_instance = new Server();
    return Server::_instance;
}

/**
 * @brief Initialize the IRC server
 * @throws IrcException if server initialization fails
 * 
 * Sets up the server socket with:
 * - Non-blocking I/O
 * - TCP/IP configuration
 * - Address reuse
 * - Binding to specified port
 * - Listen queue initialization
 */
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

/**
 * @brief Signal handler for interrupt signals
 * @param signal The signal received
 * 
 * Handles interrupt signals (SIGTSTP, SIGINT, SIGQUIT) by setting the signalInterrupt flag.
 */
void Server::signalHandler(int signal) {
    std::cerr << "Interrupt Signal (" << signal << ") received, Shutting down the Server..." << std::endl;
    signalInterrupt = true;

    return;
}

/**
 * @brief Main server loop
 * 
 * Implements the core server event loop using poll() for I/O multiplexing:
 * 1. Monitors all client sockets for activity
 * 2. Accepts new connections
 * 3. Handles client messages
 * 4. Manages client disconnections
 * 5. Processes server replies
 */
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

/**
 * @brief Send queued messages to a client
 * @param client_fd The client's file descriptor
 * 
 * Processes and sends all queued messages (serverReplies) for a specific client.
 * Messages are removed from the queue after successful transmission.
 */
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

/**
 * @brief Handle new client connections
 * 
 * Accepts new client connections and:
 * 1. Sets up non-blocking I/O
 * 2. Creates a new Client object
 * 3. Initializes client state
 * 4. Adds client to the server's client list
 */
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

/**
 * @brief Receive data from a client
 * @param fd The client's file descriptor
 * @return int Number of bytes received or error code
 * 
 * Handles raw data reception from clients, managing:
 * - Partial receives
 * - Connection errors
 * - Buffer management
 */
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

/**
 * @brief Handle client disconnection
 * @param client_fd The client's file descriptor
 * @param bytesRecv Number of bytes received (used to determine disconnection type)
 * 
 * Manages client disconnection:
 * 1. Notifies other clients/channels
 * 2. Cleans up client resources
 * 3. Updates server state
 */
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

/**
 * @brief Process received client messages
 * @param client_fd The client's file descriptor
 * 
 * Handles IRC protocol messages:
 * 1. Parses raw messages
 * 2. Validates message format
 * 3. Routes to appropriate command handlers
 * 4. Manages command responses
 */
void Server::handleClientMessage(int client_fd) {
    int bytesRecv = ft_recv(client_fd);

    if (bytesRecv <= 0) {
        handleClientDisconnection(client_fd, bytesRecv);
        return;
    }

    // Append new data to client's buffer
    _clients[client_fd]->appendToBuffer(_message);
    
    std::string& buffer = _clients[client_fd]->getBuffer();
    std::vector<std::string> commandList;
    size_t pos;

    // Find complete commands (ending with \n or \r\n)
    while ((pos = buffer.find('\n')) != std::string::npos) {
        // Extract the complete command (including the \n)
        std::string completeCommand = buffer.substr(0, pos + 1);
        
        // Remove the processed command from buffer
        buffer.erase(0, pos + 1);
        
        // Process the complete command
        std::cout << "Received complete command from client " << _clients[client_fd]->getNickname() 
                 << ": " << completeCommand;
        
        ParseMessage parsedMsg(completeCommand);
        processCommand(_clients[client_fd], parsedMsg);
    }

    return;
}

/**
 * @brief Close a client connection
 * @param client_fd The client's file descriptor
 * 
 * Performs clean client disconnection:
 * 1. Closes socket
 * 2. Removes from channels
 * 3. Cleans up resources
 * 4. Updates server state
 */
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

/**
 * @brief Get the server password
 * @return std::string The server password
 */
std::string Server::getServerPassword(void) {
    return _serverPassword;
}

/**
 * @brief Clean up server resources
 * 
 * Performs server shutdown and cleanup:
 * 1. Closes all client sockets
 * 2. Removes all clients from channels
 * 3. Cleans up server resources
 * 4. Exits the application
 */
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