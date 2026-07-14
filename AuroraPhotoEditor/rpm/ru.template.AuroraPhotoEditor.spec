Name:       ru.template.AuroraPhotoEditor
Summary:    Моё приложения для ОС Аврора
Version:    0.1
Release:    1
License:    BSD-3-Clause
URL:        https://auroraos.ru
Source0:    %{name}-%{version}.tar.bz2

Requires:   sailfishsilica-qt5 >= 0.10.9
BuildRequires:  pkgconfig(auroraapp)
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Test)
BuildRequires:  ninja

%define __provides_exclude_from ^%{_datadir}/%{name}/lib/.*$
%define __requires_exclude_from ^%{_datadir}/%{name}/lib/.*$
%define __provides_exclude ^(libabsl.*|libcpuinfo.*|libcrypto.*|libcurl.*|libdate.*|libflatbuffers.*|libnsync.*|libonnx.*|libonnxruntime.*|libprotobuf.*|libre2.*|libssl.*|libz.*|libatomic.*|libXNNPACK.*|libpthreadpool.*|libopencv.*|libjpeg.*|libpng.*)$
%define __requires_exclude ^(libabsl.*|libcpuinfo.*|libcrypto.*|libcurl.*|libdate.*|libflatbuffers.*|libnsync.*|libonnx.*|libonnxruntime.*|libprotobuf.*|libre2.*|libssl.*|libz.*|libatomic.*|libXNNPACK.*|libpthreadpool.*|libopencv.*|libjpeg.*|libpng.*)$
%define _cmake_skip_rpath %{nil}
%{expand:%(bash %{_sourcedir}/load-conan.sh)}

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

%defattr(755,root,root,-)
%dir %{_datadir}/%{name}
%dir %{_datadir}/%{name}/lib
%{_datadir}/%{name}/lib/*.so*

%defattr(644,root,root,-)
%{_datadir}/%{name}/qml
%{_datadir}/%{name}/translations
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
