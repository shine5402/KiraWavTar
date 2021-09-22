#ifndef KFR_ADAPT_H
#define KFR_ADAPT_H

#include <kfr/io/file.hpp>
#include <QFile>
#include <memory>
namespace kfr {
    template <typename T = void>
    struct qt_file_writer : abstract_writer<T>
    {
        qt_file_writer(std::unique_ptr<QFile>&& device) : device(std::move(device)) {}
        ~qt_file_writer() override {}

        using abstract_writer<T>::write;
        size_t write(const T* data, size_t size) final
        {
            return device->write(static_cast<const char*>(data), size);
        }
        imax tell() const final {
            return device->pos();
        }
        bool seek(imax offset, seek_origin origin) final
        {
            qint64 pos;
            switch (origin) {
            case seek_origin::begin: pos = offset; break;
            case seek_origin::current: pos = device->pos() + offset; break;
            case seek_origin::end: pos = device->size() + offset; break;
            }
            return device->seek(pos);
        }
        std::unique_ptr<QFile> device;
    };

    template <typename T = void>
    struct qt_file_reader : abstract_reader<T>
    {
        qt_file_reader(std::unique_ptr<QFile>&& device) : device(std::move(device)) {}
        ~qt_file_reader() override {}
        size_t read(T* data, size_t size) final {
            return device->read(static_cast<char*>(data), size);
        }

        using abstract_reader<T>::read;

        imax tell() const final { return device->pos(); }
        bool seek(imax offset, seek_origin origin) final
        {
            qint64 pos;
            switch (origin) {
            case seek_origin::begin: pos = offset; break;
            case seek_origin::current: pos = device->pos() + offset; break;
            case seek_origin::end: pos = device->size() + offset; break;
            }
            return device->seek(pos);
        }
        std::unique_ptr<QFile> device;
    };

    template <typename T = void>
    inline std::shared_ptr<qt_file_reader<T>> open_qt_file_for_reading(const QString& fileName)
    {
        auto device = std::make_unique<QFile>(fileName);
        device->open(QFile::ReadOnly);
        return std::make_shared<qt_file_reader<T>>(std::move(device));
    }

    template <typename T = void>
    inline std::shared_ptr<qt_file_writer<T>> open_qt_file_for_writing(const QString& fileName)
    {
        auto device = std::make_unique<QFile>(fileName);
        device->open(QFile::WriteOnly);
        return std::make_shared<qt_file_writer<T>>(std::move(device));
    }

    template <typename T = void>
    inline std::shared_ptr<qt_file_writer<T>> open_qt_file_for_appending(const QString& fileName)
    {
        auto device = std::make_unique<QFile>(fileName);
        device->open(QIODevice::WriteOnly | QFile::Append);
        return std::make_shared<qt_file_writer<T>>(std::move(device));
    }
}



#endif // KFR_ADAPT_H
