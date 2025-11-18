savedcmd_char_device.mod := printf '%s\n'   char_device.o | awk '!x[$$0]++ { print("./"$$0) }' > char_device.mod
