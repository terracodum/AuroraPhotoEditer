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

%define __provides_exclude_from ^%{_datadir}/%{name}/lib/.*$
%define __requires_exclude_from ^%{_datadir}/%{name}/lib/.*$
%define __provides_exclude ^(libabsl.*|libcpuinfo.*|libcrypto.*|libcurl.*|libdate.*|libflatbuffers.*|libnsync.*|libonnx.*|libonnxruntime.*|libprotobuf.*|libre2.*|libssl.*|libz.*|libatomic.*|libXNNPACK.*|libpthreadpool.*|libopencv.*|libjpeg.*|libpng.*)$
%define __requires_exclude ^(libabsl.*|libcpuinfo.*|libcrypto.*|libcurl.*|libdate.*|libflatbuffers.*|libnsync.*|libonnx.*|libonnxruntime.*|libprotobuf.*|libre2.*|libssl.*|libz.*|libatomic.*|libXNNPACK.*|libpthreadpool.*|libopencv.*|libjpeg.*|libpng.*)$
%define _cmake_skip_rpath %{nil}

%description
Короткое описание моего приложения для ОС Аврора

%prep
%autosetup

%build
ORIG_PATH=$PATH
export PATH=$(echo $PATH | sed 's|/home/mersdk/.mb2/wrappers[^:]*:||g')
export PATH="/home/mersdk/.cache/AuroraTools/5/conan/2.22.0/x86_64/bin:$PATH"
CONAN_LIB_DIR="%{_builddir}/conan-libs/"
%{set_build_flags}
mkdir -p "$CONAN_LIB_DIR"
conan profile detect || true
conan install %{_sourcedir}/.. --output-folder="$CONAN_LIB_DIR" --build=missing -pr:h %{_arch} -pr:b default -s:h compiler.version=8 -o:h onnxruntime/*:shared=True -o:h onnxruntime/*:with_xnnpack=True -o:h onnxruntime/*:with_cuda=False
export PATH=$ORIG_PATH
PKG_CONFIG_PATH="$CONAN_LIB_DIR":$PKG_CONFIG_PATH
export PKG_CONFIG_PATH

%cmake -GNinja -DCMAKE_SYSTEM_PROCESSOR=%{_arch}
%ninja_build

%install
%ninja_install

SHARED_LIBRARIES="%{buildroot}/%{_datadir}/%{name}/lib"
mkdir -p "$SHARED_LIBRARIES"

CONAN_LIB_DIR="%{_builddir}/conan-libs/"
ALL_LIBDIRS=$(grep -h "^libdir=" "$CONAN_LIB_DIR"/*.pc 2>/dev/null | cut -d= -f2 | sort -u || true)
EXECUTABLE="%{buildroot}/%{_bindir}/%{name}"

if [ -f "$EXECUTABLE" ] && [ -n "$ALL_LIBDIRS" ]; then
    NEEDED_LIBS=$(objdump -p "$EXECUTABLE" | grep NEEDED | awk '{print $2}')
    for lib in $NEEDED_LIBS; do
        for dir in $ALL_LIBDIRS; do
            if [ -f "$dir/$lib" ]; then
                cp -d "$dir/$lib"* "$SHARED_LIBRARIES"/
                break
            fi
        done
    done
fi

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
