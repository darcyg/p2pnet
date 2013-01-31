/*
 * UDPTransportSocket.h
 *
 *  Created on: 27.01.2013
 *      Author: gamepad
 */

#ifndef UDPTRANSPORTSOCKET_H_
#define UDPTRANSPORTSOCKET_H_

#include "TransportSocket.h"
#include "UDPTransportSocketEndpoint.h"
#include <boost/asio.hpp>

using namespace boost::asio;

namespace p2pnet {
namespace net {

class UDPTransportSocket: public p2pnet::net::TransportSocket {
	const unsigned short IPv4_MTU = 512;
	const unsigned short IPv6_MTU = 1024;

	io_service& asio_io_service;
	ip::udp::socket asio_socket;
protected:
	void receivedMessageHandler(TransportSocketListener* listener, char* buffer, size_t bytes_received, UDPTransportSocketEndpoint* endpoint, TransportSocketConnection connection);
	void sentMessageHandler(TransportSocketListener* listener, char* buffer, size_t bytes_sent, UDPTransportSocketEndpoint* endpoint, TransportSocketConnection connection);
public:
	UDPTransportSocket();
	virtual ~UDPTransportSocket();

	void openIPv4();
	void openIPv6();
	/*!
	 * Opens the socket, sets it to IPv4, binds on all available network interfaces and sets MTU = IPv4_MTU
	 * @param port
	 */
	void bindLocalIPv4(UDPTransportSocketEndpoint::port_t port);
	/*!
	 * Opens the socket, sets it to IPv6, binds on all available network interfaces and sets MTU = IPv6_MTU
	 * @param port
	 */
	void bindLocalIPv6(UDPTransportSocketEndpoint::port_t port);

	//Inherited from TransportSocket
	virtual void asyncReceiveFrom(TransportSocketEndpoint &endpoint, TransportSocketListener* listener, TransportSocketConnection connection = TransportSocketConnection(0, 0));
	virtual void asyncSendTo(TransportSocketEndpoint &endpoint, const std::string data, TransportSocketListener* listener, TransportSocketConnection connection = TransportSocketConnection(0, 0));

	virtual void waitReceiveFrom(TransportSocketEndpoint &endpoint, TransportSocketListener* listener, TransportSocketConnection connection = TransportSocketConnection(0, 0));
	virtual void waitSendTo(TransportSocketEndpoint &endpoint, const std::string data, TransportSocketListener* listener, TransportSocketConnection connection = TransportSocketConnection(0, 0));

	virtual MessageBundle hereReceiveFrom(TransportSocketEndpoint &endpoint, TransportSocketConnection connection = TransportSocketConnection(0, 0));
	virtual void hereSendTo(TransportSocketEndpoint &endpoint, const std::string data);
};

} /* namespace net */
} /* namespace p2pnet */
#endif /* UDPTRANSPORTSOCKET_H_ */