"""VF-07 durable intent, correlation and validity failure boundaries."""
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
import uuid

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from communication_v4_simulator import Peer, RequestState, ZERO, frame, terminal_summary, decode, put32


class OperatorTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.path = Path(self.directory.name)/'state.json'
        self.profile = {k: uuid.uuid4().hex for k in ('installation', 'peer', 'epoch')}
        self.peer = Peer('localhost', 1, self.profile['peer'], self.profile['epoch'])
        self.station = dict(key=7, recipe=8, template=9, activation=uuid.uuid4().bytes, camera_set=uuid.uuid4().bytes)

    def test_persist_before_send_recover_same_request_and_lock(self):
        with RequestState(self.path, self.profile) as state:
            raw = state.reserve(self.peer, self.station)
            self.assertEqual(json.loads(self.path.read_text())['pending'], raw.hex())
            with self.assertRaises(FileExistsError):
                with RequestState(self.path, self.profile):
                    pass
        with RequestState(self.path, self.profile) as state:
            with self.assertRaisesRegex(ValueError, 'unresolved'):
                state.reserve(self.peer, self.station)
            self.peer.session = uuid.uuid4().bytes
            recovered = state.recover(self.peer)
            self.assertEqual(raw[:32]+raw[48:], recovered[:32]+recovered[48:])
            state.complete()
            self.assertEqual(int.from_bytes(state.reserve(self.peer, self.station)[24:32], 'big'), 2)

    def test_corrupt_namespace_and_uint64_exhaustion_fail_closed(self):
        with RequestState(self.path, self.profile) as state:
            state.state['last'] = str(2**64-1)
            state.save()
            with self.assertRaisesRegex(ValueError, 'exhausted'):
                state.reserve(self.peer, self.station)
        with self.assertRaisesRegex(ValueError, 'another'):
            with RequestState(self.path, dict(self.profile, epoch=uuid.uuid4().hex)):
                pass
        self.path.write_text('{')
        with self.assertRaises(ValueError):
            with RequestState(self.path, self.profile):
                pass
        self.assertFalse(self.path.with_name('state.json.lock').exists())

    def test_station_request_correlation_and_bounded_unmatched(self):
        def reply(station, request=4, kind=111):
            return dict(station=station, request=request, kind=kind)
        self.peer.pending = [reply(8), reply(7)]
        self.assertEqual(self.peer.wait(4, {111}, 7)['station'], 7)
        self.assertEqual(len(self.peer.pending), 1)
        self.peer.receive = lambda: reply(8)
        with self.assertRaisesRegex(ValueError, 'bounded'):
            self.peer.wait(4, {111}, 7)

    def test_terminal_requires_matching_selection_and_validity(self):
        request = frame(10, 3, self.profile['peer'], self.profile['epoch'], station=7,
                        recipe=8, template=9, activation=self.station['activation'])
        raw = bytearray(1024)
        raw[:128] = request[:128]
        put32(raw, 8, 111);put32(raw, 12, 1024)
        raw[128:144] = uuid.uuid4().bytes;raw[144:160] = uuid.uuid4().bytes
        put32(raw, 192, 1);put32(raw, 196, 1);put32(raw, 200, 0);put32(raw, 952, 10)
        terminal = dict(kind=111, station=7, request=3, reason=0, raw=bytes(raw))
        summary = terminal_summary(terminal, request)
        self.assertEqual(summary['decision'], 'OK')
        self.assertFalse(summary['valid'])  # decision alone is never a successful result
        raw[88] ^= 1;terminal['raw'] = bytes(raw)
        with self.assertRaisesRegex(ValueError, 'match'):
            terminal_summary(terminal, request)
        with self.assertRaisesRegex(ValueError, 'SHA256'):
            decode(bytes(raw))

    def test_ACKED_must_echo_exact_result_digest_and_selection(self):
        raw = bytearray(1024);raw[128:144] = uuid.uuid4().bytes;raw[160:192] = bytes(range(32))
        terminal = dict(raw=bytes(raw), kind=111, station=7, request=3)
        self.peer.send = lambda _: None
        for offset in (80, 88, 128, 160):
            changed = bytearray(raw);changed[offset] ^= 1
            self.peer.wait = lambda *_: dict(raw=bytes(changed), reason=0)
            with self.assertRaisesRegex(ValueError, 'exact ACK'):
                self.peer.ack(terminal)


if __name__ == '__main__':
    unittest.main()
