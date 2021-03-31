#include "DenebTcpSocket.h"
#include <thread>
#include <QTimer>

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
	, m_receiveTimer{ new QTimer{this} }
{
	static bool globalInitNtWin = true;
	if(globalInitNtWin)
	{
		nt_winsock_init();
		globalInitNtWin = false;
	}

	connect(m_receiveTimer, &QTimer::timeout, [=]
		{
			auto code = net_test_socket(m_socket);
			if (code < 0)
				disconnected();
			else if (code > 0)
				emit received();
		});

	m_receiveTimer->setInterval(50);
}

void DenebTcpSocket::connectToHost(QString hostname, int port)
{
	std::thread{ [=]
	{
		if (m_connected)
			disconnectToHost();
		const auto connectCode = net_init_socket_client(hostname.toLatin1(), port, &m_socket);
		emit connectFinished(connectCode);
		if (!connectCode)
		{
			m_connected = true;
			emit connected();

			m_receiveTimer->start();
		}
	} }.detach();
}

void DenebTcpSocket::disconnectToHost()
{
	std::thread{ [=]
	{
		if (!m_connected)
		{
			m_connected = false;
			net_close_socket(m_socket);
			emit disconnected();

			m_receiveTimer->stop();
		}
	} }.detach();
}

bool DenebTcpSocket::test() const
{
	return net_test_socket(m_socket) > 0;
}

bool DenebTcpSocket::send(QByteArray data) const
{
	return net_writesocket(m_socket, data) == 0;
}

QByteArray DenebTcpSocket::recv() const
{
	char buff[4096] = { 0 };
	net_readsocket(m_socket, buff);
	return buff;
}

QString DenebTcpSocket::hostname() const
{
	return m_hostname;
}

int DenebTcpSocket::port() const
{
	return m_port;
}
