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

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include "ui_MainWindow.h"

#include <QLabel>
#include <QFutureWatcher>

#include <memory>

#include <dfhack-client-qt/Client.h>
#include <dfhack-client-qt/Core.h>

class MainWindow: public QMainWindow, private Ui::MainWindow
{
	Q_OBJECT
public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow() override;

private slots:
	void on_connect_action_triggered();
	void on_disconnect_action_triggered();
	void on_send_command_action_triggered();
	void on_suspend_action_triggered();
	void on_resume_action_triggered();

	void dfhackConnectionChanged(bool connected);
	void dfhackSocketError(QAbstractSocket::SocketError error, const QString &error_string);
	void dfhackTextStarted();
	void dfhackTextMessage(int begin, int end);
	void dfhackCommandStarted();
	void dfhackCommandFinished();

private:
	QLabel *connection_status;

	DFHack::Client client;
	DFHack::Core::RunCommand run_command;
	DFHack::Core::Suspend suspend;
	DFHack::Core::Resume resume;

	QFutureWatcher<DFHack::CallReply<dfproto::EmptyMessage>> command_watcher;
	QFutureWatcher<std::pair<DFHack::Color, QString>> notification_watcher;

	QTextBlockFormat command_format, notification_format, result_format;
	QTextCharFormat char_format;
};

#endif
