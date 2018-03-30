#include "myerror.h"

MyError::MyError(QObject *parent) : QObject(parent)
{

}

QString MyError::Message()
{
    return mMessage;
}

void MyError::SetMessage(QString Msg)
{
    mMessage = Msg;
}
