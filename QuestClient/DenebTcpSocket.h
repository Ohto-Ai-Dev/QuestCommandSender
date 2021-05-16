#pragma once
#include <QObject>
#include <QQueue>

class DenebTcpSocket :
	public QObject
{
	Q_OBJECT

public:
	struct SendTask
	{
		QByteArray data;
		std::function<void(QByteArray)> onReceived;
		bool isOnProcess{ false };
	};
	DenebTcpSocket(QObject* parent = nullptr);

	void connectToHost(QString hostname, int port);
	void disconnectToHost();

	bool isConnected()const;
	bool test()const;
	void write(QByteArray, std::function<void(QByteArray)>onReceived = std::function<void(QByteArray)>()) const;

	QString hostname()const;
	int port()const;
	mutable QQueue<SendTask> messageQueue;
signals:
	void connectFinished(int code);
	void connectFailed(int code);
	void connected();
	void disconnected(int code);
	void received(QByteArray, QByteArray);
private:
	QString m_hostname{ "localhost" };
	int m_port{ 0 };
	bool m_connected{ false };
	int m_socket{ 0 };
	

	QTimer* m_receiveTimer;
};
