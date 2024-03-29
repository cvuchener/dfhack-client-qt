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

#include "MainWindow.h"

#include <QFontDatabase>

#include <QtDebug>

#include <iomanip>
#include <sstream>


MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	qRegisterMetaType<DFHack::CommandResult>();
	qRegisterMetaType<DFHack::Color>();
	qRegisterMetaType<QAbstractSocket::SocketError>();

	setupUi(this);
	connection_status = new QLabel(this);
	status_bar->addPermanentWidget(connection_status);
	disconnect_action->setEnabled(false);

	command_format.setBackground(Qt::darkBlue);
	result_format.setBackground(Qt::darkGreen);

	char_format.setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

	connect(&client, &DFHack::Client::connectionChanged,
		this, &MainWindow::dfhackConnectionChanged);
	connect(&client, &DFHack::Client::socketError,
		this, &MainWindow::dfhackSocketError);
	connect(&notification_watcher, &decltype(notification_watcher)::started,
		this, &MainWindow::dfhackTextStarted);
	connect(&notification_watcher, &decltype(notification_watcher)::resultsReadyAt,
		this, &MainWindow::dfhackTextMessage);
	connect(&command_watcher, &decltype(command_watcher)::started,
		this, &MainWindow::dfhackCommandStarted);
	connect(&command_watcher, &decltype(command_watcher)::finished,
		this, &MainWindow::dfhackCommandFinished);
}

MainWindow::~MainWindow()
{
}

void MainWindow::on_connect_action_triggered()
{
	connect_action->setEnabled(false);
	connection_status->setText(tr("Connecting"));
	status_bar->clearMessage();
	client.connect("localhost", DFHack::Client::DefaultPort);
}

void MainWindow::on_disconnect_action_triggered()
{
	disconnect_action->setEnabled(false);
	connection_status->setText(tr("Disconnecting"));
	status_bar->clearMessage();
	client.disconnect();
}

static std::optional<dfproto::CoreRunCommandRequest> parse_command(const std::string &line)
{
	std::stringstream ss(line);
	std::string str;
	ss >> std::quoted(str);
	if (!ss || str.empty())
		return std::nullopt;
	qDebug() << "command" << QString::fromStdString(str);

	dfproto::CoreRunCommandRequest request;
	request.clear_arguments();
	if (str[0] == ':') {
		// treat all arguments as a single string
		request.set_command(str.substr(1));
		ss >> std::ws;
		if (std::getline(ss, str)) {
			qDebug() << "arg" << QString::fromStdString(str);
			request.add_arguments(str);
		}
	}
	else {
		// parse quoted arguments
		request.set_command(str);
		while (ss >> std::quoted(str)) {
			qDebug() << "arg" << QString::fromStdString(str);
			request.add_arguments(str);
		}
	}
	return request;
}

void MainWindow::on_send_command_action_triggered()
{
	QTextCursor cursor(console_output->document());
	cursor.movePosition(QTextCursor::End);
	cursor.insertBlock(command_format, char_format);
	cursor.insertText(command_line->text());
	console_output->setTextCursor(cursor);

	status_bar->clearMessage();

	if (auto args = parse_command(command_line->text().toStdString())) {
		auto [res, text] = core.runCommand(client, *args);
		command_watcher.setFuture(res);
		notification_watcher.setFuture(text);
	}
	else
		status_bar->showMessage(tr("Failed to parse command"));
	command_line->clear();
}

void MainWindow::on_suspend_action_triggered()
{
	core.suspend(client);
}

void MainWindow::on_resume_action_triggered()
{
	core.resume(client);
}

void MainWindow::dfhackConnectionChanged(bool connected)
{
	connection_status->setText(connected ? tr("Connected") : QString());
	connect_action->setEnabled(!connected);
	disconnect_action->setEnabled(connected);
	status_bar->clearMessage();
}

void MainWindow::dfhackSocketError(QAbstractSocket::SocketError, const QString &error_string)
{
	connection_status->setText(QString());
	connect_action->setEnabled(true);
	disconnect_action->setEnabled(false);
	status_bar->showMessage(error_string);
}

static QColor get_color_code(const QPalette &palette, DFHack::Color color)
{
	using DFHack::Color;
	switch (color) {
	case Color::Black:
		return palette.color(QPalette::WindowText);
	case Color::Blue:
		return "#000080";
	case Color::Green:
		return "#008000";
	case Color::Cyan:
		return "#008080";
	case Color::Red:
		return "#800000";
	case Color::Magenta:
		return "#800080";
	case Color::Brown:
		return "#808000";
	case Color::Grey:
		return "#c0c0c0";
	case Color::DarkGrey:
		return "#808080";
	case Color::LightBlue:
		return "#0000ff";
	case Color::LightGreen:
		return "#00ff00";
	case Color::LightCyan:
		return "#00ffff";
	case Color::LightRed:
		return "#ff0000";
	case Color::LightMagenta:
		return "#ff00ff";
	case Color::Yellow:
		return "#ffff00";
	case Color::White:
		return palette.color(QPalette::Window);
	default:
		return "";
	}
}

void MainWindow::dfhackTextStarted()
{
	QTextCursor cursor(console_output->document());
	cursor.movePosition(QTextCursor::End);
	cursor.insertBlock(notification_format, char_format);
}

void MainWindow::dfhackTextMessage(int begin, int end)
{
	for (int i = begin; i < end; ++i) {
		auto [color, text] = notification_watcher.future().resultAt(i);
		QTextCursor cursor(console_output->document());
		cursor.movePosition(QTextCursor::End);
		QTextCharFormat format = char_format;
		format.setForeground(get_color_code(console_output->palette(), color));
		cursor.insertText(text, format);
		console_output->setTextCursor(cursor);
	}
}

void MainWindow::dfhackCommandStarted()
{
	status_bar->showMessage("Executing command");
}

void MainWindow::dfhackCommandFinished()
{
	try {
		auto res = command_watcher.future().result();
		status_bar->showMessage("success");
	}
	catch (DFHack::CommandResult cr) {
		status_bar->showMessage(QString("failure: %1")
			.arg(static_cast<int>(cr))
		);
	}
}
