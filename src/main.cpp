#include <QApplication>
#include <QWidget>
#include <QKeyEvent>
#include <QListWidget>
#include <QProcess>
#include <QString>
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QListWidgetItem>
#include <QLocalSocket>
#include <QLocalServer>
#include <QDataStream>

#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>

bool verbose = false;

struct Client {
	int focusHistoryID;
	std::string WorkspaceName;
	std::string ClassName;
	std::string title;
	std::string address;

	Client(int id, std::string WorkspaceName, std::string ClassName, std::string title, std::string address)
		: focusHistoryID(id), WorkspaceName(std::move(WorkspaceName)), ClassName(std::move(ClassName)), title(std::move(title)), address(std::move(address)) {}
};

std::vector<Client> clients;

// Execute a command and return its output as a QString
QString execCommand(const QString& cmd) {
	if (verbose) std::cout << "Executing command: " << cmd.toStdString() << std::endl;
	QProcess process;
	process.start("bash", QStringList() << "-c" << cmd);
	process.waitForFinished();
	QString output = process.readAllStandardOutput();
	if (verbose) std::cout << "-- " << output.toStdString() << std::endl;
	return output;
}

// Parse the JSON output into a vector of Client objects
std::vector<Client> parseClients(const QString& jsonString) {
	QJsonDocument document = QJsonDocument::fromJson(jsonString.toUtf8());
	QJsonArray jsonArray = document.array();
	std::vector<Client> pClients;

	for (const auto& item : jsonArray) {
		QJsonObject obj = item.toObject();
		int focusHistoryID = obj.value("focusHistoryID").toInt(-1);
		QString WorkspaceName = obj.value("workspace").toObject().value("name").toString();
		QString ClassName = obj.value("class").toString();
		QString title = obj.value("title").toString();
		QString address = obj.value("address").toString();

		// Filtering out items with negative focusHistoryID
		// note: could filter special workplaces too.. mh.. nope
		if (0 <= focusHistoryID) {
			if (verbose) {
				std::cout << "adding " << address.toStdString() << std::endl;
				std::cout << "  workspace.name: " << WorkspaceName.toStdString() << std::endl;
				std::cout << "  class: " << ClassName.toStdString() << std::endl;
				std::cout << "  title: " << title.toStdString() << std::endl;
				std::cout << "  focusHistoryID: " << focusHistoryID << std::endl;
			}
			pClients.emplace_back(focusHistoryID, WorkspaceName.toStdString(), ClassName.toStdString(), title.toStdString(), address.toStdString());
		}
	}

	// Sorting the pClients by focusHistoryID
	std::sort(pClients.begin(), pClients.end(), [](const Client& a, const Client& b) {
		return a.focusHistoryID < b.focusHistoryID;
	});

	return pClients;
}

class ClientWindow : public QWidget {
	Q_OBJECT

public:
	explicit ClientWindow(QWidget* parent = nullptr) : QWidget(parent) {
		auto* layout = new QVBoxLayout(this);
		listWidget = new QListWidget(this);
		layout->addWidget(listWidget);
		setLayout(layout);

		setStyleSheet("color: rgba(180, 255, 230, 0.8); background-color: rgba(0,90,160,0.5);");

		qApp->installEventFilter(this);
		this->setWindowTitle("HyprTask");

		QString clientsJson = execCommand("hyprctl clients -j");
		clients = parseClients(clientsJson);

		for (int i = 0; i < clients.size(); ++i) {
			const auto& client = clients[i];
			QString label = QString::fromStdString(std::to_string(client.focusHistoryID) + ": " + client.ClassName + " - " + client.title).trimmed();
			if (label.isEmpty()) label = QString::fromStdString(client.address);
			auto* item = new QListWidgetItem(label);
			// item->setData(Qt::UserRole, QString::fromStdString(client.address));
			// wanna use clients.at(key) later, so changed to this...
			item->setData(Qt::UserRole, i);
			listWidget->addItem(item);
		}

		connect(listWidget, &QListWidget::itemActivated, this, &ClientWindow::onItemActivated);
	}

