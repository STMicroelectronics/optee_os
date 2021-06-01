#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (C) 2020, STMicroelectronics - All Rights Reserved
#

try:
    from elftools.elf.elffile import ELFFile
    from elftools.elf.sections import SymbolTableSection
    from elftools.elf.enums import ENUM_P_TYPE_BASE
    from elftools.elf.enums import ENUM_P_TYPE_ARM
    from elftools.elf.enums import *
except ImportError:
    print("""
***
ERROR: pyelftools python module is not installed or version < 0.25.
***
""")
    raise

from Cryptodome.Hash import SHA256
from Cryptodome.Signature import pkcs1_15
from Cryptodome.PublicKey import RSA
from Cryptodome.Signature import DSS
from Cryptodome.PublicKey import ECC
import os
import sys
import struct
import logging
import binascii


logging.basicConfig(stream=sys.stderr, level=logging.INFO)

ENUM_HASH_TYPE = dict(
    SHA256=1,
)

ENUM_SIGNATURE_TYPE = dict(
    RSA=1,
    ECC=2,
)

ENUM_BINARY_TYPE = dict(
    ELF=1,
)


def dump_buffer(buf, step=16, name="", logger=logging.info, indent=""):
    logger("%s%s:" % (indent, name))
    for i in range(0, len(buf), step):
        logger("%s    " % (indent) + " ".
               join(["%02X" % c for c in buf[i:i+step]]))
    logger("\n")


class RSA_Signature(object):

    def __init__(self, key):
        self._hasher = SHA256.new()
        self.signer = pkcs1_15.new(key)

    def hash_compute(self, segment):
        self._hasher.update(segment)

    def sign(self):
        return self.signer.sign(self._hasher)


class ECC_Signature(object):

    def __init__(self, key):
        self._hasher = SHA256.new()
        self.signer = DSS.new(key, 'fips-186-3')

    def hash_compute(self, segment):
        self._hasher.update(segment)

    def sign(self):
        return self.signer.sign(self._hasher)


Signature = {
        1: RSA_Signature,
        2: ECC_Signature,
}


class SegmentHashStruct:
    pass


class SegmentHash(object):
    '''
        Hash table based on Elf program segments
    '''
    def __init__(self, img):
        self._num_segments = img.num_segments()
        self._pack_fmt = '<%dL' % 8
        self.img = img
        self.hashProgTable = bytes()
        self._offset = 0

    def get_table(self):
        '''
            Create a segment hash table containing for each segment:
                - the segments header
                - a hash of the segment
        '''
        h = SHA256.new()
        seg = SegmentHashStruct()
        self.size = (h.digest_size + 32) * self._num_segments
        logging.debug("hash section size %d" % self.size)
        del h
        self.buf = bytearray(self.size)
        self._bufview_ = memoryview(self.buf)

        for i in range(self._num_segments):
            h = SHA256.new()
            segment = self.img.get_segment(i)
            seg.header = self.img.get_segment(i).header
            machine = self.img.get_machine_arch()
            logging.debug("compute hash for segment offset %s" % seg.header)
            h.update(segment.data())
            seg.hash = h.digest()
            logging.debug("hash computed: %s" % seg.hash)
            del h
            if machine == "ARM":
                struct.pack_into('<I', self._bufview_, self._offset,
                                 ENUM_P_TYPE_ARM[seg.header.p_type])
            else:
                struct.pack_into('<I', self._bufview_, self._offset,
                                 ENUM_P_TYPE_BASE[seg.header.p_type])
            self._offset += 4
            struct.pack_into('<7I', self._bufview_, self._offset,
                             seg.header.p_offset, seg.header.p_vaddr,
                             seg.header.p_paddr, seg.header.p_filesz,
                             seg.header.p_memsz, seg.header.p_flags,
                             seg.header.p_align)
            self._offset += 28
            struct.pack_into('<32B', self._bufview_, self._offset, *seg.hash)
            self._offset += 32
        dump_buffer(self.buf, name='hash table', indent="\t")
        return self.buf


