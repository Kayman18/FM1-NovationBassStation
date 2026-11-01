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

TEST9 also includes tooling for importing compatible user/community banks. The
Rekkerd and Rhythm Lab archive URLs found during research are recorded under
`research/`, but those archive bytes are not preloaded because the browser-to-
build binary handoff could not materialize their ZIP/RAR payloads in this
session. This is a technical limitation, not a compatibility exclusion.

The FM1 base platform, SDK/toolchain components, fonts and helper code retain
their own notices/licenses in their respective directories.
