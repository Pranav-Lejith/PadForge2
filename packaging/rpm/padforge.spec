Name:           padforge
Version:        0.1.0
Release:        1%{?dist}
Summary:        Fast native text editor with a Notepad++-style workflow

License:        GPL-3.0-or-later
URL:            https://github.com/Pranav-Lejith/PadForge2
Source0:        padforge-%{version}.tar.gz
Source1:        https://downloads.sourceforge.net/project/scintilla/scintilla/5.6.7/scintilla567.tgz
Source2:        https://downloads.sourceforge.net/project/scintilla/lexilla/5.5.4/lexilla554.tgz

BuildRequires:  cmake
BuildRequires:  ninja-build
BuildRequires:  gcc-c++
BuildRequires:  python3
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qt5compat-devel
BuildRequires:  lua-devel
BuildRequires:  desktop-file-utils
Requires:       hicolor-icon-theme

%description
PadForge is a native Linux text editor that brings the familiar Notepad++
workflow to the Qt 6 and Scintilla stack: tabbed documents, split view, syntax
highlighting for dozens of languages, find and replace with find-in-files,
bookmarks, macros and a small plugin runtime for Lua and native plugins.

%prep
%autosetup -n PadForge2-%{version}
echo '19e4476706750056f86ca70974810e4a5b08e73477e13f7d9fae78243606f6a7  %{SOURCE1}' | sha256sum --check -
echo 'fcd9ef74236a12359d1039827c93dbf827aa476c46f9fd6eef60bb96db1af2f6  %{SOURCE2}' | sha256sum --check -
mkdir -p _sources/scintilla _sources/lexilla
tar -xzf %{SOURCE1} --strip-components=1 -C _sources/scintilla
tar -xzf %{SOURCE2} --strip-components=1 -C _sources/lexilla

%build
%cmake -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DFETCHCONTENT_SOURCE_DIR_SCINTILLA=%{_builddir}/PadForge2-%{version}/_sources/scintilla \
    -DFETCHCONTENT_SOURCE_DIR_LEXILLA=%{_builddir}/PadForge2-%{version}/_sources/lexilla \
    -DFETCHCONTENT_FULLY_DISCONNECTED=ON
%cmake_build

%check
QT_QPA_PLATFORM=offscreen %ctest --output-on-failure

%install
%cmake_install
desktop-file-validate %{buildroot}%{_datadir}/applications/org.padforge.PadForge.desktop
mkdir -p %{buildroot}%{_datadir}/PadForge/plugins
cp -r plugins/lua/. %{buildroot}%{_datadir}/PadForge/plugins/

%files
%license LICENSE
%{_bindir}/padforge
%{_datadir}/applications/org.padforge.PadForge.desktop
%{_datadir}/icons/hicolor/scalable/apps/org.padforge.PadForge.svg
%{_metainfodir}/org.padforge.PadForge.metainfo.xml
%{_datadir}/PadForge/plugins/

%changelog
* Mon Oct 12 2026 PadForge contributors <padforge@users.noreply.github.com> - 0.1.0-1
- Initial development release.
