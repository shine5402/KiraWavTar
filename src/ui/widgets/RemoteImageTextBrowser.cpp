#include "RemoteImageTextBrowser.h"

#include <QAbstractTextDocumentLayout>
#include <QDebug>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QScrollBar>

RemoteImageTextBrowser::RemoteImageTextBrowser(QWidget *parent) : QTextBrowser(parent)
{
}

void RemoteImageTextBrowser::setAllowRemoteImageLoad(bool allow)
{
    qDebug() << "RemoteImageTextBrowser::setAllowRemoteImageLoad" << allow;
    m_allowRemote = allow;
    if (allow && !m_nam) {
        m_nam = new QNetworkAccessManager(this);
    }
}

QVariant RemoteImageTextBrowser::loadResource(int type, const QUrl &name)
{
    if (type == QTextDocument::ImageResource && m_allowRemote && m_nam) {
        if (name.scheme().startsWith("http")) { // http or https
            // 1. Check our internal cache
            if (m_resourceCache.contains(name)) {
                return m_resourceCache.value(name);
            }

            // 2. Avoid duplicate downloads
            if (!m_loadingUrls.contains(name)) {
                qDebug() << "RemoteImageTextBrowser::loadResource: Starting download for" << name;
                m_loadingUrls.insert(name);
                QNetworkRequest request(name);
                request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);

                QNetworkReply *reply = m_nam->get(request);
                connect(reply, &QNetworkReply::finished, this, [this, reply, name, type]() {
                    if (reply->error() == QNetworkReply::NoError) {
                        QByteArray data = reply->readAll();
                        QImage image;
                        if (image.loadFromData(data)) {
                            qDebug() << "RemoteImageTextBrowser::loadResource: Image loaded successfully";
                            // Store in cache
                            m_resourceCache.insert(name, image);

                            // Trigger layout update so the view calls loadResource again
                            emit document()->documentLayout()->update();
                        } else {
                            qDebug() << "RemoteImageTextBrowser::loadResource: Failed to load image from data";
                        }
                    } else {
                        qDebug() << "RemoteImageTextBrowser::loadResource: Network error" << reply->errorString();
                    }
                    m_loadingUrls.remove(name);
                    reply->deleteLater();
                });
            }
            // Return null variant while loading
            return QVariant();
        }
    }
    return QTextBrowser::loadResource(type, name);
}

void RemoteImageTextBrowser::onImageDownloaded()
{
    // Not used, using lambda
}
