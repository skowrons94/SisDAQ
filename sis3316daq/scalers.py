"""
Scalers, live time and counter inputs of an SIS3316 run.

The board keeps six 32-bit trigger statistic counters per channel (manual
4.9), latched at every bank swap and read by the DAQ once per readout cycle:

    internal     every trigger of the channel's internal trigger logic
    hit          internal triggers that started a Hit/Event sampling
    deadtime     triggers in a time slot where the sample logic was not ready
                 to save a Hit/Event (bank swap, memory bank almost full,
                 writing the MAW test data, finishing the ADC data)
    pileup       triggers while the sample logic was busy with an event
    veto         triggers while a veto was valid
    high_energy  suppressed high-energy triggers

From them, per channel over the run:

    dead fraction = deadtime / internal
    live fraction = 1 - deadtime / internal

It is the share of the channel's own triggers that found the board ready, so
it is a count of triggers, not a measured time. It equals the live-time
fraction of the run when the triggers come at random times (a source, the
beam); with a periodic pulser it is the share of pulses that were lost. It
does not include triggers that fell inside an event being recorded (those are
pile-up, flagged in the event and counted in the pileup counter), triggers
suppressed by a veto, external triggers (not counted by the internal trigger
counter), or anything below the trigger threshold.

A counter input is a channel that receives logic pulses (a current
integrator's output, a clock): its internal trigger counter, times a scale,
is a quantity (e.g. 1 pulse = 10 pC of beam charge). The internal counter,
not the hit counter, because it counts every pulse whether or not the board
was ready to record it: the charge delivered does not stop during dead time.

Files written into a saved run's directory (one per run, all boards):

    sis3316_scalers.csv    every stats interval, one row per enabled channel
    sis3316_summary.json   the run totals, written at the stop
"""

import csv
import json
import math
import os
import re
from datetime import datetime
from typing import Any, Dict, List, Optional, Sequence

import numpy as np

from .registers import STATISTIC_COUNTER_NAMES as NAMES
from .settings import channel_settings

CSV_NAME = "sis3316_scalers.csv"
SUMMARY_NAME = "sis3316_summary.json"
SUMMARY_FORMAT = 1

CSV_COLUMNS = (["time", "elapsed_s", "board", "channel"] + list(NAMES)
               + ["events", "temperature_c", "counter_name", "counter_value", "counter_unit"])

LIVE_TIME_DEFINITION = (
    "live_fraction = 1 - deadtime / internal, from the board's trigger statistic "
    "counters (SIS3316 manual 4.9): the share of the channel's internal triggers "
    "that found the sample logic ready. A count of triggers, not a clock: it is "
    "the live-time fraction for randomly timed triggers. Pile-up inside an event, "
    "vetoed and external triggers are not dead time here.")

# The hit counter and the events written may differ by this much before the
# run is flagged: none are expected, but a few at most cost nothing to allow.
INTEGRITY_ABS = 2
INTEGRITY_REL = 0.0   # every bank is read and counted alike: no loss is normal

_INTERNAL, _HIT, _DEADTIME = 0, 1, 2


def dead_fraction(internal: float, deadtime: float) -> Optional[float]:
    """deadtime / internal, or None when the channel had no trigger."""
    if internal <= 0:
        return None
    return min(1.0, max(0.0, float(deadtime) / float(internal)))


def live_fraction(internal: float, deadtime: float) -> Optional[float]:
    d = dead_fraction(internal, deadtime)
    return None if d is None else 1.0 - d


def metric_name(name: str, channel: int) -> str:
    """A counter's name as one Graphite path element ("Beam charge" ->
    "Beam_charge"); the channel when nothing usable is left."""
    text = str(name or "").replace("µ", "u").replace("μ", "u")
    m = re.sub(r"[^A-Za-z0-9_-]+", "_", text).strip("_")
    return m or f"ch_{channel}"


def counter_inputs(settings: Dict[str, Any], enabled: Sequence[int]) -> List[Dict[str, Any]]:
    """The enabled channels marked as counter inputs, with name, scale, unit."""
    out = []
    for ch in enabled:
        cs = channel_settings(settings, ch)
        if not cs.get("counter_input"):
            continue
        name = str(cs.get("counter_name") or "").strip() or f"Counter ch {ch}"
        out.append({"channel": int(ch), "name": name,
                    "unit": str(cs.get("counter_unit") or "").strip(),
                    "scale": float(cs.get("counter_scale", 1.0)),
                    "metric": metric_name(name, ch)})
    return out


def check_integrity(hits: int, events: int) -> Optional[Dict[str, Any]]:
    """None when the board's hit count and the events written agree within the
    tolerance, else what differs."""
    diff = int(events) - int(hits)
    if abs(diff) <= INTEGRITY_ABS:
        return None
    return {"hits": int(hits), "events": int(events), "difference": diff,
            "relative": (diff / hits) if hits else None}


def _num(v: Optional[float], digits: int = 6) -> str:
    return "" if v is None or (isinstance(v, float) and math.isnan(v)) else f"{v:.{digits}g}"


