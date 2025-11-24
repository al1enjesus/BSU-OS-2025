savedcmd_hello_module.mod := printf '%s\n'   hello_module.o | awk '!x[$$0]++ { print("./"$$0) }' > hello_module.mod
