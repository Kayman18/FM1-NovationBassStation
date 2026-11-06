# Third-party / provenance notes

The Bass Station FM1 application and DSP in `app/` and `engine/` are new
clean-room code for this project unless another file states otherwise.

The implementation was informed by publicly documented Novation Bass Station /
Bass Station Rack behavior and historical compatibility/editor material. The
proprietary Novation plug-in executable and proprietary plug-in source code are
not included.

TEST9 contains recovered **factory preset parameter/state records** for the
classic Bass Station plug-in, generated from publicly available preset-state
material. `research/BassStationStereo-Factory.pchk` is retained as recovery
evidence and `research/extract_plugin_factory_bank.py` regenerates the compact
`app/factory_bank.h` used by the firmware. These records are patch data, not the
Novation executable or DSP source.

TEST9 also includes tooling for importing compatible user/community banks.
TEST21 additionally embeds 38 native 34-byte preset records recovered from two
freely downloadable Rekkerd Bass Station banks: Old School Jungle by Chopper
(5 patches) and BassStation Leads by Pro-Sounds (33 useful patches after
removing filler/category entries). Rekkerd states its patch downloads are
provided with author permission. Attribution, source URLs, and source archive
hashes are retained in `community/README.md`.

The remaining USER slots are seeded from recovered factory-program records
already present in this source tree; no Rhythm Lab archive bytes are bundled.

The FM1 base platform, SDK/toolchain components, fonts and helper code retain
their own notices/licenses in their respective directories.

## Animatek ATEK303 SEQ behavior reference (TEST17)

TEST17's integrated 303-style sequencer was written from scratch from the
public behavior documented for Animatek ATEK303 SEQ. No ATEK303SEQ source code
is included. ATEK303 SEQ itself is GPL-3.0-or-later; its public manual and VCV
Library page are listed in `research/ATEK303SEQ-INTEGRATION.md` for provenance.
