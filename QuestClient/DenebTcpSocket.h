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

	bool test()const;
	bool send(QByteArray) const;
	QByteArray recv() const;

	
	QString hostname()const;
	int port()const;
signals:
	void connectFinished(int code);
	void connected();
	void disconnected();
	void received();
private:
	QString m_hostname;
	int m_port;
	bool m_connected{ false };
	int m_socket{ 0 };

	QTimer* m_receiveTimer;
};

