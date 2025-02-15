#include "../Includes/Server.hpp"

/**
 * @brief Default constructor for Client class
 * 
 * Initializes a new Client instance with default values:
 * - File descriptor set to 0
 * - Password authentication status set to false
 * - Empty nickname and username
 * - No channel membership
 * - Registration flags cleared
 */
Client::Client(void) : _fd(0),
                      _isCorrectPassword(false),
                      _nickname(""),
                      _username(""),
                      _channel("") {
    memset(conRegi, 0, 3);
    isRegistered = false;
    return;
}

/**
 * @brief Parameterized constructor for Client class
 * @param fd Socket file descriptor for the client connection
 * 
 * Initializes a new Client instance with a specific file descriptor
 * and default values for other member variables
 */
Client::Client(int fd) : _fd(fd),
                        _isCorrectPassword(false),
                        _nickname(""),
                        _username(""),
                        _channel("") {
    memset(conRegi, 0, 3);
    isRegistered = false;
    return;
}

/**
 * @brief Send a message to the client
 * @param message The message to send
 * @return true if message sent successfully, false otherwise
 * 
 * Sends a message to the client through their socket connection.
 * Handles error checking for the send operation.
 */
bool Client::sendMessage(const std::string &message) {
    if (send(_fd, message.c_str(), message.size(), 0) == -1) {
        return false;
    }
    return true;
}

/**
 * @brief Set the password authentication status
 * @param isCorrectPassword New authentication status
 * 
 * Updates whether the client has provided the correct server password
 */
void Client::setIsCorrectPassword(bool isCorrectPassword) {
    _isCorrectPassword = isCorrectPassword;
    return;
}

/**
 * @brief Set the client's nickname
 * @param nickname New nickname to set
 * 
 * Updates the client's nickname after validation
 */
void Client::setNickname(const std::string &nickname) {
    _nickname = nickname;
    return;
}

/**
 * @brief Set the client's username
 * @param username New username to set
 * 
 * Updates the client's username after validation
 */
void Client::setUsername(const std::string &username) {
    _username = username;
    return;
}

/**
 * @brief Get the client's nickname
 * @return Reference to the client's nickname
 * 
 * Returns the current nickname of the client.
 * Note: Returns const reference that's cast to non-const
 */
std::string &Client::getNickname(void) const {
    return const_cast<std::string &>(_nickname);
}

/**
 * @brief Get the client's username
 * @return The client's username
 */
std::string Client::getUsername(void) const {
    return _username;
}

/**
 * @brief Get the password authentication status
 * @return true if client has authenticated with correct password
 */
bool Client::getIsCorrectPassword(void) const {
    return _isCorrectPassword;
}

/**
 * @brief Set the client's socket file descriptor
 * @param value New file descriptor value
 */
void Client::setFd(int value) {
    _fd = value;
}

/**
 * @brief Append data to the client's message buffer
 * @param data The data to append
 */
void Client::appendToBuffer(const std::string& data) {
    _messageBuffer += data;
}

/**
 * @brief Get the client's message buffer
 * @return Reference to the client's message buffer
 */
std::string& Client::getBuffer() {
    return _messageBuffer;
}

/**
 * @brief Clear the client's message buffer
 */
void Client::clearBuffer() {
    _messageBuffer.clear();
}

/**
 * @brief Get the client's socket file descriptor
 * @return The client's file descriptor
 */
int Client::getFd(void) const {
    return _fd;
}

/**
 * @brief Check if a user exists on the server
 * @param nickname Nickname to check
 * @return true if user exists, false otherwise
 * 
 * Searches the server's client list for a user with the given nickname
 */
bool Server::isUserInServer(std::string nickname) {
    return std::find(_nicknames.begin(), _nicknames.end(), nickname) != _nicknames.end();
}

/**
 * @brief Get a client object by nickname
 * @param nickname Nickname to search for
 * @return Pointer to Client object if found, nullptr otherwise
 * 
 * Searches the server's client list and returns the Client object
 * for the specified nickname. Returns nullptr if not found.
 */
Client* Server::getClient(std::string nickname) {
    std::map<int, Client*>::iterator it;
    for (it = _clients.begin(); it != _clients.end(); ++it) {
        Client* client = it->second;
        if (client->getNickname() == nickname) {
            return client;
        }
    }
    return NULL;
}