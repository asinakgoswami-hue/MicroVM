/*
    Project: MicroVM
    Author: Asinak Goswami
    Initial push date: 15 November 2026

    Purpose:
    An independent educational project exploring instruction encoding,
    stack-based execution, VM state, and Java <-> C++ process communication.

    This implementation is a learning project, not a production VM.
    The author welcomes constructive criticism, experiments, and improvements.

    Copyright (c) 2026 Asinak Goswami.
    No open-source licence is currently granted. See README.md.
*/

#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <utility>
#include <stack>
#include <vector>
#include <string>
#include <climits>
#include <limits>
#include <stdexcept>
#include <chrono>
#include <thread>

using string = std::string;


// ============================================================
// ENUMERATIONS
// ============================================================

// LEARNING POINTER:
// Keep process-level success/failure separate from the VM's execution state.
// A VM can report an execution fault even if the C++ process itself remains
// capable of sending a final stack snapshot to Java.
enum class RETURN_CODES : uint32_t {
    SUCCESS_EXIT,
    FAULT_EXIT
};

enum class DESTROY_CODES : uint32_t {
    SUCCESS,
    ERROR
};

// LEARNING POINTER:
// These are VM control-flow outcomes, not operating-system exit codes.
// CONTINUE_RUN means execute another instruction; HALT is an intentional stop.
enum class EXCODES : uint32_t {
    ERROR_RUN,
    CONTINUE_RUN,
    HALT
};

// Instruction-set reference (opcode values must agree with Java Commands).
// LEARNING POINTER: Treat opcode numbers as part of the Java/C++ protocol.
// Changing a number on only one side will make the two implementations disagree.
enum OPER : uint32_t {
    OP_SUM               = 1,
    OP_SUM2              = 2,
    OP_PUSH              = 3,
    OP_SHOW              = 4,
    OP_STOP              = 5,
    OP_DISPLAY_STACK     = 6,
    OP_DESTROY_STACK_SUM = 7
};


// ============================================================
// STRUCTURES
// ============================================================

struct tempReturn {
    EXCODES execution_code;
    RETURN_CODES return_code;
};


// ============================================================
// FORWARD DECLARATIONS
// ============================================================

class MicroVM;

// Codes to translation string literal methods
string excodes_to_string(EXCODES code);
string return_codes_to_string(RETURN_CODES code);

// Instruction handling
constexpr uint32_t make_instruction(uint8_t op, int exp1, int exp2);

EXCODES execute_instruction(
    uint32_t instruction,
    std::stack<uint32_t>& stack,
    std::size_t& get_final_sum
);

// Stack operations
void destroy_stack(std::stack<uint32_t>& stack);

DESTROY_CODES destroy_stack_sum(
    std::stack<uint32_t>& stack,
    uint64_t& sum_var
);

uint64_t sum_stack(const std::stack<uint32_t>& stack);

void print_stack(const std::stack<uint32_t>& stack);

void wait_8_seconds();

// Process communication
void send_stack_data(const MicroVM& refObj , const tempReturn& status);

std::vector<uint32_t> receive_stack_data();

//============================================================
// EXCODES TO STRING
//============================================================

string excodes_to_string(EXCODES code) {
    switch (code) {
        case EXCODES::ERROR_RUN:
            return "ERROR_RUN";

        case EXCODES::CONTINUE_RUN:
            return "CONTINUE_RUN";

        case EXCODES::HALT:
            return "HALT";

        default:
            return "UNKNOWN_EXCODE";
    }
}

//============================================================
// RETURN CODES TO STRING
//============================================================

string return_codes_to_string(RETURN_CODES code) {
    switch (code) {
        case RETURN_CODES::SUCCESS_EXIT:
            return "SUCCESS_EXIT";

        case RETURN_CODES::FAULT_EXIT:
            return "FAULT_EXIT";

        default:
            return "UNKNOWN_RETURN_CODE";
    }
}


// ============================================================
// INSTRUCTION ENCODING
// ============================================================

