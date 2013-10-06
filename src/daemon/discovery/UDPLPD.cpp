/*
 * You may redistribute this program and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "UDPLPD.h"
#include "../AsioIOService.h"
#include "../udp/UDPTransportInterface.h"
#include "../protobuf/Protocol.pb.h"
#include "../../common/Version.h"

#include <memory>
#include <functional>

namespace p2pnet {
namespace discovery {

UDPLPD::UDPLPD(ConfigManager& config) : GenericLPD(config),
		m_config(config),
		m_timer(AsioIOService::getIOService()),
		m_lpd_socket(AsioIOService::getIOService()) {
	m_timer_seconds = 0;
}

UDPLPD::~UDPLPD() {
	m_timer.cancel();
}

void UDPLPD::run() {
	startReceiveLoop();
	startSendLoop();
}

void UDPLPD::waitBeforeSend() {
	m_timer.expires_from_now(boost::posix_time::seconds(m_timer_seconds));
	m_timer.async_wait(boost::bind(&UDPLPD::send, this));
}

void UDPLPD::processReceived(std::shared_ptr<boost::asio::streambuf> recv_buffer,
		size_t recv_bytes,
		std::shared_ptr<ip::udp::endpoint> mcast_endpoint_ptr) {
	std::istream buffer_stream(recv_buffer);
	// Create Protocol Buffer message
	protocol::UDPDiscoveryMessage message;

	try {
		bool parsed_well = message.ParseFromIstream(&buffer_stream);
		if(!parsed_well){
			throw(new errors::MessageReject(errors::MessageReject::Reason::PARSE_ERROR));
		}

		// This endpoint is ready to send handshake messages to
		ip::udp::endpoint dest_endpoint(mcast_endpoint_ptr->address(), message.port());

		log() << "Local <- " << dest_endpoint << std::endl;

		/*
		 * Summary: We received a message and we can create TransportSocketEndpoint from it
		 */
		auto interface_endpoint = std::make_shared<transport::UDPTransportInterfaceEndpoint>(dest_endpoint);
		transport::TransportSocketEndpoint socket_endpoint(interface_endpoint);
		// Well, trying to handshake it.
	} catch(errors::MessageReject *e) {
		log(ERROR) << e->what();
		delete e;
	}

	// We received message, continue receiving others
	receive();
}

std::string UDPLPD::getMulticastMessage() {
	protocol::UDPDiscoveryMessage message;
	message.set_port(getValue<unsigned short>("transport.udp.port"));
	message.set_version(getMyVersion());
	return message.SerializeAsString();
}

void UDPLPD::send() {
	log() << "Local -> " << target_endpoint << std::endl;
	m_lpd_socket.async_send_to(buffer(getMulticastMessage()), target_endpoint, std::bind(&UDPLPD::waitBeforeSend, this));
}

void UDPLPD::receive() {
	auto udp_buffer = std::make_shared<boost::asio::streambuf>();
	auto endpoint = std::make_shared<ip::udp::endpoint>(bind_endpoint);
	m_lpd_socket.async_receive_from(*udp_buffer, *endpoint,
			std::bind(&UDPLPD::processReceived, this, udp_buffer, std::placeholders::_2, endpoint));
}

void UDPLPD::startSendLoop() {
	std::clog << "[" << getComponentName() << "] Started sending broadcasts to: " << target_endpoint << std::endl;
	send();
}

void UDPLPD::startReceiveLoop() {
	std::clog << "[" << getComponentName() << "] Started receiving broadcasts from: " << target_endpoint << std::endl;
	receive();
}

} /* namespace discovery */
} /* namespace p2pnet */