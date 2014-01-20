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
#include "UnixAPIServer.h"
#include <boost/lexical_cast.hpp>
#include <cstdio>

namespace p2pnet {
namespace api {
namespace unix {

/* UnixAPISession */
UnixAPISession::UnixAPISession(UnixAPISocket* preallocated_unix_socket) {
	m_unix_socket = preallocated_unix_socket;
}

UnixAPISession::~UnixAPISession() {
	delete m_unix_socket;
}

void UnixAPISession::send(APIMessage message) {
	m_unix_socket->send(message);
}

/* UnixAPIServer */
UnixAPIServer::UnixAPIServer(boost::asio::io_service& io_service) :
		asio_io_service(io_service) {
	bool accepting = false;

	auto path_list = unix::getSocketPathList();
	auto it = path_list.begin();
	while((!accepting) && it != path_list.end()){
		try {
			socket_path = *it;
			std::remove(socket_path.c_str());
			acceptor_ptr = std::unique_ptr<stream_protocol::acceptor>(new stream_protocol::acceptor(asio_io_service, stream_protocol::endpoint(socket_path)));
			accepting = true;
		} catch (boost::system::system_error& e) {
			if(++it == path_list.end())	// sic! increment is here.
				throw;	// We can do nothing. No socket paths are available. Something is definitely wrong.
		}
	}
	log() << "Unix API initialized at: " << socket_path << std::endl;
	accept();
}

UnixAPIServer::~UnixAPIServer() {}

void UnixAPIServer::accept() {
	auto new_socket = new UnixAPISocket(asio_io_service);
	acceptor_ptr->async_accept(new_socket->getSocket(), std::bind(&UnixAPIServer::handleAccept, this, new_socket));
}

void UnixAPIServer::handleAccept(UnixAPISocket* new_socket) {
	auto new_session = std::make_shared<UnixAPISession>(new_socket);

	m_unix_sessions.insert(new_session);
	new_socket->assignShutdownHandler(std::bind(&UnixAPIServer::shutdown, this, std::weak_ptr<UnixAPISession>(new_session)));
	new_socket->assignReceiveHandler(std::bind(&APISession::process, new_session.get(), std::placeholders::_1));

	new_socket->startReceive();
	accept();
}

void UnixAPIServer::shutdown(std::weak_ptr<UnixAPISession> session_ptr) {
	m_unix_sessions.erase(session_ptr.lock());
}

} /* namespace unix */
} /* namespace api */
} /* namespace p2pnet */
