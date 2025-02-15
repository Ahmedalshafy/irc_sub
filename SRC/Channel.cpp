/**
 * @file Channel.cpp
 * @brief Implementation of IRC Channel functionality
 * 
 * This file implements the IRC channel features including:
 * - Channel creation and management
 * - User permissions and roles (operators, regular users)
 * - Channel modes and settings
 * - Message broadcasting
 * - Invitation system
 */

#include "../Includes/Server.hpp"
#include "../Includes/Channel.hpp"

/**
 * @brief Construct a new Channel object
 * @param channelName Name of the channel
 * @param client The client creating the channel (becomes first operator)
 * 
 * Initializes a new channel with:
 * - Default modes (topic protection enabled)
 * - First user as operator
 * - Empty invite list
 * - No user limit
 */
Channel::Channel(std::string &channelName, Client *client) : channelName(channelName), UserLimit(0)
{
    operators[client->getNickname()] = client;
    users[client->getNickname()] = client;
    modes['i'] = false;  // Invite-only mode
    modes['t'] = false;  // Topic protection
    modes['k'] = false;  // Key (password) required
    modes['o'] = false;  // Operator privileges
    modes['l'] = false;  // User limit
    setMode('t', true);  // Enable topic protection by default
}

Channel::~Channel(){}

/**
 * @brief Get a channel by name from the server
 * @param channelName Name of the channel to find
 * @return Channel& Reference to the found channel
 */
Channel &Server::getChannel(std::string channelName)
{
    std::map<std::string, Channel>::iterator chan = _channels.find(channelName);
    return chan->second;
}

/**
 * @brief Check if a channel exists on the server
 * @param channelName Name of the channel to check
 * @return true if channel exists, false otherwise
 */
bool Server::isChannelInServer(std::string &channelName)
{
    std::map<std::string, Channel>::iterator chan = _channels.find(channelName);
    if(chan != _channels.end())
    {
        return (true);
    }
    return (false);
}

/**
 * @brief Add a client to the channel
 * @param client Client to add
 * 
 * Adds a client to the channel and:
 * - Removes them from invite list if they were invited
 * - Makes them operator if channel has no operators
 */
void Channel::addClient(Client *client)
{
    std::string nick = client->getNickname();
    users[nick] = client;
    if(isInvited(nick))
    {
        inviteList.erase(nick);
    }
    if(operators.size() == 0)
    {
        operators[nick] = client;
    }
}

/**
 * @brief Set the channel topic
 * @param topic New topic for the channel
 * 
 * Sets the channel topic and enables topic protection mode
 */
void Channel::setTopic(std::string &topic)
{
    _topic = topic;
    this->setMode('t', true);
}

/**
 * @brief Check if a client is in the channel
 * @param nickname Nickname of the client to check
 * @return true if client is in the channel, false otherwise
 */
bool Channel::isClientInChannel(std::string nickname)
{
    std::map<std::string, Client *>::iterator user_itr = this->users.find(nickname);
    if(user_itr != this->users.end())
    {
        return true;
    }
    return false;
}

/**
 * @brief Check if a client is invited to the channel
 * @param nickname Nickname of the client to check
 * @return true if client is invited, false otherwise
 */
bool Channel::isInvited(std::string nickname)
{
    if (this->inviteList.find(nickname) != inviteList.end())
    {
        return true;
    }
    return false;
}

/**
 * @brief Get the list of users in the channel
 * @return std::map<std::string, Client *> Map of users in the channel
 */
std::map<std::string, Client *> Channel::getUsers( void ) const
{
    return this->users;
}

/**
 * @brief Check the status of a channel mode
 * @param c Mode character to check
 * @return true if mode is enabled, false otherwise
 */
bool Channel::checkMode(char c)
{
    if(this->modes.find(c) != this->modes.end())
    {
        return this->modes.find(c)->second;
    }
    return false;
}

/**
 * @brief Check if a client is an operator in the channel
 * @param nickname Nickname of the client to check
 * @return true if client is an operator, false otherwise
 */
bool Channel::isOperator(std::string &nickname)
{
    if(this->operators.find(nickname) != operators.end())
    {
        return true;
    }
    return false;
}

/**
 * @brief Invite a client to the channel
 * @param client Client to invite
 */
void Channel::inviteClient(Client *client)
{
    std::string nick = client->getNickname();
    this->inviteList[nick] = client;
}

