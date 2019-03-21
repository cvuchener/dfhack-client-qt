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

#include <QtDebug>

#include "ClientPrivate.h"

using namespace DFHack;

Client::Client(QObject *parent)
	: QObject(parent)
	, p(std::make_unique<ClientPrivate>(this, &thread))
{
	QObject::connect(this, &Client::connectionChanged, [this] (bool connected) {
		// Invalidate all current bindings
		QMutexLocker lock(&bindings_mutex);
		for (const auto &p: bindings)
			p.second->valid = false;
		bindings.clear();
	});
	thread.start();
}

Client::~Client()
{
	// Disconnect and wait
	std::future<DFHack::CommandResult> result;
	{
		QMutexLocker lock(&p->mutex);
		if (p->socket.state() == QAbstractSocket::ConnectedState)
			 result = p->enqueueCall(MessageHeader::RequestQuit, nullptr, nullptr, nullptr);
	}
	if (result.valid())
		result.get();
	thread.quit();
	thread.wait();
}

bool Client::connect(const QString &host, quint16 port)
{
	QMutexLocker lock(&p->mutex);
	if (p->socket.state() != QAbstractSocket::UnconnectedState ||
			p->state != State::Disconnected)
		return false;
	QMetaObject::invokeMethod(p.get(), [this, host=host, port] () {
		p->sendConnect(host, port);
	});
	return true;
}

bool Client::disconnect()
{
	QMutexLocker lock(&p->mutex);
	if (p->socket.state() != QAbstractSocket::ConnectedState)
		return false;
	return p->enqueueCall(MessageHeader::RequestQuit, nullptr, nullptr, nullptr).valid();
}

std::future<CommandResult> Client::call(int16_t id,
					const google::protobuf::MessageLite *in,
					google::protobuf::MessageLite *out,
					CallNotifier *notifier)
{
	QMutexLocker lock(&p->mutex);
	if (p->socket.state() != QAbstractSocket::ConnectedState) {
		if (notifier)
			emit notifier->finished(CommandResult::LinkFailure);
		return std::future<CommandResult>();
	}
	return p->enqueueCall(id, in, out, notifier);
}

