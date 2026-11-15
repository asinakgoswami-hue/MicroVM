# MicroVM

**Author:** Asinak Goswami  
**Initial push date:** 15 November 2026  
**Project type:** Independent educational / learning project

## About

I am a school student who built this project to learn by experimenting with
virtual-machine concepts and communication between Java and C++. I am sharing
the implementation so that other learners can explore the ideas, experiment
with the code, and avoid having to search endlessly for small, interesting
projects hidden away on someone else's computer.

This implementation is not presented as a finished or production-grade VM.
It is a record of my learning, design decisions, mistakes, and improvements.
I know there are areas that can be improved. I welcome constructive criticism,
alternative approaches, and thoughtful improvements.

If you learn from this project, I hope you use that understanding to build
something of your own rather than presenting someone else's work as yours.

## Current architecture

- **Java** builds 32-bit instructions, launches the C++ executable, sends a
  line-based request, and reads the returned stack snapshot.
- **C++** receives the instruction sequence, decodes and executes instructions,
  maintains the VM stack, and sends a final snapshot.
- **stdout** is reserved for structured protocol messages.
- **stderr** is used for diagnostic and human-readable output.

## Provisions to run this code

- **1.** The C++ executable has to be made. This code does not the provide the original executable.
- **2.** The C++ executable on g++ systems requires the command **g++ MicroVM.cpp -o MicroVM.exe**
- **3.** It is prefered to have the *ProcessHandlerMicroVM* class to be compiled for this to run smoothly. 


## Instruction format

Each instruction is a 32-bit unsigned value:

| Bits | Field | Width |
|---|---|---:|
| 31–24 | Opcode | 8 bits |
| 23–12 | Operand 1 | 12 bits |
| 11–0 | Operand 2 | 12 bits |

The Java `Encoder` and C++ `make_instruction` / decoder must agree on this
layout. A mismatch between the encoder and decoder can produce valid-looking
integers that execute with the wrong operands.

## Current instruction set

| Opcode | Name | Current behaviour |
|---:|---|---|
| 1 | `SUM` | Calculates and prints the sum of the entire stack without changing it |
| 2 | `SUM_TWO` | Adds the two immediate instruction operands and pushes the result |
| 3 | `PUSH` | Pushes operand 1 onto the stack |
| 4 | `SHOW` | Prints and removes the top stack value |
| 5 | `STOP` | Halts the VM |
| 6 | `DISPLAY_STACK` | Prints the stack from top to bottom |
| 7 | `DESTROY_STACK_SUM` | Replaces the stack with one element: the sum of its previous contents |

Note: `SUM_TWO` currently adds the instruction's two operands. It does not pop
two values from the stack. `SUM` and `DESTROY_STACK_SUM` have different effects:
the former preserves the stack; the latter replaces it with the total.

## Communication protocol

### Java to C++

```text
SENDING DATA
<instruction 1 as unsigned decimal>
<instruction 2 as unsigned decimal>
...
DATA END SUBMISSION
```

### C++ to Java

```text
CURRENT STACK DATA
<count>
<stack value 1>
<stack value 2>
...
DATA END
```

The returned stack values are ordered top-to-bottom.

## Learning notes and things to watch

- **Bit packing:** Shifts and masks must match exactly on both sides of the
  language boundary. Keep the opcode and operand widths documented.
- **Signed versus unsigned integers:** Java `int` is signed, while the C++
  instruction is represented as `uint32_t`. Unsigned decimal conversion is
  used when transmitting instruction values.
- **Streams:** Do not let diagnostic text enter the machine-readable stdout
  protocol. Keep diagnostics on stderr.
- **Single stream reader:** Do not have multiple Java readers consume the same
  process stdout. The old `displayCppOutput()` method is retained only as an
  experiment and must not be run alongside `readFromVM()`.
- **Stack inspection:** Copying a `std::stack` makes inspection non-destructive,
  but costs O(n) time and O(n) additional memory.
- **Process lifecycle:** The current Java sample should be extended to close
  streams, wait for the child process, and inspect its exit status.
- **Error handling:** Malformed C++ input currently throws an exception. A
  future version should decide how protocol errors are reported back to Java.
- **Limits:** The current destructive-sum operation applies an `INT32_MAX`
  limit. Make sure the chosen integer types and checks remain consistent as
  the VM grows.
- **Instruction exhaustion:** The current VM treats reaching the end of the
  program without `STOP` as an execution fault.

## Suggested next experiments

1. Add stack arithmetic instructions that consume existing stack values.
2. Add a maximum stack size and explicit stack overflow handling.
3. Add a `step()` operation so a Java interface can execute one instruction
   at a time.
4. Add process cleanup and structured error responses.
5. Build a Java-side instruction-set manual or assembler so programs do not
   need to be written as packed integers.

## Authorship and reuse

Copyright (c) 2026 Asinak Goswami. All rights reserved.

This repository is currently intended as a private learning archive. No
open-source licence is currently granted. If the repository is made public,
the author should choose and add a licence before inviting reuse or
redistribution. A copyright notice identifies the claimed author; it is not
a substitute for a licence granting reuse permissions.


## Author's note

I am a school student who made this project for learning. I wanted to share
the code I have made so that other learners can find creative projects to
explore instead of having to dig around for them. I know this implementation
is not perfect and may be far from it. I humbly ask that, if you take
inspiration from it, you improve it and continue learning from it. I am open
to opinions and constructive criticism; in fact, criticism is welcome because
it helps me understand where my designs can improve.

I hope this project helps someone learn something new.
