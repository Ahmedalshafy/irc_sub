/**
 * @file ParseMessage.cpp
 * @brief IRC Message Parser Implementation
 * 
 * This file implements the parsing of IRC protocol messages according to RFC 2812.
 * It handles message format: [:<prefix>] <command> [<parameters>...] [:<trailing>]
 */

#include "../Includes/Server.hpp"

/**
 * @brief Trim whitespace from both ends of a string
 * @param str String to trim
 * @return Trimmed string
 * 
 * Removes leading and trailing whitespace (space, newline, carriage return, tab)
 */
std::string ParseMessage::ft_trim(const std::string &str) const {
    std::size_t start = str.find_first_not_of(" \n\r\t");
    std::size_t end = str.find_last_not_of(" \n\r\t");

    if (start == std::string::npos || end == std::string::npos) {
        return "";
    }

    return str.substr(start, end - start + 1);
}

/**
 * @brief Split a string by delimiter
 * @param str String to split
 * @param delimiter Character to split on
 * @return Vector of substrings
 * 
 * Splits a string into parts based on a delimiter character,
 * ignoring empty parts between consecutive delimiters
 */
std::vector<std::string> ft_split(std::string str, char delimiter)
{
    std::vector<std::string> result;
    std::string word;
    
    for (std::size_t i = 0; i < str.length(); ++i)
    {
        if(str[i] != delimiter)
        {
            word += str[i];
        }
        else
        {
            if(!word.empty())
            {
                result.push_back(word);
                word.clear();
            }
        }
    }
    if(!word.empty())
    {
        result.push_back(word);
    }
    return result;
}

/**
 * @brief Process and split a string by spaces, handling IRC message format
 * @param str Input string to process
 * @return Vector of space-separated components
 * 
 * Special handling for IRC message format:
 * - Preserves content after ':' as a single parameter
 * - Handles multiple consecutive spaces
 * - Removes carriage return and newline
 */
std::vector<std::string> remove_spaces(std::string &str)
{
    std::vector<std::string> result;
    std::string word;

    for (std::size_t i = 0; i < str.length(); ++i)
    {
        if(str[i] != ' ')
        {
            word += str[i];
        }
        else
        {
            if(!word.empty())
            {
                result.push_back(word);
                word.clear();
            }
        }
    }
    if(!word.empty())
    {
        result.push_back(word);
    }
    return result;
}

/**
 * @brief Parse an IRC protocol message
 * @param message Raw message to parse
 * 
 * Parses an IRC message into its components:
 * 1. Prefix (if exists, starts with ':')
 * 2. Command (required)
 * 3. Parameters (optional)
 * 4. Trailing parameter (after ':', optional)
 * 
 * Example formats:
 * - "PRIVMSG #channel :Hello world"
 * - ":nick!user@host PRIVMSG #channel :Hello"
 * - "JOIN #channel"
 */
ParseMessage::ParseMessage(const std::string& message)
{
    if (message.empty()) {
        return;
    }

    _msg = message;
    _msgLen = static_cast<int>(message.length());
    _cmd = "";
    _params.clear();
    _trailing = "";
    _notValidParam = false;
    _errorMsg = "";
    std::string trimmedMsg = ft_trim(message);
    std::string token;
    std::istringstream iss(trimmedMsg);
    bool tagFlag = false;
    bool tagCmd = true;

    if (trimmedMsg[0] == '@') {
        tagFlag = true;
    }
    
    while (iss >> token) {
        if (tagFlag) {
            if (token[0] == ':')
            {
                _cmd = token.substr(1);
                tagFlag = false;
                tagCmd = false;
                continue;
            }

            continue;
        }
        
        if (tagCmd) {
            
            _cmd = token;
            tagCmd = false;
            continue;
        }

        if (token[0] == ':') {
            
            _trailing = ft_trim(message.substr(message.find(token) + 1)); 
            break;
        } else {
    
            if (isValid(token)) {
                
                _params.push_back(token);
            } else {
                
                _notValidParam = true;
                _errorMsg = "Invalid character in parameter: " + token;
                break;
            }   
        }
    }

    return;
}

/**
 * @brief Validate a parameter string
 * @param param Parameter to validate
 * @return true if valid, false otherwise
 * 
 * Checks if a parameter:
 * - Is not empty
 * - Contains valid characters
 * - Follows IRC parameter rules
 */
bool ParseMessage::isValid(const std::string &param) const {
    std::string invalidChars = "\n\r\t:";
    std::size_t valid = param.find_first_of(invalidChars); 

    return valid == std::string::npos;
}

/**
 * @brief Check if a string is alphanumeric
 * @param str String to check
 * @return true if string is alphanumeric, false otherwise
 * 
 * Used for validating nicknames, channel names, and other identifiers
 */
bool Server::isAlphanumeric(const std::string &str) {
    for (std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
        if (!std::isalnum(static_cast<unsigned char>(*it))) {
            return false;
        }
    }
    return true;
}