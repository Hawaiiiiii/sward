#!/usr/bin/env python3
"""
parse_game_db.py — turn Sonic Unleashed's OWN front-end database (the SurfRide
flag/sequence state machine extracted from #Application) into structured JSON the
viewer can use as the authoritative source of states + flow.

Reads the extracted #Application dir (default C:/swardbuild/xapp) and emits, next to
the viewer's manifests, three artifacts:
  db/flags.json   — every state variable (FlagList.xml): name, type, enum items, comment
  db/stages.json  — the stage catalog (SR_Enter*.seq.xml): id, stageType, country,
                    isEvil, archive, setting, presence, the ChangeStage payload + units
  db/nav.json     — the flag-conditioned router (GoTo*.seq.xml): ordered cases of
                    {guards:[{flag,op,value}], target} + default + prelude units

This is the ground truth the hand-made contracts.js was approximating.
"""
from __future__ import annotations
import json, os, re, sys
import xml.etree.ElementTree as ET

DB_DIR = sys.argv[1] if len(sys.argv) > 1 else r"C:\swardbuild\xapp"
HERE = os.path.dirname(os.path.abspath(__file__))
OUT_DIR = os.path.join(HERE, "..", "db")


def load_xml(path):
    with open(path, "r", encoding="utf-8-sig") as f:
        text = f.read()
    text = re.sub(r"<!--.*?-->", "", text, flags=re.S)   # strip comments (keep us out of ET comment nodes)
    try:
        return ET.fromstring(text)
    except ET.ParseError:
        return None


def comment_before(path, flagname):
    """Recover the JP dev comment that precedes a <Flag>'s <name> (best-effort)."""
    return None  # comments are stripped for parsing; flags.json keeps raw names (authoritative)


# ---- FlagList.xml -> flags.json ----
def parse_flags():
    p = os.path.join(DB_DIR, "FlagList.xml")
    if not os.path.exists(p):
        return []
    # keep comments this time so we can attach meanings
    with open(p, "r", encoding="utf-8-sig") as f:
        raw = f.read()
    flags = []
    # each <Flag type="..."> ... <name>NAME<item>..</item>..</name> ... </Flag>,
    # optionally preceded by <!--comment-->
    for m in re.finditer(r"(?:<!--(?P<c>.*?)-->\s*)?<Flag\s+type=\"(?P<t>[^\"]+)\">(?P<body>.*?)</Flag>", raw, flags=re.S):
        body = m.group("body")
        nm = re.search(r"<name>([^<]+)", body)
        if not nm:
            continue
        name = nm.group(1).strip()
        items = re.findall(r"<item>([^<]+)</item>", body)
        cat = ("progress" if "StgClear" in name or "BossClear" in name else
               "event" if "Event" in name else
               "town/time" if "Town" in name or "TimeState" in name else
               "mission" if "Mission" in name or "Trial" in name else
               "unlock" if "Open" in name or "Rescue" in name or "Visit" in name else
               "encyclopedia" if "LookCheck" in name else "system")
        flags.append({"name": name, "type": m.group("t"),
                      "items": items or None, "comment": (m.group("c") or "").strip() or None,
                      "category": cat})
    return flags


# ---- SR_Enter*.seq.xml -> stages.json ----
def text_of(el, tag):
    c = el.find(tag)
    return c.text.strip() if c is not None and c.text else None


def parse_stages():
    stages = []
    for fn in sorted(os.listdir(DB_DIR)):
        if not (fn.startswith("SR_Enter") and fn.endswith(".seq.xml")):
            continue
        root = load_xml(os.path.join(DB_DIR, fn))
        if root is None:
            continue
        sid = fn[:-len(".seq.xml")]
        units, change = [], None
        for su in root.iter("SequenceUnit"):
            t = text_of(su, "type")
            if t:
                units.append(t)
            if t == "ChangeStage":
                prm = su.find("param")
                if prm is not None:
                    change = {
                        "stageType": text_of(prm, "StageType"),
                        "country": text_of(prm, "CountryName"),
                        "archive": text_of(prm, "ArchiveName"),
                        "setting": text_of(prm, "SettingName"),
                        "isEvil": (text_of(prm, "IsEvil") or "").lower() == "true",
                        "append": text_of(prm, "AppendArchive"),
                    }
        if change:
            stages.append({"id": sid, **change, "units": units})
    return stages