class ImageHeader(object):
    '''
        Image header
    '''

    magic = 'HELF'   # SHDR_MAGIC
    version = 1
    sign_type = 1     # SHA256
    img_type = 1     # ELF

    MAGIC_OFFSET = 0
    VERSION_OFFSET = 4
    LENGTH_OFFSET = 8
    SIGNATURE_LEN_OFFSET = 12
    SIGNATURE_OFFSET_OFFSET = 16
    SIGNATURE_TYPE_OFFSET = 20
    HASH_LEN_OFFSET = 24
    HASH_OFFSET_OFFSET = 28
    HASH_TYPE_OFFSET = 32
    PUBLIC_INFO_LEN_OFFSET = 36
    PUBLIC_INFO_OFFSET_OFFSET = 40
    IMG_LEN_OFFSET = 44
    IMG_OFFSET_OFFSET = 48
    IMG_TYPE_OFFSET = 52

    def __init__(self):
        self.size = 56

        self.magic = 0x3543A468
        self.version = 1
        self.length = 0
        self.sign_length = 0
        self.sign_offset = 0
        self.sign_type = 0
        self.hash_length = 0
        self.hash_offset = 0
        self.hash_type = 0
        self.key_length = 0
        self.key_offset = 0
        self.img_length = 0
        self.img_offset = 0
        self.img_type = 0

        self.shdr = struct.pack('<IIIIIIIIIIIIII',
                                self.magic, self.version, self.length,
                                self.sign_length, self.sign_offset,
                                self.sign_type, self.hash_length,
                                self.hash_offset, self.hash_type,
                                self.key_length, self.key_offset,
                                self.img_length, self.img_offset,
                                self.img_type)

    def dump(self):
        logging.info("\tMAGIC\t\t= %08X" % (self.magic))
        logging.info("\tHEADER_VERSION\t= %08X" % (self.version))
        logging.info("\tHEADER_LENGTH\t= %08X" % (self.length))
        logging.info("\tSIGN_LENGTH\t= %08X" % (self.sign_length))
        logging.info("\tSIGN_OFFSET\t= %08X" % (self.sign_offset))
        logging.info("\tSIGN_TYPE\t= %08X" % (self.sign_type))
        logging.info("\tHASH_LENGTH\t= %08X" % (self.hash_length))
        logging.info("\tHASH_OFFSET\t= %08X" % (self.hash_offset))
        logging.info("\tHASH_TYPE\t= %08X" % (self.hash_type))
        logging.info("\tPKEY_LENGTH\t= %08X" % (self.key_length))
        logging.info("\tPKEY_OFFSET\t= %08X" % (self.key_offset))
        logging.info("\tIMAGE_LENGTH\t= %08X" % (self.img_length))
        logging.info("\tIMAGE_OFFSET\t= %08X" % (self.img_offset))
        logging.info("\tIMAGE_TYPE\t= %08X" % (self.img_type))

    def get_packed(self):
        return struct.pack('<IIIIIIIIIIIIII',
                           self.magic, self.version, self.length,
                           self.sign_length, self.sign_offset, self.sign_type,
                           self.hash_length, self.hash_offset, self.hash_type,
                           self.key_length, self.key_offset, self.img_length,
                           self.img_offset, self.img_type)


def get_args(logger):
    from argparse import ArgumentParser, RawDescriptionHelpFormatter
    import textwrap
    command_base = ['sign']
    command_choices = command_base
    default_key = os.path.abspath(os.path.dirname(__file__)) + \
        '/../keys/default_rproc.pem'

    parser = ArgumentParser(
        description='Sign a remote processor firmware loadable by OP-TEE.',
        usage='\n   %(prog)s command [ arguments ]\n\n'

        '   command:\n' +
        '     sign      Generate signed loadable binary \n' +
        '                 Takes arguments --in, --out --key\n' +
        '   %(prog)s --help  show available commands and arguments\n\n',
        formatter_class=RawDescriptionHelpFormatter,
        epilog=textwrap.dedent(
            '''If no key is specified, the script will default try to ''' +
            '''use the following private key:''') + '\n' + default_key
        )
    parser.add_argument(
        'command', choices=command_choices, nargs='?',
        default='sign',
        help='Command, one of [' + ', '.join(command_base) + ']')
    parser.add_argument('--key', required=False,
                        help='Name of signing key file',
                        default=default_key,
                        dest='keyf')
    parser.add_argument('--key_info', required=False,
                        help='Name file containing extra key information',
                        dest='key_infof')
    parser.add_argument('--key_type', required=False,
                        help='Type of signing key: should be RSA or ECC',
                        default='RSA',
                        dest='key_type')
    parser.add_argument(
        '--in', required=True, dest='inf',
        help='Name of firmware input file')
    parser.add_argument(
        '--out', required=False, dest='outf',
        help='Name of the signed firmware output file,' +
             ' default <in base name>.sig')

    parsed = parser.parse_args()

    # Set defaults for optional arguments.

    if parsed.outf is None:
        parsed.outf = str(parsed.inf)+'.sig'

    return parsed