class ScalerCsv:
    """The run's sis3316_scalers.csv, appended at every stats interval.
    Counts are cumulative from the run start (see board_summary for which
    banks they cover); one row per board and enabled channel."""

    def __init__(self, out_dir: str):
        os.makedirs(out_dir, exist_ok=True)
        self.path = os.path.join(out_dir, CSV_NAME)
        self._f = open(self.path, "w", newline="")
        self._w = csv.writer(self._f)
        self._w.writerow(CSV_COLUMNS)
        self._f.flush()

    def write(self, elapsed_s: float, board_id: int, channels: Sequence[int],
              totals: np.ndarray, events: np.ndarray, temperature: Optional[float],
              counters: Sequence[Dict[str, Any]]) -> None:
        now = datetime.now().isoformat(timespec="seconds")
        by_ch = {c["channel"]: c for c in counters}
        for ch in channels:
            c = by_ch.get(ch)
            self._w.writerow(
                [now, f"{elapsed_s:.3f}", board_id, ch] + [int(v) for v in totals[ch]]
                + [int(events[ch]), _num(temperature, 4)]
                + ([c["name"], _num(float(totals[ch][_INTERNAL]) * c["scale"], 10), c["unit"]]
                   if c else ["", "", ""]))
        self._f.flush()

    def close(self) -> None:
        try:
            self._f.close()
        except OSError:
            pass


def board_summary(board_id: int, name: str, channels: Sequence[int],
                  totals: np.ndarray, events_written: np.ndarray,
                  events_compared: np.ndarray, counted_s: float, duration_s: float,
                  uncounted_banks: int, temperature: Dict[str, Optional[float]],
                  counters: Sequence[Dict[str, Any]], settings: Dict[str, Any]) -> Dict[str, Any]:
    """The run totals of one board: per channel the six counters, dead and
    live fraction, and the hit-counter check against the events written."""
    chans, mismatches = [], []
    for ch in channels:
        t = [int(v) for v in totals[ch]]
        live = live_fraction(t[_INTERNAL], t[_DEADTIME])
        entry = dict(zip(NAMES, t))
        entry.update({
            "channel": int(ch),
            "events_written": int(events_written[ch]),
            "dead_fraction": dead_fraction(t[_INTERNAL], t[_DEADTIME]),
            "live_fraction": live,
            "live_time_s": None if live is None else live * counted_s,
        })
        cs = channel_settings(settings, ch)
        # External triggers record events the internal hit counter never saw,
        # so the two cannot be compared on such a channel.
        if cs.get("external_trigger"):
            entry["integrity"] = "not checked (external trigger)"
        elif not cs.get("internal_trigger", True):
            entry["integrity"] = "not checked (internal trigger off)"
        else:
            bad = check_integrity(t[_HIT], int(events_compared[ch]))
            entry["integrity"] = "ok" if bad is None else "mismatch"
            if bad is not None:
                bad["channel"] = int(ch)
                mismatches.append(bad)
        chans.append(entry)
    cin = []
    for c in counters:
        n = int(totals[c["channel"]][_INTERNAL])
        cin.append(dict(c, counts=n, value=n * c["scale"],
                        mean_rate=(n * c["scale"] / counted_s) if counted_s > 0 else None))
    return {"board_id": int(board_id), "name": name,
            "duration_s": round(float(duration_s), 3), "counted_s": round(float(counted_s), 3),
            "uncounted_banks": int(uncounted_banks),
            "temperature_c": temperature, "channels": chans, "counter_inputs": cin,
            "integrity": {"ok": not mismatches, "mismatches": mismatches,
                          "tolerance": {"absolute": INTEGRITY_ABS, "relative": INTEGRITY_REL}}}


def write_summary(out_dir: str, run: int, software: str, boards: List[Dict[str, Any]]) -> str:
    path = os.path.join(out_dir, SUMMARY_NAME)
    doc = {"format_version": SUMMARY_FORMAT, "run": int(run), "software": software,
           "written": datetime.now().isoformat(timespec="seconds"),
           "counter_names": list(NAMES), "live_time_definition": LIVE_TIME_DEFINITION,
           "boards": boards}
    tmp = path + ".tmp"
    with open(tmp, "w") as f:
        json.dump(doc, f, indent=2)
    os.replace(tmp, path)
    return path


def integrity_note(summary: Dict[str, Any]) -> Optional[str]:
    """One line for the run's notes when a board's hit counters and the events
    written disagree, else None."""
    parts = []
    for b in summary.get("boards", []):
        bad = b.get("integrity", {}).get("mismatches") or []
        if bad:
            chans = ", ".join(f"ch {m['channel']}: {m['hits']} hits, {m['events']} events"
                              for m in bad)
            parts.append(f"board {b['board_id']} ({chans})")
    if not parts:
        return None
    return ("[sis3316] The board counted a different number of hits than events were "
            "written: " + "; ".join(parts) + ". Events may be missing from the files.")
