# Fedora RPM and COPR

Build an RPM and source RPM on Fedora 44 (or in a Fedora 44 container) with:

```bash
sudo dnf install rpm-build cmake ninja-build gcc-c++ python3 \
  qt6-qtbase-devel qt6-qt5compat-devel lua-devel desktop-file-utils git curl gzip
packaging/rpm/build-rpm.sh
```

The script packages the checked-out Git commit, verifies the pinned Scintilla
and Lexilla archives, runs the RPM build and test phases, and writes both RPMs
to `dist/`.

## COPR publishing

The `Publish Fedora package to COPR` GitHub Actions workflow submits an SRPM
for tagged releases. To enable publishing:

1. Create a COPR project and choose the Fedora chroots it should build for.
2. Add the contents of your COPR API configuration file as the GitHub Actions
   repository secret `COPR_CONFIG`.
3. Add a repository variable `COPR_PROJECT` with the value `owner/project`.

The workflow can also be run manually by specifying `owner/project`. It
submits the SRPM without waiting for the remote build; COPR reports the build
result on the project page. Do not put the API configuration in the repository.
