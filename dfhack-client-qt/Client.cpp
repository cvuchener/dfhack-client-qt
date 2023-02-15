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
			p.second->result = makeFailedResult();
		bindings.clear();
	});
	thread.start();
}

Client::~Client()
{
	// Disconnect and wait
	QFuture<CommandResult> result;
	QFuture<TextNotification> notifications;
	{
		QMutexLocker lock(&p->mutex);
		if (p->socket.state() == QAbstractSocket::ConnectedState)
			 std::tie(result, notifications) = p->enqueueCall(MessageHeader::RequestQuit, nullptr, nullptr);
	}
	result.waitForFinished();
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
	return p->enqueueCall(MessageHeader::RequestQuit, nullptr, nullptr).first.isValid();
}

std::pair<QFuture<CommandResult>, QFuture<TextNotification>> Client::call(int16_t id,
					const google::protobuf::MessageLite &in,
					google::protobuf::MessageLite &out)
{
	QMutexLocker lock(&p->mutex);
	if (p->socket.state() != QAbstractSocket::ConnectedState) {
		return {makeFailedResult(), {}};
	}
	return p->enqueueCall(id, &in, &out);
}

QFuture<CommandResult> Client::makeFailedResult()
{
	QPromise<CommandResult> p;
	p.addResult(CommandResult::LinkFailure);
	p.finish();
	return p.future();
}

