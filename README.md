a custom bootloader lol

config file format
```
instant boot? Y or N
kernel path
initrd path (if none NONE)
cmdline
Shell path (if none NONE)
```
EXAMPLE CONFIG
```
N
\vmlinuz-linux
\initramfs-linux.img
root=UUID
\shellx64.efi
```

keybinds

Enter = boot kernel

S = Shutdown

D = Shell

F = Firmware setting