/**
 * @brief Set the channel key (password)
 * @param password New key for the channel
 * 
 * Sets the channel key and enables key mode
 */
void Channel::setKey(std::string &password)
{
    this->_key = password;
    this->setMode('k', true);
}

/**
 * @brief Get the channel key (password)
 * @return std::string Channel key
 */
std::string Channel::getKey() const
{
    return this->_key;
}

/**
 * @brief Get the user limit for the channel
 * @return int User limit
 */
int Channel::getUserLimit() const
{
    return UserLimit;
}

/**
 * @brief Get the channel modes as a string
 * @return std::string Channel modes
 */
std::string Channel::getModes() const
{
    std::string result = "";

    for (std::map<char, bool>::const_iterator it = modes.begin(); it != modes.end(); ++it) {
        if (it->second) {
            result += it->first;
        }
    }
    return result.empty() ? "" : "+" + result;
}

/**
 * @brief Get the channel modes as a map
 * @return std::map<char, bool> Channel modes
 */
std::map<char, bool> Channel::getModesMap( void ) const
{
    return modes;
}

/**
 * @brief Broadcast a message to all channel users
 * @param message Message to broadcast
 * 
 * Sends a message to all users in the channel
 */
void Channel::broadcastMessage(const std::string message)
{
    std::map<std::string, Client *>::iterator it;
    for (it = users.begin(); it != users.end(); ++it)
    {
        if (it->second->getFd() != -1)
        {
            it->second->serverReplies.push_back(message);
        }
    }
}

/**
 * @brief Send a message to all users except the sender
 * @param client Sender to exclude
 * @param message Message to send
 * 
 * Broadcasts a message to all channel users except the original sender
 */
void Channel::sendToOthers(Client *client, std::string message)
{
    std::map<std::string, Client *>::iterator it;
    for (it = users.begin(); it != users.end(); ++it)
    {
        if (it->second->getFd() != -1 && it->second != client)
        {
            it->second->serverReplies.push_back(message);
        }
    }
}

/**
 * @brief Set a channel mode
 * @param c Mode character to set
 * @param setting True to enable, false to disable
 * @return true if mode was set, false if invalid mode
 * 
 * Available modes:
 * - i: Invite-only
 * - t: Topic protection
 * - k: Key (password) required
 * - o: Operator privileges
 * - l: User limit
 */
bool Channel::setMode(char c, bool setting)
{
    // in case the setting is different from the previous one 
    std::map<char, bool>::iterator itr = modes.find(c);
    
    if ( setting == itr->second ) {

        return (false);
    }
        
    if(itr != modes.end())
    {
        itr->second = setting;
    }
    
    return (true);
}

/**
 * @brief Get the channel name
 * @return std::string Channel name
 */
std::string Channel::getChannelName() const
{
    return channelName;
}

/**
 * @brief Remove the channel key (password)
 */
void Channel::removeKey()
{
    _key.clear();
    this->setMode('k', false);
}

/**
 * @brief Set the user limit for the channel
 * @param limit New user limit
 * 
 * Sets the user limit and enables user limit mode if limit is greater than 0
 */
void Channel::setUserLimit(int limit)
{
    if(limit > 0)
    {
        UserLimit = limit;
        this->setMode('l', true);
    }
    else
        std::cout << "Invalid user limit inputted by the user" << std::endl;
}

/**
 * @brief Remove the user limit for the channel
 */
void Channel::removeUserLimit()
{
    UserLimit = -1;
    this->setMode('l', false);
}

/**
 * @brief Add an operator to the channel
 * @param nickname Nickname of the client to add as operator
 */
void Channel::addOperator(std::string nickname)
{
    std::map<std::string, Client *>::iterator user_itr = this->users.find(nickname);
    
    if (user_itr != this->users.end())
    {
        this->operators[user_itr->second->getNickname()] = user_itr->second;
        this->setMode('o', true);
    }
}

/**
 * @brief Remove an operator from the channel
 * @param nickname Nickname of the client to remove as operator
 */
void Channel::removeOperator(std::string nickname)
{
    std::map<std::string, Client*>::iterator operator_itr = this->operators.find(nickname);
    if (operator_itr != this->operators.end())
    {
        this->operators.erase(operator_itr);
        this->setMode('o', false);
    }
    if(this->operators.empty() && !users.empty())
    {
        operators[users.begin()->first] = users.begin()->second;
    }
}

