#ifndef GD_CONN_h
#define GD_CONN_h

#include <boost\thread\thread.hpp>
#include <boost\asio.hpp>
#include <boost\asio\socket_base.hpp>
#include <boost\smart_ptr.hpp>

class gd_connection {
public:
	gd_connection(void);

	boost::asio::io_service io_service;
	boost::shared_ptr<boost::asio::ip::tcp::socket> socket;
	boost::asio::ip::tcp::endpoint endpoint;
	boost::shared_ptr<boost::thread> thread;
	boost::asio::io_service *master_io_service;
	bool close;
};

#endif GD_CONN_h