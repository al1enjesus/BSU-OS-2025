savedcmd_sys_stats_module.mod := printf '%s\n'   sys_stats_module.o | awk '!x[$$0]++ { print("./"$$0) }' > sys_stats_module.mod
