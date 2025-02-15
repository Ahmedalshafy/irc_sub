#pragma once
#ifndef IRCLIBS_HPP
#define IRCLIBS_HPP


#include <iostream>
#include <cerrno>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cstdio>
#include <sstream>
#include <algorithm>
#include <map>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <netinet/in.h>
#include <stdlib.h>
#include <signal.h>
#include <cstring>
#include <set>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include "./IrcException.hpp"
#include "./NumericReplies.hpp"
#include "./Client.hpp"
#include "./Channel.hpp"

#endif