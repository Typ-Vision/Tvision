#!/usr/bin/env python3
"""Independent vf.socket/4 peer. Standard library only; never imports the product codec."""
import argparse
import hashlib
import json
import socket
import struct
import time
import uuid
from pathlib import Path

CONTRACT = uuid.UUID('00434634-0002-4000-8000-202610290001').bytes
PROFILE_SCHEMA = 'visionflow.communication-client-profile/1'
PROFILE_KEYS = {'schema', 'label', 'installation', 'peer', 'epoch', 'address',
                'port', 'contractRevision'}
ZERO = bytes(16)
REQUESTS = {1, 2, 3, 4, 5, 6, 10, 11, 12, 20, 21, 22, 23, 24, 25, 30}
RESPONSES = {101, 102, 103, 110, 111, 112, 113, 190, 191}


def load_profile(path):
    """Read the product-exported test profile. Strict; no discovery fallback."""
    raw = path.read_bytes()
    if not raw or len(raw) > 64 * 1024:
        raise ValueError('client profile size')
    document = json.loads(raw.decode('utf-8'))
    if not isinstance(document, dict) or set(document) != PROFILE_KEYS:
        raise ValueError('client profile has unknown or missing fields')
    if document['schema'] != PROFILE_SCHEMA:
        raise ValueError('client profile schema')
    if not isinstance(document['label'], str) or not 1 <= len(document['label'].encode('utf-8')) <= 64:
        raise ValueError('client profile label')
    if not isinstance(document['address'], str) or not document['address'] or len(document['address']) > 255:
        raise ValueError('client profile address')
    port = document['port']
    if isinstance(port, bool) or not isinstance(port, int) or not 1 <= port <= 65535:
        raise ValueError('client profile port')
    for field in ('installation', 'peer', 'epoch', 'contractRevision'):
        text = document[field]
        if not isinstance(text, str):
            raise ValueError('client profile UUID field type')
        value = uuid.UUID(text)
        if value.int == 0 or text != value.hex:
            raise ValueError('client profile UUID')
    if uuid.UUID(document['contractRevision']).bytes != CONTRACT:
        raise ValueError('client profile contract revision')
    return document



def u32(data, offset):
    return struct.unpack_from('>I', data, offset)[0]


def put32(data, offset, value):
    struct.pack_into('>I', data, offset, value)


def identity(value):
    return value if isinstance(value, bytes) else uuid.UUID(value).bytes


def frame(kind, request, peer, epoch, session=ZERO, station=0, recipe=0,
          template=0, activation=ZERO, body=None):
    if kind not in REQUESTS or not 0 < request < 2**64:
        raise ValueError('invalid kind/request uint64; wrap is forbidden')
    body = bytes(384) if body is None else bytes(body)
    if len(body) != 384:
        raise ValueError('request body must be exactly 384 bytes')
    return (struct.pack('>4sIIIIIQ', b'VF04', 4, kind, 512, 0, station, request)
            + identity(session) + identity(peer) + identity(epoch)
            + struct.pack('>II', recipe, template) + identity(activation)
            + bytes(24) + body)


def decode(raw):
    if len(raw) != 1024 or raw[:4] != b'VF04' or u32(raw, 4) != 4:
        raise ValueError('response size/magic/version')
    kind = u32(raw, 8)
    if (u32(raw, 12) != 1024 or u32(raw, 16) or any(raw[104:128])
            or kind not in RESPONSES or not int.from_bytes(raw[24:32], 'big')):
        raise ValueError('response header')
    if kind == 111:
        # W7 includes terminating NUL in the domain separator. Transport session
        # and kind/size are excluded; digest field is canonical zero.
        payload = (b'vf.socket4.terminal/1\0' + raw[20:32] + raw[48:104]
                   + raw[128:160] + bytes(32) + raw[192:])
        if hashlib.sha256(payload).digest() != raw[160:192]:
            raise ValueError('terminal SHA256 mismatch')
    return {'raw': raw, 'kind': kind, 'station': u32(raw, 20),
            'request': int.from_bytes(raw[24:32], 'big'), 'session': raw[32:48],
            'body': raw[128:], 'reason': u32(raw, 204)}


