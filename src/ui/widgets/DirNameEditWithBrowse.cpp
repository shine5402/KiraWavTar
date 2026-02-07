#include "DirNameEditWithBrowse.h"

#include <QDragEnterEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMimeData>
#include <QPushButton>

DirNameEditWithBrowse::DirNameEditWithBrowse(QWidget *parent) : QWidget(parent)
{
    setAcceptDrops(true);

    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_dirNameEdit = new QLineEdit(this);
    layout->addWidget(m_dirNameEdit);

    m_browseButton = new QPushButton(tr("Browse"), this);
    layout->addWidget(m_browseButton);

    connect(m_browseButton, &QPushButton::clicked, this, &DirNameEditWithBrowse::onBrowse);
}

QString DirNameEditWithBrowse::dirName() const
{
    return m_dirNameEdit->text();
}

void DirNameEditWithBrowse::setDirName(const QString &value)
{
    m_dirNameEdit->setText(value);
}

QString DirNameEditWithBrowse::getCaption() const
{
    return m_caption;
}

void DirNameEditWithBrowse::setCaption(const QString &value)
{
    m_caption = value;
}

QFileDialog::Options DirNameEditWithBrowse::getOptions() const
{
    return m_options;
}

void DirNameEditWithBrowse::setOptions(const QFileDialog::Options &value)
{
    m_options = value;
}

void DirNameEditWithBrowse::onBrowse()
{
    auto result = QFileDialog::getExistingDirectory(this, m_caption, dirName(), m_options);
    if (!result.isEmpty()) {
        m_dirNameEdit->setText(result);
        emit browseTriggered();
    }
}

void DirNameEditWithBrowse::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->setDropAction(Qt::LinkAction);
        event->accept();
    }
}

void DirNameEditWithBrowse::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        QFileInfo fileInfo{event->mimeData()->urls().value(0).toLocalFile()};
        QString result;
        if (fileInfo.isDir())
            result = fileInfo.absoluteFilePath();
        else
            result = fileInfo.absoluteDir().absolutePath();
        m_dirNameEdit->setText(result);
        emit dropTriggered();
        event->setDropAction(Qt::LinkAction);
        event->accept();
    }
}

void DirNameEditWithBrowse::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        m_browseButton->setText(tr("Browse"));
    }
}
