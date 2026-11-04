#!/usr/bin/env python3
"""Independent vf.socket/4 peer. Standard library only; never imports the product codec."""
import argparse
import hashlib
import json
import os
import socket
import struct
import sys
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
            'body': raw[128:], 'reason': u32(raw, 916 if kind == 103 else 204)}


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

    def wait(self, request, kinds, station=None):
        deadline = time.monotonic() + self.timeout
        while time.monotonic() < deadline:
            for index, reply in enumerate(self.pending):
                if reply['request'] == request and (station is None or reply['station'] == station) and reply['kind'] in kinds:
                    return self.pending.pop(index)
            reply = self.receive()
            if reply['request'] == request and (station is None or reply['station'] == station) and reply['kind'] in kinds:
                return reply
            if reply['kind'] in {190, 191} and reply['request'] == request and (station is None or reply['station'] == station):
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
        reply = self.wait(terminal['request'], {112}, terminal['station'])
        if (reply['reason'] or reply['raw'][80:104] != raw[80:104]
                or reply['raw'][128:144] != raw[128:144]
                or reply['raw'][160:192] != raw[160:192]):
            raise ValueError('exact ACK rejected')
        return reply


class RequestState:
    """One fsynced counter and unresolved intent per installation/peer/epoch.

    A crashed writer leaves a lock: fail closed and require operator review.
    Never reset/delete this file to resolve an uncertain production outcome.
    """
    def __init__(self, path, profile):
        self.path = Path(path)
        self.namespace = {k: profile[k] for k in ('installation', 'peer', 'epoch')}
        self.lock = self.path.with_name(self.path.name + '.lock')
        self.state = None

    def __enter__(self):
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.fd = os.open(self.lock, os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o600)
        try:
            if self.path.exists():
                self.state = json.loads(self.path.read_text(encoding='utf-8'))
                if (self.state.get('schema') != 1 or self.state.get('namespace') != self.namespace
                        or not isinstance(self.state.get('last'), str)
                        or not self.state['last'].isdigit()
                        or not 0 <= int(self.state['last']) < 2**64):
                    raise ValueError('request state corrupt or belongs to another installation/peer/epoch')
                pending = self.state.get('pending')
                if pending is not None:
                    raw = bytes.fromhex(pending)
                    if (len(raw) != 512 or u32(raw, 8) != 10 or not u32(raw, 20)
                            or int.from_bytes(raw[24:32], 'big') != int(self.state['last'])
                            or raw[48:64] != identity(self.namespace['peer'])
                            or raw[64:80] != identity(self.namespace['epoch'])):
                        raise ValueError('unresolved request state corrupt')
            else:
                self.state = dict(schema=1, namespace=self.namespace, last='0', pending=None)
                self.save()
            return self
        except BaseException:
            self.__exit__(None, None, None)
            raise

    def __exit__(self, *_):
        os.close(self.fd)
        self.lock.unlink()

    def save(self):
        temporary = self.path.with_name(self.path.name + '.tmp')
        with temporary.open('w', encoding='utf-8') as output:
            json.dump(self.state, output)
            output.flush()
            os.fsync(output.fileno())
        os.replace(temporary, self.path)

    def reserve(self, peer, station):
        if self.state['pending'] is not None:
            raise ValueError('unresolved request exists; use --recover; no new trigger sent')
        request = int(self.state['last']) + 1
        if request >= 2**64:
            raise ValueError('request counter exhausted; wrap is forbidden')
        body = bytearray(384)
        body[:16] = station['camera_set']
        raw = peer.make(10, request=request, station=station['key'], recipe=station['recipe'],
                        template=station['template'], activation=station['activation'], body=body)
        self.state.update(last=str(request), pending=raw.hex())
        self.save()  # durable before the first network write
        return raw

    def recover(self, peer):
        if not self.state['pending']:
            raise ValueError('no unresolved request to recover')
        raw = bytearray.fromhex(self.state['pending'])
        raw[32:48] = peer.session  # transport only; durable identity/body unchanged
        return bytes(raw)

    def complete(self):
        self.state['pending'] = None
        self.save()