/*
    INSTRUCTION LAYOUT (32 bits):
      [ bits 31..24: opcode ][ bits 23..12: operand 1 ][ bits 11..0: operand 2 ]

    LEARNING POINTER:
    Bit shifts place each field into its designated region; bitwise OR combines
    the fields. The decoder below must use the exact inverse shifts and masks.
    Watch out for accidentally designing Java's encoder as 8 + 24 bits while
    decoding C++ as 8 + 12 + 12 bits: that mismatch previously caused PUSH
    to read the wrong operand.
*/
constexpr uint32_t make_instruction(uint8_t op, int exp1, int exp2) {

    // FIX 1:
    // Convert operands to unsigned integers BEFORE shifting.
    // This avoids undefined behaviour from shifting negative signed values.

    uint32_t operand1 = static_cast<uint32_t>(exp1) & 0xfff;
    uint32_t operand2 = static_cast<uint32_t>(exp2) & 0xfff;

    return (
        (static_cast<uint32_t>(op) << 24) |
        (operand1 << 12) |
        operand2
    );
}


// ============================================================
// STACK OPERATIONS
// ============================================================

void destroy_stack(std::stack<uint32_t>& stack) {

    while (!stack.empty()) {
        stack.pop();
    }
}


DESTROY_CODES destroy_stack_sum(
    std::stack<uint32_t>& stack,
    uint64_t& sum_var
) {

    uint64_t sum = sum_stack(stack);

    if (sum > INT32_MAX) {

        std::cerr << "ORIGINAL STACK PRESENT" << std::endl;
        std::cerr << "INT32 DESTROY SUMMATION ERROR" << std::endl;

        return DESTROY_CODES::ERROR;
    }
    else {

        destroy_stack(stack);

        std::cerr << "ORIGINAL STACK DESTROYED" << std::endl;

        stack.push(static_cast<uint32_t>(sum));
        sum_var = sum;

        return DESTROY_CODES::SUCCESS;
    }
}


/*
    LEARNING POINTER:
    std::stack has no iterator interface for ordinary traversal. Copying it
    allows us to inspect its elements by repeatedly reading and popping the copy.
    This is intentionally non-destructive: the VM's original stack is preserved.
    Cost to remember: copying a stack takes O(n) time and O(n) extra storage.
*/
uint64_t sum_stack(const std::stack<uint32_t>& stack) {

    std::stack<uint32_t> temp(stack);
    uint64_t sum = 0;

    while (!temp.empty()) {

        sum += temp.top();
        temp.pop();
    }

    return sum;
}


/*
    LEARNING POINTER:
    The printed order is top-to-bottom, matching the order returned by
    get_stack_snapshot(). This function writes to stderr so that human-readable
    VM output cannot corrupt the machine-readable stdout protocol to Java.
*/
void print_stack(const std::stack<uint32_t>& stack) {

    std::stack<uint32_t> temp(stack);
    std::size_t size = temp.size();

    for (std::size_t i = 0; i < size; i++) {

        std::cerr << temp.top()
                  << std::endl;

        temp.pop();
    }
}


void wait_8_seconds() {

    // FIX 2:
    // Replaced the unreliable busy loop with an actual timed wait.

    std::this_thread::sleep_for(std::chrono::seconds(8));
}


// ============================================================
// INSTRUCTION EXECUTION
// ============================================================

