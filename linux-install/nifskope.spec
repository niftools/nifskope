Name:           nifskope
Version:        1.0.1
Release:        1%{?dist}
Summary:        A tool for analyzing and editing NetImmerse/Gamebryo files

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
qmake-qt4 -after TARGET=nifskope
make %{?_smp_mflags}


%install
%{__install} -Dp -m0755 nifskope $RPM_BUILD_ROOT/%{_bindir}/nifskope
%{__install} -Dp -m0644 nifskope.png $RPM_BUILD_ROOT/%{_datadir}/pixmaps/nifskope.png
%{__install} -Dp -m0644 nifskope.desktop $RPM_BUILD_ROOT/%{_datadir}/gnome/apps/Multimedia/nifskope.desktop

%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%doc README.TXT CHANGELOG.TXT
%doc doc/*.html
%doc doc/docsys.css
%doc doc/favicon.ico
%{_bindir}/nifskope
%{_datadir}/pixmaps/nifskope.png
%{_datadir}/gnome/apps/Multimedia/nifskope.desktop

%changelog
* Sun Oct 21 2007 amorilia - 1.0.1-1
- Initial package.
