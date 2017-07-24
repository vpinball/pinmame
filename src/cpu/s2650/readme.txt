----------------------------------------------------------------
|                                                              |
|                                                              |
|                          Signetics                           |
|                                                              |
|             22222      666     5555555     000               |
|            2     2    6        5          0   0              |
|                 2    6         5         0   0 0             |
|              222     666666    555555    0  0  0             |
|             2        6     6         5   0 0   0             |
|            2         6     6         5    0   0              |
|            2222222    66666    555555      000               |
|                                                              |
|         2650 MICROPROCESSOR Instruction Set Summary          |
|                                                              |
|                                                              |
|                                                              |
|                                                              |
|                                                              |
|                    _________    _________                    |
|                  _|         \__/         |_                  |
|       --> SENSE |_|1                   40|_| FLAG -->        |
|                  _|                      |_                  |
|         <-- A12 |_|2                   39|_| Vcc             |
|                  _|                      |_                  |
|         <-- A11 |_|3                   38|_| CLOCK <--       |
|                  _|                      |_  _____           |
|         <-- A10 |_|4                   37|_| PAUSE <--       |
|                  _|                      |_  _____           |
|          <-- A9 |_|5                   36|_| OPACK <--       |
|                  _|                      |_      ____        |
|          <-- A8 |_|6                   35|_| RUN/WAIT -->    |
|                  _|                      |_                  |
|          <-- A7 |_|7                   34|_| INTACK -->      |
|                  _|                      |_                  |
|          <-- A6 |_|8                   33|_| D0 <-->         |
|                  _|                      |_                  |
|          <-- A5 |_|9                   32|_| D1 <-->         |
|                  _|                      |_                  |
|          <-- A4 |_|10       2650A      31|_| D2 <-->         |
|                  _|                      |_                  |
|          <-- A3 |_|11                  30|_| D3 <-->         |
|                  _|                      |_                  |
|          <-- A2 |_|12                  29|_| D4 <-->         |
|                  _|                      |_                  |
|          <-- A1 |_|13                  28|_| D5 <-->         |
|                  _|                      |_                  |
|          <-- A0 |_|14                  27|_| D6 <-->         |
|           _____  _|                      |_                  |
|       --> ADREN |_|15                  26|_| D7 <-->         |
|                  _|                      |_  ______          |
|       --> RESET |_|16                  25|_| DBUSEN <--      |
|          ______  _|                      |_                  |
|      --> INTREQ |_|17                  24|_| OPREQ -->       |
|               _  _|                      |_  _               |
|     <-- A14-D/C |_|18                  23|_| R/W -->         |
|              __  _|                      |_                  |
|    <-- A13-E/NE |_|19                  22|_| WRP -->         |
|              __  _|                      |_                  |
|        <-- M/IO |_|20                  21|_| GND             |
|                   |______________________|                   |
|                                                              |
|                                                              |
|                                                              |
|                                                              |
|                                                              |
|                                                              |
|Written by     Jonathan Bowen                                 |
|               Programming Research Group                     |
|               Oxford University Computing Laboratory         |
|               8-11 Keble Road                                |
|               Oxford OX1 3QD                                 |
|               England                                        |
|                                                              |
|               Tel +44-865-273840                             |
|                                                              |
|Created        March 1982                                     |
|Updated        April 1985                                     |
|Issue          1.1                Copyright (C) J.P.Bowen 1985|
----------------------------------------------------------------
----------------------------------------------------------------
|Mnemonic|Op|cIOC|~|Description                   |Notes       |
|--------+--+----+-+------------------------------+------------|
|ADDA,r a|8C|****|4|Add Absolute                  |r=r+a       |
|ADDI,r i|84|****|2|Add Immediate                 |r=r+i       |
|ADDR,r l|88|****|3|Add Relative                  |r=r+l       |
|ADDZ,r  |80|****|2|Add to register Zero          |R0=R0+r     |
|ANDA,r a|4C|*---|4|Logical AND Absolute          |r=r&a       |
|ANDI,r i|44|*---|2|Logical AND Immediate         |r=r&i       |
|ANDR,r l|48|*---|3|Logical AND Relative          |r=r&l       |
|ANDZ,r  |40|*---|2|Logical AND register Zero     |R0=R0&r     |
|BCFA,d b|9C|----|3|Branch on Cond. False Absolute|If d#c, PC=b|
|BCFR,d l|98|----|3|Branch on Cond. False Relative|If d#c, PC=l|
|BCTA,d b|1C|----|3|Branch on Cond. True Absolute |If d=c, PC=b|
|BCTR,d l|18|----|3|Branch on Cond. True Relative |If d=c, PC=l|
|BDRA,r b|FC|----|3|Branch on Dec. Reg. Absolute  |r=r-1,if r#0|
|BDRR,r l|F8|----|3|Branch on Dec. Reg. Relative  |r=r-1,if r#0|
|BIRA,r b|DC|----|3|Branch on Inc. Reg. Absolute  |r=r+1,if r#0|
|BIRR,r l|D8|----|3|Branch on Inc. Reg. Relative  |r=r+1,if r#0|
|BRNA,r b|5C|----|3|Branch on Reg. Non-zero Abs.  |If r#0, PC=b|
|BRNR,r l|58|----|3|Branch on Reg. Non-zero Rel.  |If r#0, PC=l|
|BSFA,d b|BC|----|3|Branch to Sub. on False Abs.  |If d#c,calla|
|BSFR,d l|B8|----|3|Branch to Sub. on False Rel.  |If d#c,callr|
|BSNA,r b|7C|----|3|Branch to Sub. on Non-zero Abs|If d#c,calla|
|BSNR,r l|78|----|3|Branch to Sub. on Non-zero Rel|If d#c,callr|
|BSTA,d b|3C|----|3|Branch to Sub. on True Abs.   |If d=c,calla|
|BSTR,d l|38|----|3|Branch to Sub. on True Rel.   |If d=c,callr|
|BSXA   b|BF|----|3|Branch to Sub. Extended Addr. |calla       |
|BXA    b|9F|----|3|Branch to Extended Address    |PC=b        |
|COMA,r a|EC|*---|4|Compare Absolute              |r-a         |
|COMI,r i|E4|*---|2|Compare Immediate             |r-i         |
|COMR,r l|E8|*---|3|Compare Relative              |r-l         |
|COMZ,r  |E0|*---|2|Compare with register Zero    |R0-r        |
|CPSL   i|75|----|3|Clear Program Status Lower    |If i=1,PSL=0|
|CPSU   i|74|----|3|Clear Program Status Upper    |PSU=PSU&(~i)|
|DAR,r   |94|----|3|Decimal Adjust Register       |r=BCD format|
|EORA,r a|2C|*---|4|Logical Exclusive OR Absolute |r=rxa       |
|EORI,r i|24|*---|2|Logical Exclusive OR Immediate|r=rxi       |
|EORR,r l|28|*---|3|Logical Exclusive OR Relative |r=rxl       |
|EORZ,r  |20|*---|2|Logical Exclusive OR reg Zero |R0=R0xr     |
|HALT    |40|----|2|Halt                          |Wait state  |
|IORA,r a|6C|*---|4|Logical Inclusive OR Absolute |r=rva       |
|IORI,r i|64|*---|2|Logical Inclusive OR Immediate|r=rvi       |
|IORR,r l|68|*---|3|Logical Inclusive OR Relative |r=rvl       |
|IORZ,r  |60|*---|2|Logical Inclusive OR reg Zero |R0=R0vr     |
|LODA,r a|0C|*---|4|Load Absolute                 |r=a         |
|LODI,r i|04|*---|2|Load Immediate                |r=i         |
|LODR,r l|08|*---|3|Load Relative                 |r=l         |
|LODZ,r  |00|*---|2|Load register Zero            |R0=r        |
|LPSL    |93|----|2|Load Program Status Lower     |PSL=R0      |
|LPSU    |92|----|2|Load Program Status Upper     |PSU=R0      |
|NOP     |C0|----|2|No Operation                  |            |
|PPSL   i|77|----|3|Preset Program Status Lower   |If i=1,PSL=1|
|PPSU   i|76|----|3|Preset Program Status Upper   |If i=1,PSU=1|
|REDC,r  |30|*---|2|Read Control                  |r=statusNE  |
|REDD,r  |70|*---|2|Read Data                     |r=dataNE    |
|REDE,r p|54|*---|3|Read Extended                 |r=p         |
|RETC,d  |14|----|3|Return on Condition           |If d=c,ret  |
|RETE,d  |34|----|3|Return cond, Enable interrupts|ret,II=0    |
|RRL,r   |D0|----|2|Rotate Register Left          |r=->{rr}    |
|RRR,r   |50|----|2|Rotate Register Right         |r={rr}<-    |
|SPSL    |13|----|2|Store Program Status Lower    |R0=PSL      |
|SPSU    |12|----|2|Store Program Status Upper    |R0=PSU      |
|STRA,r a|CC|----|4|Store Absolute                |a=r         |
|STRR,r l|C8|----|3|Store Relative                |l=r         |
|STRZ,r  |C0|----|2|Store register Zero           |r=R0        |
|SUBA,r a|AC|****|4|Subtract Absolute             |r=r-a       |
|SUBI,r i|A4|****|2|Subtract Immediate            |r=r-i       |
|SUBR,r l|A8|****|3|Subtract Relative             |r=r-l       |
|SUBZ,r  |A0|****|2|Subtract from register Zero   |R0=R0-r     |
|TMI,r  i|F4|*---|3|Test under Mask Immediate     |r&i         |
|TPSL,r i|B5|*---|3|Test Program Status Lower     |i-PSL       |
|TPSU,r i|B4|*---|3|Test Program Status Upper     |i-PSU       |
|WRTC,r  |B0|----|2|Write Control                 |statusNE=r  |
|WRTD,r  |F0|----|2|Write Data                    |dataNE=r    |
|WRTE,r p|D4|----|3|Write Extended                |p=r         |
|ZBRR   l|9B|----|3|Zero page Branch              |PC=l        |
|ZBSR   l|BB|----|3|Zero page Branch to Subroutine|callr       |
|        |XX|    |X|8-bit opcode, machine cycles  |Hexadecimal |
----------------------------------------------------------------
----------------------------------------------------------------
|Mnemonic   |cIOC|Description                                  |
|-----------+----+---------------------------------------------|
|           |-   |Unaffected                                   |
|           |*   |Affected                                     |
|           |0   |Reset                                        |
|           |1   |Set                                          |
|           |?   |Unknown                                      |
|-----------+----+---------------------------------------------|
| S         |    |Sense (PSU bit 7)                            |
| F         |    |Flag (PSU bit 6)                             |
| II        |    |Interrupt Inhibit (PSU bit 5)                |
| SP2       |    |Stack Pointer two (PSU bit 2)                |
| SP1       |    |Stack Pointer one (PSU bit 1)                |
| SP0       |    |Stack Pointer zero (PSU bit 0)               |
|-----------+----+---------------------------------------------|
| CC1       |c   |Condition Code one (PSL bit 7)               |
| CC0       |c   |Condition Code zero (PSL bit 6)              |
| IDC       | I  |Inter-Digit Carry status (PSL bit 5)         |
| RS        |    |Register bank Select (R1-R3, PSL bit 4)      |
| WC        |    |With/without Carry (PSL bit 3)               |
| OVF       |  O |Overflow status (PSL bit 2)                  |
| COM       |    |Logical/arithmetic Compare (PSL bit 1)       |
| C         |   C|Carry/borrow status (PSL bit 0)              |
|----------------+---------------------------------------------|
| a              |16-bit extended address                      |
| b              |16-bit absolute address                      |
| c              |2-bit condition codes CC1 and CC0            |
| calla          |[SP]+=PC+3,PC=b                              |
| callr          |[SP]+=PC+2,PC=l                              |
| d              |2-bit immediate data unit                    |
| dataNE         |Non-extended data port                       |
| i              |8-bit immediate data unit                    |
| l              |8-bit relative address                       |
| p              |8-bit I/O port number                        |
| r              |Register Rn (n=0-3)                          |
| ret            |If r#0, PC=[SP]-                             |
| statusNE       |Non-extended status port                     |
|----------------+---------------------------------------------|
| PC             |Program Counter                              |
| PSL            |Program Status Lower (8-bit)                 |
| PSU            |Program Status Upper (8-bit)                 |
| R0             |Register zero - accumulator                  |
| Rn             |Register (n=0-3)                             |
| SP             |Stack pointer                                |
|----------------+---------------------------------------------|
| +              |Arithmetic addition                          |
| -              |Arithmetic subtraction                       |
| *              |Arithmetic multiplication                    |
| /              |Arithmetic division                          |
| &              |Logical AND                                  |
| ~              |Logical NOT                                  |
| v              |Logical inclusive OR                         |
| x              |Logical exclusive OR                         |
| =              |Equal or assignment                          |
| #              |Not equal                                    |
| <-             |Rotate left                                  |
| ->             |Rotate right                                 |
| [ ]            |Indirect addressing                          |
| [ ]+           |Indirect addressing, auto-increment          |
| -[ ]           |Auto-decrement, indirect addressing          |
| { }            |Combination of operands                      |
| {rr}           |If WC=1 then {C,r} else {r}                  |
| -->            |Input pin                                    |
| <--            |Output pin                                   |
| <-->           |Input/output pin                             |
|--------------------------------------------------------------|
|                                                              |
|                                                              |
|                                                              |
|                                                              |
|                                                              |
|                                                              |
|                                                              |
|                                                              |
|                                                              |
|                                                              |
|                                                              |
|                                                              |
----------------------------------------------------------------


