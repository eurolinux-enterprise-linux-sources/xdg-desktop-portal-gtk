%global xdg_desktop_portal_version 1.0.2

Name:           xdg-desktop-portal-gtk
Version:        1.0.2
Release:        1%{?dist}
Summary:        Backend implementation for xdg-desktop-portal using GTK+

License:        LGPLv2+
URL:            https://github.com/flatpak/%{name}
Source0:        https://github.com/flatpak/%{name}/releases/download/%{version}/%{name}-%{version}.tar.xz

BuildRequires:  gcc
BuildRequires:  gettext
BuildRequires:  pkgconfig(gtk+-unix-print-3.0)
BuildRequires:  pkgconfig(xdg-desktop-portal) >= %{xdg_desktop_portal_version}
%{?systemd_requires}
BuildRequires:  systemd
Requires:       dbus
Requires:       xdg-desktop-portal >= %{xdg_desktop_portal_version}

%description
A backend implementation for xdg-desktop-portal that is using GTK+ and various
pieces of GNOME infrastructure, such as the org.gnome.Shell.Screenshot or
org.gnome.SessionManager D-Bus interfaces.


%prep
%setup -q


%build
%configure --disable-silent-rules
%make_build


%install
%make_install
%find_lang %{name}


%post
%systemd_user_post %{name}.service


%preun
%systemd_user_preun %{name}.service


%files -f %{name}.lang
%license COPYING
%doc NEWS
%{_libexecdir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/dbus-1/services/org.freedesktop.impl.portal.desktop.gtk.service
%{_datadir}/xdg-desktop-portal/portals/gtk.portal
%{_userunitdir}/%{name}.service



%changelog
* Wed Sep 12 2018 David King <dking@redhat.com> - 1.0.2-1
- Update to 1.0.2 (#1570030)

* Wed Jan 18 2017 David King <amigadave@amigadave.com> - 0.5-1
- Update to 0.5

* Fri Sep 02 2016 David King <amigadave@amigadave.com> - 0.3-1
- Update to 0.3

* Fri Jul 29 2016 David King <amigadave@amigadave.com> - 0.2-1
- Update to 0.2 (#1361576)

* Wed Jul 13 2016 David King <amigadave@amigadave.com> - 0.1-1
- Initial Fedora packaging
