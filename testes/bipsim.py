#!/usr/bin/env python3
"""Simulador simples do BIP IV para conferir o assembly gerado. Uso: bipsim.py prog.asm entrada1 entrada2 ..."""
import sys
def s16(x):
    x &= 0xFFFF
    return x - 0x10000 if x & 0x8000 else x
src = open(sys.argv[1]).read().splitlines()
inputs = [int(x) for x in sys.argv[2:]]
mem, addr, text, labels = {}, {}, [], {}
sec = None; nxt = 0
for line in src:
    line = line.split(';')[0].strip()
    if not line: continue
    if line in ('.data', '.text'): sec = line; continue
    if sec == '.data':
        name, vals = [p.strip() for p in line.split(':', 1)]
        addr[name] = nxt
        for v in vals.split(','): mem[nxt] = int(v); nxt += 1
    else:
        if line.endswith(':'): labels[line[:-1]] = len(text); continue
        op, arg = (line.split(None, 1) + ['0'])[:2]
        text.append((op.upper(), arg.strip()))
acc = 0; indr = 0; out = []; pc = 0; steps = 0; ula = 0; pilha = []
def rd(a):
    global inputs
    if a == '$in_port': return inputs.pop(0)
    if a == '$indr': return indr
    return mem[addr[a]]
def wr(a, v):
    global indr
    if a == '$out_port': out.append(s16(v)); return
    if a == '$indr': indr = s16(v); return
    mem[addr[a]] = s16(v)
while True:
    steps += 1
    assert steps < 100000, 'loop infinito?'
    op, a = text[pc]; pc += 1
    imm = lambda: int(a, 0)
    if op == 'HLT': break
    elif op == 'LD': acc = rd(a)
    elif op == 'LDI': acc = imm()
    elif op == 'STO': wr(a, acc)
    elif op == 'ADD': acc = ula = s16(acc + rd(a))
    elif op == 'ADDI': acc = ula = s16(acc + imm())
    elif op == 'SUB': acc = ula = s16(acc - rd(a))
    elif op == 'SUBI': acc = ula = s16(acc - imm())
    elif op == 'JMP': pc = labels[a]
    elif op in ('BEQ', 'BNE', 'BGT', 'BGE', 'BLT', 'BLE'):
        cond = {'BEQ': ula == 0, 'BNE': ula != 0, 'BGT': ula > 0, 'BGE': ula >= 0, 'BLT': ula < 0, 'BLE': ula <= 0}[op]
        if cond: pc = labels[a]
    elif op == 'CALL': pilha.append(pc); pc = labels[a]
    elif op == 'RETURN': pc = pilha.pop()
    elif op == 'AND': acc = s16(acc & rd(a))
    elif op == 'ANDI': acc = s16(acc & imm())
    elif op == 'OR': acc = s16(acc | rd(a))
    elif op == 'ORI': acc = s16(acc | imm())
    elif op == 'XOR': acc = s16(acc ^ rd(a))
    elif op == 'XORI': acc = s16(acc ^ imm())
    elif op == 'NOT': acc = s16(~acc)
    elif op == 'SLL': acc = s16(acc << imm())
    elif op == 'SRL': acc = s16((acc & 0xFFFF) >> imm())
    elif op == 'LDV': acc = mem[addr[a] + indr]
    elif op == 'STOV': mem[addr[a] + indr] = s16(acc)
    else: raise SystemExit('instrucao desconhecida: ' + op)
print('saida:', out)
print('memoria:', {k: [mem[addr[k] + i] for i in range(n)] for k, n in
      ((k, (sorted(list(addr.values()) + [nxt])[sorted(addr.values()).index(v) + 1] - v)) for k, v in addr.items())})
