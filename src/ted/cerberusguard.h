#ifndef CERBERUSGUARD_H
#define CERBERUSGUARD_H
#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include "cerberusapplication.h"

class CerberusGuard : public QObject
{
    Q_OBJECT
public:
    explicit CerberusGuard(QObject *parent = 0);
    ~CerberusGuard();

    void listen(QString name);
    bool hasPrevious(QString name, QStringList arguments);
signals:
    void newInstance();
public slots:
    void newConnection();
    void readyRead();
private:
    QLocalServer m_server;
    QLocalSocket *m_socket;
};

#endif // CERBERUSGUARD_H
