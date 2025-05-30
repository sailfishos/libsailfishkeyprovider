Name:    libsailfishkeyprovider
License: LGPLv2
URL:     https://github.com/sailfishos/libsailfishkeyprovider
Version: 1.0.1
Release: 1
Source0: %{name}-%{version}.tar.bz2
Summary: Library providing access to decoded OAuth2 keys
BuildRequires:  pkgconfig(Qt5Core)

%description
%{summary}.

%package devel
Summary:  Development package for libsailfishkeyprovider
Requires: %{name} = %{version}-%{release}

%description devel
%{summary}.

%package tests
Summary:  Tests for libsailfishkeyprovider
Requires: %{name} = %{version}-%{release}
Requires: blts-tools

%description tests
%{summary}.

%package keygen
Summary:  Encoding utility for API keys

%description keygen
Better than modify the source code of and then running unit tests.

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5 "VERSION=%{version}"
%make_build

%install
%qmake5_install

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%license license.lgpl
%{_libdir}/libsailfishkeyprovider.so.*

%files devel
%{_libdir}/libsailfishkeyprovider.so
%{_includedir}/libsailfishkeyprovider/sailfishkeyprovider.h
%{_includedir}/libsailfishkeyprovider/sailfishkeyprovider_iniparser.h
%{_includedir}/libsailfishkeyprovider/sailfishkeyprovider_processmutex.h
%{_libdir}/pkgconfig/libsailfishkeyprovider.pc

%files tests
/opt/tests/libsailfishkeyprovider/tst_keyprovider
/opt/tests/libsailfishkeyprovider/tests.xml

%files keygen
%{_bindir}/sailfish-keyprovider-keygen
