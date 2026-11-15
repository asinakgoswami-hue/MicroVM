/**
 * Public enumeration for Command readability
 * 
 * Each command has it's own operation code, hard-tied to the C++ implementation
 * 
 * Commands
 */

public enum Commands{
    SUM(1), 
    SUM_TWO(2),
    PUSH(3),
    SHOW(4),
    STOP(5),
    DISPLAY_STACK(6),
    DESTROY_SUM(7),
    NO_COMMAND(0xffa); // No command given operation , gives stderr stream message

    private int opcode = 0; // private operation code setter variable

    //Default parameterized constructor to set opcode to enum values.
    Commands(int opcode){
        this.opcode = opcode;
    }   

    //Public method to use operation code in different places such as
    //the Encoder class
    public int getOpcode(){
        return opcode;
    }

}