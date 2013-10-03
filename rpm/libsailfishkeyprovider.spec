Name: libsailfishkeyprovider
License: LGPLv2.1
Version: 0.0.10
Release: 1
Source0: %{name}-%{version}.tar.bz2
Summary: Library providing access to decoded OAuth2 keys
Group:   System/Libraries
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
%{_libdir}/pkgconfig/libsailfishkeyprovider.pc

%package tests
Summary:  Tests for libsailfishkeyprovider
Group:    System/Libraries
Requires: %{name} = %{version}-%{release}

%description tests
%{summary}.

%files tests
/opt/tests/libsailfishkeyprovider/tst_keyprovider
/opt/tests/libsailfishkeyprovider/tests.xml

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5
make

%install
%qmake5_install

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%post devel
/sbin/ldconfig

%postun devel
/sbin/ldconfig
