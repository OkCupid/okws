#include "suio++.h"
#include "xdrsnappy.h"

struct suioSink : public snappy::Sink {

    suio &uio;
    size_t written;

    suioSink(suio *_uio) : uio(*_uio), written{0} {}
    virtual void Append(const char *bytes, size_t n) override {
        uio.copy(bytes, n);
        written += n;
    }
};

struct suioSource : public snappy::Source {

    const iovec *iov;
    int left;
    size_t offset;

    suioSource(suio *_uio) : iov(_uio->iov()), left(_uio->iovcnt()), offset(0) {}
    virtual size_t Available() const override {
        return iovsize(iov, left) - offset;
    }
    virtual const char *Peek(size_t *len) override {
        *len = iov->iov_len - offset;
        return static_cast<char *>(iov->iov_base) + offset;
    }
    virtual void Skip(size_t n) override {
        offset += n;
        while (offset >= iov->iov_len) {
            offset -= iov->iov_len;
            iov++;
            left--;
        }
    }
};

size_t snappy_compress(suio *from, suio *to) {
    suioSource source{from};
    suioSink sink{to};
    snappy::Compress(&source, &sink);
    return sink.written;
}
