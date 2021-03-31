#include "QuestClient.h"
#include <QtWidgets/QApplication>

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
	QuestClient w;
	w.show();
	return a.exec();
}