class Peer:
    def __init__(self, host, port, peer, epoch, log=None, timeout=8):
        self.host, self.port = host, port
        self.peer, self.epoch = identity(peer), identity(epoch)
        self.session = ZERO
        self.socket = None
        self.pending = []
        self.log = log
        self.timeout = timeout
        self.sequence = 1

    def event(self, event, **values):
        if self.log:
            self.log.write(json.dumps({'monotonic': time.monotonic(), 'event': event, **values}) + '\n')
            self.log.flush()

    def connect(self):
        self.socket = socket.create_connection((self.host, self.port), self.timeout)
        self.socket.settimeout(self.timeout)
        self.pending.clear()
        self.session = ZERO
        body = bytearray(384)
        body[:16] = CONTRACT
        request = self.make(1, body=body)
        self.send(request)
        reply = self.wait(int.from_bytes(request[24:32], 'big'), {101})
        if reply['reason'] or reply['session'] == ZERO or reply['body'][16:32] != CONTRACT:
            raise ValueError('HELLO contract/authority rejected')
        self.session = reply['session']
        return reply

    def close(self):
        if self.socket:
            self.socket.close()
            self.socket = None

    def make(self, kind, request=None, **kwargs):
        if request is None:
            request = self.sequence
            self.sequence += 1
        return frame(kind, request, self.peer, self.epoch, self.session, **kwargs)

    def send(self, raw, split=None, delay=0):
        self.event('tx', kind=u32(raw, 8), request=str(int.from_bytes(raw[24:32], 'big')),
                   station=u32(raw, 20), sha256=hashlib.sha256(raw).hexdigest(), bytes=len(raw))
        if split is None:
            self.socket.sendall(raw)
        else:
            self.socket.sendall(raw[:split])
            if delay:
                time.sleep(delay)
            self.socket.sendall(raw[split:])

    def receive(self):
        raw = bytearray()
        deadline = time.monotonic() + self.timeout
        while len(raw) < 1024:
            self.socket.settimeout(max(0.001, deadline - time.monotonic()))
            chunk = self.socket.recv(1024 - len(raw))
            if not chunk:
                raise EOFError('peer closed mid-response')
            raw.extend(chunk)
        reply = decode(bytes(raw))
        if raw[48:64] != self.peer or raw[64:80] != self.epoch:
            raise ValueError('foreign peer/epoch response')
        if reply['kind'] != 101 and self.session != ZERO and reply['session'] != self.session:
            raise ValueError('foreign transport session response')
        self.event('rx', kind=reply['kind'], request=str(reply['request']),
                   station=reply['station'], reason=reply['reason'],
                   result=raw[128:144].hex(), digest=raw[160:192].hex())
        return reply

    def wait(self, request, kinds):
        deadline = time.monotonic() + self.timeout
        while time.monotonic() < deadline:
            for index, reply in enumerate(self.pending):
                if reply['request'] == request and reply['kind'] in kinds:
                    return self.pending.pop(index)
            reply = self.receive()
            if reply['request'] == request and reply['kind'] in kinds:
                return reply
            if reply['kind'] in {190, 191} and reply['request'] == request:
                raise ValueError(f"request {request} rejected: {reply['reason']}")
            self.pending.append(reply)
            if len(self.pending) > 256:
                raise ValueError('bounded simulator unmatched-response queue exhausted')
        raise TimeoutError(f'request {request} deadline')

    def ack(self, terminal):
        raw = terminal['raw']
        body = raw[128:144] + raw[160:192] + bytes(336)
        ack = self.make(11, request=terminal['request'], station=terminal['station'],
                        recipe=u32(raw, 80), template=u32(raw, 84), activation=raw[88:104], body=body)
        self.send(ack)
        reply = self.wait(terminal['request'], {112})
        if reply['reason']:
            raise ValueError('exact ACK rejected')
        return reply


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host', help='listener address; profile address is used when omitted')
    parser.add_argument('--port', type=int, help='listener port; profile port is used when omitted')
    parser.add_argument('--profile', type=Path, help='product-exported test client profile JSON')
    parser.add_argument('--peer', help='locally enrolled peer UUID; overrides the profile')
    parser.add_argument('--epoch', help='locally enrolled persistent epoch UUID; overrides the profile')
    parser.add_argument('--plan', type=Path, help='JSON list: kind, station/recipe/template/activation, body_hex, request decimal string, expect_kind, ack')
    parser.add_argument('--log', type=Path, required=True, help='JSONL protocol/failure log')
    args = parser.parse_args()
    profile = load_profile(args.profile) if args.profile else None
    peer_id = args.peer or (profile['peer'] if profile else None)
    epoch_id = args.epoch or (profile['epoch'] if profile else None)
    if not peer_id or not epoch_id:
        parser.error('--peer and --epoch are required unless --profile provides them')
    host = args.host or (profile['address'] if profile else '127.0.0.1')
    port = args.port or (profile['port'] if profile else 5000)
    args.log.parent.mkdir(parents=True, exist_ok=True)
    with args.log.open('w', encoding='utf-8') as log:
        peer = Peer(host, port, peer_id, epoch_id, log)
        try:
            if profile:
                peer.event('profile', label=profile['label'], installation=profile['installation'],
                           schema=profile['schema'])
            peer.connect()
            operations = json.loads(args.plan.read_text(encoding='utf-8')) if args.plan else [{'kind': 2, 'expect_kind': 102}]
            for operation in operations:
                if operation['kind'] not in {2, 3} and 'request' not in operation:
                    raise ValueError('mutating plan operations require an explicit persistent uint64 request ID')
                fields = {k: operation[k] for k in ('station', 'recipe', 'template', 'activation') if k in operation}
                if 'body_hex' in operation:
                    fields['body'] = bytes.fromhex(operation['body_hex'])
                raw = peer.make(operation['kind'], request=int(operation['request']) if 'request' in operation else None, **fields)
                peer.send(raw, split=operation.get('split'), delay=operation.get('delay_ms', 0) / 1000)
                reply = peer.wait(int.from_bytes(raw[24:32], 'big'), {operation.get('expect_kind', 111)})
                if operation.get('ack'):
                    peer.ack(reply)
            peer.event('completed', physical='BLOCKED', exactly_once=False)
        except Exception as error:
            peer.event('failure', error=str(error))
            raise
        finally:
            peer.close()


if __name__ == '__main__':
    main()
