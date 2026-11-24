# Compilador Sueco Go

## Build

```bash
make
```

## Rodar

```bash
./compilador_sueco_go arquivo.swgo
```

## Atualizar o código gerado

Com o ZIP padrão:

```bash
make update-generated
```

Com outro ZIP:

```bash
make update-generated ZIP="caminho/pacote.zip"
```

## Geração de código BIP (T4)

Quando a compilação dá certo, o compilador gera o assembly do BIP (Bipide 3.0) e salva em `arquivo.asm`
(na IDE ele aparece na aba "Código BIP" e pode ser salvo com o botão "Salvar .asm").

- `.data`: variáveis e vetores da tabela de símbolos (`x : 0`, `v : 0, 0, 0`) e temporários `_tempN`.
- Atribuição, `plus`, `minus`, `bitoch`, `biteller`, `bitxor`, `bitinte`, `vänster`/`höger` (por constante), `++`/`--`.
- Vetores com `$indr` + `LDV`/`STOV`; entrada `std_läsa()` com `$in_port`; saída `std_skriva()` com `$out_port`.
- Só inteiros. O que o BIP não tem (gånger, delat, rest) gera um aviso e o comando é ignorado.
- `och`, `eller` e `inte` funcionam em condições (`om`, `medan`, `gör ... medan`, `för`), com curto-circuito: o segundo operando só é avaliado se o primeiro não decidiu o resultado. Guardar o resultado numa variável não é gerado (aviso).

## Desvios e laços (T5)

- Comparação vira `SUB` + desvio (`BGT`, `BGE`, `BLT`, `BLE`, `BEQ`, `BNE`).
- `om`/`annars`, `medan`, `gör ... medan` e `för` usam rótulos numerados (`FIMSE1`, `ENQUANTO2`, `PARA3`...),
  guardados numa pilha, então funcionam aninhados.
- No `för`, o código do incremento é guardado e colocado no fim do laço.

## Sub-rotinas (T6)

- Cada `funktion` vira um rótulo (`_somar:`) com `RETURN 0` no fim; o programa começa com `JMP _PRINCIPAL`.
- Parâmetros por cópia: cada argumento é copiado para a variável do parâmetro antes do `CALL` (vetores são copiados posição por posição).
- O valor de `ge` fica no acumulador, então a chamada pode ser usada em expressões e atribuições.
- Quantidade, ordem e tipo dos parâmetros são conferidos na análise semântica, com mensagens de erro.
- Limitação: como os parâmetros ficam em posições fixas de memória, recursão não funciona no BIP.
