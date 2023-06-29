# `weave`, the `burrow` assembler for `squirm`

## Features
### Comments
```s
.start: # the entry point
    ldi %a, 0x6000 # load 0x6000 into register `a`
```
### Macros
```s
!macro mov a b:
    ldi %m, 0x00
    or $a, $b, %m

.start:
    mov %a, %b
```

## Grammar
```ebnf
program =
    { macro | label_start | instruction }, EOF;

macro =
    '!', "macro", IDENTIFIER, { IDENTIFIER }, ':', { TOKEN | ( '$', IDENTIFIER ) }, ';';

label_start =
    label, ':', NL;

label =
    '.', IDENTIFIER;

instruction =
    IDENTIFIER, [ argument, { ',', argument }], NL;

argument =
    register | label | number | char | string;

register =
    '%', IDENTIFIER;

number = HEX | BIN | DEC;

char = '\'', (CHAR | ('\\', CHAR)), '\'';

string = '"', { CHAR }, '"';
```
