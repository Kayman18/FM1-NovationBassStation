# Publishing firmware

Keep the browsable source in `source/` and update it in the same commit as each new firmware build. Build the new firmware and source archive from that source, verify their checksums, and update the README to link to the new files.

Keep only the current build on `main`. In each release commit, replace the previous build's firmware, recovery image, source archive, and web editor ZIP. Replace the corresponding release notes, checksums, validation reports, and release-specific workflow as well. Retain the SDK binaries and reference image under `source/` that the build uses.
