#pragma once
#ifndef PARSEMESG_HPP
#define PARSEMESG_HPP



class ParseMessage {

	private:

		int 						_msgLen;
		std::string					_msg;
		std::string					_cmd;
		std::vector<std::string>	_params;
		std::string					_trailing;
		bool						_notValidParam;
		std::string					_errorMsg;

	public:

		ParseMessage( const std::string &message );

		void 						displayCommand(  const ParseMessage &parsedMessage ) const;
		bool						isValid( const std::string &param ) const;
		std::string					ft_trim( const std::string &str ) const;

		int							getMsgLen( void ) const { return _msgLen; }
		std::string					getMsg( void ) const { return _msg; }
		std::string					getCmd( void ) const { return _cmd; }
		std::vector<std::string>	getParams( void ) const { return _params; }
		std::string					getTrailing( void ) const { return _trailing; }
		std::string					getErrorMsg( void ) const { return _errorMsg; }
};

#endif /* PARSEMSG_HPP */