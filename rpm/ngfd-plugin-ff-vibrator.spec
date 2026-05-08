Name:       ngfd-plugin-ff-vibrator
Summary:    Droid Vibrator FF plugin for ngfd
Version:    1.0.0
Release:    1
License:    LGPLv2+
URL:        https://github.com/mer-hybris/ngfd-plugin-droid-vibrator
Source:     %{name}-%{version}.tar.gz
Requires:   ngfd >= 1.0
BuildRequires:  cmake
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(ngf-plugin) >= 1.0
Conflicts:  ngfd-plugin-droid-vibrator

%description
This package contains the Droid Vibrator FF plugin
for the non-graphical feedback daemon.

%prep
%setup -q -n %{name}-%{version}

%build
%cmake -DFF_VIBRATOR=ON
%make_build

%install
%make_install

%files
%license COPYING
%doc README
%{_libdir}/ngf/libngfd_droid-vibrator.so
%{_datadir}/ngfd/plugins.d/50-droid-vibrator.ini