/*
    INSTRUCTION DECODING:
    The masks isolate each field after shifting it down to the low bits.
    Keep these offsets synchronized with make_instruction() and Java Encoder.

    LEARNING POINTER:
    This function receives the stack by reference because instructions mutate
    VM state. get_final_sum is also a reference so OP_SUM can update the VM's
    stored sum. Passing it by value would only modify a local copy.
*/
EXCODES execute_instruction(
    uint32_t instruction,
    std::stack<uint32_t>& stack,
    std::size_t& get_final_sum
) {

    uint16_t operation_byte = (instruction >> 24) & 0xff;

    uint16_t exp1 = (instruction >> 12) & 0xfff;
    uint16_t exp2 = instruction & 0xfff;


    switch (operation_byte) {

        case OP_PUSH: {

            stack.push(exp1);

            return EXCODES::CONTINUE_RUN;
        }


        case OP_SUM: {

            // SUM is defined here as a non-destructive sum of the entire stack.
            // It does not pop values or push the result onto the stack.
            // Consider whether the stored sum should be updated only after all
            // validation succeeds; currently assignment occurs before the limit check.
            if (stack.empty()) {

                std::cerr << "UNDERFLOW SUM" << std::endl;

                return EXCODES::ERROR_RUN;
            }

            uint64_t sum = sum_stack(stack);

            get_final_sum = sum;

            if (sum > INT32_MAX) {

                std::cerr << "INT32 MAX LIMIT ERROR" << std::endl;

                return EXCODES::ERROR_RUN;
            }

            std::cerr << "STACK SUM = "
                      << sum
                      << std::endl;

            return EXCODES::CONTINUE_RUN;
        }


        case OP_SHOW: {

            if (stack.empty()) {

                std::cerr << "UNDERFLOW" << std::endl;

                return EXCODES::ERROR_RUN;
            }

            std::cerr << "VALUE ON STACK = "
                      << stack.top()
                      << std::endl;

            stack.pop();

            return EXCODES::CONTINUE_RUN;
        }


        case OP_SUM2: {

            // This opcode adds the two immediate operands encoded in the
            // instruction and pushes their result. It does NOT pop two values
            // from the existing stack. That distinction belongs in the VM manual.
            stack.push(static_cast<uint32_t>(exp1 + exp2));

            return EXCODES::CONTINUE_RUN;
        }


        case OP_DISPLAY_STACK: {

            print_stack(stack);

            return EXCODES::CONTINUE_RUN;
        }


        case OP_DESTROY_STACK_SUM: {

            // Destructive operation: calculate the total, clear the old stack,
            // then push the total as the sole remaining element.
            // The helper checks the configured INT32_MAX limit before clearing,
            // so a failed operation should leave the original stack intact.
            uint64_t sum_destroy = 0;

            DESTROY_CODES code = destroy_stack_sum(
                stack,
                sum_destroy
            );

            if (code == DESTROY_CODES::SUCCESS) {

                std::cerr << "STACK SUM = "
                          << sum_destroy
                          << std::endl;

                print_stack(stack);
            }
            else {

                // FIX 3:
                // DESTROY_CODES cannot be returned as EXCODES.
                // Translate the operation failure into a VM execution error.

                return EXCODES::ERROR_RUN;
            }

            return EXCODES::CONTINUE_RUN;
        }


        case OP_STOP: {

            // FIX 4:
            // STOP is now represented by HALT.
            // HALT means successful VM execution termination.

            return EXCODES::HALT;
        }


        default: {

            return EXCODES::ERROR_RUN;
        }
    }
}


// ============================================================
// MICROVM CLASS
// ============================================================

/*
    MICROVM STATE:
      opvec         - the program/instruction sequence
      stack         - the VM's operand stack
      count         - next instruction position
      sum_all_stack - most recently stored stack sum

    LEARNING POINTER:
    Keeping execution state inside the VM object makes it possible to add
    methods such as reset(), load_program(), step(), and inspect_state() later.
*/
class MicroVM {

    private:

        std::vector<uint32_t> opvec;
        std::stack<uint32_t> stack;

        std::size_t count = 0;

        std::size_t sum_all_stack = 0;

    public:

        explicit MicroVM() = default;

        explicit MicroVM(std::vector<uint32_t> tapVec)
            : opvec(std::move(tapVec)) {

                std::cerr << "[VM] Constructed with "
                          << opvec.size()
                          << " instructions\n";

            }

        explicit MicroVM(std::stack<uint32_t> tapStack)
            : stack(std::move(tapStack)) {}


        std::size_t get_sizeof_stack() const {

            // FIX 5:
            // stack.size() returns std::size_t, not int.

            return stack.size();
        }

