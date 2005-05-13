# Spec file for Chord

Summary: OKWS: The OK Web Server
Name: okws
Version: 0.1
Release: 1
Copyright: BSD 
Group: Applications/Internet
Source: http://www.okws.org/dist/okws-0.1.tar.gz
URL: http://www.okws.org
Packager: Jeremy Stribling (strib@pdos.lcs.mit.edu)
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot
Requires: sfs >= 0.8, db4 >= 4.0
BuildRequires: sfs >= 0.8

%description
Lightweight, extensible, secure Web server, built on the 
SFS async libraries.

%prep
%setup -q
%configure

%build
make

%install
rm -rf $RPM_BUILD_ROOT
make install-strip DESTDIR=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%pre
%post


%files
%defattr(-,root,root)
%{_bindir}/okld
%{_bindir}/okdbg
%{_bindir}/okmgr
%{_bindir}/pub
%{_bindir}/txarpcc
%{_includedir}/okws
%{_includedir}/okws-%{version}/abuf.h
%{_includedir}/okws-%{version}/ahparse.h
%{_includedir}/okws-%{version}/ahttp.h
%{_includedir}/okws-%{version}/ahutil.h
%{_includedir}/okws-%{version}/amt.h
%{_includedir}/okws-%{version}/aparse.h
%{_includedir}/okws-%{version}/axprtfd.h
%{_includedir}/okws-%{version}/cgi.h
%{_includedir}/okws-%{version}/clist.h
%{_includedir}/okws-%{version}/email.h
%{_includedir}/okws-%{version}/fd_prot.h
%{_includedir}/okws-%{version}/fd_prot.x
%{_includedir}/okws-%{version}/fhash.h
%{_includedir}/okws-%{version}/form.h
%{_includedir}/okws-%{version}/hdr.h
%{_includedir}/okws-%{version}/holdtab.h
%{_includedir}/okws-%{version}/httpconst.h
%{_includedir}/okws-%{version}/inhdr.h
%{_includedir}/okws-%{version}/kmp.h
%{_includedir}/okws-%{version}/lbalance.h
%{_includedir}/okws-%{version}/mpfd.h
%{_includedir}/okws-%{version}/ok.h
%{_includedir}/okws-%{version}/okconst.h
%{_includedir}/okws-%{version}/okdbg-int.h
%{_includedir}/okws-%{version}/okdbg.h
%{_includedir}/okws-%{version}/okerr.h
%{_includedir}/okws-%{version}/oklog.h
%{_includedir}/okws-%{version}/okprot.h
%{_includedir}/okws-%{version}/okprot.x
%{_includedir}/okws-%{version}/okwc.h
%{_includedir}/okws-%{version}/okwsconf.h
%{_includedir}/okws-%{version}/pair.h
%{_includedir}/okws-%{version}/parr.h
%{_includedir}/okws-%{version}/parse.h
%{_includedir}/okws-%{version}/pjail.h
%{_includedir}/okws-%{version}/pslave.h
%{_includedir}/okws-%{version}/pub.h
%{_includedir}/okws-%{version}/pub_parse.h
%{_includedir}/okws-%{version}/puberr.h
%{_includedir}/okws-%{version}/pubutil.h
%{_includedir}/okws-%{version}/recycle.h
%{_includedir}/okws-%{version}/resp.h
%{_includedir}/okws-%{version}/suiolite.h
%{_includedir}/okws-%{version}/svq.h
%{_includedir}/okws-%{version}/txa.h
%{_includedir}/okws-%{version}/txa_prot.h
%{_includedir}/okws-%{version}/txa_prot.x
%{_includedir}/okws-%{version}/web.h
%{_includedir}/okws-%{version}/web_prot.h
%{_includedir}/okws-%{version}/web_prot.x
%{_includedir}/okws-%{version}/xpub.h
%{_includedir}/okws-%{version}/xpub.x
%{_includedir}/okws-%{version}/zstr.h
%{_libdir}/okws
%{_libdir}/okws-%{version}/libahttp.a
%{_libdir}/okws-%{version}/libahttp.la
%{_libdir}/okws-%{version}/libamt.a
%{_libdir}/okws-%{version}/libamt.la
%{_libdir}/okws-%{version}/libaok.a
%{_libdir}/okws-%{version}/libaok.la
%{_libdir}/okws-%{version}/libpub.a
%{_libdir}/okws-%{version}/libpub.la
%{_libdir}/okws-%{version}/libweb.a
%{_libdir}/okws-%{version}/libweb.la
%{_libdir}/okws-%{version}/okd
%{_libdir}/okws-%{version}/oklogd
%{_libdir}/okws-%{version}/pubd


%changelog
* Mon May 9 2005 Jeremy Stribling <strib@mit.edu>
- Initial SPEC