def discover(peer, category=1, revision=ZERO):
    rows, key, seen = [], 0, set()
    for _ in range(4096):
        body = bytearray(384)
        put32(body, 0, category)
        put32(body, 4, key)
        body[8:24] = revision
        raw = peer.make(3, body=body)
        peer.send(raw)
        reply = peer.wait(int.from_bytes(raw[24:32], 'big'), {103}, 0)
        data = reply['raw']
        if reply['reason']:
            # UNKNOWN_KEY with an empty category is not an invented entry.
            if key == 0 and reply['reason'] == 10:
                return rows, revision
            raise ValueError(f"catalogue rejected: {reply['reason']}; refresh selection before retry")
        current = data[128:144]
        if revision != ZERO and current != revision:
            raise ValueError('catalogue changed; no trigger sent')
        revision = current
        actual, size = u32(data, 148), u32(data, 200)
        if (u32(data, 144) != category or not actual or actual in seen or size > 64
                or (key and actual != key) or u32(data, 268) != 256
                or u32(data, 272) != 1 or u32(data, 276) != 256):
            raise ValueError('invalid or cyclic catalogue response')
        seen.add(actual)
        row = dict(key=actual, name=data[204:204+size].decode('utf-8'), raw=data)
        if category == 1:
            row.update(recipe=u32(data, 348), template=u32(data, 352),
                       activation=data[360:376], camera_set=data[376:392])
        rows.append(row)
        key = u32(data, 912)
        if not key:
            return rows, revision
    raise ValueError('catalogue page limit')


def station_status(peer, station):
    raw = peer.make(2, station=station['key'])
    peer.send(raw)
    reply = peer.wait(int.from_bytes(raw[24:32], 'big'), {102}, station['key'])
    data = reply['raw']
    if (reply['reason'] or (u32(data, 80), u32(data, 84), data[88:104]) !=
            (station['recipe'], station['template'], station['activation'])):
        raise ValueError('station selection changed; refresh before explicitly triggering')
    return dict(ready=bool(u32(data, 884)), busy=bool(u32(data, 888)), pending=u32(data, 892))


def terminal_summary(terminal, request):
    raw = terminal['raw']
    if (terminal['kind'] != 111 or raw[20:32] != request[20:32]
            or raw[80:104] != request[80:104] or raw[128:144] == ZERO
            or raw[144:160] == ZERO or u32(raw, 952) != 10):
        raise ValueError('terminal does not match station/request/selection/production cycle')
    execution, decision, valid = u32(raw, 192), u32(raw, 196), u32(raw, 200)
    if (execution not in {1, 2, 3} or decision not in {0, 1, 2} or valid not in {0, 1}
            or (execution != 1 and (decision or valid))):
        raise ValueError('invalid terminal execution/decision/validity')
    number_bits, flag_bits, text_bits = u32(raw, 208), u32(raw, 212), u32(raw, 220)
    if number_bits >> 16 or text_bits >> 4 or u32(raw, 216) & ~flag_bits:
        raise ValueError('invalid terminal slot validity')
    return dict(station=terminal['station'], request=str(terminal['request']),
                result=raw[128:144].hex(), digest=raw[160:192].hex(), camera=u32(raw, 948),
                recipe=u32(raw, 80), template=u32(raw, 84), activation=raw[88:104].hex(),
                execution={1:'DONE', 2:'ERROR', 3:'CANCELED'}[execution],
                decision={0:'NOT_EVALUATED', 1:'OK', 2:'NOK'}[decision], valid=bool(valid),
                numbers={str(i): struct.unpack_from('>i', raw, 280+4*i)[0]
                         for i in range(16) if number_bits & (1 << i)},
                flags={str(i): bool(u32(raw, 216) & (1 << i))
                       for i in range(32) if flag_bits & (1 << i)},
                delivery='AWAITING_ACK', reason=terminal['reason'])


