.\output\BinConvert  -oad .\output\boot\bk3633_boot.bin  .\output\stack\bk3633_stack.bin  .\output\app\bk3633_app.bin -m 0x7800 -l 0x7900 -v 0x0026 -rom_v 0x0014 -e 00000000 00000000 00000000 00000000
del .\output\app\bk3633_app_merge_boot_stack.out
del .\output\app\bk3633_app_stack_oad.bin
