%define __provides_exclude_from ^%{_datadir}/%{name}/lib/.*$
%define __requires_exclude_from ^%{_datadir}/%{name}/lib/.*$
%define __provides_exclude ^(libabsl.*|libcpuinfo.*|libcrypto.*|libcurl.*|libdate.*|libflatbuffers.*|libnsync.*|libonnx.*|libonnxruntime.*|libprotobuf.*|libre2.*|libssl.*|libz.*|libatomic.*|libXNNPACK.*|libpthreadpool.*)$
%define __requires_exclude ^(libabsl.*|libcpuinfo.*|libcrypto.*|libcurl.*|libdate.*|libflatbuffers.*|libnsync.*|libonnx.*|libonnxruntime.*|libprotobuf.*|libre2.*|libssl.*|libz.*|libatomic.*|libXNNPACK.*|libpthreadpool.*)$
%define _cmake_skip_rpath %{nil}


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
BuildRequires:  conan
BuildRequires:  ninja

%description
Короткое описание моего приложения для ОС Аврора

%prep
%autosetup

%build
CONAN_LIB_DIR="%{_builddir}/conan-libs/"
%{set_build_flags}
rm -f "$CONAN_LIB_DIR/conanrun.sh"
OLD_PATH=$PATH
export PATH=$(echo $PATH | sed 's|/home/mersdk/.mb2/wrappers[^:]*:||g')
conan-install-if-modified --source-folder="%{_sourcedir}/.." --output-folder="$CONAN_LIB_DIR" -vwarning --build=missing
export PATH=$OLD_PATH
PKG_CONFIG_PATH="$CONAN_LIB_DIR":$PKG_CONFIG_PATH
export PKG_CONFIG_PATH

%cmake -GNinja -DCMAKE_SYSTEM_PROCESSOR=%{_arch}
%ninja_build

%install
%ninja_install

EXECUTABLE="%{buildroot}/%{_bindir}/%{name}"
CONAN_LIB_DIR="%{_builddir}/conan-libs/"
SHARED_LIBRARIES="%{buildroot}/%{_datadir}/%{name}/lib"
mkdir -p "$SHARED_LIBRARIES"

if [ "%{_arch}" = "x86_64" ]; then
    LDD_WRAPPER_DIR="%{_builddir}/.ldd-wrapper"
    mkdir -p "$LDD_WRAPPER_DIR"
    cat > "$LDD_WRAPPER_DIR/ldd" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

rc=0
many=0
if [ "$#" -gt 1 ]; then
    many=1
fi

for f in "$@"; do
    if [ "$many" -eq 1 ]; then
        echo "${f}:"
    fi
    LD_PRELOAD= /lib64/ld-linux-x86-64.so.2 --library-path "${LD_LIBRARY_PATH:-}" --list "$f" || rc=$?
done

exit "$rc"
EOF
    chmod +x "$LDD_WRAPPER_DIR/ldd"
    export PATH="$LDD_WRAPPER_DIR:$PATH"
fi
conan-deploy-libraries "$EXECUTABLE" "$CONAN_LIB_DIR" "$SHARED_LIBRARIES"

%files
%defattr(755,root,root,755)
%{_bindir}/%{name}
%dir %{_datadir}/%{name}
%dir %{_datadir}/%{name}/lib
%{_datadir}/%{name}/lib/*.so*
%defattr(644,root,root,755)
%{_datadir}/%{name}/models
%{_datadir}/%{name}/qml
%{_datadir}/%{name}/translations
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
