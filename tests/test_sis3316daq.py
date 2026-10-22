"""Hardware-free tests of sis3316daq: python -m unittest discover -s tests"""

import json
import os
import sys
import tempfile
import time
import unittest

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from sis3316daq import DAQ, default_settings, validate  # noqa: E402
from sis3316daq.events import decode_bank, header_words  # noqa: E402
from sis3316daq.settings import channel_settings  # noqa: E402
from sis3316daq.transport import UdpLink  # noqa: E402

try:
    import h5py
except ImportError:
    h5py = None


def make_records(n, ch=3, fmt=0x8, raw=4, energy=1000, ts0=100, pileup_every=0):
    hdr = header_words(fmt)
    rec = np.zeros((n, hdr + 1 + raw // 2), np.uint32)
    ts = ts0 + np.arange(n, dtype=np.uint64) * 1000 + (1 << 33)
    rec[:, 0] = ((ts >> np.uint64(32)).astype(np.uint32) << 16) | (ch << 4) | fmt
    rec[:, 1] = (ts & np.uint64(0xFFFFFFFF)).astype(np.uint32)
    if fmt & 0x8:
        rec[:, hdr - 2] = np.uint32(7)
        rec[:, hdr - 1] = (energy + np.arange(n)).astype(np.uint32)
    rec[:, hdr] = 0xE0000000 | (raw // 2)
    if pileup_every:
        rec[::pileup_every, hdr] |= 1 << 26
    if raw:
        rec[:, hdr + 1:] = (np.arange(raw, dtype=np.uint16) + 8000).view(np.uint32)
    return rec


class SettingsTests(unittest.TestCase):
    def test_defaults_are_complete_and_valid(self):
        s = validate(default_settings())
        self.assertEqual(s["board"]["sample_clock_mhz"], 250.0)
        self.assertFalse(channel_settings(s, 0)["enabled"])

    def test_channel_override_and_clamping(self):
        s = default_settings()
        s["channel_defaults"]["trigger_peaking"] = 7          # odd -> even
        s["channels"]["2"] = {"enabled": True, "energy_peaking": 99999}
        s["board"]["raw_samples"] = 100001
        v = validate(s)
        self.assertEqual(channel_settings(v, 0)["trigger_peaking"], 6)
        self.assertTrue(channel_settings(v, 2)["enabled"])
        self.assertEqual(channel_settings(v, 2)["energy_peaking"], 2044)
        self.assertEqual(v["board"]["raw_samples"], 32000)

    def test_bad_enum_and_clock_are_refused(self):
        s = default_settings()
        s["channels"]["1"] = {"input_range": "10V"}
        with self.assertRaises(ValueError):
            validate(s)
        s = default_settings()
        s["board"]["sample_clock_mhz"] = 300
        with self.assertRaises(ValueError):
            validate(s)


class DecoderTests(unittest.TestCase):
    def test_fast_path(self):
        rec = make_records(5, raw=6, pileup_every=2)
        f, waves, maw = decode_bank(rec.ravel())
        self.assertEqual(f["timestamp"].size, 5)
        self.assertEqual(int(f["timestamp"][0]), (1 << 33) + 100)
        self.assertEqual(f["energy_max"].tolist(), [1000, 1001, 1002, 1003, 1004])
        self.assertEqual(f["energy_start"].tolist(), [7] * 5)
        self.assertEqual(f["pileup"].tolist(), [True, False, True, False, True])
        self.assertEqual(f["channel_id"].tolist(), [3] * 5)
        self.assertEqual(waves.shape, (5, 6))
        self.assertEqual(waves[0].tolist(), [8000, 8001, 8002, 8003, 8004, 8005])
        self.assertIsNone(maw)

    def test_negative_energy_is_signed(self):
        rec = make_records(1, energy=-5)
        f, _, _ = decode_bank(rec.ravel())
        self.assertEqual(int(f["energy_max"][0]), -5)

    def test_walker_keeps_good_records_before_garbage(self):
        good = make_records(3, raw=4).ravel()
        words = np.concatenate([good, np.array([0x12345678, 0, 0, 0, 0, 0], np.uint32)])
        f, waves, _ = decode_bank(words)
        self.assertEqual(f["timestamp"].size, 3)
        self.assertEqual(waves.shape, (3, 4))

    def test_empty_bank(self):
        f, waves, maw = decode_bank(np.zeros(0, np.uint32))
        self.assertEqual(f["timestamp"].size, 0)
        self.assertIsNone(waves)


class BatchingTests(unittest.TestCase):
    def test_adc_fpga_registers_are_never_batched(self):
        addrs = [0x20, 0x28, 0x1010, 0x1014, 0x60, 0x2010]
        self.assertEqual(UdpLink._batches(addrs, lambda a: a),
                         [[0x20, 0x28], [0x1010], [0x1014], [0x60], [0x2010]])

    def test_vme_registers_batch_up_to_the_request_limit(self):
        addrs = [0x20] * 70
        self.assertEqual([len(b) for b in UdpLink._batches(addrs, lambda a: a)], [64, 6])


@unittest.skipIf(h5py is None, "h5py not installed")
class MockDaqTests(unittest.TestCase):
    def test_mock_run_writes_hdf5_and_spectra(self):
        s = default_settings()
        s["board"]["raw_samples"] = 100
        s["board"]["readout_interval_ms"] = 50
        s["channels"]["1"] = {"enabled": True}
        with tempfile.TemporaryDirectory() as tmp:
            d = DAQ(tmp, run=12, stats_interval_ms=200)
            d.add_board("SIS3316", 4, "mock", s, mock=True)
            d.start()
            time.sleep(0.6)
            self.assertTrue(d.is_running())
            spectrum = d.energy(0, 1)
            d.stop()
            self.assertGreater(int(spectrum.sum()), 0)
            # amplitude 350 x peaking 200 / divider 200 -> bin ~350
            self.assertAlmostEqual(int(spectrum.argmax()), 350, delta=3)
            self.assertEqual(d.waveform(0, 1).size, 100)
            self.assertEqual(d.diagnostics()[4]["comm_errors"], 0)
            stats = d.stats()[0]
            self.assertEqual(stats["board_id"], 4)
            self.assertEqual(len(stats["channels"]), 16)
            files = sorted(f for f in os.listdir(tmp) if f.endswith(".h5"))
            self.assertEqual(files, ["run_12_0000.h5"])
            with h5py.File(os.path.join(tmp, files[0])) as f:
                g = f["board_4"]
                self.assertEqual(f.attrs["run"], 12)
                self.assertEqual(g["events"].shape[0], g["waveforms"].shape[0])
                self.assertGreater(g["events"].shape[0], 0)
                self.assertTrue(set(g["events"]["channel"][:]) == {1})
                self.assertTrue(json.loads(g.attrs["settings"])["channels"]["1"]["enabled"])

    def test_settings_that_change_the_file_layout_are_refused_while_running(self):
        s = default_settings()
        s["channels"]["0"] = {"enabled": True}
        with tempfile.TemporaryDirectory() as tmp:
            d = DAQ(tmp, run=1, write=False)
            d.add_board("SIS3316", 0, "mock", s, mock=True)
            d.start()
            try:
                bad = json.loads(json.dumps(s))
                bad["board"]["raw_samples"] = 64
                with self.assertRaises(ValueError):
                    d.update_settings(0, bad)
                ok = json.loads(json.dumps(s))
                ok["channels"]["0"]["trigger_threshold"] = 55
                d.update_settings(0, ok)
            finally:
                d.stop()

    def test_hits_for_coincidences(self):
        """take_hits() hands out every hit once, time-ordered per channel, in
        caendaq's record layout; off until enabled; intervals fill alongside."""
        from sis3316daq.daq import HIT_DTYPE, INTERVAL_BINS
        s = default_settings()
        s["board"]["readout_interval_ms"] = 50
        s["channels"]["0"] = {"enabled": True}
        s["channels"]["5"] = {"enabled": True}
        d = DAQ(write=False)
        d.add_board("SIS3316", 2, "mock", s, mock=True)
        d.start()
        try:
            time.sleep(0.3)
            self.assertEqual(d.take_hits(0).size, 0)          # not enabled yet
            d.enable_hits(0)
            time.sleep(0.6)
            first = d.take_hits(0)
            time.sleep(0.3)
            second = d.take_hits(0)
        finally:
            d.stop()
        self.assertEqual(first.dtype, HIT_DTYPE)
        self.assertEqual(HIT_DTYPE.itemsize, 16)
        self.assertGreater(first.size, 0)
        self.assertEqual(set(np.unique(first["channel"])), {0, 5})
        both = np.concatenate((first, second))
        for ch in (0, 5):
            t = both["time_ps"][both["channel"] == ch]
            self.assertTrue(np.all(np.diff(t) > 0), "hits repeated or out of order")
        self.assertEqual(d.hits_dropped(0), 0)
        iv = d.interval(0, 0)
        self.assertEqual(iv.size, INTERVAL_BINS)
        self.assertGreater(int(iv.sum()), 0)


class ScalerMathTests(unittest.TestCase):
    def test_live_fraction(self):
        from sis3316daq import scalers as S
        self.assertAlmostEqual(S.live_fraction(1000, 50), 0.95)
        self.assertAlmostEqual(S.dead_fraction(1000, 50), 0.05)
        self.assertIsNone(S.live_fraction(0, 0))           # no trigger: unknown, not 100 %
        self.assertEqual(S.dead_fraction(10, 20), 1.0)      # clamped

    def test_integrity_tolerance(self):
        from sis3316daq import scalers as S
        self.assertIsNone(S.check_integrity(100000, 100000))
        self.assertIsNone(S.check_integrity(100000, 99950))     # within 0.1 %
        self.assertIsNone(S.check_integrity(10, 8))             # within 2
        bad = S.check_integrity(100000, 99000)
        self.assertEqual(bad["difference"], -1000)

    def test_counter_input_settings_and_scaling(self):
        from sis3316daq import scalers as S
        s = default_settings()
        s["channels"]["14"] = {"enabled": True, "counter_input": True,
                               "counter_name": " Beam charge ", "counter_scale": "10",
                               "counter_unit": "pC"}
        s["channels"]["3"] = {"enabled": True}
        v = validate(s)
        cs = channel_settings(v, 14)
        self.assertEqual((cs["counter_name"], cs["counter_scale"], cs["counter_unit"]),
                         ("Beam charge", 10.0, "pC"))
        c = S.counter_inputs(v, [3, 14])
        self.assertEqual(len(c), 1)
        self.assertEqual(c[0]["metric"], "Beam_charge")
        self.assertEqual(S.metric_name("", 7), "ch_7")
        totals = np.zeros((16, 6), np.int64)
        totals[14] = [1234, 1200, 34, 0, 0, 0]
        summ = S.board_summary(1, "SIS3316", [3, 14], totals, np.full(16, 1200),
                               np.full(16, 1200), 10.0, 10.5, 1,
                               {"last": 40.0, "min": 40.0, "max": 40.0}, c, v)
        cin = summ["counter_inputs"][0]
        # The internal trigger counter: every pulse, dead time or not.
        self.assertEqual(cin["counts"], 1234)
        self.assertAlmostEqual(cin["value"], 12340.0)
        self.assertAlmostEqual(cin["mean_rate"], 1234.0)
        for bad in (0, float("nan"), "x"):
            s["channels"]["14"]["counter_scale"] = bad
            with self.assertRaises(ValueError):
                validate(s)


@unittest.skipIf(h5py is None, "h5py not installed")
class ScalerRunTests(unittest.TestCase):
    def _run(self, tmp, settings, seconds=0.9, tamper=None):
        d = DAQ(tmp, run=7, stats_interval_ms=200)
        d.add_board("SIS3316", 1, "mock", settings, mock=True)
        d.boards[0].dev.dead_share = 0.2
        d.start()
        time.sleep(seconds)
        if tamper:
            tamper(d)
        live = d.scalers(0)
        d.stop()
        return d, live

    def _settings(self):
        s = default_settings()
        s["board"]["readout_interval_ms"] = 50
        s["board"]["raw_samples"] = 0
        s["channels"]["2"] = {"enabled": True}
        s["channels"]["14"] = {"enabled": True, "counter_input": True,
                               "counter_name": "Beam charge", "counter_scale": 10,
                               "counter_unit": "pC"}
        return s

    def test_scaler_csv_summary_and_hdf5(self):
        import csv
        from sis3316daq import scalers as S
        with tempfile.TemporaryDirectory() as tmp:
            d, live = self._run(tmp, self._settings())
            self.assertTrue(live["running"])
            self.assertEqual([c["name"] for c in live["counters"]], ["Beam charge"])
            with open(os.path.join(tmp, S.CSV_NAME)) as f:
                rows = list(csv.DictReader(f))
            self.assertEqual(list(rows[0].keys()), S.CSV_COLUMNS)
            self.assertEqual({r["channel"] for r in rows}, {"2", "14"})
            self.assertGreaterEqual(len(rows), 2 * 4)        # >= 4 intervals + the final row
            ch14 = [r for r in rows if r["channel"] == "14"]
            last = ch14[-1]
            self.assertEqual(last["counter_name"], "Beam charge")
            self.assertAlmostEqual(float(last["counter_value"]), 10 * int(last["internal"]))
            self.assertEqual(float(last["temperature_c"]), 40.0)
            # cumulative: never decreasing
            internal = [int(r["internal"]) for r in ch14]
            self.assertEqual(internal, sorted(internal))

            with open(os.path.join(tmp, S.SUMMARY_NAME)) as f:
                summ = json.load(f)
            self.assertEqual(summ["run"], 7)
            b = summ["boards"][0]
            self.assertTrue(b["integrity"]["ok"], b["integrity"])
            ch2 = next(c for c in b["channels"] if c["channel"] == 2)
            self.assertEqual(ch2["internal"], ch2["hit"] + ch2["deadtime"])
            self.assertGreater(ch2["deadtime"], 0)
            self.assertAlmostEqual(ch2["live_fraction"], 1 - ch2["deadtime"] / ch2["internal"])
            self.assertEqual(ch2["integrity"], "ok")
            # The last CSV row is the summary's totals.
            self.assertEqual(int(last["internal"]),
                             next(c for c in b["channels"] if c["channel"] == 14)["internal"])
            cin = b["counter_inputs"][0]
            self.assertAlmostEqual(cin["value"], 10 * cin["counts"])
            self.assertGreater(b["counted_s"], 0.5)
            self.assertGreaterEqual(b["uncounted_banks"], 1)     # the first bank

            with h5py.File(os.path.join(tmp, "run_7_0000.h5")) as f:
                g = f["board_1"]
                self.assertEqual(int(g.attrs["statistic_totals"][2][0]), ch2["internal"])
                self.assertAlmostEqual(float(g.attrs["live_fraction"][2]), ch2["live_fraction"])
                self.assertTrue(np.isnan(g.attrs["live_fraction"][0]))
                self.assertEqual(json.loads(g.attrs["run_summary"])["board_id"], 1)
            self.assertIsNone(S.integrity_note(summ))

    def test_hit_counter_mismatch_is_reported(self):
        from sis3316daq import scalers as S

        def lose_events(d):
            # The board says it recorded 500 more hits on ch 2 than were read.
            dev = d.boards[0].dev
            dev._stats[2, 0] += 500
            dev._stats[2, 1] += 500
        with tempfile.TemporaryDirectory() as tmp:
            d, _ = self._run(tmp, self._settings(), tamper=lose_events)
            b = d.run_summary["boards"][0]
            self.assertFalse(b["integrity"]["ok"])
            self.assertEqual([m["channel"] for m in b["integrity"]["mismatches"]], [2])
            self.assertEqual(b["integrity"]["mismatches"][0]["difference"], -500)
            note = S.integrity_note(d.run_summary)
            self.assertIn("ch 2", note)
            self.assertTrue(note.startswith("[sis3316]"))
            self.assertEqual(note.count("\n"), 0)

    def test_not_saved_run_writes_no_files(self):
        with tempfile.TemporaryDirectory() as tmp:
            d = DAQ(tmp, run=3, write=False, stats_interval_ms=200)
            d.add_board("SIS3316", 1, "mock", self._settings(), mock=True)
            d.start()
            time.sleep(0.5)
            d.stop()
            self.assertEqual(os.listdir(tmp), [])
            self.assertEqual(len(d.run_summary["boards"]), 1)
            self.assertIsNotNone(d.scalers(0)["summary"])

    def test_graphite_lines(self):
        import socket
        import threading
        srv = socket.socket()
        srv.bind(("127.0.0.1", 0))
        srv.listen(1)
        got = []

        def serve():
            conn, _ = srv.accept()
            with conn:
                buf = b""
                while True:
                    part = conn.recv(65536)
                    if not part:
                        break
                    buf += part
                got.append(buf.decode())
        t = threading.Thread(target=serve, daemon=True)
        t.start()
        d = DAQ(write=False, graphite_host="127.0.0.1", graphite_port=srv.getsockname()[1],
                graphite_prefix="test.rates")
        d.add_board("SIS3316", 1, "mock", self._settings(), mock=True)
        rates = [{"board_id": 1, "write_rate": 0.0, "temperature_c": 41.5,
                  "channels": [{"channel": 2, "event_rate": 1.0, "pileup_rate": 0.0,
                                "lost_rate": 0.1, "satu_rate": 0.0, "trigger_rate": 1.1,
                                "dead_fraction": 0.0909}],
                  "counters": [{"metric": "Beam_charge", "counts": 5, "total": 50.0, "rate": 10.0}]}]
        d._push_graphite(rates)
        t.join(timeout=2)
        srv.close()
        text = got[0]
        self.assertIn("test.rates.bo_1.temperature 41.50 ", text)
        self.assertIn("test.rates.bo_1.ch_2.deadFraction 0.090900 ", text)
        self.assertIn("test.rates.bo_1.counter.Beam_charge.total 50 ", text)
        self.assertIn("test.rates.bo_1.counter.Beam_charge.rate 10 ", text)


if __name__ == "__main__":
    unittest.main()
