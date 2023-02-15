/*
 * Copyright 2019 Clement Vuchener
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef DFHACK_CLIENT_QT_DFHACK_CLIENT_PRIVATE_H
#define DFHACK_CLIENT_QT_DFHACK_CLIENT_PRIVATE_H

#include <QMutex>
#include <QTcpSocket>
#include <queue>
#include <QPromise>

#include <dfhack-client-qt/Client.h>

namespace DFHack
{

struct HandshakePacket
{
	static constexpr std::size_t MagicSize = 8;
	static constexpr char RequestMagic[MagicSize] = {'D','F','H','a','c','k','?','\n'};
	static constexpr char ReplyMagic[MagicSize] = {'D','F','H','a','c','k','!','\n'};

	char magic[MagicSize];
	int version;
};

struct MessageHeader
{
	static constexpr int16_t ReplyResult = -1;
	static constexpr int16_t ReplyFail = -2;
	static constexpr int16_t ReplyText = -3;
	static constexpr int16_t RequestQuit = -4;

	static constexpr int32_t MaxMessageSize = 64*1024*1024;

	int16_t id;
	int32_t size;
};

enum class State {
	Disconnected,
	Connecting,
	Ready,
	WaitingForMessageHeader,
	WaitingForMessageContent,
	Disconnecting,
};

class ClientPrivate: public QObject
{
	Q_OBJECT
public:
	ClientPrivate(Client *client, QThread *thread, QObject *parent = nullptr);
	~ClientPrivate() override;

	Client *client;
	QTcpSocket socket;
	QMutex mutex;
	State state = State::Disconnected;
	MessageHeader header; // current message header
	struct call_t {
		int id;
		const google::protobuf::MessageLite *in;
		google::protobuf::MessageLite *out;
		QPromise<CommandResult> result;
		QPromise<TextNotification> notifications;

		call_t(int id,
		       const google::protobuf::MessageLite *in,
		       google::protobuf::MessageLite *out);

		void finish(CommandResult result);
	};
	std::queue<call_t> call_queue;

	void sendConnect(const QString &host, quint16 port);
	std::pair<QFuture<CommandResult>, QFuture<TextNotification>> enqueueCall(
			int id,
			const google::protobuf::MessageLite *in,
			google::protobuf::MessageLite *out);

private:
	void sendCall();
	void readyRead();
	void connected();
	void disconnected();
	void error(QAbstractSocket::SocketError error);

	void readData();
	void sendNextCall();

	void finishCall(CommandResult result);

	template<typename T> bool read(T *data);
	bool read(QByteArray &data, int size);
	template<typename T> bool write(const T *data);
	bool write(const char *data, int size);
};

} // namespace DFHack

#endif
