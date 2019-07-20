org 0000h
start:
  di
  ld sp, 3fffh
  im 2
  ld a, 02h
  ld I, a
  ld de, str_console_prompt
  call print_string
  ei

iloop:
  halt
  jr iloop

print_string:
  push af 
print_string_loop: 
  ld a, (de)
  or a
  jr z, print_string_end
  out ($81), a 
  inc de
  jr print_string_loop 
print_string_end:
  pop af
  ret
  
ihdlr_unknown:
  di
  push de
  ld de, str_unknownint
  call print_string
  pop de
  ei
  reti

ihdlr_second_timer:
  di
  push de
  ld de, str_bestint
  call print_string
  pop de
  ei
  reti

str_unknownint:
  defb 'Unknown Interrupt Vector',0

str_bestint:
  defb 'Best Interrupt Vector',0

str_console_prompt:
  defb 'TeensyZ80>',0
  
; interrupt vector table
; ???
org 0100h
int_vector_table:
  dw ihdlr_unknown
  dw ihdlr_second_timer
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
  dw ihdlr_unknown
end
