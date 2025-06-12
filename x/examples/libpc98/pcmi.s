.file "pcmi.s"

.global _g_pcm_fifo_buffer
.global _g_pcm_fifo_size
.global _g_pcm_old_handler
.global _g_pcm_int_handler
.global _g_pcm_buffer_empty

.data
_g_pcm_fifo_buffer: # const signed char*
    .space 4

_g_pcm_fifo_size: # unsigned short
    .space 2

#_g_pcm_old_handler: # __dpmi_paddr (long + short)
#    .space 8

_g_pcm_buffer_empty: # bool
    .space 1

.text
.p2align 2
_g_pcm_int_handler:
    push %ds
    push %es
    push %fs
    push %gs
    pushal

	incw _g_pcm_fifo_size # HACK: for now just increase the size to show it's working

    # Copy the data
    # TODO

    # Report that we're done
    mov $0x20, %al
    out %al, $0x20

    popal
    pop %gs
    pop %fs
    pop %es
    pop %ds
    sti
    iret