def rsa_key(keyf):
    return RSA.importKey(open(keyf).read())


def ecc_key(keyf):
    return ECC.import_key(open(keyf).read())


key_type = {
        1: rsa_key,
        2: ecc_key,
}


def rsa_sig_size(key):
    return key.size_in_bytes()


def ecc_sig_size(key):
    # to be improve...
    # DSA size is N/4  so 64 for DSA (L,N) = (2048, 256)
    return 64


sig_size_type = {
        1: rsa_sig_size,
        2: ecc_sig_size,
}


def main():
    from Cryptodome.Signature import pss
    from Cryptodome.Hash import SHA256
    from Cryptodome.PublicKey import RSA
    import base64
    import logging
    import struct

    logging.basicConfig()
    logger = logging.getLogger(os.path.basename(__file__))

    args = get_args(logger)

    # Initialise the header */
    s_header = ImageHeader()

    get_key = key_type.get(ENUM_SIGNATURE_TYPE[args.key_type],
                           lambda: "Invalid sign type")
    key = get_key(args.keyf)

    if not key.has_private():
        logger.error('Provided key cannot be used for signing, ' +
                     'please use offline-signing mode.')
        sys.exit(1)

    # Firmware image
    input_file = open(args.inf, 'rb')
    img = ELFFile(input_file)

    # need to reopen the file to get the raw data
    with open(args.inf, 'rb') as f:
        bin_img = f.read()
    img_size = len(bin_img)
    logging.debug("image size %d" % img_size)
    s_header.img_length = img_size
    s_header.img_type = ENUM_BINARY_TYPE['ELF']

    # Hash table chunk
    h = SHA256.new()

    # Compute the hash table
    hash_table = SegmentHash(img)
    hash = hash_table.get_table()

    s_header.hash_offset = s_header.size
    s_header.hash_length = hash_table.size
    s_header.hash_type = ENUM_HASH_TYPE['SHA256']
    # Get padding to align on 64 bytes
    hash_align = s_header.hash_length % 8

    # Key information chunk
    if args.key_infof:
        with open(args.key_infof, 'rb') as f:
            key_info = f.read()
        s_header.key_length = sys.getsizeof(key_info)
        s_header.key_offset = s_header.hash_offset + s_header.hash_length + \
            hash_align
        # Get padding to align on 64 bytes
        key_info_align = s_header.key_length % 8
    else:
        key_info_align = 0

    # Signature chunk
    s_header.sign_type = ENUM_SIGNATURE_TYPE[args.key_type]

    sign_size = sig_size_type.get(ENUM_SIGNATURE_TYPE[args.key_type],
                                  lambda: "Invalid sign type")(key)
    s_header.sign_length = sign_size

    if args.key_infof:
        s_header.sign_offset = s_header.key_offset + s_header.key_length + \
                           key_info_align
    else:
        s_header.sign_offset = s_header.hash_offset + s_header.hash_length + \
                               hash_align

    s_header.img_offset = s_header.sign_offset + sign_size

    s_header.length = s_header.size + s_header.hash_length + hash_align + \
        s_header.key_length + key_info_align + s_header.sign_length

    header = s_header.get_packed()

    # Generate signature
    signer = Signature.get(ENUM_SIGNATURE_TYPE[args.key_type])(key)

    signer.hash_compute(header)
    signer.hash_compute(bytes(hash))
    if args.key_infof:
        signer.hash_compute(key_info)

    signature = signer.sign()
    if len(signature) != sign_size:
        raise Exception(("Actual signature length is not equal to ",
                         "the computed one: {} != {}".
                         format(len(signature), sign_size)))

    s_header.dump()

    with open(args.outf, 'wb') as f:
        f.write(header)
        f.write(hash)
        if hash_align:
            f.write(bytearray(hash_align))
        if args.key_infof:
            if key_info_align:
                f.write(key_info)
                f.write(bytearray(key_info_align))
        f.write(signature)
        f.write(bytearray(sign_size - s_header.sign_length))
        f.write(bin_img)


if __name__ == "__main__":
    main()
