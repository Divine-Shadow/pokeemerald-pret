#!/usr/bin/env python3
"""Inject VBlank processing at each ARM instruction boundary in DMA enqueue.

Run: uv run --with unicorn --with pyelftools python
     dev_scripts/test_dma3_interrupt.py pokeemerald.elf
Uses the compiled ROM code; generated ROMs and packages are not committed.

The interrupt saves/restores CPU state and runs the real ProcessDma3Requests.
Only queue completion is checked: Unicorn does not emulate GBA DMA hardware,
so this is not a rendering test or a reproduction of a particular saved battle.
"""

import argparse
import struct

from elftools.elf.elffile import ELFFile
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB
from unicorn.arm_const import UC_ARM_REG_LR, UC_ARM_REG_PC, UC_ARM_REG_SP
from unicorn.arm_const import UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("elf")
    args = parser.parse_args()
    with open(args.elf, "rb") as source:
        elf = ELFFile(source)
        symbols = {symbol.name: symbol.entry.st_value
                   for symbol in elf.get_section_by_name(".symtab").iter_symbols()}
        segments = [(segment.header.p_vaddr, segment.data())
                    for segment in elf.iter_segments()
                    if segment.header.p_type == "PT_LOAD"]

    stop = 0x0203F000

    def machine():
        cpu = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
        for start, size in ((0x02000000, 0x40000), (0x03000000, 0x8000),
                            (0x04000000, 0x1000), (0x08000000, 0x2000000)):
            cpu.mem_map(start, size)
        for address, data in segments:
            cpu.mem_write(address, data)
        # VBlank, before its DMA deadline.
        cpu.mem_write(0x04000006, bytes([160, 0]))
        cpu.reg_write(UC_ARM_REG_SP, 0x03007E00)
        return cpu

    def call(cpu, name, arguments=()):
        for reg, value in zip((UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3), arguments):
            cpu.reg_write(reg, value)
        cpu.reg_write(UC_ARM_REG_LR, stop | 1)
        cpu.emu_start(symbols[name] | 1, stop, count=10000)
        assert cpu.reg_read(UC_ARM_REG_PC) == stop, name + " did not return"
        return cpu.reg_read(UC_ARM_REG_R0)

    def pending(cpu):
        requests = symbols["sDma3Requests"]
        return [i for i in range(128)
                if struct.unpack("<H", cpu.mem_read(requests + 16 * i + 8, 2))[0]]

    failures = []
    trials = 0
    for operation in ("RequestDma3Copy", "RequestDma3Fill"):
        for initial_cursor in (0, 127):
            for boundary in range(256):
                cpu = machine()
                call(cpu, "ClearDma3Requests")
                cpu.mem_write(symbols["sDma3RequestCursor"], bytes([initial_cursor]))
                # A prior copy is pending when the menu queues another transfer.
                call(cpu, "RequestDma3Copy", (0x02000000, 0x02001000, 4, 1))
                arguments = (0x02000004 if operation.endswith("Copy") else 0x12345678,
                             0x02001004, 4, 1)
                for reg, value in zip((UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3), arguments):
                    cpu.reg_write(reg, value)
                cpu.reg_write(UC_ARM_REG_LR, stop | 1)
                cpu.reg_write(UC_ARM_REG_PC, symbols[operation] | 1)
                for _ in range(boundary):
                    if cpu.reg_read(UC_ARM_REG_PC) == stop:
                        break
                    cpu.emu_start(cpu.reg_read(UC_ARM_REG_PC) | 1, stop, count=1)
                if cpu.reg_read(UC_ARM_REG_PC) == stop:
                    break
                interrupted_pc = cpu.reg_read(UC_ARM_REG_PC)
                context = cpu.context_save()
                # Model the interrupt's separate stack and register preservation.
                cpu.reg_write(UC_ARM_REG_SP, 0x03007000)
                call(cpu, "ProcessDma3Requests")
                cpu.context_restore(context)
                cpu.emu_start(interrupted_pc | 1, stop, count=10000)
                assert cpu.reg_read(UC_ARM_REG_PC) == stop
                for _ in range(3):
                    call(cpu, "ProcessDma3Requests")
                trials += 1
                if pending(cpu):
                    failures.append(f"{operation}, cursor {initial_cursor}, "
                                    f"interrupt at {interrupted_pc:#010x}: stranded slots {pending(cpu)}")

    for failure in failures:
        print("FAIL:", failure)
    print(f"{trials} interrupt schedules checked; {len(failures)} stranded queues")
    raise SystemExit(bool(failures))


if __name__ == "__main__":
    main()
