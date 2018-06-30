#include "server.h"
#include "gd_connection.h"

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

using boost::asio::ip::tcp;

StatServer::StatServer(boost::asio::io_service *io_service,
	const boost::asio::ip::tcp::endpoint &endpoint) {
	this->io_service = io_service;
	this->failed = false; // indicator whether construction failed

						  // it is a common problem to find that the port we bind to
						  // is already in use (say, another instance of this program)
	try {
		this->acceptor = new boost::asio::ip::tcp::acceptor(*io_service);

		// Open the acceptor with the option to reuse the address
		// (i.e. SO_REUSEADDR)
		this->acceptor->open(endpoint.protocol());
		this->acceptor->set_option(
			boost::asio::ip::tcp::acceptor::reuse_address(true)
		);
		this->acceptor->bind(endpoint);
		this->acceptor->listen();
	}
	catch (boost::system::system_error e) {
		std::cerr << "Error binding to " << endpoint.address().to_string() << ":" << endpoint.port() << ": " << e.what() << std::endl;
		this->failed = true;
		return;
	}

	// successful bind!
	// Now create a new "my_connection" object to receive new accepted socket
	this->connection = boost::shared_ptr<gd_connection>(
		new gd_connection()
		);
	this->connection->master_io_service = this->io_service;
	this->acceptor->async_accept(
		*(this->connection->socket), // new connection is stored here
		this->connection->endpoint, // where the remote address is stored
		boost::bind(
			&StatServer::handle_accept, // function to call on accept()
			this, // object functions need a pointer to their object
			boost::asio::placeholders::error // argument to call-back function
		)
	);
}

void StatServer::handle_accept(const boost::system::error_code& error) {
	if (error) {
		// accept failed
		std::cerr << "Acceptor failed: " << error.message() << std::endl;
		return;
	}

	std::cout << "Accepted connection from " << this->connection->endpoint.address().to_string() << ":" << this->connection->endpoint.port() << std::endl;

	// time to create a thread and let THAT deal with the socket synchronously!
	this->connection->thread = boost::shared_ptr<boost::thread>(
		new boost::thread(worker, this->connection)
		);

	// re-build accept call
	// we need a new socket/connection class
	this->connection = boost::shared_ptr<gd_connection>(
		new gd_connection()
		);
	this->connection->master_io_service = this->io_service;
	this->acceptor->async_accept(
		*(this->connection->socket),
		this->connection->endpoint,
		boost::bind(
			&StatServer::handle_accept,
			this,
			boost::asio::placeholders::error
		)
	);
}

void StatServer::set_result(boost::optional<boost::system::error_code>* destination, boost::system::error_code source)
{
	destination->reset(source);
}

void StatServer::set_bytes_result(boost::optional<boost::system::error_code>* error_destination, size_t * transferred_destination, boost::system::error_code error_source, size_t transferred_source)
{
	error_destination->reset(error_source);
	*transferred_destination = transferred_source;
}

ssize_t StatServer::read_with_timeout(boost::asio::ip::tcp::socket & socket, void * buf, size_t count, int seconds)
{
	boost::optional<boost::system::error_code> timer_result;
	boost::optional<boost::system::error_code> read_result;
	size_t bytes_transferred;

	// set up a timer on the io_service for this socket
	boost::asio::deadline_timer timer(socket.get_io_service());
	timer.expires_from_now(boost::posix_time::seconds(seconds));
	timer.async_wait(
		boost::bind(
			set_result,
			&timer_result,
			boost::asio::placeholders::error
		)
	);

	// set up asynchronous read (respond when ANY data is received)
	boost::asio::async_read(
		socket,
		boost::asio::buffer((char *)buf, count),
		boost::asio::transfer_at_least(1),
		boost::bind(
			set_bytes_result,
			&read_result,
			&bytes_transferred,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred
		)
	);

	socket.get_io_service().reset();

	// set default result to zero (timeout) because another thread
	// may call io_service().stop() deliberately to interrupt the
	// the read (say, if it wanted to signal the thread that there
	// was data to write)
	ssize_t result = 0;
	bool resultset = false;
	while (socket.get_io_service().run_one()) {
		if (read_result) {
			//boost::system::error_code e = (*read_result);
			//std::cerr << "read_result was " << e.message() << std::endl;
			timer.cancel();
			if (resultset == false) {
				result = (bytes_transferred <= 0) ? -1 : bytes_transferred;
				resultset = true;
			}
			read_result.reset();
		}
		else if (timer_result) {
			socket.cancel();
			if (resultset == false) {
				result = 0;
				resultset = 0;
			}
			timer_result.reset();
		}
	}

	return(result);
}

ssize_t StatServer::write_with_timeout(boost::asio::ip::tcp::socket & socket, void const * buf, size_t count, int seconds)
{
	boost::optional<boost::system::error_code> timer_result;
	boost::optional<boost::system::error_code> write_result;
	size_t bytes_transferred;

	boost::asio::deadline_timer timer(socket.get_io_service());
	timer.expires_from_now(boost::posix_time::seconds(seconds));
	timer.async_wait(
		boost::bind(
			set_result,
			&timer_result,
			boost::asio::placeholders::error
		)
	);

	boost::asio::async_write(
		socket,
		boost::asio::buffer((char *)buf, count),
		boost::asio::transfer_at_least(count), // want to transfer ALL of it
		boost::bind(
			set_bytes_result,
			&write_result,
			&bytes_transferred,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred
		)
	);

	socket.get_io_service().reset();

	size_t result = -1;
	bool resultset = false;
	while (socket.get_io_service().run_one()) {
		if (write_result) {
			timer.cancel();
			if (resultset == false) {
				result = (bytes_transferred <= 0) ? -1 : bytes_transferred;
				resultset = true;
			}
			write_result.reset();
		}
		else if (timer_result) {
			socket.cancel();
			if (resultset == false) {
				result = 0;
				resultset = true;
			}
			timer_result.reset();
		}
	}
	return(result);
}

void StatServer::worker(boost::shared_ptr<gd_connection> connection) {
	boost::asio::ip::tcp::socket &socket = *(connection->socket);
	socket.non_blocking();

	char acBuffer[1024];
	std::string line("");

	while (connection->close == false) {
		ssize_t bytes_read = read_with_timeout(
			socket, // socket to read
			acBuffer, // buffer to read into
			sizeof(acBuffer), // maximum size of buffer
			1 // timeout in seconds
		);

		if (bytes_read < 0)
			break; // connection error or close

		if (bytes_read == 0) {
			continue; // timeout

			char const *pend = acBuffer + bytes_read;
			char const *pstart = acBuffer;
			char const *pchar = pstart;
			// buffer may legitimately contain '\0' from network
			// so we must always ensure we don't go over the number
			// of bytes actually read
			while ((pchar < pend) && (*pchar != '\0')) {
				if ((*pchar != '\n') && (*pchar != '\r')) {
					pchar++;
					continue;
				}

				// non-blank line detected?
				if (pchar > pstart) {
					line += std::string(pstart, pchar - pstart);
					// ***THIS IS WHAT WE ULTIMATELY WANTED TO ACHIEVE!!!***
					process_line(*connection, line);
					line = "";
				}

				// skip over newlines
				while ((pchar < pend) && ((*pchar == '\n') || (*pchar == '\r')))
					pchar++;

				pstart = pchar;
				continue;
			}

			if (pchar > pstart) {
				// put remaining non-terminated text into line buffer
				line += std::string(pstart, pchar - pstart);
			}
		} // while connection not to be closed

	}
}

void StatServer::process_line(gd_connection &connection, std::string line) {
	if(line.compare())
}