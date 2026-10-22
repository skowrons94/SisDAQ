# SisDAQ

Pure-Python data acquisition for **Struck** digitizers. It currently supports the
**SIS3316** 16-channel digitizer, as the `sis3316daq` package; other Struck
boards can join as their own packages.

`sis3316daq` drives the SIS3316
over Ethernet (UDP): board initialisation, configuration from a settings
dictionary, double-bank readout, online decoding into energy spectra and
waveforms, HDF5 run files, trigger statistics, and the board's own utilities
(on-board energy histograms, statistic counters, filter traces).

It needs only `numpy` (and `h5py` to write run files), runs on any PC with a
network link to the board, and has a simulated board for working without
hardware. It is the SIS3316 backend of [WebDAQ](https://github.com/skowrons94/WebDAQ)
(the LUNA experiment's run control), next to
[CaenDAQ](https://github.com/skowrons94/CaenDAQ) for CAEN digitizers, but it
does not depend on it.

```bash
pip install "sis3316daq[files] @ git+https://github.com/skowrons94/SisDAQ.git@v1.0.0"
# or, to work on it:
git clone https://github.com/skowrons94/SisDAQ.git && pip install -e "SisDAQ[files]"
```

## Quick start

```python
from sis3316daq import DAQ, default_settings

s = default_settings()
s["board"]["raw_samples"] = 1000                       # waveform samples per event
s["channels"]["14"] = {"enabled": True,                # channels are numbered 0-15
                       "trigger_threshold": 100,       # ADC counts of pulse height
                       "tau_factor": 17}               # decay correction (~120 us)

daq = DAQ(out_dir="data/run7", run=7)                  # write=False: spectra only
daq.add_board("SIS3316", board_id=3, host="192.168.1.100", settings=s)  # mock=True: no hardware
daq.start()
...
daq.stats()                    # per-channel event, pile-up, lost and trigger rates
daq.energy(0, 14)              # online energy spectrum (numpy array)
daq.waveform(0, 14, "pileup")  # latest trace of a pile-up event ('any', 'good', 'pileup')
daq.onboard_histogram(0, 14)   # the board's own 64K-bin energy histogram
daq.statistic_counters(0)      # (16, 6) trigger statistics since the run start
daq.interval(0, 14)            # time between consecutive hits, log bins from 1 ns
daq.scalers(0)                 # run totals, live time per channel, temperature, counter inputs
daq.stop()
daq.run_summary                # the run's totals and hit-counter check (also after write=False)
```

A written run also gets `sis3316_scalers.csv` (the six statistic counters per
channel every stats interval, cumulative, with the temperature) and, at the
stop, `sis3316_summary.json` and the same totals as HDF5 attributes. Live time
is `1 - deadtime / internal` per channel; a channel with `counter_input` set
reports its internal trigger count times `counter_scale` in `counter_unit`
(a current integrator's pulses, say). Details in `sis3316daq/scalers.py`.

For online coincidences, `daq.enable_hits(0)` keeps a 16-byte record per hit
(`HIT_DTYPE`: `time_ps`, `energy` = spectrum bin, `channel`, `flags` bit 0 =
pile-up) and `daq.take_hits(0)` returns them since the previous call, the same
layout as CaenDAQ's, so one coincidence builder serves both. Off by default,
bounded at about a million hits between two takes.

`Sis3316` (`board.py`) is the low-level driver, for scripts that drive the board
directly: `open()`, `initialize(settings)`, `arm()` / `swap()`, `read_bank()`,
`energy_histogram()`, `statistic_counters()`, `temperature()`.

## Network

The SIS3316 has no ARP of its own: its MAC address is `00:00:56:31:6x:xx`, with
`xxx` the board's serial number in hex, and the PC maps an IP address onto it
with a static ARP entry (SIS3316 Ethernet addendum, sec. 2.1). Use a network
interface dedicated to the board(s), with a fixed address and no gateway:

```bash
# PC side, e.g. with NetworkManager
sudo nmcli con add type ethernet ifname eno1 con-name sis3316 \
     ipv4.method manual ipv4.addresses 192.168.1.1/24 ipv4.never-default yes ipv6.method disabled
# the board: serial 108 = 0x06c -> 00:00:56:31:60:6c
sudo ip neigh replace 192.168.1.100 lladdr 00:00:56:31:60:6c dev eno1 nud permanent
```

The ARP entry does not survive a reboot; a NetworkManager dispatcher script
(`/etc/NetworkManager/dispatcher.d/`) running the same command when the
interface comes up makes it permanent. If the serial number is unknown, every
MAC from `00:00:56:31:60:00` to `00:00:56:31:6f:ff` can be mapped onto its own
IP and probed with a link-register read: the IP that answers gives the serial.

## Settings

`settings.py` defines the model (also what WebDAQ stores per board):

```python
{
  "board":            {...},   # board-wide: clock, readout, event window, waveforms, NIM I/O
  "channel_defaults": {...},   # applied to every channel
  "channels":         {"14": {"enabled": True, ...}},   # sparse per-channel overrides
}
```

Thresholds are in ADC counts of pulse height (the driver multiplies by the
trigger peaking time); times are in samples (4 ns at 250 MHz). `validate()`
clamps everything to what the hardware accepts and refuses unknown values;
`channel_settings(s, ch)` gives a channel's effective settings. Channels start
switched off. During a run, `DAQ.update_settings()` applies new settings at the
next bank swap; the clock and the waveform and filter-trace lengths, which
shape the run file, are refused until the run stops.

## Run files

`run_<N>_<cycle:04d>.h5`, rotated by size, flushed after every bank. Per board:

| Dataset | Content |
|---|---|
| `/board_<id>/events` | one row per hit: `channel`, `timestamp` (samples), `energy_max`, `energy_start`, `pileup`, `peak_high`, `peak_index`, `maw_max` |
| `/board_<id>/waveforms` | raw samples, row-aligned with `events` (when recorded) |
| `/board_<id>/maw` | filter (MAW) traces, row-aligned with `events` (when recorded) |

The board information and the full settings are attributes of `/board_<id>`.

```python
import h5py, json
with h5py.File("data/run7/run_7_0000.h5") as f:
    ev = f["board_3"]["events"][:]
    ch14 = ev[ev["channel"] == 14]
    energy = ch14["energy_max"] / 200          # / divider -> pulse height in ADC counts
    settings = json.loads(f["board_3"].attrs["settings"])
```

Rows are written bank by bank and channel by channel: sort by `timestamp` when
the order matters.

## Firmware notes

Measured on VME FPGA V3316-2004 and ADC FPGA A_0250_0004 (250 MHz, 14 bit), and
handled by the driver:

* **Legacy UDP protocol.** Firmware before V3316-2008 has no packet-identifier
  byte and does not answer ping. `UdpLink` detects the protocol generation when
  it opens.
* **Fixed reply port.** The board answers to UDP port `0xE000` whatever the
  source port, so the PC socket binds `0xE000`: one program per PC can talk to
  the board at a time.
* **No multi-register requests on ADC FPGA registers.** A read in a request
  that already read the same ADC FPGA returns that previous value, and writes
  land on the neighbouring register or are dropped. The transport sends one
  request per ADC FPGA register (`0x1000`–`0xFFFFF`) and batches only VME FPGA
  registers.
* **Timestamp-Clear clears neither the on-board energy histograms nor the
  trigger statistic counters.** The histograms are zeroed by a memory write at
  every start; the counters are reported relative to the run start.
* **Raw window.** With all 16 channels recording, windows near 64K samples
  corrupt the end of some events; `validate()` caps them at 32000.

## Tests

```bash
python -m unittest discover -s tests       # no hardware needed
```

## License

MIT, see `LICENSE`.