# ---- GoTo*.seq.xml -> nav.json (flatten nested Switch as AND-guards) ----
def first_target(el):
    """The sequence/stage a branch dispatches to: a nested MicroSequence FileName,
    or a direct ChangeStage StageType."""
    for su in el.findall("SequenceUnit"):
        t = text_of(su, "type")
        prm = su.find("param")
        if t == "MicroSequence" and prm is not None:
            fn = text_of(prm, "FileName")
            if fn:
                return fn
        if t == "ChangeStage" and prm is not None:
            return "ChangeStage:" + (text_of(prm, "StageType") or "?")
    return None


def walk_switch(el, guards, cases):
    """Recursively flatten <Switch><Case flag op value>..</Case><Default>..</Default>."""
    sw = el.find("Switch")
    if sw is None:
        tgt = first_target(el)
        if tgt:
            cases.append({"guards": list(guards), "target": tgt})
        return
    for case in sw.findall("Case"):
        g = guards + [{"flag": case.get("flag"), "op": case.get("operation"), "value": case.get("value")}]
        if case.find("Switch") is not None:
            walk_switch(case, g, cases)
        else:
            tgt = first_target(case)
            cases.append({"guards": g, "target": tgt})
    dflt = sw.find("Default")
    if dflt is not None:
        if dflt.find("Switch") is not None:
            walk_switch(dflt, guards, cases)
        else:
            tgt = first_target(dflt)
            cases.append({"guards": list(guards), "target": tgt, "default": True})


def parse_nav():
    nav = []
    for fn in sorted(os.listdir(DB_DIR)):
        if not (fn.startswith("GoTo") and fn.endswith(".seq.xml")):
            continue
        root = load_xml(os.path.join(DB_DIR, fn))
        if root is None:
            continue
        cases = []
        walk_switch(root, [], cases)
        # prelude units that run before/around the switch (SetFlag/AutoSave/SwapDisk)
        prelude = [text_of(su, "type") for su in root.findall("SequenceUnit") if text_of(su, "type")]
        nav.append({"id": fn[:-len(".seq.xml")], "cases": cases, "prelude": prelude})
    return nav


def screen_for(stage):
    """Map a game stage to the viewer screen (manifest id) that represents it."""
    st, arc = stage.get("stageType"), (stage.get("archive") or "")
    if st == "Title": return "title"
    if st == "Menu": return "world_map"
    if st == "Ending": return "end"
    if st == "SelectStage": return "gate"          # stage-select frontend
    if st == "Extra": return "exstage"
    if st == "Labo": return "mediaroom"            # Tails lab / media room
    if st in ("Town", "ETF"): return "town"
    if st == "Action":
        if arc.startswith("Boss"): return "boss"
        return "sonic_hud"                          # day/werehog both -> HUD (split later)
    return None


def main():
    if not os.path.isdir(DB_DIR):
        print("DB dir not found:", DB_DIR); return 1
    os.makedirs(OUT_DIR, exist_ok=True)
    flags = parse_flags()
    stages = parse_stages()
    nav = parse_nav()
    for s in stages:
        s["screen"] = screen_for(s)
    for name, data in (("flags", flags), ("stages", stages), ("nav", nav)):
        with open(os.path.join(OUT_DIR, name + ".json"), "w", encoding="utf-8") as f:
            json.dump(data, f, ensure_ascii=False, separators=(",", ":"))
    # also emit a single loadable JS global for the viewer (synchronous, like contracts.js)
    js = ("/* game-db.js - Sonic Unleashed's OWN front-end state DB, generated by\n"
          "   tools/parse_game_db.py from #Application (FlagList + the SR_Enter and GoTo seqs).\n"
          "   window.SWARD_GAMEDB = flags, stages, nav - the authoritative states + flow. */\n"
          "window.SWARD_GAMEDB = " +
          json.dumps({"flags": flags, "stages": stages, "nav": nav}, ensure_ascii=False, separators=(",", ":")) + ";\n")
    with open(os.path.join(HERE, "..", "game-db.js"), "w", encoding="utf-8") as f:
        f.write(js)
    ui = [s for s in stages if s["stageType"] in ("Title", "Menu", "SelectStage", "EntryPoint", "Logo", "Ending")]
    print(f"flags.json  : {len(flags)} state variables")
    print(f"stages.json : {len(stages)} stages ({len(ui)} UI/frontend) — types: " +
          ", ".join(sorted({s['stageType'] or '?' for s in stages})))
    print(f"nav.json    : {len(nav)} flag-conditioned routers, "
          f"{sum(len(n['cases']) for n in nav)} total transition cases")
    print("-> " + os.path.relpath(OUT_DIR, HERE))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
