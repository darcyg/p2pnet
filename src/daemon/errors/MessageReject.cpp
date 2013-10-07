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
#include "MessageReject.h"
#include <sstream>

namespace p2pnet {
namespace errors {

MessageReject::MessageReject(Reason reason) :
		m_reason(reason) {}

MessageReject::MessageReject(std::string custom_reason) :
		m_comment(custom_reason),
		m_reason(Reason::CUSTOM) {}

MessageReject::MessageReject(Reason reason, std::string comment) :
		m_comment(comment),
		m_reason(reason) {}

std::string MessageReject::what() const {
	std::ostringstream what;
	what << "Rejected";
	if(m_reason != CUSTOM){
		what << ". ";
		switch(m_reason){
		case CRC_MISMATCH:
			what << "CRC mismatch";
			break;
		case KEY_INVALID:
			what << "Public key failure";
			break;
		case PARSE_ERROR:
			what << "Protobuf parse error";
			break;
		case ENCRYPTION_NEEDED:
			what << "Encryption needed for this type of message";
			break;
		default:;
		}
	}

	if(!m_comment.empty()){
		what << ": " << m_comment;
	}

	what << ".";
	return what.str();
}

} /* namespace errors */
} /* namespace p2pnet */