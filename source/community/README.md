# Bass Station community patches

TEST16 includes a read-only COMMUNITY bank containing 38 distinct Bass Station plug-in patches recovered losslessly from two freely downloadable Rekkerd community banks:

- **Old School Jungle** — Chopper: 5 patches.
- **BassStation Leads** — Pro-Sounds: 33 non-default patches after removing the category marker and Initial Program filler.

The original patch archives are published by Rekkerd with author permission. The firmware stores only the original 34-byte Bass Station native patch records and display names; USER storage remains untouched.

Sources:
- https://rekkerd.org/patches/plug-in/bass-station/
- https://rekkerd.org/bin/presets/bass_station_jungle_.zip
- https://rekkerd.org/bin/presets/ProSounds_BassStation_Leads.zip

Source archive SHA-256:

- `Old-School-Jungle.zip`: `bf02aa8e0e3a0d938b66304b379079414aa4dd0127b6c4c7a089fab89b9eeb7a`
- `ProSounds-BassStation-Leads.zip`: `6da7231abffda6232f00828589e7cc01027b310f3af6401c0e65054ff25b3cda`

The original archives are not required at runtime; TEST16 embeds only the selected native patch records and names in `app/community_bank.h`.
