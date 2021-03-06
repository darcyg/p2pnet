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
#include "UDPInterfaceEndpoint.h"

namespace p2pnet {
namespace transport {

UDPInterfaceEndpoint::UDPInterfaceEndpoint() : IPInterfaceEndpoint("UDP") {}
UDPInterfaceEndpoint::UDPInterfaceEndpoint(boost::asio::ip::address ip, port_t port) : IPInterfaceEndpoint(ip, port, "UDP") {}
UDPInterfaceEndpoint::UDPInterfaceEndpoint(const std::string& ip, port_t port) : IPInterfaceEndpoint(ip, port, "UDP") {}

std::shared_ptr<InterfaceEndpoint> UDPInterfaceEndpoint::clone(){
	auto new_endpoint = std::make_shared<UDPInterfaceEndpoint>(getAsioIP(), getPort());
	return new_endpoint;
}

} /* namespace transport */
} /* namespace p2pnet */
