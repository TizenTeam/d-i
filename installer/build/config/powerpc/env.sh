# This script is sourced into the environment that daily-build is ran in...

TARGETS='build_powerpc_cdrom build_powerpc_netboot build_powerpc_hd-media build_powerpc_gtk-miniiso build_powerpc64_netboot build_powerpc64_gtk-miniiso' 

# this will overwrite $TARGETS right before the miboot build...
TARGETS_MIBOOT='build_powerpc_floppy_root build_powerpc_floppy_net-drivers build_powerpc_floppy_cd-drivers build_powerpc_floppy_boot build_powerpc_floppy_boot-ofonly'

export TARGETS TARGETS_MIBOOT
