.section .aerokernel,"a"

.globl __aerokernel_start
__aerokernel_start:
.incbin AEROKERNEL_PATH
.globl __aerokernel_end
__aerokernel_end:
