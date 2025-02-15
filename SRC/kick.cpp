/**
 * @file kick.cpp
 * @brief Implementation of IRC KICK command
 * 
 * This file implements the KICK command which allows channel operators
 * to remove users from a channel. The command follows RFC 2812 specifications
 * for channel operator privileges and user management.
 */

#include "../Includes/Client.hpp"
#include "../Includes/Server.hpp"

/**
 * @brief Handle the KICK command
 * @param client Pointer to the client issuing the KICK command
 * @param ParsedMsg Parsed message containing command parameters
 * 
 * Command format: KICK <channel> <user>{,<user>} [<comment>]
 * 
 * Handles the following cases:
 * 1. Permission validation:
 *    - Sender must be channel operator
 *    - Sender must be in the channel
 * 2. Multiple user kicks with a single command
 * 3. Optional kick message/comment
 * 4. Error conditions:
 *    - Channel doesn't exist
 *    - Target user not in channel
 *    - Insufficient privileges
 *    - Self-kick prevention
 */
void Server::handelKickCommand(Client *client, const ParseMessage &ParsedMsg)
{
    std::vector<std::string> params = ParsedMsg.getParams();

    // Check for required parameters
    if (params.size() < 2) {
        client->serverReplies.push_back(ERR_NEEDMOREPARAMS(client->getNickname(), "KICK"));
        return;
    }

    // Get kick message if provided
    std::string trailingMessage = ParsedMsg.getTrailing().empty() ? "" : ParsedMsg.getTrailing();
    std::string channelName = params[0];

    // Validate channel existence
    if (!isChannelInServer(channelName)) {
        client->serverReplies.push_back(ERR_NOSUCHCHANNEL(client->getNickname(), channelName));
        return;
    }
    Channel &channel = getChannel(channelName);

    // Check if kicker is in the channel
    if (!channel.isClientInChannel(client->getNickname())) {
        client->serverReplies.push_back(ERR_NOTONCHANNEL(client->getNickname(), channelName));
        return;
    }

    // Verify operator privileges
    if (!channel.isOperator(client->getNickname())) {
        client->serverReplies.push_back(ERR_CHANOPRIVSNEEDED(client->getNickname(), channelName));
        return;
    }

    // Process each target user
    std::vector<std::string> users = ft_split(params[1], ',');
    for (std::vector<std::string>::iterator it = users.begin(); it != users.end(); ++it)
    {
        std::string targetNick = *it;
        
        // Prevent self-kick
        if (targetNick == client->getNickname()) {
            client->serverReplies.push_back(": localhost  482 " + client->getNickname() + " " + channelName + " :You can't kick yourself\r\n");
            continue;
        }

        // Validate target user
        Client *targetClient = getClient(targetNick);
        if (!targetClient || !channel.isClientInChannel(targetNick)) {
            client->serverReplies.push_back(ERR_USERNOTINCHANNEL(client->getNickname(), targetNick, channelName));
            continue;
        }

        // Broadcast kick message and remove user
        std::string kickMsg = RPL_KICK(user_id(client->getNickname(), client->getUsername()), 
                                     channelName, 
                                     targetClient->getNickname(), 
                                     trailingMessage);
        channel.broadcastMessage(kickMsg);
        channel.removeClient(targetClient);

        // Remove channel if it's empty
        if (channel.getUsers().empty()) {
            _channels.erase(channelName);
        }
    }
}