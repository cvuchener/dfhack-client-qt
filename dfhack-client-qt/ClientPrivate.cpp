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

#include "ClientPrivate.h"

#include <QtDebug>

#include <dfhack-client-qt/Notifier.h>

using namespace DFHack;

constexpr char HandshakePacket::RequestMagic[HandshakePacket::MagicSize];
constexpr char HandshakePacket::ReplyMagic[HandshakePacket::MagicSize];

ClientPrivate::call_t::call_t(int id,
			      const google::protobuf::MessageLite *in,
			      google::protobuf::MessageLite *out,
			      CallNotifier *notifier)
	: id(id)
	, in(in)
	, out(out)
	, notifier(notifier)
{
}

ClientPrivate::ClientPrivate(Client *client, QThread *thread, QObject *parent)
	: QObject(parent)
	, client(client)
	, socket(this)
{
	moveToThread(thread);

	connect(&socket, &QIODevice::readyRead,
	        this, &ClientPrivate::readyRead);
	connect(&socket, &QAbstractSocket::connected,
	        this, &ClientPrivate::connected);
	connect(&socket, &QAbstractSocket::disconnected,
	        this, &ClientPrivate::disconnected);
	connect(&socket, &QAbstractSocket::errorOccurred,
		this, &ClientPrivate::error);
}

ClientPrivate::~ClientPrivate()
{
}

void ClientPrivate::sendConnect(const QString &host, quint16 port)
{
	QMutexLocker lock(&mutex);
	assert(socket.state() == QAbstractSocket::UnconnectedState);
	assert(state == State::Disconnected);

#ifdef DFHACK_CLIENT_QT_DEBUG
	qDebug() << "connecting to host";
#endif
	socket.connectToHost(host, port);
}

std::future<CommandResult> ClientPrivate::enqueueCall(
		int id,
		const google::protobuf::MessageLite *in,
		google::protobuf::MessageLite *out,
		CallNotifier *notifier)
{
	call_queue.emplace(id, in, out, notifier);
	auto future = call_queue.back().promise.get_future();
#ifdef DFHACK_CLIENT_QT_DEBUG
	qDebug() << "queue RPC call";
#endif
	if (state == State::Ready && call_queue.size() == 1)
		QMetaObject::invokeMethod(this, &ClientPrivate::sendCall);
	return future;
}

void ClientPrivate::sendCall()
{
	QMutexLocker lock(&mutex);
#ifdef DFHACK_CLIENT_QT_DEBUG
	qDebug() << "waking for sending call";
#endif
	sendNextCall();
}

void ClientPrivate::sendNextCall()
{
	assert(socket.state() == QAbstractSocket::ConnectedState);
	assert(state == State::Ready);
	assert(!call_queue.empty());

	auto &call = call_queue.front();
#ifdef DFHACK_CLIENT_QT_DEBUG
	qDebug() << "send next call" << call.id;
#endif
	std::string msg;
	MessageHeader hdr;
	if (call.id == MessageHeader::RequestQuit) {
		hdr.id = MessageHeader::RequestQuit;
		hdr.size = 0;
		state = State::Disconnecting;
		write(&hdr);
	}
	else {
		std::string msg = call.in->SerializeAsString();
		hdr.id = call.id;
		hdr.size = msg.size();
		state = State::WaitingForMessageHeader;
		write(&hdr);
		write(msg.data(), msg.size());
	}
	if (call.notifier)
		emit call.notifier->started();

}

void ClientPrivate::readyRead()
{
	QMutexLocker lock(&mutex);
#ifdef DFHACK_CLIENT_QT_DEBUG
	qDebug() << "waking for incoming data";
#endif
	readData();
}

