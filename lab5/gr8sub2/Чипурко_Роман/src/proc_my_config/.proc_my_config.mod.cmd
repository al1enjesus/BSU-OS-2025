savedcmd_proc_my_config.mod := printf '%s\n'   proc_my_config.o | awk '!x[$$0]++ { print("./"$$0) }' > proc_my_config.mod
