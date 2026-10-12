#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
rpm_topdir="${RPM_TOPDIR:-${repo_root}/build/rpmbuild}"
version="$(sed -nE 's/^[[:space:]]*VERSION[[:space:]]+([0-9][0-9.]*)$/\1/p' \
    "${repo_root}/CMakeLists.txt" | head -n 1)"

if [[ -z "${version}" ]]; then
    echo "Could not determine PadForge version from CMakeLists.txt" >&2
    exit 1
fi
if ! git -c safe.directory="${repo_root}" -C "${repo_root}" rev-parse --verify HEAD >/dev/null 2>&1; then
    echo "Run this script from a Git checkout so the source snapshot is reproducible" >&2
    exit 1
fi

mkdir -p "${rpm_topdir}"/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}
git -c safe.directory="${repo_root}" -C "${repo_root}" archive --format=tar \
    --prefix="PadForge2-${version}/" HEAD |
    gzip -n > "${rpm_topdir}/SOURCES/padforge-${version}.tar.gz"
download_source() {
    local url="$1"
    local destination="$2"
    if [[ ! -f "${destination}" ]]; then
        curl -fL --retry 3 --connect-timeout 20 "${url}" -o "${destination}"
    fi
}
download_source \
    https://downloads.sourceforge.net/project/scintilla/scintilla/5.6.7/scintilla567.tgz \
    "${rpm_topdir}/SOURCES/scintilla567.tgz"
download_source \
    https://downloads.sourceforge.net/project/scintilla/lexilla/5.5.4/lexilla554.tgz \
    "${rpm_topdir}/SOURCES/lexilla554.tgz"
printf '%s  %s\n' \
    19e4476706750056f86ca70974810e4a5b08e73477e13f7d9fae78243606f6a7 \
    "${rpm_topdir}/SOURCES/scintilla567.tgz" \
    fcd9ef74236a12359d1039827c93dbf827aa476c46f9fd6eef60bb96db1af2f6 \
    "${rpm_topdir}/SOURCES/lexilla554.tgz" | sha256sum --check
cp "${repo_root}/packaging/rpm/padforge.spec" "${rpm_topdir}/SPECS/padforge.spec"

rpmbuild -ba "${rpm_topdir}/SPECS/padforge.spec" \
    --define "_topdir ${rpm_topdir}" \
    --define "_sourcedir ${rpm_topdir}/SOURCES"

mkdir -p "${repo_root}/dist"
cp "${rpm_topdir}/RPMS/$(uname -m)"/padforge-*.rpm "${repo_root}/dist/"
cp "${rpm_topdir}/SRPMS"/padforge-*.src.rpm "${repo_root}/dist/"
printf 'Built RPMs in %s/dist\n' "${repo_root}"
