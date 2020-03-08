Name:		megazeux
Version:	2.92c
Release:	1%{?dist}

Summary:	A simple game creation system (GCS)

Group:		Amusements/Games
License:	GPLv2+
URL:		https://www.digitalmzx.com/
Source:		megazeux-2.92c.tar.xz
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root

BuildRequires:	SDL2-devel
BuildRequires:	libvorbis-devel
BuildRequires:	libpng-devel
BuildRequires:	zlib-devel

%description

MegaZeux is a Game Creation System (GCS), inspired by Epic MegaGames' ZZT,
first released by Alexis Janson in 1994. It has since been ported from 16-bit
MS DOS C/ASM to portable SDL C/C++, improving its platform support and
hardware compatibility.

Many features have been added since the original DOS version, and with the
help of an intuitive editor and a simple but powerful object-oriented
programming language, MegaZeux allows you to create your own ANSI-esque games
regardless of genre.

See https://www.digitalmzx.com/ for more information.

%prep
%setup -q -n mzx292c

%build
./config.sh --platform unix --enable-release --as-needed-hack \
            --libdir %{_libdir} \
            --gamesdir %{_bindir} \
            --sharedir %{_datadir} \
            --bindir %{_datadir}/megazeux/bin \
            --sysconfdir %{_sysconfdir}
%{__make}

%install
rm -rf "$RPM_BUILD_ROOT"
make install DESTDIR=${RPM_BUILD_ROOT}

%clean
rm -rf "$RPM_BUILD_ROOT"

%files
%defattr(-,root,root,-)
%doc docs/changelog.txt docs/macro.txt
%{_bindir}/megazeux
%{_bindir}/mzxrun
%{_libdir}/megazeux
%{_datadir}/megazeux
%{_datadir}/applications/megazeux.desktop
%{_datadir}/applications/mzxrun.desktop
%{_datadir}/doc/megazeux
%{_datadir}/icons/megazeux.png
%{_sysconfdir}/megazeux-config

%changelog
* Sun Mar 08 2020 Alice Rowan <petrifiedrowan@gmail.com> 2.92c-1
- new upstream version, fix outdated info

* Sat Dec 22 2012 Alistair John Strachan <alistair@devzero.co.uk> 2.84c-1
- new upstream version

* Sat Jun 02 2012 Alistair John Strachan <alistair@devzero.co.uk> 2.84-1
- new upstream version

* Sat Nov 26 2009 Alistair John Strachan <alistair@devzero.co.uk> 2.83-1
- new upstream version

* Thu Nov 27 2008 Alistair John Strachan <alistair@devzero.co.uk> 2.82b-1
- initial RPM release
