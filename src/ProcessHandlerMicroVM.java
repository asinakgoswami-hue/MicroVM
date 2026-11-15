/*
    Project: MicroVM
    Author: Asinak Goswami
    Initial push date: 15 November 2026
    Purpose: Java process launcher and line-based protocol handler.

    Copyright (c) 2026 Asinak Goswami.
    No open-source licence is currently granted. See README.md.
*/

import java.io.*;
import java.util.ArrayList;

public class ProcessHandlerMicroVM {

    /*
        Protocol markers shared with the C++ program.
        Keep spelling and capitalization identical on both sides.
    */
    @SuppressWarnings("unused")
    private static final String
        DATA_END_SUBMIT = "DATA END SUBMISSION",
        DATA_END = "DATA END",
        RESPONSE_HEADER = "CURRENT STACK DATA";

    private static Commands receivedCommand;
    private static ProcessBuilder builder;
    private static Process process;
    private static ArrayList<Integer> list;

    /*
        This is a small text-to-enum helper. It currently recognizes only a
        subset of the VM instruction names; update it if you later use it for
        the complete instruction set. The current main() builds instructions
        directly through Encoder, so this helper is not part of that path.
    */
    private static Commands getCommandHelper(String comm) {
        if (comm.equals("STOP")) return Commands.STOP;
        else if (comm.equals("PUSH")) return Commands.PUSH;
        else if (comm.equals("DESTROY_SUM")) return Commands.DESTROY_SUM;
        else if (comm.equals("SUM_TWO")) return Commands.SUM_TWO;
        else if (comm.equals("SHOW")) return Commands.SHOW;
        else return Commands.NO_COMMAND;
    }

    private static BufferedReader reader;
    private static BufferedWriter writer;

    // The executable must be discoverable from the process working directory,
    // or this should be replaced with a configured/absolute executable path.
    private static final String MICRO_VM_FILENAME = "MicroVM.exe";

    public static void startVM() {
        try {
            list = new ArrayList<>();

            builder = new ProcessBuilder(MICRO_VM_FILENAME);

            /*
                C++ stderr is inherited by the Java process's terminal.
                C++ stdout remains connected to reader and is reserved for
                structured protocol messages. Do not mix the two streams.
            */
            builder.redirectError(ProcessBuilder.Redirect.INHERIT);

            process = builder.start();

            reader = new BufferedReader(
                new InputStreamReader(process.getInputStream())
            );

            writer = new BufferedWriter(
                new OutputStreamWriter(process.getOutputStream())
            );

        } catch (Exception e) {
            // For a reusable library, consider propagating this exception
            // instead of printing it and allowing later null dereferences.
            e.printStackTrace();
        }
    }

    /*
        Writes exactly one protocol line. Flushing after each line is simple
        and prevents the child process from waiting on data still in Java's
        output buffer. For larger transfers, batching and flushing once may
        be more efficient.
    */
    private static void writeToCpp(String str) throws IOException {
        writer.write(str);
        writer.newLine();
        writer.flush();
    }

    /*
        OLD READER EXPERIMENT:
        This commented-out parser was an earlier attempt at reading output.
        Keep only one active reader for a process stream: two readers competing
        for stdout can consume each other's lines or leave one blocked.
    */
    /*
        String line = "";

        while (!(line = reader.readLine()).equals(TERMINATE)) {
            if (line.equals("STARTING PROCESS")) {
                line = reader.readLine();

                if (line.equals("CURRENT STACK DATA")) {
                    String tempLine = "";

                    while (!(tempLine = reader.readLine()).equals(DATA_END)) {
                        list.add(Integer.parseInt(tempLine));
                    }
                }
            }
        }
    */

