/**
 * @file join.cpp
 * @brief Implementation of IRC JOIN command
 * 
 * This file implements the JOIN command handler which allows clients to join IRC channels.
 * It handles channel creation, access control, and user management according to RFC 2812.
 */

#include "../Includes/Server.hpp"

/**
 * @brief Handle the JOIN command from a client
 * @param client Pointer to the client requesting to join
 * @param ParsedMsg Parsed message containing command parameters
 * 
 * Command format: JOIN <channel>{,<channel>} [<key>{,<key>}]
 * 
 * Handles the following cases:
 * 1. Channel creation if it doesn't exist
 * 2. Channel access control:
 *    - Invite-only mode (+i)
 *    - Key protection (+k)
 *    - User limit (+l)
 * 3. Multiple channel joins with corresponding keys
 * 4. Error conditions:
 *    - Invalid channel name
 *    - User already in channel
 *    - Channel is full
 *    - Invite-only restriction
 *    - Incorrect key
 */
void Server::joinCommand(Client *client, const ParseMessage &ParsedMsg)
{
    std::vector<std::string> params = ParsedMsg.getParams();
    std::vector<std::string> key_list;
    std::vector<std::string>::iterator itr_key;
    std::vector<std::string>::iterator itr_chan;
    std::string response = "";
    bool allowedJoin = true;

    // Validate parameter count
    if(params.size() > 2) {return ;};  // Maximum of channels and keys lists
    if(params.size() < 1)
    {
        client->serverReplies.push_back(ERR_NEEDMOREPARAMS(client->getNickname(), "JOIN"));
        return;
    }

    // Split channel and key lists
    std::vector<std::string> chan_list = ft_split(params[0], ',');
    if(params.size() > 1)
    {
        key_list = ft_split(params[1], ',');
    }

    // Process each channel
    itr_key = key_list.begin();
    for (itr_chan = chan_list.begin(); itr_chan != chan_list.end(); ++itr_chan)
    {
        std::string chanName = *itr_chan;
        
        // Validate channel name format
        if (chanName.at(0) != '#' && chanName.at(0) != '&')
        {
            continue;  // Invalid channel prefix
        }

        // Handle existing channel
        if (isChannelInServer(chanName))
        {
            Channel &tempChannel = getChannel(chanName);
            
            // Check if user is already in channel
            if(tempChannel.isClientInChannel(client->getNickname()))
            {
                response = ERR_USERONCHANNEL(client->getUsername(), client->getNickname(), chanName);
                allowedJoin = false;
            }
            // Check user limit
            else if(!tempChannel.isInvited(client->getNickname()) && tempChannel.checkMode('l') 
                    &&  static_cast<int>(tempChannel.getUsers().size()) >= tempChannel.getUserLimit())
            {
                response = ERR_CHANNELISFULL(client->getNickname(), chanName);
                allowedJoin = false;
            }
            // Check invite-only mode
            else if(tempChannel.checkMode('i')
                && !tempChannel.isInvited(client->getNickname()))
            {
                response = ERR_INVITEONLYCHAN(client->getNickname(), chanName);
                allowedJoin = false;
            }
            // Check key protection
            else if (tempChannel.checkMode('k'))
            {
                if (itr_key != key_list.end() && *itr_key == tempChannel.getKey())
                {
                    ++itr_key;
                }
                else
                {
                    response = ERR_BADCHANNELKEY(client->getNickname(), chanName);
                    allowedJoin = false;
                }
            }

            // Process successful join
            if (allowedJoin) 
            {
                response = RPL_JOIN(user_id(client->getNickname(), client->getUsername()), chanName);
                tempChannel.removeInvite(client->getNickname());
                tempChannel.broadcastMessage(response);
                tempChannel.addClient(client);
                response = greetJoinedUser(*client, tempChannel);
            }
            client->serverReplies.push_back(response);
            break;
        }
        // Create new channel
        else
        {
            response = RPL_JOIN(user_id(client->getNickname(), client->getUsername()), chanName);
            _channels.insert(make_pair(chanName, Channel(chanName, client)));
            response = greetJoinedUser(*client, getChannel(chanName));
            client->serverReplies.push_back(response);
        }
    }
}
