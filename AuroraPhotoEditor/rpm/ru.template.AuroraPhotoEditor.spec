%define __provides_exclude_from ^%{_datadir}/%{name}/lib/.*$
%define __requires_exclude_from ^%{_datadir}/%{name}/lib/.*$
%define __requires_exclude ^(libtensorflow-lite.*|libopencv_.*|libpthreadpool.*|libabsl_.*|libcpuinfo.*|libeight_bit_int_gemm.*|libfarmhash.*|libfft.*|libflatbuffers.*|libruy_.*)$
%define _cmake_skip_rpath %{nil}
%{expand:%(bash %{_sourcedir}/load-conan.sh)}

Name:       ru.template.AuroraPhotoEditor
Summary:    Моё приложения для ОС Аврора
Version:    0.1
Release:    2
License:    BSD-3-Clause
URL:        https://auroraos.ru
Source0:    %{name}-%{version}.tar.bz2

Requires:   sailfishsilica-qt5 >= 0.10.9
BuildRequires:  pkgconfig(auroraapp)
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  ninja

%description
Короткое описание моего приложения для ОС Аврора

%prep
%autosetup

%build
OLD_PATH=$PATH
export PATH=/usr/bin:$PATH
%conan_install
export PATH=$OLD_PATH
%conan_cmake -GNinja %{_sourcedir}/..
%ninja_build

%install
%ninja_install
%conan_deploy_libraries

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%dir %{_datadir}/%{name}
%{_datadir}/%{name}/lib/
%defattr(644,root,root,755)
%{_datadir}/%{name}/qml
%{_datadir}/%{name}/translations
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
