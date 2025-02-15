/**
 * @file modeCommand.cpp
 * @brief Implementation of IRC MODE command
 * 
 * This file implements the MODE command which allows changing channel and user modes.
 * It supports various channel modes according to RFC 2812:
 * - i: Invite-only channel
 * - t: Protected topic
 * - k: Channel key (password)
 * - o: Channel operator
 * - l: User limit
 */

#include "../Includes/Server.hpp"

/**
 * @brief Handle channel key mode (+k/-k)
 * @param client Client requesting the mode change
 * @param channel Target channel
 * @param isAdding True if adding mode, false if removing
 * @param params Command parameters
 * @param paramIndex Current parameter index
 * @return True if mode was changed successfully
 * 
 * Handles setting and removing channel keys (passwords)
 * - +k <key>: Set channel key
 * - -k: Remove channel key
 */
bool Server::handleKeyMode(Client *client, Channel &channel, bool isAdding,
      std::vector<std::string> &params, std::size_t &paramIndex)
{
    std::map<char, bool> modesMap = channel.getModesMap();
    std::map<char, bool>::iterator itr = modesMap.find('k');
    
    // Check if mode is already in desired state
    if (isAdding == itr->second) {
        return (false);
    }

    // Adding key mode
    if (isAdding && paramIndex < params.size()) {
        if(isAlphanumeric(params[paramIndex])) {
            channel.setKey(params[paramIndex++]);
            std::string key(channel.getKey().size(), '*');
            client->serverReplies.push_back(RPL_CHANNELMODEISWITHKEY(client->getNickname(), 
                                          channel.getChannelName(), channel.getModes(), key));
            return (true);
        }
        else {
            client->serverReplies.push_back(ERR_INVALIDMODEPARAM(client->getNickname(),
                                          channel.getChannelName(),'k', params[paramIndex++]));
            return(false);
        }
        return (true);
    } 
    // Adding key but missing parameter
    else if (isAdding) {
        client->serverReplies.push_back(ERR_NEEDMOREPARAMS(client->getNickname(), "MODE +k"));
    } 
    // Removing key
    else {
        channel.removeKey();
        return(true);
    }

    return (false);
}

/**
 * @brief Handle channel user limit mode (+l/-l)
 * @param client Client requesting the mode change
 * @param channel Target channel
 * @param isAdding True if adding mode, false if removing
 * @param params Command parameters
 * @param paramIndex Current parameter index
 * @return True if mode was changed successfully
 * 
 * Handles setting and removing channel user limits
 * - +l <limit>: Set user limit
 * - -l: Remove user limit
 */
bool Server::handleLimitMode(Client *client, Channel &channel, bool isAdding,
      std::vector<std::string> &params, std::size_t &paramIndex)
{
    int UserLimit;
    std::map<char, bool> modesMap = channel.getModesMap();
    std::map<char, bool>::iterator itr = modesMap.find('l');

    if (isAdding == itr->second) {
        return (false);
    }

    // Adding limit mode
    if (isAdding && paramIndex < params.size()) {
        UserLimit = std::atoi(params[paramIndex++].c_str());
        if (UserLimit > 0) {
            channel.setUserLimit(UserLimit);
            client->serverReplies.push_back(RPL_CHANNELMODEISWITHKEY(client->getNickname(),
                                          channel.getChannelName(), channel.getModes(),
                                          params[paramIndex - 1]));
            return (true);
        }
        else {
            client->serverReplies.push_back(ERR_INVALIDMODEPARAM(client->getNickname(),
                                          channel.getChannelName(), 'l',
                                          params[paramIndex - 1]));
            return (false);
        }
    }
    // Adding limit but missing parameter
    else if (isAdding) {
        client->serverReplies.push_back(ERR_NEEDMOREPARAMS(client->getNickname(), "MODE +l"));
    }
    // Removing limit
    else {
        channel.removeUserLimit();
        return (true);
    }
    return (false);
}

/**
 * @brief Handle channel operator mode (+o/-o)
 * @param client Client requesting the mode change
 * @param channel Target channel
 * @param isAdding True if adding mode, false if removing
 * @param params Command parameters
 * @param paramIndex Current parameter index
 * @return True if mode was changed successfully
 * 
 * Handles setting and removing channel operator status
 * - +o <nickname>: Grant operator status
 * - -o <nickname>: Remove operator status
 */
bool Server::handleOperatorMode(Client *client, Channel &channel, bool isAdding,
      std::vector<std::string> &params, std::size_t &paramIndex)
{
    std::map<char, bool> modesMap = channel.getModesMap();
    std::map<char, bool>::iterator itr = modesMap.find('o');
    
    // Check if mode is already in desired state
    if (isAdding == itr->second) {
        return (false);
    }

    // Adding operator mode
    if (paramIndex < params.size())
    {
        std::string targetNick = params[paramIndex++];
        if(!channel.isClientInChannel(targetNick))
        {
            client->serverReplies.push_back(ERR_USERNOTINCHANNEL(client->getNickname(), targetNick, channel.getChannelName()));
            return (false);
        }
        if (isAdding)
            channel.addOperator(targetNick);
        else
            channel.removeOperator(targetNick);
        return (true);
    }
    // Missing parameter
    else
    {
        client->serverReplies.push_back(ERR_NEEDMOREPARAMS(client->getNickname(), "MODE o"));
    }
    return (false);
}

