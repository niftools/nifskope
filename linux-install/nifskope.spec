Name:           nifskope
Version:        1.0.1
Release:        1%{?dist}
Summary:        A tool for analyzing and editing NetImmerse/Gamebryo files.

Group:          Applications/Multimedia
License:        BSD
URL:            http://niftools.sourceforge.net
Source0:        nifskope-%{version}.tar.bz2
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  qt4-devel
Requires:       qt4

%description
NifSkope is a tool for analyzing and editing NetImmerse/Gamebryo files.

%prep
%setup -q


%build
qmake-qt4 TARGET=nifskope
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%doc README.TXT CHANGELOG.TXT
%{_bindir}/nifskope


%changelog
* Sun Oct 21 2007 amorilia - 1.0.1-1
- Initial package.
