%define desktop_vendor niftools

Name:           nifskope
Version:        1.0.3
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
rm -rf $RPM_BUILD_ROOT
%{__install} -Dp -m0755 nifskope $RPM_BUILD_ROOT/%{_bindir}/nifskope
%{__install} -Dp -m0644 nifskope.png $RPM_BUILD_ROOT/%{_datadir}/pixmaps/nifskope.png
%{__install} -d $RPM_BUILD_ROOT/%{_datadir}/nifskope/doc
%{__install} -d $RPM_BUILD_ROOT/%{_datadir}/nifskope/shaders
%{__install} -Dp -m0644 nif.xml $RPM_BUILD_ROOT/%{_datadir}/nifskope/nif.xml
%{__install} -Dp -m0644 kfm.xml $RPM_BUILD_ROOT/%{_datadir}/nifskope/kfm.xml
%{__install} -Dp -m0644 shaders/*.frag $RPM_BUILD_ROOT/%{_datadir}/nifskope/shaders
%{__install} -Dp -m0644 shaders/*.prog $RPM_BUILD_ROOT/%{_datadir}/nifskope/shaders
%{__install} -Dp -m0644 shaders/*.vert $RPM_BUILD_ROOT/%{_datadir}/nifskope/shaders
%{__install} -Dp -m0644 doc/*.html $RPM_BUILD_ROOT/%{_datadir}/nifskope/doc
%{__install} -Dp -m0644 doc/docsys.css $RPM_BUILD_ROOT/%{_datadir}/nifskope/doc
%{__install} -Dp -m0644 doc/favicon.ico $RPM_BUILD_ROOT/%{_datadir}/nifskope/doc

%if %{?_without_freedesktop:1}0
        %{__install} -Dp -m0644 nifskope.desktop %{buildroot}%{_datadir}/gnome/apps/Multimedia/nifskope.desktop
%else
	%{__install} -d -m0755 %{buildroot}%{_datadir}/applications/
	desktop-file-install --vendor %{desktop_vendor}    \
		--add-category X-Red-Hat-Base              \
		--dir %{buildroot}%{_datadir}/applications \
		nifskope.desktop
%endif

%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%doc README.TXT CHANGELOG.TXT
%{_bindir}/nifskope
%{_datadir}/pixmaps/nifskope.png
%{_datadir}/nifskope/nif.xml
%{_datadir}/nifskope/kfm.xml
%{_datadir}/nifskope/shaders/*.vert
%{_datadir}/nifskope/shaders/*.frag
%{_datadir}/nifskope/shaders/*.prog
%{_datadir}/nifskope/doc/*.html
%{_datadir}/nifskope/doc/docsys.css
%{_datadir}/nifskope/doc/favicon.ico
%{?_without_freedesktop:%{_datadir}/gnome/apps/Multimedia/nifskope.desktop}
%{!?_without_freedesktop:%{_datadir}/applications/%{desktop_vendor}-nifskope.desktop}

%changelog
* Sat Nov 10 2007 amorilia - 1.0.3-1
* Sat Nov 10 2007 amorilia - 1.0.2-1
- Small bugs fixed.
- Including shaders.
* Sun Oct 21 2007 amorilia - 1.0.1-1
- Initial package.
