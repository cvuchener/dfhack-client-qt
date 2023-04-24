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
#include <QFutureWatcher>

int main(int argc, char *argv[])
{
	QCoreApplication app(argc, argv);
	DFHack::Client client;

	QObject::connect(&client, &DFHack::Client::socketError, [](QAbstractSocket::SocketError, const QString &error) {
		qCritical() << "socket error:" << error;
	});
	QObject::connect(&client, &DFHack::Client::notification, [](DFHack::Color, const QString &text) {
		qInfo() << text;
	});

	QFutureWatcher<void> watcher;
	QObject::connect(&watcher, &QFutureWatcher<void>::finished, &app, &QCoreApplication::quit);

	DFHack::Core::RunCommand run_command(&client);
	DFHack::Core::Suspend suspend(&client);
	DFHack::Core::Resume resume(&client);

	watcher.setFuture(client.connect("localhost", DFHack::Client::DefaultPort).then([&](bool success) {
		if (!success)
			throw std::runtime_error("Failed to connect");
		auto args = run_command.args();
		args.set_command("ls");
		args.clear_arguments();
		return run_command.call(args).first;
	}).unwrap().then([&](DFHack::CallReply<dfproto::EmptyMessage> reply) {
		qInfo() << "command result:" << static_cast<int>(reply.cr);
		return suspend.call().first;
	}).unwrap().then([&](DFHack::CallReply<dfproto::IntMessage> reply) {
		qInfo() << "suspend: " << static_cast<int>(reply.cr);
		return resume.call().first;
	}).unwrap().then([&](DFHack::CallReply<dfproto::IntMessage> reply) {
		qInfo() << "resume: " << static_cast<int>(reply.cr);
		return client.disconnect();
	}).unwrap().onFailed([](std::exception &e) {
		qCritical() << e.what();
	}));
	return app.exec();
}
