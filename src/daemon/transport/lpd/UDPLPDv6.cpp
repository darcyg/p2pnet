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

#include "UDPLPDv6.h"

namespace p2pnet {
namespace transport {
namespace lpd {

UDPLPDv6::UDPLPDv6(ConfigManager& config) : UDPLPD(config) {
	readConfig();
}
UDPLPDv6::~UDPLPDv6() {
	m_lpd_socket.set_option(ip::multicast::leave_group(m_target_address));
}

unsigned short UDPLPDv6::getUDPPort(){
	return m_config.getConfig().get("net.sockets.udpv6.port", 2185);
}

void UDPLPDv6::readConfig() {
	this->m_timer_seconds = getConfigValueOrDefault<unsigned int>("net.lpd.udpv6.timer");
	this->m_target_address = ip::address::from_string(getConfigValueOrDefault<std::string>("net.lpd.udpv6.mcast.host"));
	this->m_target_port = getConfigValueOrDefault<unsigned short>("net.lpd.udpv6.mcast.port");
	this->m_bind_address = ip::address::from_string(getConfigValueOrDefault<std::string>("net.lpd.udpv6.local_ip"));
}

void UDPLPDv6::initSocket() {
	if(!initialized){
		m_lpd_socket.open(ip::udp::v6());

		m_lpd_socket.set_option(ip::multicast::join_group(m_target_address));
		m_lpd_socket.set_option(ip::multicast::enable_loopback(false));
		m_lpd_socket.set_option(ip::udp::socket::reuse_address(true));
		m_lpd_socket.set_option(ip::v6_only(true));

		m_lpd_socket.bind(ip::udp::endpoint(m_bind_address, m_target_port));
		initialized = true;
	}
}

} /* namespace lpd */
} /* namespace net */
} /* namespace p2pnet */