#include "gd_connection.h"

gd_connection::gd_connection(void) : close(false) {
	this->socket = boost::shared_ptr<boost::asio::ip::tcp::socket>(
		new boost::asio::ip::tcp::socket(this->io_service)
	);
}