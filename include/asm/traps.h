#ifndef TRAPS_H
#define TRAPS_H

void TrapsInit();

void DivideError();
void DebugError();
void NMIError();
void Int3Error();
void OverFlowError();
void BoundsError();
void InvalidOPError();
void DeviceNotAvailableError();
void DoubleFaultError();
void CoprocessorSegmentOverRunError();
void InvalidTSSError();
void SegmentNotPresentError();
void StackSegmentError();
void GeneralProtectionError();
void CoprocessorError();
void PageFaultError();
void AlignCheckError();
void MachineCheckError();
void SIMDExceptionError();


#endif
