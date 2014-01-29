#include <xdrmisc.h>
#include <xdr_suio.h>
#include <snappy.h>
#include <snappy-sinksource.h>

size_t snappy_compress(suio *from, suio *to);

typedef bool_t (&xdr_arg_cb_t)(XDR *, void *);

template <xdr_arg_cb_t Underlying>
bool_t snappy_xdr_arg(XDR *xdrs, void *objp) {
    if (xdrs->x_op == XDR_ENCODE) {
        xdrsuio x_uncomp{XDR_ENCODE};

        if (!Underlying(x_uncomp.xdrp(), objp)) { return 0; }

        uint32_t &sizep = *(uint32_t *)xdrsuio_inline(xdrs, 4);

        uint32_t size = snappy_compress(x_uncomp.uio(), xsuio(xdrs));
        sizep = htonl(size);
        if (size % 4) { xsuio(xdrs)->fill('\0', 4 - size % 4); }
        return 1;

    } else if (xdrs->x_op == XDR_DECODE) {
        uint32_t compressed_length;

        if (!xdr_getint(xdrs, compressed_length)) { return 0; }

        // This is the only way to get bytes out of an XDR stream, right?
        mstr compressed(compressed_length);
        xdr_getpadbytes(xdrs, compressed, compressed_length);

        size_t uncompressed_length;
        if (!snappy::GetUncompressedLength(
            compressed, compressed_length, &uncompressed_length
        )) { return 0; }

        mstr uncompressed(uncompressed_length);

        if (!snappy::RawUncompress(
            compressed, compressed_length, uncompressed
        )) { return 0; }

        xdrmem x_uncompressed(uncompressed, uncompressed_length);

        if (!Underlying(x_uncompressed.xdrp(), objp)) { return 0; }
        return 1;
    }
    return Underlying(xdrs, objp);
}
