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

#ifndef BASE_CONVERT_H_
#define BASE_CONVERT_H_

#include <string>

namespace p2pnet {
namespace crypto {

std::string encodeToHex(std::string data);
std::string decodeFromHex(std::string hex_string);

std::string encodeToBase58(std::string data);
std::string decodeFromBase58(std::string base58);

std::string encodeToBase64(std::string data);
std::string decodeFromBase64(std::string base64);

}
}

#endif /* BASE_CONVERT_H_ */
