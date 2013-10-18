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
#include "OverlayConnection.h"
#include "../databases/PersonalKeyStorage.h"
#include "../transport/TransportSocket.h"
#include <algorithm>

namespace p2pnet {
namespace overlay {

OverlayConnection::OverlayConnection(overlay::TH th) : th_endpoint(th) {
	pks = databases::PersonalKeyStorage::getInstance();
	log() << "New Overlay Connection initiated with TH:" << th_endpoint.toBase58() << std::endl;
}
OverlayConnection::~OverlayConnection() {}

void OverlayConnection::sendRaw(std::string data) {
	auto& transport_socket_connections = transport::TransportSocket::getInstance()->m_connections;

	std::shared_ptr<transport::TransportConnection> conn;

	// Searching for at least one "living" TransportConnection
	for(auto& conn_it : m_tse){
		auto sock_it = transport_socket_connections.find(conn_it);
		if(sock_it->second->connected()){
			conn = sock_it->second;
			break;
		}
	}

	if(conn){
		conn->send(data);
	}else{
		// If there are no connections alive, we store messages to "suspended".
		// And, when they will be sent after some connections arrive.
		suspended_messages.push_front(data);
	}
}

protocol::OverlayMessageStructure OverlayConnection::generateReplySkel(const protocol::OverlayMessageStructure& recv_message){
	protocol::OverlayMessageStructure new_message;

	new_message.mutable_header()->set_src_th(recv_message.header().dest_th());
	new_message.mutable_header()->set_dest_th(recv_message.header().src_th());