void ClientPrivate::readData()
{
	assert(socket.state() == QAbstractSocket::ConnectedState);
#ifdef DFHACK_CLIENT_QT_DEBUG
	qDebug() << "read data" << static_cast<int>(state);
#endif

	switch (state) {
	case State::Connecting: {
		HandshakePacket packet;
		if (!read(&packet))
			return;

		if (!std::equal(&packet.magic[0], &packet.magic[HandshakePacket::MagicSize],
				HandshakePacket::ReplyMagic)) {
			qCritical() << "Handshake message mismatch" << QByteArray(packet.magic, HandshakePacket::MagicSize);
			state = State::Disconnected;
			socket.close();
			return;
		}
#ifdef DFHACK_CLIENT_QT_DEBUG
		qDebug() << "handshake ok";
#endif

		state = State::Ready;
		emit client->connectionChanged(true);
		break;
	}
	case State::WaitingForMessageHeader:
		if (!read(&header))
			return;
		if (header.id == MessageHeader::ReplyFail) {
			if (header.size < -3 || header.size > 3)
				finishCall(CommandResult::LinkFailure);
			else
				finishCall(static_cast<CommandResult>(header.size));
		}
		else {
			state = State::WaitingForMessageContent;
		}
		break;

	case State::WaitingForMessageContent: {
		auto &call = call_queue.front();
		QByteArray reply;
		if (!read(reply, header.size))
			return;
		switch (header.id) {
		case MessageHeader::ReplyResult:
			if (!call.out->ParseFromArray(reply.data(), reply.size()))
				finishCall(CommandResult::LinkFailure);
			else
				finishCall(CommandResult::Ok);
			break;
		case MessageHeader::ReplyText: {
			dfproto::CoreTextNotification text;
			if (!text.ParseFromArray(reply.data(), reply.size())) {
				qCritical() << "Failed to parse CoreTextNotification";
			}
			for (const auto &fragment: text.fragments()) {
				auto text = QString::fromStdString(fragment.text());
#ifdef DFHACK_CLIENT_QT_DEBUG
				qDebug() << "DFHack notification:" << text;
#endif
				if (call.notifier) {
					emit call.notifier->notification(
							static_cast<Color>(fragment.color()),
							text);
				}
			}
			state = State::WaitingForMessageHeader;
			break;
		}
		default:
			qCritical() << "Unknown message id in header";
			finishCall(CommandResult::LinkFailure);
		}
		break;
	}
	default:
		qCritical() << "Unexpected data";
		return;
	}
	if (state != State::Ready)
		readData();
	else if (!call_queue.empty())
		sendNextCall();
}

void ClientPrivate::finishCall(CommandResult result)
{
#ifdef DFHACK_CLIENT_QT_DEBUG
	qDebug() << "call finished" << static_cast<int>(result);
#endif
	state = State::Ready;
	auto call = std::move(call_queue.front());
	call_queue.pop();
	call.promise.set_value(result);
	if (call.notifier)
		emit call.notifier->finished(result);
}

void ClientPrivate::connected()
{
	QMutexLocker lock(&mutex);
#ifdef DFHACK_CLIENT_QT_DEBUG
	qDebug() << "handshake";
#endif

	HandshakePacket packet;
	std::copy_n(HandshakePacket::RequestMagic, HandshakePacket::MagicSize, packet.magic);
	packet.version = 1;
	state = State::Connecting;
	write(&packet);
}

void ClientPrivate::disconnected()
{
	QMutexLocker lock(&mutex);
#ifdef DFHACK_CLIENT_QT_DEBUG
	qDebug() << "disconnected";
#endif
	if (state != State::Disconnecting) {
		qWarning() << "Socket unexpectedly disconnected";
	}
	state = State::Disconnected;
	socket.close();
	// cancel pending calls
	while (!call_queue.empty()) {
		auto &call = call_queue.front();
		if (call.notifier)
			call.notifier->finished(CommandResult::LinkFailure);
		call.promise.set_value(CommandResult::LinkFailure);
		call_queue.pop();
	}
	emit client->connectionChanged(false);
}

void ClientPrivate::error(QAbstractSocket::SocketError error)
{
	if (state == State::Disconnecting && error == QAbstractSocket::RemoteHostClosedError)
		return;
	qCritical() << "DFHack client socket error:" << socket.errorString();
	emit client->socketError(error, socket.errorString());
}

template<typename T> bool ClientPrivate::read(T *data)
{
	if (socket.bytesAvailable() < sizeof(T))
		return false;
	if (-1 == socket.read(reinterpret_cast<char *>(data), sizeof(T))) {
		qCritical() << "Failed to read data from socket";
		state = State::Disconnected;
		socket.close();
		return false;
	}
	return true;
}

bool ClientPrivate::read(QByteArray &data, int size)
{
	if (socket.bytesAvailable() < size)
		return false;
	data.resize(size);
	if (-1 == socket.read(data.data(), size)) {
		qCritical() << "Failed to read data from socket";
		state = State::Disconnected;
		socket.close();
		return false;
	}
	return true;
}

template<typename T> bool ClientPrivate::write(const T *data)
{
	if (-1 == socket.write(reinterpret_cast<const char *>(data), sizeof(T))) {
		qCritical() << "Failed to write data to socket";
		state = State::Disconnected;
		socket.close();
		return false;
	}
	return true;
}

bool ClientPrivate::write(const char *data, int size)
{
	if (-1 == socket.write(data, size)) {
		qCritical() << "Failed to write data to socket";
		state = State::Disconnected;
		socket.close();
		return false;
	}
	return true;
}