    public static void readFromVM() throws Exception {
        list.clear();

        String line = "";

        // Skip any preceding stdout lines until the response header appears.
        // Prefer keeping stdout protocol-only so this search is normally immediate.
        while ((line = reader.readLine()) != null) {
            if (line.equals(RESPONSE_HEADER)) {
                break;
            }
        }

        if (line == null) {
            throw new IOException("Missing stack response header");
        }

        String countElements = reader.readLine();

        if (countElements == null) {
            throw new IOException("Missing stack element count");
        }

        int count = Integer.parseInt(countElements);

        if (count < 0) {
            throw new IOException("Invalid stack element count");
        }

        /*
            The count controls exactly how many values are read. This is safer
            than reading until a value happens to resemble a terminator.
            A future hardening step is to impose a maximum accepted count.
        */
        for (int i = 0; i < count; i++) {
            String value = reader.readLine();

            if (value == null) {
                throw new IOException("Incomplete stack transmission");
            }

            list.add(Integer.parseUnsignedInt(value));
        }

        String terminator = reader.readLine();

        if (!DATA_END.equals(terminator)) {
            throw new IOException(
                "Invalid stack response terminator : " + terminator
            );
        }

        // This confirms the response was fully parsed; it does not itself
        // prove that the child process has exited. Use process.waitFor() to
        // wait for termination and retrieve the actual process exit code.
        System.out.println("[JAVA] C++ response received.");
    }

    public static void sendToVM(ArrayList<Integer> arrayList) throws Exception {
        /*
            The header and ending delimiter frame the request. Each instruction
            is sent as unsigned decimal text because Java int is signed even
            though the packed instruction is treated as a uint32_t by C++.
        */
        writeToCpp("SENDING DATA");

        for (Integer i : arrayList) {
            String str = Integer.toUnsignedString((int) i);
            writeToCpp(str);
        }

        writeToCpp("DATA END SUBMISSION");
    }

    public static int[] getOriginalDataFromStack() {
        return list.stream()
                    .mapToInt(Integer::intValue)
                    .toArray();
    }

    /*
        Do not call this after readFromVM() on the same reader.
        Both methods consume the same stdout stream; this method would compete
        with the protocol parser and may block waiting for more output.
        It is retained here as an experiment, not part of the current flow.
    */
    private static void displayCppOutput() throws IOException {
        String line;

        System.out.println("\n===== C++ OUTPUT =====");

        while ((line = reader.readLine()) != null) {
            System.out.println("[CPP → JAVA] " + line);
        }

        System.out.println("===== END OF C++ OUTPUT =====");
    }

    public static void main(String[] args) throws Exception {

        // Launch the child process before writing the request.
        startVM();

        System.out.println("C++ alive: " + process.isAlive());

        ArrayList<Integer> instructions = new ArrayList<>();

        /*
            SAMPLE PROGRAM / MANUAL:
              PUSH 15          -> push 15
              PUSH 25          -> push 25
              SUM              -> calculate and print total, preserve stack
              PUSH 40          -> push 40
              SUM_TWO 7, 8     -> push 15 (sum of immediate operands)
              SHOW             -> print and pop the current top value
              DISPLAY_STACK    -> print stack top-to-bottom
              DESTROY_SUM      -> replace stack with its total
              STOP             -> halt successfully

            Expected final stack: [80]
            Stack display order is top-first.
        */
        instructions.add(Encoder.encode(Commands.PUSH, 15));
        instructions.add(Encoder.encode(Commands.PUSH, 25));
        instructions.add(Encoder.encode(Commands.SUM));
        instructions.add(Encoder.encode(Commands.PUSH, 40));
        instructions.add(Encoder.encode(Commands.SUM_TWO, 7, 8));
        instructions.add(Encoder.encode(Commands.SHOW));
        instructions.add(Encoder.encode(Commands.DISPLAY_STACK));
        instructions.add(Encoder.encode(Commands.DESTROY_SUM));
        instructions.add(Encoder.encode(Commands.STOP));

        sendToVM(instructions);
        readFromVM();

        System.out.println(
            "\n====================== RECEIVED STACK DATA ======================"
        );
        System.out.println(list);

        /*
            LEARNING POINTER:
            A robust process lifecycle should also close writer/reader and call
            process.waitFor(), then inspect the exit code. This sample focuses
            on the one-request/one-response handshake and leaves that cleanup
            as a next development step.
        */
    }
}