        //Copy stack into a snapshot..
        /*
            A snapshot is a detached representation of the current stack.
            It preserves the VM's stack and gives the communication layer a
            simple vector to serialize. Values are returned top-to-bottom.
        */
        std::vector<uint32_t> get_stack_snapshot() const{
            std::stack<uint32_t> copy = stack;
            
            std::vector<uint32_t> result;
            result.reserve(copy.size());

            while(!copy.empty()){
                result.push_back(copy.top());
                copy.pop();
            }   

            return result;
        }


        // Non-destructive stack inspection
        void get_all_data_from_stack() const {
            print_stack(stack);
        }


        /*
            run() is the VM's instruction-dispatch loop.
            It advances count, executes one instruction, then interprets the
            returned execution code to decide whether to continue, halt, or fail.
            Later, a step() method could expose one-instruction-at-a-time execution
            for a Java GUI/debugger.
        */
        tempReturn run(void) {

            // FIX 6:
            // Execute instructions continuously until HALT or ERROR.
            // Do not return after an ordinary CONTINUE_RUN instruction.

            while (count < opvec.size()) {

                uint32_t instruction = opvec[count++];

                /*
                EXCODES val = execute_instruction(
                    instruction,
                    stack
                );
                */

                std::cerr << "[VM] Executing instruction "
                        << count - 1 << '\n';

                std::cerr << "[VM] Opcode: "
                        << ((instruction >> 24) & 0xFF) << '\n';

                EXCODES val = execute_instruction(instruction, stack , sum_all_stack);

                std::cerr << "[VM] Execution status: "
                        << excodes_to_string(val) << '\n';

                std::cerr << "[VM] Stack size: "
                        << stack.size() << '\n';

                if (val == EXCODES::CONTINUE_RUN) {

                    continue;
                }


                if (val == EXCODES::HALT) {

                    // FIX 7:
                    // HALT is a successful execution outcome.

                    return {
                        .execution_code = EXCODES::HALT,
                        .return_code = RETURN_CODES::SUCCESS_EXIT
                    };
                }


                if (val == EXCODES::ERROR_RUN) {

                    return {
                        .execution_code = EXCODES::ERROR_RUN,
                        .return_code = RETURN_CODES::FAULT_EXIT
                    };
                }
            }


            // FIX 8:
            // Reaching the end without OP_STOP is treated as an error.
            // This enforces explicit VM termination.

            return {
                .execution_code = EXCODES::ERROR_RUN,
                .return_code = RETURN_CODES::FAULT_EXIT
            };
        }


        void append_instructions(
            const std::vector<uint32_t>& vector
        ) {

            opvec.insert(
                opvec.end(),
                vector.begin(),
                vector.end()
            );
        }

        uint32_t get_sum(){
            return sum_all_stack;
        }
};


// ============================================================
// PROCESS COMMUNICATION
// ============================================================

/*
    JAVA RESPONSE PROTOCOL:
      CURRENT STACK DATA
      <element count>
      <one unsigned integer per line, top-to-bottom>
      DATA END

    IMPORTANT:
    stdout is reserved for this protocol. Send diagnostics and human-readable
    VM output to stderr, otherwise Java's parser may mistake diagnostics for data.
*/
void send_stack_data(const MicroVM& refObj , const tempReturn& status) {
    //Retrive the VM stack
    std::vector<uint32_t> temp = refObj.get_stack_snapshot();

    //Send the response header
    std::cout << "CURRENT STACK DATA" << std::endl;

    //Send the number of elements
    std::cout << temp.size() << '\n';

    //Send the stack elements
    for(uint32_t value : temp){
        std::cout << value << '\n';
    }

    //End the transmission via a end header
    std::cout << "DATA END" << '\n';

    // Execution diagnostics belong on stderr.
    std::cerr << "[CPP] Execution: "
              << excodes_to_string(status.execution_code)
              << '\n';

    std::cerr << "[CPP] Return code: "
              << return_codes_to_string(status.return_code)
              << '\n';

}


// ============================================================
// PROCESS DATA RECEPTION
// ============================================================

