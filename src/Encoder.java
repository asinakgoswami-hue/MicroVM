/*
    Project: MicroVM
    Author: Asinak Goswami
    Initial push date: 15 November 2026
    Purpose: Educational Java-side instruction encoder.

    Copyright (c) 2026 Asinak Goswami.
    No open-source licence is currently granted. See README.md.
*/

public class Encoder {

    /*
        The C++ decoder expects a 32-bit instruction arranged as:
        [8-bit opcode][12-bit operand1][12-bit operand2].

        LEARNING POINTER:
        An instruction is just a packed integer. Shifting moves a field into
        position; masking ensures that only the intended bits are present.
        The Java encoder and C++ decoder must always agree on this layout.
    */
    private static final int OPERATION_BIT_NUMBER = 24;
    private static final int OPERAND_BIT_ONE = 12;
    private static final int MAX_OPERAND = 0xfff;

    public static int encode(
        Commands command,
        int operand1,
        int operand2
    ) {
        /*
            Validate before packing. Without this check, a value larger than
            12 bits could overlap the neighbouring field and corrupt the opcode
            or the other operand.
        */
        if (
            operand1 < 0 || operand1 > MAX_OPERAND
                         ||
            operand2 < 0 || operand2 > MAX_OPERAND
        ) {
            throw new IllegalArgumentException(
                "Operands must be within 12 bits"
            );
        }

        return (
            (command.getOpcode() & 0xff) << OPERATION_BIT_NUMBER
            |
            (operand1 << OPERAND_BIT_ONE)
            |
            operand2
        );
    }

    // Convenience overload for instructions that use only operand 1.
    public static int encode(Commands command, int operand1) {
        return encode(command, operand1, 0);
    }

    // Convenience overload for instructions with no operands.
    public static int encode(Commands command) {
        return encode(command, 0, 0);
    }
}
