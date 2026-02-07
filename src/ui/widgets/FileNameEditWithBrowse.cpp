#include "FileNameEditWithBrowse.h"
#include "ui_FileNameEditWithBrowse.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QMimeData>

FileNameEditWithBrowse::FileNameEditWithBrowse(QWidget *parent) : QWidget(parent), ui(new Ui::FileNameEditWithBrowse)
{
    ui->setupUi(this);

    connect(ui->browseButton, &QPushButton::clicked, this, &FileNameEditWithBrowse::browse);

    setAcceptDrops(true);
}

FileNameEditWithBrowse::~FileNameEditWithBrowse()
{
    delete ui;
}

QString FileNameEditWithBrowse::processFileName(const QString &fileName) const
{
    const bool isSurroundByDoubleQuotes = fileName.startsWith("\"") && fileName.endsWith("\"");
    const bool isSurroundBySingleQuotes = fileName.startsWith("'") && fileName.endsWith("'");
    if (isSurroundByDoubleQuotes || isSurroundBySingleQuotes)
        return fileName.mid(1, fileName.size() - 2);
    return fileName;
}

QString FileNameEditWithBrowse::fileName() const
{
    QString fileName;
    if (!isMultipleMode())
        fileName = ui->fileNameEdit->text();
    else
        fileName = ui->fileNameEdit->text().split(m_multipleModeSeparator, Qt::SkipEmptyParts).value(0);

    fileName = processFileName(fileName);
    return fileName;
}

void FileNameEditWithBrowse::setFileName(const QString &value)
{
    ui->fileNameEdit->setText(value);
}

QStringList FileNameEditWithBrowse::fileNames() const
{
    Q_ASSERT(isMultipleMode());

    return ui->fileNameEdit->text().split(m_multipleModeSeparator, Qt::SkipEmptyParts);
}

void FileNameEditWithBrowse::setFileNames(const QStringList &value)
{
    ui->fileNameEdit->setText(value.join(m_multipleModeSeparator));
}

FileNameEditWithBrowse::Purpose FileNameEditWithBrowse::getPurpose() const
{
    return m_purpose;
}

void FileNameEditWithBrowse::setPurpose(const Purpose &value)
{
    m_purpose = value;
}

void FileNameEditWithBrowse::openFilePathBrowse()
{
    if (!isMultipleMode()) {
        auto fileName = QFileDialog::getOpenFileName(this, m_caption, m_dir, m_filter, m_selectedFilter, m_options);
        if (!fileName.isEmpty())
            ui->fileNameEdit->setText(fileName);
        emit browseTriggered();
    } else {
        auto fileNames = QFileDialog::getOpenFileNames(this, m_caption, m_dir, m_filter, m_selectedFilter, m_options);
        if (!fileNames.isEmpty()) {
            ui->fileNameEdit->setText(fileNames.join(m_multipleModeSeparator));
            emit browseTriggered();
        }
    }
}

void FileNameEditWithBrowse::saveFilePathBrowse()
{
    Q_ASSERT(!isMultipleMode());
    auto fileName = QFileDialog::getSaveFileName(this, m_caption, m_dir, m_filter, m_selectedFilter, m_options);
    if (!fileName.isEmpty()) {
        ui->fileNameEdit->setText(fileName);
        emit browseTriggered();
    }
}

QString *FileNameEditWithBrowse::getSelectedFilter() const
{
    return m_selectedFilter;
}

void FileNameEditWithBrowse::setSelectedFilter(QString *value)
{
    m_selectedFilter = value;
}

void FileNameEditWithBrowse::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->setDropAction(Qt::LinkAction);
        event->accept();
    }
}

void FileNameEditWithBrowse::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        if (!isMultipleMode())
            ui->fileNameEdit->setText(event->mimeData()->urls().value(0).toLocalFile());
        else {
            auto urls = event->mimeData()->urls();
            QStringList fileNames;
            for (const auto &url : urls) {
                fileNames.append(url.toLocalFile());
            }
            ui->fileNameEdit->setText(fileNames.join(m_multipleModeSeparator));
        }

        emit dropTriggered();
        event->setDropAction(Qt::LinkAction);
        event->accept();
    }
}

void FileNameEditWithBrowse::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    };
}

QString FileNameEditWithBrowse::getMultipleModeSeparator() const
{
    return m_multipleModeSeparator;
}

void FileNameEditWithBrowse::setMultipleModeSeparator(const QString &value)
{
    m_multipleModeSeparator = value;
}

bool FileNameEditWithBrowse::isMultipleMode() const
{
    return m_multipleMode;
}

void FileNameEditWithBrowse::setMultipleMode(bool multipleMode)
{
    m_multipleMode = multipleMode;
}

QFileDialog::Options FileNameEditWithBrowse::getOptions() const
{
    return m_options;
}

void FileNameEditWithBrowse::setOptions(const QFileDialog::Options &value)
{
    m_options = value;
}

void FileNameEditWithBrowse::setParameters(const QString &caption, const QString &dir, const QString &filter,
                                           QString *selectedFilter, QFileDialog::Options options)
{
    setCaption(caption);
    setDir(dir);
    setFilter(filter);
    setSelectedFilter(selectedFilter);
    setOptions(options);
}

QString FileNameEditWithBrowse::getFilter() const
{
    return m_filter;
}

void FileNameEditWithBrowse::setFilter(const QString &value)
{
    m_filter = value;
}

QString FileNameEditWithBrowse::getDir() const
{
    return m_dir;
}

void FileNameEditWithBrowse::setDir(const QString &value)
{
    m_dir = value;
}

QString FileNameEditWithBrowse::getCaption() const
{
    return m_caption;
}

void FileNameEditWithBrowse::setCaption(const QString &value)
{
    m_caption = value;
}

void FileNameEditWithBrowse::browse()
{
    switch (m_purpose) {
    case Open:
        openFilePathBrowse();
        break;
    case Save:
        saveFilePathBrowse();
        break;
    }
}

FileNameEditWithOpenBrowse::FileNameEditWithOpenBrowse(QWidget *parent) : FileNameEditWithBrowse(parent)
{
    setPurpose(Open);
}

FileNameEditWithSaveBrowse::FileNameEditWithSaveBrowse(QWidget *parent) : FileNameEditWithBrowse(parent)
{
    setPurpose(Save);
}
