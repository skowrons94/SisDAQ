"""
Acquisition engine for one or more SIS3316 boards, the Python counterpart of
caendaq.DAQ.

Per board a reader thread runs the double-bank readout:

    arm bank 1 -> wait (readout interval, or earlier when a channel reaches
    the bank address threshold) -> swap to the other bank -> read every
    enabled channel's previous bank -> decode -> spectra + HDF5 -> repeat

A stats thread turns event counts into rates every stats interval and pushes
them to Graphite with the same metric names caendaq uses:

    <prefix>.bo_<board id>.writeRate
    <prefix>.bo_<board id>.ch_<ch>.{totalRate,pileRate,lostRate,satuRate}

and, SIS3316 only, triggerRate and deadFraction per channel, the board's
temperature and every counter input under its name:

    <prefix>.bo_<board id>.temperature
    <prefix>.bo_<board id>.counter.<name>.{counts,total,rate}

A saved run also gets sis3316_scalers.csv and sis3316_summary.json (live time,
hit-counter check; see scalers.py).

A board that stops answering is marked failed (board_failures counts the
episodes) and the reader keeps trying to re-initialise it.
"""

import logging
import socket
import threading
import time
from typing import Any, Dict, List, Optional

import numpy as np

from . import registers as R
from . import scalers as S
from .board import ACQ_ADDRESS_THRESHOLD, Sis3316
from .events import decode_bank
from .mock import MockSis3316
from .settings import apply_defaults, channel_settings, validate
from .transport import Sis3316Error
from .writer import Hdf5RunWriter

logger = logging.getLogger(__name__)

_FAILED_AFTER = 5            # consecutive failed cycles before a board counts as failed
_RETRY_INTERVAL_S = 2.0      # re-initialisation attempts of a failed board

# Hits for an online coincidence builder: the same 16-byte records as
# caendaq.HIT_DTYPE, so one builder serves both. energy is the spectrum bin
# (energy_max / histogram_divider), flags bit0 = pile-up.
HIT_DTYPE = np.dtype([("time_ps", "<i8"), ("energy", "<u2"), ("channel", "u1"),
                      ("flags", "u1"), ("reserved", "<u4")])
HIT_CAPACITY = 1 << 20       # hits held between two take_hits(), ~16 MB

# Time between consecutive hits of a channel, as caendaq.interval(): log bins
# from 1 ns to 10 s, INTERVAL_BINS_PER_DECADE per decade.
INTERVAL_BINS_PER_DECADE = 100
INTERVAL_BINS = 10 * INTERVAL_BINS_PER_DECADE


class _Board:
    def __init__(self, name: str, board_id: int, host: str, settings: Dict[str, Any],
                 port: int, local_ip: str, mock: bool):
        self.name = name
        self.board_id = int(board_id)
        self.host = host
        self.settings = validate(settings)
        self.mock = mock
        self.dev = MockSis3316(host) if mock else Sis3316(host, port, local_ip=local_ip)
        self.info: Dict[str, Any] = {}
        self.lock = threading.RLock()
        self.thread: Optional[threading.Thread] = None
        self.pending_settings: Optional[Dict[str, Any]] = None
        self.reset_spectra()
        self.events = np.zeros(R.N_CHANNELS, np.int64)       # cumulative per channel
        self.pileups = np.zeros(R.N_CHANNELS, np.int64)
        # The six statistic counters summed over the banks whose counts are
        # known (a bank right after the start, a recovery or a settings change
        # has no previous latch to subtract). Replaced, not updated in place,
        # so the stats thread always sees one consistent bank boundary.
        self.stat_totals = np.zeros((R.N_CHANNELS, 6), np.int64)
        # Events of those same banks, for the check against the hit counter.
        self.events_counted = np.zeros(R.N_CHANNELS, np.int64)
        self.counted_s = 0.0          # time the counted banks span
        self.uncounted_banks = 0
        self.last_stat = None
        self.last_stat_t = 0.0
        self.temperature: Optional[float] = None
        self.temp_range: List[float] = []
        self.temp_due = 0.0
        # Statistic counters at the run start: V3316-2004 does not clear them
        # with the Timestamp-Clear, so counts are reported relative to this.
        self.stat_baseline = None
        # diagnostics, the shape WebDAQ's board_diagnostics() expects
        self.buffers_read = 0
        self.bytes_read = 0
        self.comm_errors = 0
        self.events_decoded = 0
        self.board_failures = 0
        self.failed = False
        self.consecutive_errors = 0
        self.last_error = ""
        self.hits_enabled = False
        self.hits: List[np.ndarray] = []
        self.hits_held = 0
        self.hits_dropped = 0

    def reset_spectra(self) -> None:
        bins = int(self.settings["board"]["spectrum_bins"])
        self.spectra = np.zeros((R.N_CHANNELS, bins), np.uint32)
        self.waveforms: List[Optional[np.ndarray]] = [None] * R.N_CHANNELS
        # Latest waveform of a clean event and of a pile-up event, for viewers
        # that want one kind rather than whichever came last.
        self.waveforms_good: List[Optional[np.ndarray]] = [None] * R.N_CHANNELS
        self.waveforms_pileup: List[Optional[np.ndarray]] = [None] * R.N_CHANNELS
        self.maws: List[Optional[np.ndarray]] = [None] * R.N_CHANNELS
        self.intervals = np.zeros((R.N_CHANNELS, INTERVAL_BINS), np.uint32)
        self.last_time = np.full(R.N_CHANNELS, -1, np.int64)    # samples

    def enabled_channels(self) -> List[int]:
        return [ch for ch in range(R.N_CHANNELS)
                if channel_settings(self.settings, ch)["enabled"]]


