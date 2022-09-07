#!/bin/bash
pushd lib/vip-functional-library/ciphers/aes/tinyCrypt
make
popd
pushd lib/vip-functional-library
make
popd