	return new_message;
}

protocol::OverlayMessageStructure_Payload_Part_ConnectionPart
OverlayConnection::generateConnectionPartPUBKEY(bool ack){
	protocol::OverlayMessageStructure_Payload_Part_ConnectionPart conn_part;

	conn_part.set_ack(ack);
	conn_part.set_src_ecdsa_pubkey(pks->getMyPublicKey().toBinaryString());

	return conn_part;
}

protocol::OverlayMessageStructure_Payload_Part_ConnectionPart
OverlayConnection::generateConnectionPartECDH(bool ack){
	protocol::OverlayMessageStructure_Payload_Part_ConnectionPart conn_part;

	conn_part.set_ack(ack);
	if(!bool(ecdh_key))
		ecdh_key.generateKey();
	std::string ecdh_public = ecdh_key.derivePublicKey();
	conn_part.set_src_ecdh_pubkey(ecdh_public);

	std::string signature = pks->getMyPrivateKey().sign(ecdh_public+th_endpoint.toBinaryString());
	conn_part.set_signature(signature);

	return conn_part;
}

void OverlayConnection::addKeyRotationPart(protocol::OverlayMessageStructure& answ_message,
		bool& send_answ,
		std::shared_ptr<crypto::PrivateKeyDSA> old_dsa_private) {
	if(old_dsa_private != nullptr && !(pks->getMyPrivateKey().derivePublicKey() == old_dsa_private->derivePublicKey())){
		send_answ = true;	// We should notify remote endpoint about our key rotation.
		protocol::OverlayMessageStructure_Payload_Part_KeyRotationPart part_to_serialize;

		std::string old_key_s = old_dsa_private->derivePublicKey().toBinaryString();
		std::string new_key_s = pks->getMyPublicKey().toBinaryString();

		auto payload_ptr = answ_message.mutable_payload()->add_payload_parts();
		payload_ptr->set_payload_type(payload_ptr->KEY_ROTATION);

		part_to_serialize.set_old_ecdsa_key(old_key_s);
		part_to_serialize.set_new_ecdsa_key(new_key_s);

		part_to_serialize.set_old_signature(old_dsa_private->sign(old_key_s+new_key_s));
		part_to_serialize.set_new_signature(pks->getMyPrivateKey().sign(old_key_s+new_key_s));
	}
}

void OverlayConnection::processTransmissionControlPart(protocol::OverlayMessageStructure& answ_message,
		bool& send_answ,
		const protocol::OverlayMessageStructure& recv_message,
		const protocol::OverlayMessageStructure_Payload_Part& part){
	if(part.payload_type() == part.TRANSMISSION_CONTROL
			&& answ_message.header().prio() == answ_message.header().RELIABLE){
		protocol::OverlayMessageStructure_Payload_Part_TransmissionControlPart uns_tcpart;
		uns_tcpart.ParseFromString(part.serialized_part());

		auto payload_ptr = answ_message.mutable_payload()->add_payload_parts();
		payload_ptr->set_payload_type(payload_ptr->TRANSMISSION_CONTROL);

		protocol::OverlayMessageStructure_Payload_Part_TransmissionControlPart part_to_serialize;

		part_to_serialize.set_seq_num(seq_counter++);
		*(part_to_serialize.mutable_ack_num()->Add()) = uns_tcpart.seq_num();
		payload_ptr->set_serialized_part(part_to_serialize.SerializeAsString());
	}
}

bool OverlayConnection::isReady() const {
	return state == ESTABLISHED;
}

void OverlayConnection::updateTransportSocketEndpoint(transport::TransportSocketEndpoint from) {
	auto it = std::find(m_tse.begin(), m_tse.end(), from);
	if(it == m_tse.end()){
		m_tse.push_front(from);
	}else{
		m_tse.erase(it);	// TODO: some spoofing attack could be attempted here. For example, one packet from malicious ip could block any connection to this peer.
		m_tse.push_front(from);
	}
}

void OverlayConnection::send(std::string data) {
}

void OverlayConnection::process(std::string data, transport::TransportSocketEndpoint from) {
	updateTransportSocketEndpoint(from);

	// Reading message structure from string.
	protocol::OverlayMessageStructure recv_message;
	if(!recv_message.ParseFromString(data)){
		return;	// Drop.
	}

	std::shared_ptr<crypto::PrivateKeyDSA> our_historic_ecdsa_privkey;
	if(recv_message.header().has_dest_th()){
		overlay::TH dest_th = overlay::TH::fromBinaryString(recv_message.header().dest_th());
		our_historic_ecdsa_privkey = pks->getPrivateKeyOfTH(dest_th);
	}

	if(our_historic_ecdsa_privkey != nullptr || !recv_message.header().has_dest_th()){
		// So, this message is for us.
		protocol::OverlayMessageStructure answ_message = generateReplySkel(recv_message);
		bool send_answer = false;

		addKeyRotationPart(answ_message, send_answer, our_historic_ecdsa_privkey);

		for(auto& payload_part : recv_message.payload().payload_parts()){
			processTransmissionControlPart(answ_message, send_answer, recv_message, payload_part);
			processConnectionPart(answ_message, send_answer, recv_message, payload_part);
		}

		if(send_answer)
			sendRaw(answ_message.SerializeAsString());
	}else{
		// This message is completely stale, or it is intended to be retransmitted.
	}
}

void OverlayConnection::processConnectionPart(protocol::OverlayMessageStructure& answ_message,
		bool& send_answ,
		const protocol::OverlayMessageStructure& recv_message,
		const protocol::OverlayMessageStructure_Payload_Part& part) {
	if(part.payload_type() == part.CONNECTION_PUBKEY){
		processConnectionPartPUBKEY(answ_message, send_answ, recv_message, part);
	}else if(part.payload_type() == part.CONNECTION_ECDH){
		processConnectionPartECDH(answ_message, send_answ, recv_message, part);
	}else if(part.payload_type() == part.CONNECTION_ACK){
		processConnectionPartACK(answ_message, send_answ, recv_message, part);
	}
}

void OverlayConnection::processConnectionPartPUBKEY(protocol::OverlayMessageStructure& answ_message,
		bool& send_answ,
		const protocol::OverlayMessageStructure& recv_message,
		const protocol::OverlayMessageStructure_Payload_Part& part){
	protocol::OverlayMessageStructure::Payload::Part::ConnectionPart conn_part;
	if(!conn_part.ParseFromString(part.serialized_part())){
		return;	// TODO: On parse error we just drop packet now. TODO MessageReject.
	}

	auto recv_dsa_pubkey = crypto::PublicKeyDSA::fromBinaryString(conn_part.src_ecdsa_pubkey());

	if((crypto::Hash(recv_dsa_pubkey) == th_endpoint)	// So, we check if this message contains genuine ECDSA public key of connected TH.
			&& recv_dsa_pubkey.validate()){	// And then we validate this ECDSA public key using mathematical methods.
		log() << "Received public key from: TH:" << th_endpoint.toBase58() << std::endl;
		public_key = recv_dsa_pubkey;
	}else{
		return;	// Drop. TODO MessageReject.
	}

	protocol::OverlayMessageStructure::Payload::Part* new_part_ptr = answ_message.mutable_payload()->add_payload_parts();
	send_answ = true;

	if(!conn_part.ack()){	// We received PUBKEY message, so we need to send back PUBKEY_ACK.
		new_part_ptr->set_payload_type(new_part_ptr->CONNECTION_PUBKEY);
		new_part_ptr->set_serialized_part(generateConnectionPartPUBKEY(true).SerializeAsString());

		state = PUBKEY_RECEIVED;
	}else{	// We received PUBKEY_ACK message, so we need to send back ECDH.
		new_part_ptr->set_payload_type(new_part_ptr->CONNECTION_ECDH);
		new_part_ptr->set_serialized_part(generateConnectionPartECDH(false).SerializeAsString());

		state = ECDH_SENT;
	}
}

void OverlayConnection::processConnectionPartECDH(protocol::OverlayMessageStructure& answ_message,
		bool& send_answ,
		const protocol::OverlayMessageStructure& recv_message,
		const protocol::OverlayMessageStructure_Payload_Part& part){
}

void OverlayConnection::processConnectionPartACK(protocol::OverlayMessageStructure& answ_message,
		bool& send_answ,
		const protocol::OverlayMessageStructure& recv_message,
		const protocol::OverlayMessageStructure_Payload_Part& part){
}

/*void OverlayConnection::processConnectionECDHMessage(protocol::OverlayMessageStructure message){
	auto payload = message.payload();
	protocol::OverlayMessageStructure::Payload::ConnectionPart conn_part;
	if(conn_part.ParseFromString(payload.serialized_payload())){
		bool ack = conn_part.ack();	// It means, that this is an answer.

		auto message_dsa_pubkey = crypto::PublicKeyDSA::fromBinaryString(conn_part.src_ecdsa_pubkey());

		if((crypto::Hash(message_dsa_pubkey) == th_endpoint) && message_dsa_pubkey.validate()){
			log() << "Received public key from: TH:" << th_endpoint.toBase58() << std::endl;
			public_key = message_dsa_pubkey;
		}else{
			return;	// Drop.
		}

		// Then we generate new message.
		protocol::OverlayMessageStructure new_message;
		new_message.mutable_header()->set_src_th(
				databases::PersonalKeyStorage::getInstance()->getMyTransportHash().toBinaryString());
		new_message.mutable_header()->set_dest_th(th_endpoint.toBinaryString());

		protocol::OverlayMessageStructure::Payload::ConnectionPart new_conn_part;
		if(!ack){	// We received PUBKEY message, so we need to send back PUBKEY_ACK.
			new_message.mutable_payload()->set_message_type(new_message.payload().CONNECTION_PUBKEY);
			new_conn_part.set_ack(true);
			new_conn_part.set_src_ecdsa_pubkey(databases::PersonalKeyStorage::getInstance()->getMyPublicKey().toBinaryString());
			state = PUBKEY_RECEIVED;
		}else{
			new_message.mutable_payload()->set_message_type(new_message.payload().CONNECTION_ECDH);
			// We received PUBKEY_ACK message, so we need to send back ECDH.
			ecdh_key.generateKey();
			std::string ecdh_public = ecdh_key.derivePublicKey();
			new_conn_part.set_src_ecdh_pubkey(ecdh_public);

			std::string signature = databases::PersonalKeyStorage::getInstance()->getMyPrivateKey().sign(ecdh_public+th_endpoint.toBinaryString());
			new_conn_part.set_signature(signature);

			state = ECDH_SENT;
		}
		new_message.mutable_payload()->set_serialized_payload(new_conn_part.SerializeAsString());
		sendRaw(new_message.SerializeAsString());
	}
}*/


} /* namespace overlay */
} /* namespace p2pnet */