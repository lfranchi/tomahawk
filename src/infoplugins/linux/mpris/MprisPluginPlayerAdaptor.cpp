/*
 * This file was generated by qdbusxml2cpp version 0.7
 * Command line was: qdbusxml2cpp -a mprispluginplayeradaptor -c MprisPluginPlayerAdaptor MprisPluginPlayerAdaptor.xml
 *
 * qdbusxml2cpp is Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#include "MprisPluginPlayerAdaptor.h"
#include <QMetaObject>
#include <QByteArray>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>

/*
 * Implementation of adaptor class MprisPluginPlayerAdaptor
 */

MprisPluginPlayerAdaptor::MprisPluginPlayerAdaptor(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    // constructor
    setAutoRelaySignals(true);
}

MprisPluginPlayerAdaptor::~MprisPluginPlayerAdaptor()
{
    // destructor
}

bool MprisPluginPlayerAdaptor::canControl() const
{
    // get the value of property CanControl
    return qvariant_cast< bool >(parent()->property("CanControl"));
}

bool MprisPluginPlayerAdaptor::canGoNext() const
{
    // get the value of property CanGoNext
    return qvariant_cast< bool >(parent()->property("CanGoNext"));
}

bool MprisPluginPlayerAdaptor::canGoPrevious() const
{
    // get the value of property CanGoPrevious
    return qvariant_cast< bool >(parent()->property("CanGoPrevious"));
}

bool MprisPluginPlayerAdaptor::canPause() const
{
    // get the value of property CanPause
    return qvariant_cast< bool >(parent()->property("CanPause"));
}

bool MprisPluginPlayerAdaptor::canPlay() const
{
    // get the value of property CanPlay
    return qvariant_cast< bool >(parent()->property("CanPlay"));
}

bool MprisPluginPlayerAdaptor::canSeek() const
{
    // get the value of property CanSeek
    return qvariant_cast< bool >(parent()->property("CanSeek"));
}

QString MprisPluginPlayerAdaptor::loopStatus() const
{
    // get the value of property LoopStatus
    return qvariant_cast< QString >(parent()->property("LoopStatus"));
}

void MprisPluginPlayerAdaptor::setLoopStatus(const QString &value)
{
    // set the value of property LoopStatus
    parent()->setProperty("LoopStatus", qVariantFromValue(value));
}

double MprisPluginPlayerAdaptor::maximumRate() const
{
    // get the value of property MaximumRate
    return qvariant_cast< double >(parent()->property("MaximumRate"));
}

QVariantMap MprisPluginPlayerAdaptor::metadata() const
{
    // get the value of property Metadata
    return qvariant_cast< QVariantMap >(parent()->property("Metadata"));
}

double MprisPluginPlayerAdaptor::minimumRate() const
{
    // get the value of property MinimumRate
    return qvariant_cast< double >(parent()->property("MinimumRate"));
}

QString MprisPluginPlayerAdaptor::playbackStatus() const
{
    // get the value of property PlaybackStatus
    return qvariant_cast< QString >(parent()->property("PlaybackStatus"));
}

qlonglong MprisPluginPlayerAdaptor::position() const
{
    // get the value of property Position
    return qvariant_cast< qlonglong >(parent()->property("Position"));
}

double MprisPluginPlayerAdaptor::rate() const
{
    // get the value of property Rate
    return qvariant_cast< double >(parent()->property("Rate"));
}

void MprisPluginPlayerAdaptor::setRate(double value)
{
    // set the value of property Rate
    parent()->setProperty("Rate", qVariantFromValue(value));
}

bool MprisPluginPlayerAdaptor::shuffle() const
{
    // get the value of property Shuffle
    return qvariant_cast< bool >(parent()->property("Shuffle"));
}

void MprisPluginPlayerAdaptor::setShuffle(bool value)
{
    // set the value of property Shuffle
    parent()->setProperty("Shuffle", qVariantFromValue(value));
}

double MprisPluginPlayerAdaptor::volume() const
{
    // get the value of property Volume
    return qvariant_cast< double >(parent()->property("Volume"));
}

void MprisPluginPlayerAdaptor::setVolume(double value)
{
    // set the value of property Volume
    parent()->setProperty("Volume", qVariantFromValue(value));
}

void MprisPluginPlayerAdaptor::Next()
{
    // handle method call org.mpris.MediaPlayer2.Player.Next
    QMetaObject::invokeMethod(parent(), "Next");
}

void MprisPluginPlayerAdaptor::OpenUri(const QString &Uri)
{
    // handle method call org.mpris.MediaPlayer2.Player.OpenUri
    QMetaObject::invokeMethod(parent(), "OpenUri", Q_ARG(QString, Uri));
}

void MprisPluginPlayerAdaptor::Pause()
{
    // handle method call org.mpris.MediaPlayer2.Player.Pause
    QMetaObject::invokeMethod(parent(), "Pause");
}

void MprisPluginPlayerAdaptor::Play()
{
    // handle method call org.mpris.MediaPlayer2.Player.Play
    QMetaObject::invokeMethod(parent(), "Play");
}

void MprisPluginPlayerAdaptor::PlayPause()
{
    // handle method call org.mpris.MediaPlayer2.Player.PlayPause
    QMetaObject::invokeMethod(parent(), "PlayPause");
}

void MprisPluginPlayerAdaptor::Previous()
{
    // handle method call org.mpris.MediaPlayer2.Player.Previous
    QMetaObject::invokeMethod(parent(), "Previous");
}

void MprisPluginPlayerAdaptor::Seek(qlonglong Offset)
{
    qDebug() << Q_FUNC_INFO;
    // handle method call org.mpris.MediaPlayer2.Player.Seek
    QMetaObject::invokeMethod(parent(), "Seek", Q_ARG(qlonglong, Offset));
}

void MprisPluginPlayerAdaptor::SetPosition(const QDBusObjectPath &TrackId, qlonglong Position)
{
    qDebug() << Q_FUNC_INFO;
    // handle method call org.mpris.MediaPlayer2.Player.SetPosition
    QMetaObject::invokeMethod(parent(), "SetPosition", Q_ARG(QDBusObjectPath, TrackId), Q_ARG(qlonglong, Position));
}

void MprisPluginPlayerAdaptor::Stop()
{
    // handle method call org.mpris.MediaPlayer2.Player.Stop
    QMetaObject::invokeMethod(parent(), "Stop");
}

