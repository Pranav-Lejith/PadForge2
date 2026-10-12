#!/usr/bin/env bash
set -euo pipefail

workspace=/workspace
pkgbuild_dir="$workspace/packaging/arch"
repo_dir="$workspace/public/pacman/x86_64"

: "${PADFORGE_TAG:?Set PADFORGE_TAG to the release tag}"
: "${PADFORGE_SIGNING_KEY:?Set PADFORGE_SIGNING_KEY}"
: "${PADFORGE_SIGNING_PASSPHRASE:?Set PADFORGE_SIGNING_PASSPHRASE}"
: "${PADFORGE_SIGNING_FINGERPRINT:?Set PADFORGE_SIGNING_FINGERPRINT}"

version="${PADFORGE_TAG#v}"
if [[ ! "$version" =~ ^[0-9]+([.][0-9]+)*$ ]]; then
    echo "Unsupported PadForge release tag: $PADFORGE_TAG" >&2
    exit 1
fi
source_version="$(sed -n 's/^[[:space:]]*VERSION[[:space:]]\([0-9][0-9.]*\)$/\1/p' \
    "$workspace/CMakeLists.txt" | head -n1)"
if [[ "$source_version" != "$version" ]]; then
    echo "Release tag $PADFORGE_TAG does not match source version $source_version." >&2
    exit 1
fi

pacman -Sy --noconfirm archlinux-keyring
pacman -Syu --noconfirm --needed \
    base-devel cmake ninja qt6-base qt6-5compat lua54 \
    hicolor-icon-theme python git gnupg shadow

source_archive="$(mktemp)"
curl -fL \
    "https://github.com/Pranav-Lejith/PadForge2/archive/refs/tags/${PADFORGE_TAG}.tar.gz" \
    -o "$source_archive"
source_hash="$(sha256sum "$source_archive" | cut -d' ' -f1)"
unlink "$source_archive"

sed -i "s/^pkgver=.*/pkgver=${version}/" "$pkgbuild_dir/PKGBUILD"
sed -i "s/^sha256sums=.*/sha256sums=('${source_hash}')/" "$pkgbuild_dir/PKGBUILD"
useradd --create-home builder
chown -R builder:builder "$pkgbuild_dir"
runuser -u builder -- sh -c "cd '$pkgbuild_dir' && makepkg --printsrcinfo > .SRCINFO && makepkg --syncdeps --noconfirm --cleanbuild"

package_file="$(find "$pkgbuild_dir" -maxdepth 1 -type f \
    -name 'padforge-*.pkg.tar.zst' ! -name '*-debug-*' -print -quit)"
if [[ -z "$package_file" ]]; then
    echo 'makepkg did not produce the PadForge package.' >&2
    exit 1
fi

install -d "$repo_dir"
install -m644 "$package_file" "$repo_dir/"

gpg_home=/root/.gnupg
install -d -m700 "$gpg_home"
printf '%s\n' "$PADFORGE_SIGNING_KEY" | gpg --batch --import
actual_fingerprint="$(gpg --batch --with-colons --list-secret-keys \
    | awk -F: '$1 == "fpr" { print $10; exit }')"
if [[ "$actual_fingerprint" != "$PADFORGE_SIGNING_FINGERPRINT" ]]; then
    echo 'Imported repository signing key has an unexpected fingerprint.' >&2
    exit 1
fi

passphrase_file=/tmp/padforge-signing-passphrase
install -m600 /dev/null "$passphrase_file"
printf '%s' "$PADFORGE_SIGNING_PASSPHRASE" > "$passphrase_file"
trap 'unlink "$passphrase_file" 2>/dev/null || true' EXIT
printf 'PadForge signing key check\n' > /tmp/padforge-signing-check
gpg --batch --yes --pinentry-mode loopback \
    --passphrase-file "$passphrase_file" \
    --local-user "$PADFORGE_SIGNING_FINGERPRINT" \
    --detach-sign /tmp/padforge-signing-check
unlink /tmp/padforge-signing-check
unlink /tmp/padforge-signing-check.sig

gpg --batch --yes --pinentry-mode loopback \
    --passphrase-file "$passphrase_file" \
    --local-user "$PADFORGE_SIGNING_FINGERPRINT" \
    --detach-sign "$repo_dir/$(basename "$package_file")"

GPGKEY="$PADFORGE_SIGNING_FINGERPRINT" repo-add \
    --sign --include-sigs -k "$PADFORGE_SIGNING_FINGERPRINT" \
    "$repo_dir/padforge.db.tar.gz" "$repo_dir/$(basename "$package_file")"

# GitHub Pages serves regular files most consistently; publish the aliases that
# pacman requests as copies rather than symlinks.
for db in padforge.db padforge.files; do
    cp --remove-destination "$repo_dir/${db}.tar.gz" "$repo_dir/$db"
    cp --remove-destination "$repo_dir/${db}.tar.gz.sig" "$repo_dir/${db}.sig"
done

install -m644 "$workspace/packaging/arch/padforge.gpg" "$workspace/public/padforge.gpg"
install -m644 "$workspace/packaging/arch/padforge.conf" "$workspace/public/padforge.conf"
install -m644 "$workspace/packaging/arch/index.html" "$workspace/public/index.html"