/*
    JAVA REQUEST PROTOCOL:
      SENDING DATA
      <one unsigned decimal instruction per line>
      DATA END SUBMISSION

    LEARNING POINTER:
    A delimiter makes the message boundary explicit. Validate every numeric
    field before converting it to uint32_t. stoull is used because valid
    uint32_t instruction bit patterns can have decimal values above INT32_MAX.
*/
std::vector<uint32_t> receive_stack_data() {

    string str;

    std::vector<uint32_t> stack_data_with_instruction;

    string temp;

    bool submission_ended = false;

    while (std::getline(std::cin, temp)) {

        if (temp == "DATA END SUBMISSION") {

            submission_ended = true;
            break;
        }

        // FIX 11:
        // stoull supports the full unsigned 32-bit instruction range.
        // stoi would fail for values above INT32_MAX.

        std::size_t processed = 0;

        try {

            unsigned long long tempNum = std::stoull(temp, &processed);

            if (
                processed != temp.size() ||
                tempNum > std::numeric_limits<uint32_t>::max()
            ) {

                throw std::runtime_error("INVALID INSTRUCTION VALUE");
            }

            stack_data_with_instruction.push_back(
                static_cast<uint32_t>(tempNum)
            );
        }
        catch (const std::exception&) {

            throw std::runtime_error("MALFORMED INSTRUCTION DATA");
        }
    }


    // FIX 12:
    // Ensure that the submission actually has an ending delimiter.

    if (!submission_ended) {

        throw std::runtime_error("INCOMPLETE DATA SUBMISSION");
    }

    return stack_data_with_instruction;
}


// ============================================================
// MAIN
// ============================================================

/*
int main(void) {

    std::cerr << "[CPP] Waiting for header\n";

    string line;

    if (!std::getline(std::cin, line)) {
        std::cerr << "[CPP] Input stream closed\n";
        return 1;
    }

    std::cerr << "[CPP] Received header: " << line << '\n';
    std::vector<uint32_t> stack_data;

    if (line == "SENDING DATA") {
        stack_data = receive_stack_data();
    }else {
        std::cerr << "[CPP] Invalid submission header\n";
        return 1;
    }
    std::cerr << "Instruction count: "
          << stack_data.size() << '\n';

    for (uint32_t instruction : stack_data) {
        std::cerr << "Instruction: "
                << instruction << '\n';
    }

    MicroVM micro_obj(stack_data);

    std::cerr << "[CPP] About to call VM::run()\n";

    tempReturn run_return_value = micro_obj.run();

    send_stack_data(micro_obj , run_return_value);

    std::cerr << "[CPP] VM::run() returned\n";

    std::cerr << "Execution result: "
          << excodes_to_string(run_return_value.execution_code)
          << '\n';

    std::cout << excodes_to_string(run_return_value.execution_code)
          << " "
          << return_codes_to_string(run_return_value.return_code)
          << '\n';

    std::cerr << "[CPP] Reached end of main()\n";      
    return 0;
}
*/

/*
    PROCESS ENTRY POINT:
    This is the bridge between the outside Java process and the VM:
    validate request header -> receive program -> construct VM -> run -> reply.

    LOOK OUT FOR:
    Exceptions from receive_stack_data() are not caught here yet. A malformed
    request can therefore terminate the process without a structured response.
    A later protocol revision could send a dedicated error response.
*/
int main(void) {

    std::string line;

    // 1. Validate the incoming communication header.
    if (!std::getline(std::cin, line) || line != "SENDING DATA") {
        std::cerr << "[CPP] Invalid communication header.\n";
        return 1;
    }

    // 2. Receive instructions from Java.
    std::vector<uint32_t> instructions = receive_stack_data();

    // 3. Construct the virtual machine.
    MicroVM micro_obj(instructions);

    // 4. Execute the instructions.
    auto run_return_value = micro_obj.run();

    // 5. Transmit the final stack snapshot to Java.
    send_stack_data(micro_obj , run_return_value);

    return 0;
}

