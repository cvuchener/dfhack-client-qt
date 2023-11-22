/*
 * Copyright 2023 Clement Vuchener
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <QCoreApplication>

#include <dfhack-client-qt/Client.h>
#include <dfhack-client-qt/Core.h>

#include <QtDebug>
#include <iostream>

struct ClientThread
{
	DFHack::Client client;
	QThread thread;

	ClientThread() {
		client.moveToThread(&thread);
		thread.start();
	}

	~ClientThread() {
		thread.quit();
		thread.wait();
	}
};

template <typename T>
T sync(QFuture<T> &&future)
{
	future.waitForFinished();
	return future.result();
}

int main(int argc, char *argv[])
{
	QCoreApplication app(argc, argv);

	ClientThread client_thread;
	DFHack::Client &client = client_thread.client;

	QObject::connect(&client, &DFHack::Client::socketError, [](QAbstractSocket::SocketError, const QString &error) {
		qCritical() << "socket error:" << error;
	});

	if (!sync(client.connect("localhost", DFHack::Client::DefaultPort))) {
		qCritical() << "Failed to connect";
		return -1;
	}

	DFHack::Core core;
	{
		auto in = core.runCommand.args();
		in.set_command("ls");
		in.clear_arguments();
		auto [reply, notifications] = core.runCommand(client, in);
		notifications.waitForFinished();
		for (const auto &n: notifications.results())
			std::cout << n.second.toStdString();
		qInfo() << "command result:" << make_error_code(reply.result().cr).message();
	}

	{
		auto [reply, notifications] = core.suspend(client);
		auto r = reply.result();
		qInfo() << "suspend result:" << make_error_code(r.cr).message();
		if (r)
			qInfo() << "suspend value:" << r->value();
	}

	{
		auto [reply, notifications] = core.resume(client);
		auto r = reply.result();
		qInfo() << "resume result:" << make_error_code(r.cr).message();
		if (r)
			qInfo() << "resume value:" << r->value();
	}

	client.disconnect().waitForFinished();
	return 0;
}
