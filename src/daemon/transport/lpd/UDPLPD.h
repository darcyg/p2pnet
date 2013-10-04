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

#ifndef UDPLPD_H_
#define UDPLPD_H_

#include "GenericLPD.h"
#include "../../../common/Config.h"
#include "../../databases/NetDBStorage.h"
#include "../../protobuf/Protocol.pb.h"
#include "../../messaging/RejectException.h"
#include <boost/asio.hpp>
#include <string>
#include <memory>

using namespace boost::asio;

namespace p2pnet {
namespace transport {
namespace lpd {

class UDPLPD: public GenericLPD {
	void waitBeforeSend();

	bool reject(messaging::Reason);
	bool reject(messaging::Reason, std::string comment);
	/**
	 * This function is invoked on receiving packets. It must perform necessary checks for packet integrity.
	 * Its arguments are from asio::socket, as it works as callback.
	 */
	void processReceived(size_t bytes, std::shared_ptr<ip::udp::endpoint> endpoint, char* recv_buffer);

	/**
	 * Message to be sent to UDP multicast.
	 * @return Protobuf structure, ready to be serialized.
	 */
	messaging::protocol::UDPLPDMessage generateLPDMessage();

protected:
	ConfigManager& m_config;

	io_service& m_io_service;
	bool initialized = false;

	/**
	 * This function is virtual, so it returns UDP port for given protocol, for IPv4 or IPv6.
	 * @return
	 */
	virtual unsigned short getUDPPort() = 0;

	/**
	 * This timer is used between sending two messages. Default is to wait 10 seconds.
	 */
	deadline_timer m_timer;
	unsigned int m_timer_seconds = 0;

	//! boost::asio multicast UDP Socket
	ip::udp::socket m_lpd_socket;

	ip::address m_target_address;
	unsigned short m_target_port = 0;

	ip::address m_bind_address;

	/**
	 * Message, that we are going to send is stored here.
	 */

public:
	UDPLPD(ConfigManager& config);
	virtual ~UDPLPD();

	void run();

	virtual void readConfig() = 0;
	virtual void initSocket() = 0;
	virtual std::string getComponentName(){
		return "UDPLPD";
	}

	void configChanged();

	void send();
	void receive();

	void startSend();
	void startReceive();
};

} /* namespace lpd */
} /* namespace net */
} /* namespace p2pnet */
#endif /* UDPLPD_H_ */