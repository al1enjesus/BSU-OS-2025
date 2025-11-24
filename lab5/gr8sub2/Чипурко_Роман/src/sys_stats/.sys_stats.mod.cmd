savedcmd_sys_stats.mod := printf '%s\n'   sys_stats.o | awk '!x[$$0]++ { print("./"$$0) }' > sys_stats.mod
