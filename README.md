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
- Só inteiros. O resto (gånger, relacionais, desvios, laços, sub-rotinas) fica para o T5/T6 e gera um aviso.
