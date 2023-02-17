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

	QFutureWatcher<void> watcher;
	QObject::connect(&watcher, &QFutureWatcher<void>::finished, &app, &QCoreApplication::quit);

	DFHack::Core::RunCommand run_command(&client);
	DFHack::Core::Suspend suspend(&client);
	DFHack::Core::Resume resume(&client);

	run_command.in.set_command("ls");
	run_command.in.clear_arguments();
	watcher.setFuture(client.connect("localhost", DFHack::Client::DefaultPort).then([&](bool success) {
		if (!success)
			throw std::runtime_error("Failed to connect");
		return run_command.call().first;
	}).unwrap().then([&](DFHack::CommandResult cr) {
		qInfo() << "command result:" << static_cast<int>(cr);
		return suspend.bind();
	}).unwrap().then([&](bool success) {
		if (!success)
			throw std::runtime_error("Failed to bind suspend");
		return suspend.call().first;
	}).unwrap().then([&](DFHack::CommandResult cr) {
		qInfo() << "suspend: " << static_cast<int>(cr);
		return resume.bind();
	}).unwrap().then([&](bool success) {
		if (!success)
			throw std::runtime_error("Failed to bind resume");
		return resume.call().first;
	}).unwrap().then([&](DFHack::CommandResult cr) {
		qInfo() << "resume: " << static_cast<int>(cr);
		return client.disconnect();
	}).unwrap().onFailed([](std::exception &e) {
		qCritical() << e.what();
	}));
	return app.exec();
}
