start:
add eax, ecx
mov r8d, ecx
imul r8d, r8d
add edx, r8d
add ecx, 0x1
cmp edi, ecx
jne start
