Name:           xdg-desktop-portal-gtk
Version:        0.5
Release:        1%{?dist}
Summary:        Backend implementation for xdg-desktop-portal using GTK+

License:        LGPLv2+
URL:            https://github.com/flatpak/%{name}
Source0:        https://github.com/flatpak/releases/download/%{version}/%{name}-%{version}.tar.xz
# https://github.com/flatpak/xdg-desktop-portal-gtk/pull/44
Patch0:         xdg-desktop-portal-gtk-0.5-old-gtk.patch

BuildRequires:  pkgconfig(gtk+-unix-print-3.0)
BuildRequires:  pkgconfig(xdg-desktop-portal)
Requires:       dbus
Requires:       xdg-desktop-portal

%description
A backend implementation for xdg-desktop-portal that is using GTK+ and various
pieces of GNOME infrastructure, such as the org.gnome.Shell.Screenshot or
org.gnome.SessionManager D-Bus interfaces.


%prep
%setup -q
%patch0 -p1


%build
%configure --disable-silent-rules
%make_build


%install
%make_install
%find_lang %{name}


%files -f %{name}.lang
%license COPYING
%doc NEWS
%{_libexecdir}/%{name}
%{_datadir}/dbus-1/services/org.freedesktop.impl.portal.desktop.gtk.service
%{_datadir}/xdg-desktop-portal/portals/gtk.portal



%changelog
* Wed Jan 18 2017 David King <amigadave@amigadave.com> - 0.5-1
- Update to 0.5

* Fri Sep 02 2016 David King <amigadave@amigadave.com> - 0.3-1
- Update to 0.3

* Fri Jul 29 2016 David King <amigadave@amigadave.com> - 0.2-1
- Update to 0.2 (#1361576)

* Wed Jul 13 2016 David King <amigadave@amigadave.com> - 0.1-1
- Initial Fedora packaging
