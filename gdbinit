set remote hardware-breakpoint-limit 1
set remote hardware-watchpoint-limit 1

break panic

# set debug xtensa 4

# mem 0x40200000 0x40300000 ro
# mem 0x00000000 0x40200000 rw
# mem 0x40300000 0xFFFFFFFF rw
