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

#include <dfhack-client-qt/Client.h>

#include <QEventLoop>
#include <QFutureWatcher>
#include <QTcpSocket>

#include <queue>

#include <QtDebug>

using namespace DFHack;

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
	Handshake,
	Ready,
	WaitingForMessageHeader,
	WaitingForMessageContent,
	Disconnecting,
};

struct call_t {
	int id;
	const google::protobuf::MessageLite *in;
	google::protobuf::MessageLite *out;
	QPromise<CommandResult> result;
	QPromise<TextNotification> notifications;

	call_t(int id,
	       const google::protobuf::MessageLite *in,
	       google::protobuf::MessageLite *out)
		: id(id)
		, in(in)
		, out(out)
	{
	}

	void finish(CommandResult cr)
	{
#ifdef DFHACK_CLIENT_QT_DEBUG
			qDebug() << "finished call" << static_cast<int>(cr);
#endif
		result.addResult(cr);
		result.finish();
		notifications.finish();
	}
};

struct Client::Private
{
	QTcpSocket socket;
	State state = State::Disconnected;
	MessageHeader header; // current message header
	std::queue<call_t> call_queue;
	QPromise<bool> connect_promise;

	Private(QObject *parent): socket(parent) {}
};


Client::Client(QObject *parent)
	: QObject(parent)
	, p(std::make_unique<Client::Private>(this))
{
	QObject::connect(&p->socket, &QIODevice::readyRead,
	        this, &Client::readyRead);
	QObject::connect(&p->socket, &QAbstractSocket::connected,
	        this, &Client::connected);
	QObject::connect(&p->socket, &QAbstractSocket::disconnected,
	        this, &Client::disconnected);
	QObject::connect(&p->socket, &QAbstractSocket::errorOccurred,
		this, &Client::error);
}

Client::~Client()
{
	invalidateBindings();
	if (p->state != State::Disconnected) {
		// Disconnect and wait
		moveToThread(QThread::currentThread());
		auto [result, notifications] = enqueueCall(MessageHeader::RequestQuit, nullptr, nullptr);
		QFutureWatcher<CommandResult> watcher;
		QEventLoop loop;
		QObject::connect(&watcher, &QFutureWatcher<CommandResult>::finished,
			&loop, &QEventLoop::quit);
		watcher.setFuture(result);
		loop.exec();
	}
}

QFuture<bool> Client::connect(const QString &host, quint16 port)
{
	QPromise<bool> promise;
	auto res = promise.future();
	QMetaObject::invokeMethod(this, [this, host=host, port, promise = std::move(promise)]() mutable {
		switch (p->state) {
		case State::Disconnected:
#ifdef DFHACK_CLIENT_QT_DEBUG
			qDebug() << "connecting to host";
#endif
			p->state = State::Connecting;
			p->connect_promise = std::move(promise);
			p->connect_promise.start();
			p->socket.connectToHost(host, port);
			return;
		case State::Connecting:
		case State::Handshake:
			promise.start();
			p->connect_promise.future().then([promise = std::move(promise)](bool success) mutable {
				promise.addResult(success);
				promise.finish();
			});
			return;
		default:
			promise.start();
			promise.addResult(true);
			promise.finish();
			return;
		}
	});
	return res;
}

QFuture<void> Client::disconnect()
{
	return enqueueCall(MessageHeader::RequestQuit, nullptr, nullptr).first
		.then([](CommandResult){});
}

std::pair<QFuture<CommandResult>, QFuture<TextNotification>> Client::call(int16_t id,
					const google::protobuf::MessageLite &in,
					google::protobuf::MessageLite &out)
{
	return enqueueCall(id, &in, &out);
}

std::pair<QFuture<CommandResult>, QFuture<TextNotification>> Client::enqueueCall(
		int id,
		const google::protobuf::MessageLite *in,
		google::protobuf::MessageLite *out)
{
	call_t call(id, in, out);
	auto result = call.result.future();
	auto notifications = call.notifications.future();
	QMetaObject::invokeMethod(this, [this, call = std::move(call)]() mutable {
			if (p->socket.state() != QAbstractSocket::ConnectedState) {
#ifdef DFHACK_CLIENT_QT_DEBUG
				qDebug() << "call with unconnected client";
#endif
				call.finish(CommandResult::LinkFailure);
				return;
			}
#ifdef DFHACK_CLIENT_QT_DEBUG
			qDebug() << "queue RPC call" << call.id;
#endif
			p->call_queue.push(std::move(call));
			if (p->state == State::Ready)
				sendNextCall();
		});
	return {result, notifications};
}

void Client::sendNextCall()
{
	assert(p->socket.state() == QAbstractSocket::ConnectedState);
	assert(p->state == State::Ready);
	assert(!p->call_queue.empty());

	auto &call = p->call_queue.front();
#ifdef DFHACK_CLIENT_QT_DEBUG
	qDebug() << "send next call" << call.id;
#endif
	call.result.start();
	call.notifications.start();

	std::string msg;
	MessageHeader hdr;
	if (call.id == MessageHeader::RequestQuit) {
		hdr.id = MessageHeader::RequestQuit;
		hdr.size = 0;
		p->state = State::Disconnecting;
		write(&hdr);
		call.finish(CommandResult::Ok);
		p->call_queue.pop();
		p->socket.disconnectFromHost();
	}
	else {
		std::string msg = call.in->SerializeAsString();
		hdr.id = call.id;
		hdr.size = msg.size();
		p->state = State::WaitingForMessageHeader;
		write(&hdr);
		write(msg.data(), msg.size());
	}
}

