#ifndef STATSERVER_h
#define STATSERVER_h

#include <iostream>
#include <boost\thread\mutex.hpp>
#include <boost\thread\thread.hpp>
#include <boost\asio.hpp>
#include <boost\smart_ptr.hpp>
#include <boost\asio\basic_socket.hpp>
#include <boost\bind.hpp>
#include <boost\optional.hpp>
#include "gd_connection.h"

class StatServer {
public:
	boost::mutex mtx;
	StatServer(boost::asio::io_service *io_service,
		const boost::asio::ip::tcp::endpoint &endpoint
	);

	void handle_accept(const boost::system::error_code& error);
	void worker(boost::shared_ptr<gd_connection> connection);
	bool failed;

private:
	void set_result(boost::optional<boost::system::error_code> *destination,
		boost::system::error_code source);

	void set_bytes_result(boost::optional<boost::system::error_code> *error_destination,
		size_t *transferred_destination,
		boost::system::error_code error_source,
		size_t transferred_source);

	ssize_t read_with_timeout(
		boost::asio::ip::tcp::socket &socket,
		void *buf,
		size_t count,
		int seconds
	);

	ssize_t write_with_timeout(
		boost::asio::ip::tcp::socket &socket,
		void const *buf,
		size_t count,
		int seconds
	);

	void process_line(gd_connection &connection, std::string line);

	boost::asio::io_service *io_service;
	boost::asio::ip::tcp::endpoint endpoint;
	boost::asio::ip::tcp::acceptor *acceptor;
	boost::shared_ptr<gd_connection> connection;
};

#endif STATSERVER_h
