Name:    libsailfishkeyprovider
License: LGPLv2
URL:     https://git.sailfishos.org/mer-core/libsailfishkeyprovider
Version: 1.0.1
Release: 1
Source0: %{name}-%{version}.tar.bz2
Summary: Library providing access to decoded OAuth2 keys
BuildRequires:  qt5-qmake

%description
%{summary}.

%files
%{_libdir}/libsailfishkeyprovider.so.*

%package devel
Summary:  Development package for libsailfishkeyprovider
Group:    System/Libraries
Requires: %{name} = %{version}-%{release}

%description devel
%{summary}.

%files devel
%{_libdir}/libsailfishkeyprovider.so
%{_includedir}/libsailfishkeyprovider/sailfishkeyprovider.h
%{_includedir}/libsailfishkeyprovider/sailfishkeyprovider_iniparser.h
%{_includedir}/libsailfishkeyprovider/sailfishkeyprovider_processmutex.h
%{_libdir}/pkgconfig/libsailfishkeyprovider.pc

%package tests
Summary:  Tests for libsailfishkeyprovider
Group:    System/Libraries
Requires: %{name} = %{version}-%{release}
Requires: blts-tools

%description tests
%{summary}.

%files tests
/opt/tests/libsailfishkeyprovider/tst_keyprovider
/opt/tests/libsailfishkeyprovider/tests.xml

%prep
%setup -q -n %{name}-%{version}


%package keygen
Summary:  Encoding utility for API keys

%description keygen
Better than modify the source code of and then running unit tests.

%files keygen
%{_bindir}/sailfish-keyprovider-keygen

%build
%qmake5 "VERSION=%{version}"
make

%install
%qmake5_install

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig
