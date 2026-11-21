"""Manage the official release key and verify release manifest signatures."""

import argparse
import base64
from getpass import getpass
from hashlib import sha256
import os
from pathlib import Path
import tempfile

PUBLIC_KEY = Path(__file__).with_name('release-key.pub')


def signing_key_path():
    return Path(os.environ['LOCALAPPDATA']) / 'Freak Melee Publisher' / 'release-key.dpapi'


def modulus(key):
    numbers = key.public_key().public_numbers()
    if key.key_size != 3072 or numbers.e != 65537:
        raise ValueError('Expected a 3072-bit RSA release key with exponent 65537')
    return base64.b64encode(numbers.n.to_bytes(384, 'big')).decode('ascii')


def write_new_key_file(path, protected_bytes):
    fd, temp_name = tempfile.mkstemp(dir=path.parent)
    pending = Path(temp_name)
    try:
        with os.fdopen(fd, 'wb') as out:
            out.write(protected_bytes)
            out.flush()
            os.fsync(out.fileno())
        if os.name == 'nt':
            os.rename(pending, path)  # Windows refuses to replace an existing destination.
        else:
            os.link(pending, path)
    finally:
        pending.unlink(missing_ok=True)


def store_private(key, path):
    import win32crypt
    from cryptography.hazmat.primitives import serialization
    der = key.private_bytes(serialization.Encoding.DER, serialization.PrivateFormat.PKCS8,
                            serialization.NoEncryption())
    encrypted = win32crypt.CryptProtectData(der, 'Freak Melee release key', None, None, None, 1)
    path.parent.mkdir(parents=True, exist_ok=True)
    write_new_key_file(path, encrypted)


def load_private(path):
    import win32crypt
    from cryptography.hazmat.primitives import serialization
    _, der = win32crypt.CryptUnprotectData(path.read_bytes(), None, None, None, 1)
    return serialization.load_der_private_key(der, None)


def public_modulus(path):
    value = path.read_text(encoding='ascii').strip()
    raw = base64.b64decode(value, validate=True)
    if len(raw) != 384 or raw[0] < 128 or not raw[-1] & 1:
        raise ValueError('Invalid RSA public modulus')
    return value


def render_launcher(template, public, issued):
    source = template.read_bytes().replace(b'\r\n', b'\n')
    pin = b'@FREAK_MELEE_RELEASE_KEY@'
    timestamp = b'@FREAK_MELEE_RELEASE_TIME@'
    if source.count(pin) != 1 or source.count(timestamp) != 1:
        raise ValueError('Expected one public key and one release timestamp in the launcher template')
    return source.replace(pin, public.encode('ascii')).replace(timestamp, str(issued).encode('ascii'))


def signature_name(manifest):
    return 'manifest-' + sha256(manifest).hexdigest() + '.sig'


def verify(manifest, signature, public):
    from cryptography.hazmat.primitives import hashes
    from cryptography.hazmat.primitives.asymmetric import padding, rsa
    n = int.from_bytes(base64.b64decode(public, validate=True), 'big')
    rsa.RSAPublicNumbers(65537, n).public_key().verify(
        signature, manifest, padding.PKCS1v15(), hashes.SHA256())


def sign(manifest, public):
    from cryptography.hazmat.primitives import hashes
    from cryptography.hazmat.primitives.asymmetric import padding
    key = load_private(signing_key_path())
    if modulus(key) != public:
        raise ValueError('The publishing key does not match the public release key')
    return key.sign(manifest, padding.PKCS1v15(), hashes.SHA256())


def main():
    from cryptography.hazmat.primitives import serialization
    from cryptography.hazmat.primitives.asymmetric import rsa
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='command', required=True)
    commands.add_parser('init', help='Create a new Windows-protected key and print its public modulus')
    commands.add_parser('public', help='Print the public modulus of the Windows-protected key')
    commands.add_parser('backup', help='Save an encrypted backup on offline media').add_argument('file', type=Path)
    commands.add_parser('restore', help='Restore a matching backup on a publishing PC').add_argument('file', type=Path)
    args = parser.parse_args()
    path = signing_key_path()
    if args.command == 'init':
        key = rsa.generate_private_key(65537, 3072)
        store_private(key, path)
        print(modulus(key))
    elif args.command == 'public':
        print(modulus(load_private(path)))
    elif args.command == 'backup':
        key = load_private(path)
        if modulus(key) != public_modulus(PUBLIC_KEY):
            parser.error('The installed key does not match the public release key')
        password = getpass('New offline backup passphrase: ')
        if not password or password != getpass('Repeat passphrase: '):
            parser.error('Enter the same nonempty backup passphrase twice')
        encrypted = key.private_bytes(serialization.Encoding.PEM, serialization.PrivateFormat.PKCS8,
                                      serialization.BestAvailableEncryption(password.encode()))
        if modulus(serialization.load_pem_private_key(encrypted, password.encode())) != modulus(key):
            parser.error('Backup verification failed')
        write_new_key_file(args.file, encrypted)
    elif args.command == 'restore':
        key = serialization.load_pem_private_key(args.file.read_bytes(), getpass('Backup passphrase: ').encode())
        if not isinstance(key, rsa.RSAPrivateKey) or modulus(key) != public_modulus(PUBLIC_KEY):
            parser.error('This backup does not match the public release key')
        store_private(key, path)


if __name__ == '__main__':
    main()