/**
 * @brief Process a single channel mode change
 * @param client Client requesting the mode change
 * @param channel Target channel
 * @param mode Mode character to process
 * @param isAdding True if adding mode, false if removing
 * @param params Command parameters
 * @param paramIndex Current parameter index
 * @return True if mode was changed successfully
 * 
 * Handles individual mode changes by delegating to specific handlers
 */
bool Server::processSingleChannelMode(Client *client, Channel &channel,
    char mode, bool isAdding, std::vector<std::string> &params,
    std::size_t &paramIndex)
{
    switch (mode)
    {
    case 'i':
        return (channel.setMode('i', isAdding));
    case 'k':
        return (handleKeyMode(client, channel, isAdding, params, paramIndex));
    case 'l':
        return (handleLimitMode(client, channel, isAdding, params, paramIndex));
    case 't':
        return (channel.setMode('t', isAdding));
    case 'o':
        return (handleOperatorMode(client, channel, isAdding, params,
                paramIndex));
    case 'b':
        return (false);
    default:
        client->serverReplies.push_back(ERR_UNKNOWNMODE(client->getNickname(), std::string(1,
                    mode)));
        return (false);
    }
}

/**
 * @brief Process multiple channel modes
 * @param client Client requesting the mode changes
 * @param channel Target channel
 * @param params Command parameters
 * 
 * Processes a mode string that may contain multiple mode changes
 * Example: MODE #channel +k-l key
 */
void Server::processChannelModes(Client *client, Channel &channel,
      std::vector<std::string> &params)
{
    bool	isAdding;
    std::size_t	paramIndex;
    char	mode;

    std::string modeString = params[1];
    isAdding = true;
    paramIndex = 2;

    std::string modeStr;
    for (std::size_t i = 0; i < modeString.length(); ++i)
    {
        mode = modeString[i];
        if (mode == '+' || mode == '-')
        {
            isAdding = (mode == '+');
            continue;
        }
        if (processSingleChannelMode(client, channel, mode, isAdding, params, paramIndex)) {

            modeStr += isAdding == false ? std::string(1,'-') + std::string(1, mode) : std::string(1,'+') + std::string(1, mode);
        }
    }
    
    if ( modeStr.empty() == false ) {
        std::string modeChanges = MODE_CHANNELCHANGEMODE(user_id(client->getNickname(), client->getUsername()), channel.getChannelName(), modeStr);
        channel.broadcastMessage(modeChanges);
    }
}

/**
 * @brief Handle channel mode changes
 * @param client Client requesting the mode changes
 * @param channelName Target channel name
 * @param params Command parameters
 * 
 * Main handler for channel mode changes, validates permissions
 * and delegates to mode processors
 */
void Server::handleChannelMode(Client *client, std::string &channelName,
      std::vector<std::string> &params)
{
    if (!Server::isChannelInServer(channelName))
    {
        client->serverReplies.push_back(ERR_NOSUCHCHANNEL(client->getNickname(),
                channelName));
        return ;
    }
    Channel &channel = Server::getChannel(channelName);
    const std::string nick = client->getNickname();
    if (params.size() == 1)
    {
        client->serverReplies.push_back(RPL_CHANNELMODEIS(client->getNickname(), channelName,
                channel.getModes()));
    }
    else
    {
        if (!channel.isOperator(const_cast<std::string &>(client->getNickname())))
        {
            client->serverReplies.push_back(ERR_CHANOPRIVSNEEDED(client->getNickname(),
                    channelName));
            return ;
        }
        processChannelModes(client, channel, params);
    }
}

/**
 * @brief Main MODE command handler
 * @param client Client issuing the MODE command
 * @param parsedMsg Parsed message containing command parameters
 * 
 * Command format: MODE <target> [<modestring> [<mode arguments>...]]
 * 
 * Handles both channel and user modes:
 * 1. Channel modes:
 *    - i: Invite-only
 *    - t: Protected topic
 *    - k: Channel key
 *    - o: Channel operator
 *    - l: User limit
 * 2. User modes (not implemented in this version)
 */
void Server::handelModeCommand(Client *client, const ParseMessage &parsedMsg)
{
    std::vector<std::string> params = parsedMsg.getParams();

    if(parsedMsg.getTrailing().empty() == false)
    {
        std::vector<std::string> splitTrailing = ft_split(parsedMsg.getTrailing(), ' ');
        params.insert(params.end(), splitTrailing.begin(), splitTrailing.end());
    }

    if (params.size() < 1)
    {
        client->serverReplies.push_back(ERR_NEEDMOREPARAMS(client->getNickname(), "MODE"));
        return ;
    }
    std::string target = params[0];
    if (target[0] == '#' || target[0] == '&')
    {
        handleChannelMode(client, target, params);
    }
    else
    {
        if ( !isUserInServer( target ) ) {

            client->serverReplies.push_back(ERR_NOSUCHCHANNEL(client->getNickname(), target));
        }
    }
}