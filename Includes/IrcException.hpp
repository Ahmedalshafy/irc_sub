#pragma once
#ifndef IRCEXCEPTION_HPP
# define IRCEXCEPTION_HPP

#include <exception>
#include <string>

class IrcException : public std::exception {

	private:

		std::string _message;

	public:

		explicit IrcException( const std::string &message ) : _message( message ) {}
		virtual ~IrcException() throw() {}
		virtual const char *what() const throw() {
			
			return _message.c_str();
		}
};

#endif /* IRCEXCEPTION_HPP */