class DAQ:
    def __init__(self, out_dir: str = ".", run: int = 0, write: bool = True,
                 max_file_bytes: int = 0, stats_interval_ms: int = 1000,
                 graphite_host: str = "", graphite_port: int = 2003,
                 graphite_prefix: str = "ancillary.rates", software: str = ""):
        self.out_dir = out_dir
        self.run = int(run)
        self.write = bool(write)
        self.max_file_bytes = int(max_file_bytes or 0)
        self.stats_interval_ms = max(100, int(stats_interval_ms))
        self.graphite = (graphite_host, int(graphite_port), graphite_prefix)
        self.software = software
        self.boards: List[_Board] = []
        self.writer: Optional[Hdf5RunWriter] = None
        self._running = threading.Event()
        self._stats_thread: Optional[threading.Thread] = None
        self._rates: List[Dict[str, Any]] = []
        self._rates_lock = threading.Lock()
        self._last_bytes_written = 0
        self._t_start = 0.0
        self._t_stop = 0.0
        self._scaler_csv: Optional[S.ScalerCsv] = None
        self.run_summary: Optional[Dict[str, Any]] = None

    # ── setup ──────────────────────────────────────────────────────────────
    def add_board(self, name: str, board_id: int, host: str, settings: Dict[str, Any],
                  port: int = 0xE000, local_ip: str = "", mock: bool = False) -> int:
        if self._running.is_set():
            raise RuntimeError("cannot add a board while running")
        self.boards.append(_Board(name, board_id, host, settings, port, local_ip, mock))
        return len(self.boards) - 1

    def connect(self, idx: int) -> Dict[str, Any]:
        """Open (if needed) and identify a board; usable while stopped."""
        b = self.boards[idx]
        with b.lock:
            if not b.info:
                b.info = b.dev.open()
            return dict(b.info)

    def set_graphite(self, host: str, port: int, prefix: str) -> None:
        self.graphite = (host, int(port), prefix)

    # ── run control ────────────────────────────────────────────────────────
    def start(self) -> bool:
        if self._running.is_set():
            return True
        if self.write:
            self.writer = Hdf5RunWriter(self.out_dir, self.run, self.max_file_bytes,
                                        software=self.software)
        for b in self.boards:
            self._start_board(b)
            if self.writer is not None:
                self.writer.add_board(
                    b.board_id, b.name, b.info, b.settings, b.dev.sample_clock_mhz,
                    b.settings["board"]["raw_samples"],
                    b.settings["board"]["maw_test_buffer_length"])
        self._t_start = time.monotonic()
        self._t_stop = 0.0
        self.run_summary = None
        self._scaler_csv = None
        if self.write:
            try:
                self._scaler_csv = S.ScalerCsv(self.out_dir)
            except OSError as e:
                logger.error(f"SIS3316: cannot write {S.CSV_NAME} in {self.out_dir}: {e}")
        self._running.set()
        for b in self.boards:
            b.thread = threading.Thread(target=self._reader, args=(b,), daemon=True,
                                        name=f"SIS3316-{b.board_id}")
            b.thread.start()
        self._stats_thread = threading.Thread(target=self._stats_loop, daemon=True,
                                              name="SIS3316-stats")
        self._stats_thread.start()
        return True

    def _start_board(self, b: _Board) -> None:
        with b.lock:
            if not b.info:
                b.info = b.dev.open()
            b.dev.initialize(b.settings)
            if b.settings["board"]["onboard_histograms"]:
                b.dev.clear_energy_histograms(b.enabled_channels())
            b.reset_spectra()
            b.events[:] = 0
            b.pileups[:] = 0
            b.stat_totals = np.zeros((R.N_CHANNELS, 6), np.int64)
            b.events_counted = np.zeros(R.N_CHANNELS, np.int64)
            b.counted_s = 0.0
            b.uncounted_banks = 0
            b.temp_range = []
            b.temp_due = 0.0
            b.last_stat = None
            b.dev.clear_timestamp()
            b.stat_baseline = b.dev.statistic_counters()
            b.dev.arm(bank2=False)
            # One swap straight away: it latches the counters (which a
            # Timestamp-Clear does not reset on this firmware) as the run's
            # baseline, so the first real bank is counted too. The bank closed
            # by it holds the millisecond since the arm, given up for this.
            first_bank2 = b.dev.swap()
            b.dev.previous_bank_addresses(first_bank2)
            b.last_stat = b.dev.statistic_counters()
            b.last_stat_t = time.monotonic()
            b.failed = False
            b.consecutive_errors = 0

    def stop(self) -> bool:
        if not self._running.is_set():
            return True
        self._running.clear()
        for b in self.boards:
            if b.thread is not None:
                b.thread.join(timeout=10)
                b.thread = None
        # The run ends when the readers have read their last bank, not when the
        # stats loop (asleep for up to one interval) notices.
        self._t_stop = time.monotonic()
        if self._stats_thread is not None:
            self._stats_thread.join(timeout=5)
            self._stats_thread = None
        if self.writer is not None:
            self.writer.close()
        self._finish_scalers()
        return True

    def is_running(self) -> bool:
        return self._running.is_set()

    def close(self) -> None:
        self.stop()
        for b in self.boards:
            with b.lock:
                try:
                    b.dev.close()
                except Exception:
                    pass
                b.info = {}

    # ── readout ────────────────────────────────────────────────────────────
    def _reader(self, b: _Board) -> None:
        next_swap = time.monotonic()
        while True:
            running = self._running.is_set()
            try:
                if b.failed:
                    if not running:
                        break
                    self._recover(b)
                    next_swap = time.monotonic()
                    continue
                interval = b.settings["board"]["readout_interval_ms"] / 1000.0
                # Wait for the interval, swapping early on the address threshold
                while running and time.monotonic() < next_swap + interval:
                    time.sleep(0.02)
                    if self._running.is_set():
                        with b.lock:
                            if b.dev.acquisition_status() & ACQ_ADDRESS_THRESHOLD:
                                break
                    else:
                        running = False
                next_swap = time.monotonic()
                self._cycle(b, final=not running)
                b.consecutive_errors = 0
                if not running:
                    break
            except Sis3316Error as e:
                b.comm_errors += 1
                b.consecutive_errors += 1
                b.last_error = str(e)
                logger.warning(f"SIS3316 {b.board_id}: readout error: {e}")
                if b.consecutive_errors >= _FAILED_AFTER and not b.failed:
                    b.failed = True
                    b.board_failures += 1
                    logger.error(f"SIS3316 {b.board_id}: board failed ({e})")
                if not running:
                    break
                time.sleep(0.1)
        with b.lock:
            try:
                b.dev.disarm()
            except Sis3316Error:
                pass

    def _recover(self, b: _Board) -> None:
        time.sleep(_RETRY_INTERVAL_S)
        if not self._running.is_set():
            return
        with b.lock:
            try:
                try:
                    b.dev.close()
                except Exception:
                    pass
                b.info = b.dev.open()
                b.dev.initialize(b.settings)
                # Re-initialising may restart the board's counters: start the
                # deltas afresh rather than read a wrap-around as 4e9 triggers.
                b.last_stat = None
                b.dev.arm(bank2=False)
                b.failed = False
                b.consecutive_errors = 0
                logger.info(f"SIS3316 {b.board_id}: recovered")
            except Exception as e:
                b.last_error = str(e)

    def _cycle(self, b: _Board, final: bool = False) -> None:
        with b.lock:
            read_bank2 = b.dev.swap()
            # The final bank is read like any other and the board disarmed only
            # afterwards (Struck's order: swap, wait for the swap, read the
            # previous bank, disarm). Disarming first left the swap unconfirmed:
            # the bank addresses timed out at 0 words and the last readout
            # interval of every run was never written, though its hits were
            # counted.
            nwords = b.dev.previous_bank_addresses(read_bank2)
            stat = b.dev.statistic_counters()
            stat_t = time.monotonic()
            # The temperature register is on the VME FPGA (one plain read):
            # once per stats interval, with the bank, rather than from another
            # thread that would wait for this lock.
            if stat_t >= b.temp_due:
                b.temp_due = stat_t + self.stats_interval_ms / 1000.0
                try:
                    t = float(b.dev.temperature())
                    b.temperature = t
                    b.temp_range = [min(b.temp_range[0], t), max(b.temp_range[1], t)] \
                        if b.temp_range else [t, t]
                except Sis3316Error:
                    pass             # a missed reading must not cost the bank
            banks = {}
            for ch in b.enabled_channels():
                if nwords[ch]:
                    banks[ch] = b.dev.read_bank(ch, read_bank2, nwords[ch])
                    b.bytes_read += 4 * nwords[ch]
            b.buffers_read += 1
            if final:
                b.dev.disarm()
            settings_applied = False
            if b.pending_settings is not None and not final:
                self._apply_pending(b)
                settings_applied = True
        # The counters are latched at the swap, so this delta covers exactly
        # the bank read below. A delta of 2^31 or more in one bank is no count
        # (it would take ~10 GHz of triggers) but a counter that restarted.
        counted = False
        if b.last_stat is not None:
            delta = (stat.astype(np.int64) - b.last_stat.astype(np.int64)) % (1 << 32)
            if int(delta.max()) < (1 << 31):
                counted = True
                b.stat_totals = b.stat_totals + delta
                b.counted_s += stat_t - b.last_stat_t
        if not counted:
            b.uncounted_banks += 1
        b.last_stat = stat
        b.last_stat_t = stat_t
        if settings_applied:
            # The bank read above is whole and counted; the one now filling
            # lost the events between the swap and the re-arm, so it is the
            # one left out of the counts.
            b.last_stat = None
        bank_events = np.zeros(R.N_CHANNELS, np.int64)
        maw_len = b.settings["board"]["maw_test_buffer_length"]
        for ch, words in banks.items():
            fields, waves, maws = decode_bank(words, maw_len)
            n = fields["timestamp"].size
            if n == 0:
                continue
            b.events_decoded += n
            b.events[ch] += n
            bank_events[ch] = n
            b.pileups[ch] += int(fields["pileup"].sum())
            cs = channel_settings(b.settings, ch)
            div = max(1, int(cs["histogram_divider"]))
            e = fields["energy_max"]
            idx = e[e >= 0] // div
            idx = idx[idx < b.spectra.shape[1]]
            b.spectra[ch] += np.bincount(idx, minlength=b.spectra.shape[1]).astype(np.uint32)
            if waves is not None:
                b.waveforms[ch] = waves[-1].copy()
                pile = np.flatnonzero(fields["pileup"])
                clean = np.flatnonzero(~fields["pileup"])
                if pile.size:
                    b.waveforms_pileup[ch] = waves[pile[-1]].copy()
                if clean.size:
                    b.waveforms_good[ch] = waves[clean[-1]].copy()
            if maws is not None:
                b.maws[ch] = maws[-1].copy()
            self._fill_intervals(b, ch, fields["timestamp"])
            if b.hits_enabled:
                # As in the spectrum: a negative energy is no energy.
                good = e >= 0
                self._keep_hits(b, ch, {k: v[good] for k, v in fields.items()},
                                np.clip(e[good] // div, 0, 0xFFFF))
            if self.writer is not None:
                self.writer.append(b.board_id, ch, fields, waves, maws)
        if counted:
            b.events_counted = b.events_counted + bank_events
        if self.writer is not None:
            self.writer.flush()

    @staticmethod
    def _fill_intervals(b: _Board, ch: int, ts: np.ndarray) -> None:
        t = np.concatenate(([b.last_time[ch]], ts.astype(np.int64))) if b.last_time[ch] >= 0 \
            else ts.astype(np.int64)
        dt = np.diff(t)
        dt = dt[dt > 0] * (1000.0 / b.dev.sample_clock_mhz)          # ns
        if dt.size:
            bins = np.clip(np.floor(np.log10(dt) * INTERVAL_BINS_PER_DECADE), 0,
                           INTERVAL_BINS - 1).astype(np.int64)
            b.intervals[ch] += np.bincount(bins, minlength=INTERVAL_BINS).astype(np.uint32)
        if ts.size:
            b.last_time[ch] = max(int(b.last_time[ch]), int(ts.max()))

    @staticmethod
    def _keep_hits(b: _Board, ch: int, fields: Dict[str, np.ndarray], energy: np.ndarray) -> None:
        n = fields["timestamp"].size
        room = max(0, HIT_CAPACITY - b.hits_held)
        if n > room:
            b.hits_dropped += n - room
            n = room
        if n == 0:
            return
        h = np.zeros(n, HIT_DTYPE)
        ps_per_sample = 1e6 / b.dev.sample_clock_mhz
        h["time_ps"] = np.round(fields["timestamp"][:n].astype(np.float64) * ps_per_sample).astype(np.int64)
        h["energy"] = energy[:n]
        h["channel"] = ch
        h["flags"] = fields["pileup"][:n].astype(np.uint8)
        with b.lock:
            b.hits.append(h)
            b.hits_held += n

    def _apply_pending(self, b: _Board) -> None:
        """Reconfigure between two banks (lock held). Re-arming restarts the
        bank armed at the swap, so the events of the few milliseconds between
        the swap and the reconfiguration are lost."""
        s = b.pending_settings
        b.pending_settings = None
        old_bins = b.spectra.shape[1]
        old_dividers = [channel_settings(b.settings, ch)["histogram_divider"] for ch in range(R.N_CHANNELS)]
        b.dev.disarm()
        b.dev.configure(s)
        b.settings = s
        b.dev.arm(bank2=b.dev.bank2_armed)
        # A new divider is a new energy scale: one spectrum must not mix two.
        new_dividers = [channel_settings(s, ch)["histogram_divider"] for ch in range(R.N_CHANNELS)]
        if int(s["board"]["spectrum_bins"]) != old_bins or new_dividers != old_dividers:
            b.reset_spectra()

    def update_settings(self, idx: int, settings: Dict[str, Any]) -> Dict[str, Any]:
        """Validate and apply new settings. While running they take effect at
        the next bank swap (raw length and MAW length are fixed for a run,
        since the run file's waveform arrays have a fixed width)."""
        b = self.boards[idx]
        s = validate(settings)
        if self._running.is_set():
            for key in ("raw_samples", "maw_test_buffer_length", "sample_clock_mhz"):
                if s["board"][key] != b.settings["board"][key]:
                    raise ValueError(f"'{key}' cannot change while a run is running")
            with b.lock:
                b.pending_settings = s
        else:
            with b.lock:
                b.settings = s
                b.reset_spectra()
        return s

    # ── statistics ─────────────────────────────────────────────────────────
    def _stats_loop(self) -> None:
        prev = {b.board_id: (b.events.copy(), b.pileups.copy(), b.stat_totals) for b in self.boards}
        prev_t = time.monotonic()
        while self._running.is_set():
            time.sleep(self.stats_interval_ms / 1000.0)
            now = time.monotonic()
            dt = max(1e-3, now - prev_t)
            rates = []
            written = self.writer.bytes_written if self.writer is not None else 0
            write_rate = (written - self._last_bytes_written) / dt
            self._last_bytes_written = written
            for b in self.boards:
                pe, pp, pstat = prev[b.board_id]
                ev, pu, stat = b.events.copy(), b.pileups.copy(), b.stat_totals
                d = stat - pstat
                chans = []
                for ch in range(R.N_CHANNELS):
                    chans.append({
                        "board": b.board_id, "channel": ch,
                        "event_rate": float(ev[ch] - pe[ch]) / dt,
                        "pileup_rate": float(pu[ch] - pp[ch]) / dt,
                        "lost_rate": float(d[ch, 2]) / dt,
                        "satu_rate": 0.0,
                        # The board's own count of every internal trigger
                        # (recorded or not); SIS3316 only.
                        "trigger_rate": float(d[ch, 0]) / dt,
                        # Of this interval's triggers, the share lost to dead
                        # time (None without triggers); SIS3316 only.
                        "dead_fraction": S.dead_fraction(d[ch, 0], d[ch, 2]),
                    })
                counters = [dict(c, counts=int(stat[c["channel"], 0]),
                                 total=float(stat[c["channel"], 0]) * c["scale"],
                                 rate=float(d[c["channel"], 0]) * c["scale"] / dt)
                            for c in S.counter_inputs(b.settings, b.enabled_channels())]
                prev[b.board_id] = (ev, pu, stat)
                rates.append({
                    "name": b.name, "board_id": b.board_id,
                    "write_rate": write_rate / max(1, len(self.boards)),
                    "board_failures": b.board_failures, "failed": b.failed,
                    "temperature_c": b.temperature,
                    "counters": counters,
                    "channels": chans,
                })
            prev_t = now
            with self._rates_lock:
                self._rates = rates
            # Once stopping, the final row is the stop's own (_finish_scalers).
            if self._running.is_set():
                self._write_scalers()
            self._push_graphite(rates)

    def _write_scalers(self) -> None:
        if self._scaler_csv is None:
            return
        elapsed = time.monotonic() - self._t_start
        try:
            for b in self.boards:
                chans = b.enabled_channels()
                self._scaler_csv.write(elapsed, b.board_id, chans, b.stat_totals, b.events,
                                       b.temperature, S.counter_inputs(b.settings, chans))
        except (OSError, ValueError) as e:
            logger.error(f"SIS3316: writing {S.CSV_NAME} failed, no more scaler rows this run: {e}")
            self._scaler_csv.close()
            self._scaler_csv = None

    def _finish_scalers(self) -> None:
        """At the stop: a last scaler row with the final bank, the run summary
        (sis3316_summary.json and the boards' HDF5 attributes) and the check
        of the hit counters against the events written."""
        self._write_scalers()
        if self._scaler_csv is not None:
            self._scaler_csv.close()
            self._scaler_csv = None
        duration = (self._t_stop or time.monotonic()) - self._t_start
        boards = []
        for b in self.boards:
            chans = b.enabled_channels()
            temp = {"last": b.temperature,
                    "min": b.temp_range[0] if b.temp_range else None,
                    "max": b.temp_range[1] if b.temp_range else None}
            summ = S.board_summary(b.board_id, b.name, chans, b.stat_totals, b.events,
                                   b.events_counted, b.counted_s, duration, b.uncounted_banks,
                                   temp, S.counter_inputs(b.settings, chans), b.settings)
            for m in summ["integrity"]["mismatches"]:
                logger.warning(f"SIS3316 {b.board_id} ch {m['channel']}: the board counted "
                               f"{m['hits']} hits but {m['events']} events were read")
            boards.append(summ)
        self.run_summary = {"run": self.run, "write": self.write, "boards": boards}
        if not self.write:
            return
        try:
            S.write_summary(self.out_dir, self.run, self.software, boards)
        except OSError as e:
            logger.error(f"SIS3316: cannot write {S.SUMMARY_NAME}: {e}")
        if self.writer is not None:
            for summ in boards:
                try:
                    self.writer.add_run_summary(summ["board_id"], summ)
                except Exception as e:
                    logger.error(f"SIS3316 {summ['board_id']}: run summary not stored "
                                 f"in the HDF5 files: {e}")

    def _push_graphite(self, rates: List[Dict[str, Any]]) -> None:
        host, port, prefix = self.graphite
        if not host:
            return
        epoch = int(time.time())
        lines = []
        for b in rates:
            bo = f"{prefix}.bo_{b['board_id']}"
            lines.append(f"{bo}.writeRate {b['write_rate']:.3f} {epoch}")
            if b.get("temperature_c") is not None:
                lines.append(f"{bo}.temperature {b['temperature_c']:.2f} {epoch}")
            for c in b["channels"]:
                base = f"{bo}.ch_{c['channel']}."
                lines.append(f"{base}totalRate {c['event_rate']:.3f} {epoch}")
                lines.append(f"{base}pileRate {c['pileup_rate']:.3f} {epoch}")
                lines.append(f"{base}lostRate {c['lost_rate']:.3f} {epoch}")
                lines.append(f"{base}satuRate {c['satu_rate']:.3f} {epoch}")
                lines.append(f"{base}triggerRate {c.get('trigger_rate', 0.0):.3f} {epoch}")
                if c.get("dead_fraction") is not None:
                    lines.append(f"{base}deadFraction {c['dead_fraction']:.6f} {epoch}")
            for c in b.get("counters", []):
                base = f"{bo}.counter.{c['metric']}."
                lines.append(f"{base}counts {c['counts']} {epoch}")
                lines.append(f"{base}total {c['total']:.10g} {epoch}")
                lines.append(f"{base}rate {c['rate']:.6g} {epoch}")
        try:
            with socket.create_connection((host, port), timeout=0.5) as s:
                s.sendall(("\n".join(lines) + "\n").encode())
        except OSError:
            pass                     # best effort, like caendaq's Graphite sink

    def stats(self) -> List[Dict[str, Any]]:
        with self._rates_lock:
            if self._rates:
                return [dict(r) for r in self._rates]
        return [{"name": b.name, "board_id": b.board_id, "write_rate": 0.0,
                 "board_failures": b.board_failures, "failed": b.failed,
                 "channels": [{"board": b.board_id, "channel": ch, "event_rate": 0.0,
                               "pileup_rate": 0.0, "lost_rate": 0.0, "satu_rate": 0.0}
                              for ch in range(R.N_CHANNELS)]} for b in self.boards]

    def diagnostics(self) -> Dict[int, Dict[str, Any]]:
        out = {}
        for b in self.boards:
            out[b.board_id] = {
                "buffers_read": b.buffers_read,
                "bytes_read": b.bytes_read,
                "bytes_written": self.writer.bytes_written if self.writer is not None else 0,
                "blocks_dropped": 0,
                "comm_errors": b.comm_errors,
                "events_decoded": b.events_decoded,
                "board_failures": b.board_failures,
                "failed": b.failed,
                "last_error": b.last_error,
            }
        return out

    # ── live data ──────────────────────────────────────────────────────────
    def energy(self, idx: int, ch: int) -> np.ndarray:
        return self.boards[idx].spectra[ch].copy()

    def waveform(self, idx: int, ch: int, kind: str = "any") -> np.ndarray:
        """Latest waveform of a channel: of any event, or only of a 'good'
        (no pile-up) or 'pileup' event. The SIS3316 has no saturation flag in
        its event header, so 'saturated' is always empty."""
        b = self.boards[idx]
        w = {"good": b.waveforms_good, "pileup": b.waveforms_pileup,
             "saturated": [None] * R.N_CHANNELS}.get(kind, b.waveforms)[ch]
        return np.zeros(0, np.uint16) if w is None else w.copy()

    def interval(self, idx: int, ch: int) -> np.ndarray:
        """Time between consecutive hits: INTERVAL_BINS log bins from 1 ns."""
        return self.boards[idx].intervals[ch].copy()

    # ── hits for online coincidences ───────────────────────────────────────
    def enable_hits(self, idx: int, on: bool = True) -> None:
        """Start (or stop) keeping this board's hits for take_hits(); at most
        HIT_CAPACITY are held between takes, beyond that they are dropped and
        counted."""
        b = self.boards[idx]
        with b.lock:
            b.hits_enabled = bool(on)
            if not on:
                b.hits, b.hits_held = [], 0

    def take_hits(self, idx: int) -> np.ndarray:
        """Every hit since the last call (HIT_DTYPE), in readout order:
        time-ordered per channel, not across channels."""
        b = self.boards[idx]
        with b.lock:
            hits, b.hits, b.hits_held = b.hits, [], 0
        return np.concatenate(hits) if hits else np.zeros(0, HIT_DTYPE)

    def hits_dropped(self, idx: int) -> int:
        return int(self.boards[idx].hits_dropped)

    def maw(self, idx: int, ch: int) -> np.ndarray:
        m = self.boards[idx].maws[ch]
        return np.zeros(0, np.int32) if m is None else m.copy()

    def onboard_histogram(self, idx: int, ch: int, nbins: int = R.HISTOGRAM_BINS) -> np.ndarray:
        b = self.boards[idx]
        with b.lock:
            if not b.info:
                b.info = b.dev.open()
            return b.dev.energy_histogram(ch, nbins).copy()

    def clear_onboard_histograms(self, idx: int, channels=None) -> None:
        """Zero on-board histograms. While running this happens between banks."""
        b = self.boards[idx]
        chans = list(range(R.N_CHANNELS)) if channels is None else list(channels)
        with b.lock:
            if not b.info:
                b.info = b.dev.open()
            running = self._running.is_set()
            if running:
                b.dev.disarm()
            b.dev.clear_energy_histograms(chans)
            if running:
                b.dev.arm(bank2=b.dev.bank2_armed)

    def statistic_counters(self, idx: int) -> np.ndarray:
        """(16, 6) trigger statistic counts since the last run start."""
        b = self.boards[idx]
        with b.lock:
            if not b.info:
                b.info = b.dev.open()
            now = b.dev.statistic_counters().astype(np.int64)
        if b.stat_baseline is None:
            return now
        return (now - b.stat_baseline.astype(np.int64)) % (1 << 32)

    def scalers(self, idx: int) -> Dict[str, Any]:
        """Run totals of the statistic counters with dead and live fraction,
        the latest interval's rates, the temperature and the counter inputs:
        of the running run, or of the last one after the stop. Read from what
        the readout already collected, so it costs the board nothing."""
        b = self.boards[idx]
        running = self._running.is_set()
        chans = b.enabled_channels()
        totals, events = b.stat_totals, b.events.copy()
        with self._rates_lock:
            latest = next((r for r in self._rates if r["board_id"] == b.board_id), None)
        rows = []
        for ch in range(R.N_CHANNELS):
            t = [int(v) for v in totals[ch]]
            row = dict(zip(S.NAMES, t))
            row.update({"channel": ch, "enabled": ch in chans, "events": int(events[ch]),
                        "dead_fraction": S.dead_fraction(t[0], t[2]),
                        "live_fraction": S.live_fraction(t[0], t[2])})
            if latest and running:
                c = latest["channels"][ch]
                row.update({"trigger_rate": c.get("trigger_rate"), "event_rate": c["event_rate"],
                            "interval_dead_fraction": c.get("dead_fraction")})
            rows.append(row)
        counters = []
        for c in S.counter_inputs(b.settings, chans):
            n = int(totals[c["channel"], 0])
            rate = next((x["rate"] for x in (latest or {}).get("counters", [])
                         if x["channel"] == c["channel"]), None) if running else None
            counters.append(dict(c, counts=n, total=n * c["scale"], rate=rate))
        return {"running": running, "started": self._t_start > 0,
                "elapsed_s": (time.monotonic() - self._t_start) if running else None,
                "counted_s": b.counted_s, "uncounted_banks": b.uncounted_banks,
                "temperature_c": b.temperature, "names": list(S.NAMES),
                "channels": rows, "counters": counters,
                "summary": next((x for x in (self.run_summary or {}).get("boards", [])
                                 if x["board_id"] == b.board_id), None) if not running else None}

    def board_info(self, idx: int) -> Dict[str, Any]:
        b = self.boards[idx]
        info = dict(b.info)
        if self._running.is_set() and b.temperature is not None:
            # The readout reads it every stats interval: no extra request.
            info["temperature_c"] = b.temperature
        else:
            try:
                with b.lock:
                    if info:
                        info["temperature_c"] = b.dev.temperature()
            except Sis3316Error:
                pass
        info.update({"name": b.name, "board_id": b.board_id, "host": b.host,
                     "sample_clock_mhz": b.dev.sample_clock_mhz, "failed": b.failed})
        return info
