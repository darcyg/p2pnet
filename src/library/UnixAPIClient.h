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
#ifndef UNIXAPI_H_
#define UNIXAPICLIENT_H_

#include "../common/api/UnixAPISocket.h"

namespace p2pnet {
namespace api {
namespace unix {

class UnixAPIClient : public APIClient {
	std::string socket_path;

	UnixAPISocket socket;
public:
	UnixAPIClient(boost::asio::io_service& io_service);
	virtual ~UnixAPIClient();

	void send(APIMessage message);

	void shutdown();

	void connect();
};

} /* namespace unix */
} /* namespace api */
} /* namespace p2pnet */

#endif /* UNIXAPICLIENT_H_ */