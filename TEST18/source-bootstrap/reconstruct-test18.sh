#!/usr/bin/env bash
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$here/../.." && pwd)"
base="$repo_root/TEST15/FM1-BassStation-TEST15-Project-Source.tar.xz"
out_dir="${1:-$PWD/FM1-BassStation-TEST18-Project-Source}"
out_archive="${2:-$PWD/FM1-BassStation-TEST18-Project-Source.tar.xz}"

test -f "$base" || {
  echo "Missing TEST15 base archive: $base" >&2
  exit 1
}

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

echo "Verifying TEST15 base..."
echo "d5762b8bdc3c63acce653563da9a357bb863aeb14670ca85b17c4b6bc444a332  $base" | sha256sum -c -

cat "$here/test16-overlay"/part_* | base64 -d > "$tmp/test16-overlay.tar.xz"
echo "d3c04a00b8338f5c13b97eaadce0b7833244569fa5868caec48548270b5199ee  $tmp/test16-overlay.tar.xz" | sha256sum -c -

cat "$here/test18-overlay"/part_* | base64 -d > "$tmp/test18-overlay.tar.xz"
echo "768c807a5cf01cd87414c585b92d2a80bf88fbcd1fa55dccba32c62f1b232374  $tmp/test18-overlay.tar.xz" | sha256sum -c -

rm -rf "$out_dir"
mkdir -p "$out_dir"

tar -xJf "$base" -C "$out_dir" --strip-components=1
tar -xJf "$tmp/test16-overlay.tar.xz" -C "$out_dir"
tar -xJf "$tmp/test18-overlay.tar.xz" -C "$out_dir"

# TEST18 project-source package intentionally omits generated/published release
# payloads and the temporary TEST16 community-bank staging files.
rm -rf "$out_dir/release" "$out_dir/community"
rm -f "$out_dir/app/community_bank.h"
rm -f "$out_dir/FM1-BassStation-TEST16-Package-Validation.json"

count="$(find "$out_dir" -type f | wc -l | tr -d ' ')"
test "$count" = "77" || {
  echo "Unexpected TEST18 source file count: $count (expected 77)" >&2
  exit 1
}

tree_hash="$(
  cd "$out_dir"
  find . -type f -print0 | sort -z | xargs -0 sha256sum | sha256sum | awk '{print $1}'
)"
test "$tree_hash" = "f18068d5ed54aa0e201305a56c401f8630f722e752042828e88fe8e0a3cb45e5" || {
  echo "TEST18 source tree hash mismatch: $tree_hash" >&2
  exit 1
}

rm -f "$out_archive"
tar -cJf "$out_archive" -C "$(dirname "$out_dir")" "$(basename "$out_dir")"

echo "TEST18 source reconstructed and verified."
echo "Source directory: $out_dir"
echo "Archive:          $out_archive"
echo "Tree SHA256:      $tree_hash"
