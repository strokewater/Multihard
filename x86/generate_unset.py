fd=open("unset_idt_gate.c", 'w')
fd.write('#include "exception.h"')
fd.write('\n')
fd.write('#include "asm/pm.h"')
fd.write("\n")
for i in range(0, 255):
    fd.write("void UnsetIDTGate%d() { panic(\"UnsetIDTGate%d\"); } " % (i, i))
    fd.write("\n")

fd.write("void InitUnsetIDTGate() {")
fd.write('\n')
for i in range(0, 255):
    fd.write("\tSetupIDTGate(%d, SelectorFlatC, (addr)UnsetIDTGate%d);" % (i, i))
    fd.write("\n")
fd.write("}")
fd.close()