	void handleCommand(const QString& command) {
		if (command == "next") {
			Cycle(1);
		} else if (command == "back") {
			Cycle(-1);
		}
	}

public:
	bool Cycle(int Step) {
		int currentIndex = std::max(listWidget->currentRow(), 0);
		int itemCount = listWidget->count();
		if (verbose) std::cout << "cycle next from " << currentIndex << " of " << itemCount << std::endl;
		currentIndex += Step;
		if (itemCount <= currentIndex) currentIndex = 0;
		else if (currentIndex < 0) currentIndex = itemCount - 1;
		if (verbose) std::cout << "setCurrentRow: " << currentIndex << std::endl;
		listWidget->setCurrentItem(listWidget->item(currentIndex));
		//execCommand("hyprctl dispatch focuswindow address:" + listWidget->item(currentIndex)->data(Qt::UserRole).toString());
		return true;
	}

protected:
	bool eventFilter(QObject *obj, QEvent *event) override {
		
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
		
		int currentIndex = listWidget->currentRow();
		int itemCount = listWidget->count();
		
		if (verbose) std::cout << "eventFilter type: " << event->type() << ", of key: " << keyEvent->key() << std::endl;
		
		if (event->type() == QEvent::KeyRelease) {
			
			if (keyEvent->key() == Qt::Key_Alt) {
				if (verbose) std::cout << "Alt released on " << currentIndex << std::endl;
				onItemActivated(listWidget->item(currentIndex));
			}
			
		} else if (event->type() == QEvent::KeyPress) {

			if (verbose) std::cout << "currentIndex " << currentIndex << ", itemCount " << itemCount << std::endl;

			switch (keyEvent->key()) {
				case Qt::Key_Escape: currentIndex = 0;
				case Qt::Key_Alt:
				case Qt::Key_Space:
					if (verbose) std::cout << "Escape from " << currentIndex << "!" << std::endl;
					onItemActivated(listWidget->item(currentIndex));
					break;
				case Qt::Key_Tab: return Cycle(1); break; // Tab: cycle forwards
				case Qt::Key_Backtab: return Cycle(-1); break; // Shift+Tab: cycle backwards
				case Qt::Key_Down: return Cycle(1); break; // Tab: cycle forwards
				case Qt::Key_Up: return Cycle(-1); break; // Tab: cycle forwards
				default:
					if (verbose) std::cout << "default: nothing" << std::endl;
					return QObject::eventFilter(obj, event);
					break;
			}
		}

		return QObject::eventFilter(obj, event);
	}

private slots:
	void onItemActivated(QListWidgetItem* item) {
		//QString address = item->data(Qt::UserRole).toString();
		int key = item->data(Qt::UserRole).toInt();
		if (verbose) std::cout << "onItemActivated key " << key << std::endl;
		auto Client = clients.at(key);
		QString address = QString::fromStdString(Client.address);
		
		if (verbose) std::cout << "Switching to " << Client.address << " on workspace " << Client.WorkspaceName << std::endl;
		
		// Always focus then workspace then to top. Any other order seems to fuck things up.
		execCommand("hyprctl dispatch focuswindow address:" + address);
		execCommand("hyprctl dispatch workspace " + QString::fromStdString(Client.WorkspaceName));
		/*if (Client.WorkspaceName == "special:magic") {
			execCommand("hyprctl dispatch workspace " + QString::fromStdString(Client.WorkspaceName));
		}*/
		execCommand("hyprctl dispatch bringactivetotop");

		QApplication::quit();
	}

private:
	QListWidget* listWidget;
};

int main(int argc, char* argv[]) {
	QApplication app(argc, argv);

	bool Master = true;

	QDataStream Slave;

	int CycleCue = 0;

	if (verbose) std::cout << "Application name: HyprTask" << std::endl;
	QCoreApplication::setApplicationName("HyprTask");

	// Singleton pattern implementation
	QLocalSocket socket;
	socket.connectToServer("HyprTaskApplicationID");
	if (socket.waitForConnected(1000)) {
		Master = false;
		if (verbose) std::cout << "Socket connected" << std::endl;
		// Send commands to already running instance
		Slave.setDevice(&socket);
	}

	// Parse command-line arguments for verbose flag
	for (int i = 1; i < argc; ++i) {
		if (std::string(argv[i]) == "verbose") {
			verbose = true;
			std::cout << "Verbose mode enabled." << std::endl;
		}
		else if (Master) {
			if (std::string(argv[i]) == "next") CycleCue = 1;
			else if (std::string(argv[i]) == "back") CycleCue = -1;
			else std::cout << "Invalid argument: " << std::string(argv[i]) << std::endl;
		} else {
			if (verbose) std::cout << "Sending argv: " << argv[i] << std::endl;
			Slave << QString::fromLatin1(argv[i]);
		}
	}

	if (!Master) {
		socket.flush(); // Ensure data is written to the socket
		socket.disconnectFromServer();
		// Slave no longer useful. Kill in favor of first singleton.
		return 0;
	}

	// Launch server on first singleton instance
	QLocalServer::removeServer("HyprTaskApplicationID"); // Cleanup any previous instance listener
	QLocalServer localServer; // Prepare local server for listening to new connections
	if (!localServer.listen("HyprTaskApplicationID")) {
		qCritical() << "Cannot start local server for singleton application.";
		return -1;
	}
	if (verbose) std::cout << "Server launched" << std::endl;

	ClientWindow window;
	window.show();

	// Handle incoming connections
	QObject::connect(&localServer, &QLocalServer::newConnection, [&]() {
		QLocalSocket* clientSocket = localServer.nextPendingConnection();
		QObject::connect(clientSocket, &QLocalSocket::readyRead, [&]() {
			QDataStream in(clientSocket);
			QString command;
			while (!in.atEnd()) {
				in >> command;
				if (verbose) std::cout << "Received command: " << command.toStdString() << std::endl;
				window.handleCommand(command);
			}
		});
	});

	if (CycleCue) window.Cycle(CycleCue);

	return app.exec();
}

#include "main.moc"
