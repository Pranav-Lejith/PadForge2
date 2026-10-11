Name:           padforge
Version:        0.1.0
Release:        1%{?dist}
Summary:        Fast native text editor with a Notepad++-style workflow

License:        GPL-3.0-or-later
URL:            https://github.com/padforge/padforge
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  ninja-build
BuildRequires:  gcc-c++
BuildRequires:  python3
BuildRequires:  qt6-qtbase-devel
BuildRequires:  lua-devel
Requires:       qt6-qtbase
Requires:       hicolor-icon-theme

%description
PadForge is a native Linux text editor that brings the familiar Notepad++
workflow to the Qt 6 and Scintilla stack: tabbed documents, split view, syntax
highlighting for dozens of languages, find and replace with find-in-files,
bookmarks, macros and a small plugin runtime for Lua and native plugins.

%prep
%autosetup -n %{name}-%{version}

%build
%cmake -DCMAKE_BUILD_TYPE=Release
%cmake_build

%check
QT_QPA_PLATFORM=offscreen %ctest --output-on-failure

%install
%cmake_install
mkdir -p %{buildroot}%{_datadir}/PadForge/plugins
cp -r plugins/lua/. %{buildroot}%{_datadir}/PadForge/plugins/

%files
%license LICENSE
%{_bindir}/padforge
%{_datadir}/applications/org.padforge.PadForge.desktop
%{_datadir}/icons/hicolor/scalable/apps/org.padforge.PadForge.svg
%{_datadir}/metainfo/org.padforge.PadForge.metainfo.xml
%{_datadir}/PadForge/plugins/

%changelog
* Sun Oct 11 2026 PadForge contributors - 0.1.0-1
- Initial development release.
