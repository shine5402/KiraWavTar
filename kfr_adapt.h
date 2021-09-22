#ifndef KFR_ADAPT_H
#define KFR_ADAPT_H

#include <kfr/io/file.hpp>
#include <QFile>
namespace kfr {
    template <typename T = void>
    struct qt_file_writer : abstract_writer<T>
    {
        qt_file_writer(QFileDevice* device) : device(device) {}
        ~qt_file_writer() override {}

        using abstract_writer<T>::write;
        size_t write(const T* data, size_t size) final
        {
            return device->write(data, size);
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
        QFileDevice* device;
    };

    template <typename T = void>
    struct qt_file_reader : abstract_reader<T>
    {
        qt_file_reader(QFileDevice* device) : device(device) {}
        ~qt_file_reader() override {}
        size_t read(T* data, size_t size) final {
            return device->read(data, size);
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
        QFileDevice* device;
    };
}



#endif // KFR_ADAPT_H
