from genTools import *


if "__main__" == __name__:
    maxOpcode = 5

    with open("ElCommand_template.h", "r") as file:
        template = file.read()
    movRegTable = genMovTable()
    movDefines = ""

    for label, opcode in movRegTable.items():
        label = label.upper()
        movDefines = movDefines + "#define MOV_" + label + " " + ", ".join(map(hex, opcode)) + "\n"

    template = template.replace("{{DEFINES}}",
                                registerDefines() +
                                movDefines +
                                movImmDefines() +
                                registerPopDefines() +
                                registerPushDefines() +
                                addCommandDefines() +
                                subCommandDefines() +
                                imulCommandDefines() +
                                idivDefines() +
                                xorDefines() +
                                cmpDefines() +
                                testDefines() +
                                xchgRax())
    template = template.replace("{{REGNUM}}", str(len(registers)))

    template = template.replace("{{TABLES}}",
                                movTable() +
                                pushPopTable() +
                                movRspMemTable() +
                                movRbpMemTable() +
                                subTable() +
                                addTable() +
                                xorTable() +
                                testTable() +
                                cmpTable() +
                                imulTable() +
                                xchgRaxTable()+
                                idivPopTable())

    with open("ElCommand.h", "w") as outputFile:
        outputFile.write(template)
