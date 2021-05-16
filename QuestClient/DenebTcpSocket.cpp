#include "DenebTcpSocket.h"
#include <QTimer>
#include <QEventLoop>

extern "C"
{
	int nt_winsock_init();
	int net_init_socket_client(const char*, int, int*);
	int net_writesocket(int, const char*);
	int net_readsocket(int, char*);
	int net_test_socket(int);
	int net_close_socket(int);
}

DenebTcpSocket::DenebTcpSocket(QObject* parent)
	: QObject(parent)
	, m_receiveTimer(new QTimer{ this })
{
	static bool globalInitNtWin = true;
	if (globalInitNtWin)
	{
		nt_winsock_init();
		globalInitNtWin = false;
	}
	m_receiveTimer->setInterval(50);
	connect(m_receiveTimer, &QTimer::timeout, this, [=]
		{
			const auto code = net_test_socket(m_socket);
			if (code < 0)
				disconnectToHost();
			else if (code > 0)
			{
				char buff[4096] = { 0 };
				net_readsocket(m_socket, buff);
				QByteArray recvData{ buff };
				auto task = messageQueue.dequeue();
				if (task.onReceived)
					task.onReceived(recvData);
				emit received(task.data, recvData);
			}
			if(!messageQueue.isEmpty()&&!messageQueue.head().isOnProcess)
			{
				messageQueue.head().isOnProcess = true;
				net_writesocket(m_socket, messageQueue.head().data);
			}
		}, Qt::QueuedConnection);
}

void DenebTcpSocket::connectToHost(QString hostname, int port)
{
	if (m_connected)
		disconnectToHost();
	const auto connectCode = net_init_socket_client(hostname.toLatin1(), port, &m_socket);
	emit connectFinished(connectCode);
	if (connectCode == 0)
	{
		m_connected = true;
		emit connected();

		m_receiveTimer->start(50);
	}
	else
	{
		m_connected = false;
		emit connectFailed(connectCode);
	}
}

void DenebTcpSocket::disconnectToHost()
{
	if (!m_connected)
	{
		m_connected = false;
		emit disconnected(net_close_socket(m_socket));
	}
	m_receiveTimer->stop();
}

bool DenebTcpSocket::isConnected() const
{
	return m_connected;
}

bool DenebTcpSocket::test() const
{
	return net_test_socket(m_socket) > 0;
}

void DenebTcpSocket::write(QByteArray data, std::function<void(QByteArray)> onReceived) const
{
	messageQueue.enqueue({ data,onReceived });
}

QString DenebTcpSocket::hostname() const
{
	return m_hostname;
}

int DenebTcpSocket::port() const
{
	return m_port;
}