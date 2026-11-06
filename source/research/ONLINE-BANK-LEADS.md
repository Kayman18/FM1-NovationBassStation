# TEST11 online Bass Station bank sources

TEST11 now accepts compatible original Bass Station Rack SysEx and classic
Bass Station plug-in `tffp` state banks through `tools/prepare_bassstation_bank.py`.
Community/user material is kept in USER slots and is never labelled as factory.

## Community banks located

### Rekkerd

Original Bass Station plug-in banks located on Rekkerd include:

- Old School Jungle / Chopper
  `https://rekkerd.org/bin/presets/bass_station_jungle_.zip`
- ProSounds BassStation Leads
  `https://rekkerd.org/bin/presets/ProSounds_BassStation_Leads.zip`

### Rhythm Lab

Rhythm Lab describes an original Novation BassStation collection as **7 banks
and 108 single patches**:

`https://rhythm-lab.com/sstorage/44/2015/03/rhythm-lab.com_novation_bass_station.rar`

Compatible community banks are desirable for the separate USER/COMMUNITY bank,
but redistribution still requires usable archive bytes and suitable permission.
The remaining limitation in this build session is technical: the browsing bridge exposes the archive URLs/pages but does not
materialize ZIP/RAR binary payloads into the firmware build container. Therefore
these archive bytes are not falsely claimed as preloaded in TEST11.

Once the actual archive is supplied locally, the TEST11 importer can scan ZIP
containers directly and can import Rack frames or classic plug-in `tffp` state
records without Rack-range clamping. RAR must first be unpacked by an external
RAR-capable utility, after which its contained files can be passed to the importer.

## Official / historical recovery sources

- Novation original Bass Station keyboard restore material remains useful for
  cross-checking older keyboard dumps.
- Historical Bass Station Rack editor source confirms the 76-byte Rack single
  program wire format used by TEST11.
- TEST11 no longer depends on the Rack ROM to obtain the plug-in factory bank: the
  complete 100-program plug-in `tffp` bank has been recovered directly.