/**
 * @brief Get the channel topic
 * @return std::string Channel topic
 */
std::string Channel::getTopic() const
{
    return _topic;
}

/**
 * @brief Remove an invitation from the channel
 * @param invite Nickname of the client to remove from invite list
 */
void Channel::removeInvite(std::string &invite)
{
    std::map<std::string, Client*>::iterator invite_itr = this->inviteList.find(invite);
    if (invite_itr != this->inviteList.end())
    {
        this->inviteList.erase(invite_itr);
    }
}

/**
 * @brief Remove a client from the channel
 * @param client Client to remove
 * 
 * Removes the client from the channel and:
 * - Removes them from operators list if they were an operator
 * - Removes them from users list
 */
void Channel::removeClient(Client *client)
{
    this->removeOperator(client->getNickname());
    std::map<std::string, Client*>::iterator users_itr = this->users.find(client->getNickname());
    if (users_itr != this->users.end())
    {
        this->users.erase(users_itr);
    }
}

/**
 * @brief Trim a string of leading and trailing whitespace
 * @param text String to trim
 * @return std::string Trimmed string
 */
std::string ft_trim(std::string text)
{
    std::size_t first = text.find_first_not_of(" \n\r\t");
    std::size_t last = text.find_last_not_of(" \n\r\t");

    if (first == std::string::npos || last == std::string::npos) {
        return "";
    }
    return text.substr(first, (last - first + 1));
}

/**
 * @brief Add a channel to the server
 * @param channel Channel to add
 */
void Server::addChannel(Channel &channel)
{
    _channels.insert(std::make_pair(channel.getChannelName(), channel));
}

/**
 * @brief Generate a greeting message for a user joining a channel
 * @param client Client joining the channel
 * @param channel Channel being joined
 * @return std::string Greeting message
 */
std::string Server::greetJoinedUser(Client &client, Channel &channel)
{
    std::string reply;

    reply = RPL_JOIN(user_id(client.getNickname(), client.getUsername()), channel.getChannelName());
    if (channel.getUsers().size() == 1)
        reply += MODE_CHANNELMSG(channel.getChannelName(), channel.getModes());
    if (channel.getTopic().empty() == false) // if has a topic append it to the message
        reply += RPL_TOPIC(client.getNickname(), channel.getChannelName(), channel.getTopic());
    reply += RPL_NAMREPLY(client.getNickname(), '@', channel.getChannelName(), channel.getUsersList());
    reply += RPL_ENDOFNAMES(client.getUsername(), channel.getChannelName());
    return reply;
}

/**
 * @brief Get the list of users in the channel as a string
 * @return std::string List of users in the channel
 */
std::string Channel::getUsersList()
{
    std::string memberList;
    std::map<std::string, Client *> users = this->getUsers();
    for (std::map<std::string, Client*>::const_iterator iter = users.begin(); iter != users.end(); ++iter) {
        const Client* currentMember = iter->second;
        if (isOperator(currentMember->getNickname())) {
            memberList += "@";
        }
        memberList += currentMember->getNickname() + " ";
    }
    return ft_trim(memberList);
}

/**
 * @brief Update a user's nickname in the channel
 * @param oldNick Previous nickname
 * @param newNick New nickname
 * 
 * Updates nickname in:
 * - Users list
 * - Operators list
 * - Invite list
 */
void Channel::updateNickname(std::string oldNick, std::string newNick)
{
    std::map<std::string, Client *>::iterator user_itr = this->users.find(oldNick);
    if(user_itr != this->users.end())
    {
        Client *client = user_itr->second;
        this->users.erase(user_itr);
        this->users[newNick] = client;
    }
    std::map<std::string, Client *>::iterator operator_itr = this->operators.find(oldNick);
    if(operator_itr != this->operators.end())
    {
        Client *client = operator_itr->second;
        this->operators.erase(operator_itr);
        this->operators[newNick] = client;
    }
    std::map<std::string, Client *>::iterator invite_itr = this->inviteList.find(oldNick);
    if (invite_itr != this->inviteList.end()) {

        Client *client = invite_itr->second;
        this->inviteList.erase(invite_itr);
        this->inviteList[newNick] = client;
    }
}

// Channel member lists
std::map<std::string, Client *> operators;   // Channel operators
std::map<std::string, Client *> users;       // All channel users
std::map<std::string, Client *> inviteList;  // Invited users
std::map<char, bool> modes;                  // Channel modes