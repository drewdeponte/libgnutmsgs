#!/usr/bin/env python

import base64
import binascii
hex_sha1 = raw_input()
raw_sha1 = binascii.unhexlify(hex_sha1)
encoded = base64.b32encode(raw_sha1)
print "Encoded sha1:", binascii.hexlify(encoded)
print "sha1 hash len:",len(encoded)
