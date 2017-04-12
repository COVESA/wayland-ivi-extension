SUMMARY = "Wayland IVI Extension"
DESCRIPTION = "GENIVI Layer Management API based on Wayland IVI Extension"
HOMEPAGE = "http://projects.genivi.org/wayland-ivi-extension"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${S}/LICENSE;md5=1f1a56bb2dadf5f2be8eb342acf4ed79"

# define ADIT specifics
ADIT_SOURCE_GIT = "wayland-extensions"
ADIT_SOURCE_PATH = ""

# include adit cmake definitions
require recipes/adit-package/adit-package-cmake.inc

DEPENDS += "${WAYLAND_COMPOSITOR_PKG}"

RDEPENDS_${PN} += "${WAYLAND_COMPOSITOR_PKG} bash"

FILES_${PN} += "${FILES_SOLIBSDEV} ${libdir}/weston /opt"
FILES_${PN}-dev_remove = "${FILES_SOLIBSDEV}"

# avoid qa error for .so symlink - this is currently needed caused by so files
INSANE_SKIP_${PN} += "dev-so"

EXTRA_OECMAKE += "-DLIB_SUFFIX=${@d.getVar('baselib', True).replace('lib', '')} \
                  -DWITH_ILM_INPUT=ON \
"

do_install_append() {
  # install smoketest
  install -d ${D}/opt/platform
  install -m 0755 ${S}/recipes/wayland-extensions/files/weston-smoketest.sh ${D}/opt/platform

  # install weston configuration
  install -d ${D}/etc/xdg/weston
  install -m 0644 ${S}/recipes/wayland-extensions/files/weston.ini ${D}/etc/xdg/weston
}

OPTION_GPU_HW_INTEL = "${@base_conditional('GPU_HW_VENDOR', 'INTEL', 'ON ', 'OFF', d)}"

cmake_do_generate_toolchain_file_append() {
 echo "option (IVI_SHARE \"Use graphics hardware is INTEL, enable ivi_share protocol\" ${OPTION_GPU_HW_INTEL})" >> ${WORKDIR}/toolchain.cmake
}