void Client::readyRead()
{
	assert(p->socket.state() == QAbstractSocket::ConnectedState);
#ifdef DFHACK_CLIENT_QT_DEBUG
	qDebug() << "read data" << static_cast<int>(p->state);
#endif

	while (p->state != State::Ready) {
		switch (p->state) {
		case State::Handshake: {
			HandshakePacket packet;
			if (!read(&packet))
				return;

			if (!std::equal(&packet.magic[0], &packet.magic[HandshakePacket::MagicSize],
					HandshakePacket::ReplyMagic)) {
				qCritical() << "Handshake message mismatch" << QByteArray(packet.magic, HandshakePacket::MagicSize);
				p->state = State::Disconnected;
				p->socket.close();
				finishConnection(false);
				return;
			}
#ifdef DFHACK_CLIENT_QT_DEBUG
			qDebug() << "handshake ok";
#endif

			p->state = State::Ready;
			finishConnection(true);
			break;
		}
		case State::WaitingForMessageHeader:
			if (!read(&p->header))
				return;
			if (p->header.id == MessageHeader::ReplyFail) {
				if (p->header.size < -3 || p->header.size > 3)
					finishCall(CommandResult::LinkFailure);
				else
					finishCall(static_cast<CommandResult>(p->header.size));
			}
			else {
				p->state = State::WaitingForMessageContent;
			}
			break;

		case State::WaitingForMessageContent: {
			auto &call = p->call_queue.front();
			QByteArray reply;
			if (!read(reply, p->header.size))
				return;
			switch (p->header.id) {
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
					call.notifications.addResult(TextNotification {
							static_cast<Color>(fragment.color()),
							text
						});
				}
				p->state = State::WaitingForMessageHeader;
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
	}
	if (!p->call_queue.empty())
		sendNextCall();
}

void Client::connected()
{
#ifdef DFHACK_CLIENT_QT_DEBUG
	qDebug() << "handshake";
#endif

	p->state = State::Handshake;
	HandshakePacket packet;
	std::copy_n(HandshakePacket::RequestMagic, HandshakePacket::MagicSize, packet.magic);
	packet.version = 1;
	write(&packet);
}

void Client::disconnected()
{
#ifdef DFHACK_CLIENT_QT_DEBUG
	qDebug() << "disconnected";
#endif
	if (p->state != State::Disconnecting) {
		qWarning() << "Socket unexpectedly disconnected";
	}
	bool during_connection = p->state == State::Connecting || p->state == State::Handshake;
	p->state = State::Disconnected;
	p->socket.close();
	// cancel pending calls
	while (!p->call_queue.empty()) {
		auto &call = p->call_queue.front();
		call.finish(CommandResult::LinkFailure);
		p->call_queue.pop();
	}
	invalidateBindings();
	if (during_connection)
		finishConnection(false);
	emit connectionChanged(false);
}

void Client::error(QAbstractSocket::SocketError error)
{
	if (p->state == State::Disconnecting && error == QAbstractSocket::RemoteHostClosedError)
		return;
	qCritical() << "DFHack client socket error:" << p->socket.errorString();
	if (p->state == State::Connecting)
		finishConnection(false);
	p->state = State::Disconnected;
	emit socketError(error, p->socket.errorString());
}

void Client::finishConnection(bool success)
{
	p->connect_promise.addResult(success);
	p->connect_promise.finish();
	if (success)
		emit connectionChanged(success);
}

void Client::finishCall(CommandResult result)
{
#ifdef DFHACK_CLIENT_QT_DEBUG
	qDebug() << "call finished" << static_cast<int>(result);
#endif
	p->state = State::Ready;
	auto call = std::move(p->call_queue.front());
	p->call_queue.pop();
	call.finish(result);
}

template<typename T> bool Client::read(T *data)
{
	if (p->socket.bytesAvailable() < qint64(sizeof(T)))
		return false;
	if (-1 == p->socket.read(reinterpret_cast<char *>(data), sizeof(T))) {
		qCritical() << "Failed to read data from socket";
		p->state = State::Disconnected;
		p->socket.close();
		return false;
	}
	return true;
}

bool Client::read(QByteArray &data, qint64 size)
{
	if (p->socket.bytesAvailable() < size)
		return false;
	data.resize(size);
	if (-1 == p->socket.read(data.data(), size)) {
		qCritical() << "Failed to read data from socket";
		p->state = State::Disconnected;
		p->socket.close();
		return false;
	}
	return true;
}

template<typename T> bool Client::write(const T *data)
{
	return write(reinterpret_cast<const char *>(data), sizeof(T));
}

bool Client::write(const char *data, qint64 size)
{
	while (size > 0) {
		qint64 r;
		if (-1 == (r = p->socket.write(data, size))) {
			qCritical() << "Failed to write data to socket";
			p->state = State::Disconnected;
			p->socket.close();
			return false;
		}
		data += r;
		size -= r;
	}
	return true;
}

void Client::invalidateBindings()
{
	QMutexLocker lock(&bindings_mutex);
	for (const auto &p: bindings)
		p.second->result = makeFailedResult();
	bindings.clear();
}

QFuture<CommandResult> Client::makeFailedResult()
{
	QPromise<CommandResult> p;
	p.addResult(CommandResult::LinkFailure);
	p.finish();
	return p.future();
}

