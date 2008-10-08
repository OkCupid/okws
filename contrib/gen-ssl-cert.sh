#!/bin/sh


OPENSSL=/usr/bin/openssl

if [ $# -ne 1 ] ; then
    echo "usage: $0 <stem>"
    exit 2
fi

stem=$1

$OPENSSL genrsa 1024 > $stem.key
$OPENSSL req -new -x509 -nodes -sha1 -days 365 -key $stem.key -out $stem.cert

cat $stem.cert $stem.key > $stem.pem

echo "Output is $stem.pem"