def production(peer, profile, args):
    hello = peer.connect()
    if hello['raw'][128:144] != identity(profile['installation']):
        raise ValueError('profile belongs to another installation')
    state_path = args.state or args.profile.parent / ('.vf4-' + profile['peer'] + '-' + profile['epoch'] + '.json')
    with RequestState(state_path, profile) as state:
        if args.recover:
            raw = state.recover(peer)
        else:
            stations, revision = discover(peer)
            recipes, revision = discover(peer, 2, revision)
            templates, revision = discover(peer, 3, revision)
            for station in stations:
                status = station_status(peer, station)
                record = dict(station=station['key'], name=station['name'], recipe=station['recipe'],
                              template=station['template'], activation=station['activation'].hex(), **status)
                peer.event('station', **record)
                print(json.dumps(record, ensure_ascii=False), flush=True)
            if not args.trigger:
                return
            if state.state['pending']:
                raise ValueError('unresolved request exists; use --recover; no new trigger sent')
            matches = [s for s in stations if str(s['key']) == args.station or s['name'] == args.station]
            if len(matches) != 1:
                raise ValueError('--station must match one discovered name or wire key')
            station = matches[0]
            if (station['recipe'] not in {r['key'] for r in recipes}
                    or station['template'] not in {t['key'] for t in templates}
                    or station['activation'] == ZERO or station['camera_set'] == ZERO):
                raise ValueError('station has no complete published selection')
            status = station_status(peer, station)
            if not status['ready'] or status['busy'] or status['pending']:
                raise ValueError('station is not ready or has pending delivery; no trigger sent')
            raw = state.reserve(peer, station)
        request, station_key = int.from_bytes(raw[24:32], 'big'), u32(raw, 20)
        peer.send(raw)  # recovery sends the SAME identity/body; never a new cycle request
        reply = peer.wait(request, {110, 111}, station_key)
        if reply['kind'] == 110:
            if reply['reason']:
                raise ValueError('ACCEPTED carried a failure reason')
            peer.event('accepted', station=station_key, request=str(request), delivery='NOT_COMPLETED')
            print(f'ACCEPTED station={station_key} request={request}; awaiting terminal', flush=True)
            reply = peer.wait(request, {111}, station_key)
        summary = terminal_summary(reply, raw)
        peer.event('terminal', **summary)
        print(json.dumps(summary, ensure_ascii=False), flush=True)
        peer.ack(reply)
        state.complete()  # only exact ACKED commits completion locally
        summary['delivery'] = 'ACKED'
        peer.event('delivery', **summary)
        print(json.dumps(summary, ensure_ascii=False), flush=True)
        if args.expect_decision and (summary['decision'] != args.expect_decision or not summary['valid']):
            raise ValueError('terminal decision/validity differs from the expected surface result')


def main():
    sys.stdout.reconfigure(encoding='utf-8')
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host', help='listener address; profile address is used when omitted')
    parser.add_argument('--port', type=int, help='listener port; profile port is used when omitted')
    parser.add_argument('--profile', type=Path, help='product-exported test client profile JSON')
    parser.add_argument('--peer', help='locally enrolled peer UUID; overrides the profile')
    parser.add_argument('--epoch', help='locally enrolled persistent epoch UUID; overrides the profile')
    parser.add_argument('--plan', type=Path, help='JSON list: kind, station/recipe/template/activation, body_hex, request decimal string, expect_kind, ack')
    parser.add_argument('--stations', action='store_true', help='discover actual station keys and readiness')
    parser.add_argument('--station', help='exact discovered station name or wire key')
    parser.add_argument('--trigger', action='store_true', help='send ONE durable production request, then exact ACK')
    parser.add_argument('--recover', action='store_true', help='recover the unresolved request; never create a new request')
    parser.add_argument('--state', type=Path, help='durable counter/intent file; retain across runs')
    parser.add_argument('--expect-decision', choices=['OK', 'NOK', 'NOT_EVALUATED'])
    parser.add_argument('--log', type=Path, required=True, help='JSONL protocol/failure log')
    args = parser.parse_args()
    if args.trigger and not args.station:
        parser.error('--trigger requires --station (discover with --stations)')
    if args.recover and args.trigger:
        parser.error('--recover and --trigger are mutually exclusive')
    if (args.stations or args.trigger or args.recover) and (not args.profile or args.plan):
        parser.error('production mode requires --profile and cannot be combined with --plan')
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
            if args.stations or args.trigger or args.recover:
                production(peer, profile, args)
                peer.event('completed', physical='BLOCKED', exactly_once=False)
                return
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
