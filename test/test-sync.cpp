#include <QCoreApplication>

#include <dfhack-client-qt/Client.h>
#include <dfhack-client-qt/Core.h>

#include <QtDebug>

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

int main(int argc, char *argv[])
{
	QCoreApplication app(argc, argv);

	ClientThread client_thread;
	DFHack::Client &client = client_thread.client;

	QEventLoop loop;
	QObject::connect(&client, &DFHack::Client::connectionChanged, &loop, &QEventLoop::quit);
	QObject::connect(&client, &DFHack::Client::socketError, &loop, &QEventLoop::quit);
	client.connect("localhost", DFHack::Client::DefaultPort);
	loop.exec();

	{
		DFHack::Core::RunCommand run_command(&client);
		run_command.in.set_command("ls");
		run_command.in.clear_arguments();
		auto [cr, notifications] = run_command.call();
		cr.waitForFinished();
		for (const auto &n: notifications)
			qInfo() << n.second;
		qInfo() << "command result:" << static_cast<int>(cr.result());
	}

	{
		DFHack::Core::Suspend suspend(&client);
		if (!suspend.bind().result()) {
			qCritical() << "Failed to bind suspend";
			return -1;
		}
		auto [cr, notifications] = suspend.call();
		cr.waitForFinished();
		qInfo() << "suspend: " << static_cast<int>(cr.result());
	}
	{
		DFHack::Core::Resume resume(&client);
		if (!resume.bind().result()) {
			qCritical() << "Failed to bind resume";
			return -1;
		}
		auto [cr, notifications] = resume.call();
		cr.waitForFinished();
		qInfo() << "resume: " << static_cast<int>(cr.result());
	}

	//client.disconnect();
	//loop.exec();
	return 0;
}
