#pragma once
#include <QObject>

class DenebTcpSocket :
	public QObject
{
	Q_OBJECT

public:
	DenebTcpSocket(QObject* parent = nullptr);

	void connectToHost(QString hostname, int port);
	void disconnectToHost();

	bool isConnected()const;
	bool test()const;
	bool write(QByteArray) const;
	QByteArray read() const;

	bool waitReceived(int msec = 3000) const;

	QString hostname()const;
	int port()const;
signals:
	void connectFinished(int code);
	void connectFailed(int code);
	void connected();
	void disconnected(int code);
	void received();
private:
	QString m_hostname{ "localhost" };
	int m_port{ 0 };
	bool m_connected{ false };
	int m_socket{ 0 };
	mutable bool hasMessage{ false };

	QTimer* m_receiveTimer;
};
