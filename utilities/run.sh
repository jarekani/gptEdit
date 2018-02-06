

qemu-system-x86_64 -debugcon file:uefi_debug.log -global isa-debugcon.iobase=0x402 -bios /usr/share/edk2.git/ovmf-x64/OVMF-pure-efi.fd -drive file=uefi.img,if=ide
