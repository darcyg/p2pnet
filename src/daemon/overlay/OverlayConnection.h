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
#ifndef OVERLAYCONNECTION_H_
#define OVERLAYCONNECTION_H_

#include "TH.h"
#include "../transport/TransportConnection.h"
#include "../../common/crypto/ECDH.h"
#include "../../common/crypto/AES.h"
#include "../../common/crypto/PublicKeyDSA.h"
#include "../protobuf/Protocol.pb.h"
#include "../databases/PersonalKeyStorage.h"
#include <deque>
#include <unordered_map>

// TODO: fetch this thing from configuration
#define KEY_ROTATION_LIMIT 5

namespace p2pnet {
namespace overlay {

class OverlayConnection : public Loggable, public std::enable_shared_from_this {
	overlay::TH th_endpoint;
	crypto::PublicKeyDSA public_key;

	crypto::ECDH ecdh_key;
	crypto::AES aes_key;

	uint32_t seq_counter = 0;

	std::deque<transport::TransportSocketEndpoint> m_tse;
	// This is about messages, that we can't deliver, because all the TransportSockets are inactive.
	std::deque<std::string> suspended_data;
	std::deque<std::string> suspended_messages;	// These messages are not delivered, as we didn't set up encryption.
	std::unordered_map<uint32_t, std::string> sent_message_buffer;
	std::deque<uint32_t> processed_messages;	// To avoid double-processing. If our ACK messages were not delivered well.

	boost::asio::deadline_timer udp_key_rotation_limit;
	bool udp_key_rotation_locked = false;

	enum States {
		CLOSED = 0,
		PUBKEY_SENT = 1,
		PUBKEY_RECEIVED = 2,
		ECDH_SENT = 3,
		ECDH_RECEIVED = 4,
		ESTABLISHED = 5
	} state = CLOSED;

	databases::PersonalKeyStorage* pks;

	/**
	 * It is a function, that tries to manage TransportConnection directly.
	 * This function is about connectivity, and "send" is about encryption.
	 * @param data
	 */
	void sendData(std::string data);
	void sendMessage(const protocol::OverlayMessage& send_message);

	bool performLocalKeyRotation(const protocol::OverlayMessage& recv_message);

	protocol::OverlayMessage generateReplySkel(const protocol::OverlayMessage& recv_message);
	protocol::OverlayMessage_Payload_ConnectionPart generateConnectionPart(protocol::OverlayMessage_Payload_ConnectionPart_ConnectionStage for_stage);
	protocol::OverlayMessage_Payload_KeyRotationPart generateKeyRotationPart(const protocol::OverlayMessage& send_message, std::shared_ptr<crypto::PrivateKeyDSA> our_hist_key);

	void processConnectionPart(const protocol::OverlayMessage& recv_message,
			const protocol::OverlayMessage_Payload& decrypted_payload = protocol::OverlayMessage_Payload::default_instance());
	void processConnectionPartPUBKEY(const protocol::OverlayMessage& recv_message);
	void processConnectionPartECDH(const protocol::OverlayMessage& recv_message);
	void processConnectionPartACK(const protocol::OverlayMessage& recv_message);

public:
	OverlayConnection(overlay::TH th);
	virtual ~OverlayConnection();

	bool isReady() const;

	void updateTSE(const transport::TransportSocketEndpoint& from, bool verified = false);

	void send(const protocol::OverlayMessage& send_message);
	void process(const protocol::OverlayMessage& recv_message, const transport::TransportSocketEndpoint& from);
	void process(const protocol::ConnectionRequestMessage& recv_message, const transport::TransportSocketEndpoint& from);

	std::string getComponentName(){return "OverlayConnection";}
};

} /* namespace overlay */
} /* namespace p2pnet */

#endif /* OVERLAYCONNECTION_H_ */
