savedcmd_proc_module.mod := printf '%s\n'   proc_module.o | awk '!x[$$0]++ { print("./"$$0) }' > proc_module.mod
